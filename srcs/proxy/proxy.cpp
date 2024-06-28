#include "proxy/proxy.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <string_view>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "proxy/event.hpp"
#include "proxy/parse.hpp"

xk7::Proxy::Proxy(std::shared_ptr<ALogger> logger_query, std::shared_ptr<ALogger> logger_proxy,
				std::string_view server_host, std::string_view server_port)
	:	logger_query_{ logger_query }, logger_proxy_{ logger_proxy },
		server_host_{ server_host }, server_port_{ server_port }
{

}

bool xk7::Proxy::bind(std::string_view proxy_host, std::string_view proxy_port)
{
	epoll_event ev;

	fd_listener = create_proxy(proxy_host, proxy_port, max_connections);
	if (fd_listener == code_error)
		return false;

	fd_epoll = epoll_create1(0);
	if (fd_epoll == -1)
	{
		if (logger_proxy_)
			logger_proxy_->error(std::string("EPoll error: ") + std::strerror(errno));
		return false;
	}

	ev.data.fd = fd_listener;
	ev.events = EPOLLIN;

	if (epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_listener, &ev) == -1)
	{
		if (logger_proxy_)
			logger_proxy_->error(std::string("EPoll adding error: ") + std::strerror(errno));
		return false;
	}

	if (logger_proxy_)
		logger_proxy_->info(std::string("Server listen ") + std::string(proxy_host) + ':' + std::string(proxy_port));

	return true;
}

bool xk7::Proxy::run(std::function<bool()> exit_conditional)
{
	while (!exit_conditional())
	{
		auto count_events = epoll_wait(fd_epoll, events, max_connections, -1);

		if (count_events == -1)
		{
			if (logger_proxy_)
				logger_proxy_->error(std::string("EPoll wait error: ") + std::strerror(errno));

			close(fd_listener);
			return false;
		}

		for (decltype(count_events) i = 0; i < count_events; ++i)
		{
			if (events[i].data.fd == fd_listener)
			{
				auto [fd_client, fd_server] = create_client(fd_epoll, fd_listener);
				if (fd_client != code_error && logger_proxy_)
						logger_proxy_->info(std::string("New connection: client: ") +
							std::to_string(fd_client) + "; server: " + std::to_string(fd_server));	
				continue;
			}
			
			handle_transfer_data(events[i]);		
		}

		logger_query_->flush();
	}

	return true;
}

int xk7::Proxy::create_proxy(std::string_view host, std::string_view port, std::size_t max_connection)
{
	addrinfo*	dest_info{ nullptr };
	addrinfo*	curr_info{ nullptr };
	addrinfo	dest_hint{};
	int			result_socket{code_error};

	dest_hint.ai_family = AF_UNSPEC;
	dest_hint.ai_socktype = SOCK_STREAM;

	if (auto status = getaddrinfo(host.data(), port.data(), &dest_hint, &dest_info); status == -1 || dest_info == nullptr)
	{
		if (logger_proxy_)
			logger_proxy_->error(std::string("Get address info error: ") + gai_strerror(status));
		return code_error;
	}
	
	for (curr_info = dest_info; curr_info != nullptr; curr_info = curr_info->ai_next)
	{
		result_socket = socket(curr_info->ai_family, curr_info->ai_socktype, curr_info->ai_protocol);
		
		if (result_socket == -1)
			continue;

		// Fixed message "Address already in use"
		if (int yes = 1; setsockopt(result_socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) == -1)
		{
			close(result_socket);
			continue;
		}

		if (::bind(result_socket, curr_info->ai_addr, curr_info->ai_addrlen) == 0)
			break;
	}

	if (curr_info == nullptr)
	{
		if (result_socket != -1)
			close(result_socket);
		if (logger_proxy_)
			logger_proxy_->error(std::string("Bind error: ") + std::strerror(errno));
		return code_error;
	}
		
	freeaddrinfo(dest_info);
	dest_info = nullptr;

	if (listen(result_socket, max_connection) == -1)
	{
		close(result_socket);
		if (logger_proxy_)
			logger_proxy_->error(std::string("Listen error: ") + std::strerror(errno));
		return code_error;
	}

	return result_socket;
}

bool xk7::Proxy::set_nonblocking(int fd_socket)
{
    int flags = fcntl(fd_socket, F_GETFL, 0);

    if (flags == -1)
	{
		if (logger_proxy_)
			logger_proxy_->error(std::string("Get flags error: ") + std::strerror(errno));
		return false;
	}
		
    flags |= O_NONBLOCK;

    if (fcntl(fd_socket, F_SETFL, flags) == -1) 
	{
		if (logger_proxy_)
			logger_proxy_->error(std::string("Set flags error: ") + std::strerror(errno));
		return false;
	}

    return true;
}

int xk7::Proxy::create_client_conn(int fd_listener)
{
	int	fd_client{0};

	if (fd_client = accept(fd_listener, nullptr, 0); fd_client == -1)
	{
		if (logger_proxy_)
			logger_proxy_->error(std::string("Accept error: ") + std::strerror(errno));
		return code_error;
	}
		
	if (!set_nonblocking(fd_client))
	{
		close(fd_client);
		return code_error;
	}

	return fd_client;
}

int xk7::Proxy::create_server_conn()
{
	addrinfo*	server_info{ nullptr };
	addrinfo	server_hint{};

	server_hint.ai_socktype = SOCK_STREAM;

	if (int status = getaddrinfo(server_host_.data(), server_port_.data(), &server_hint, &server_info); status != 0 || server_info == nullptr)
	{
		if (logger_proxy_)
			logger_proxy_->error(std::string("Get server info error: ") + gai_strerror(status));
		return code_error;
	}

	auto fd_server = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if (fd_server == -1)
	{
		if (logger_proxy_)
			logger_proxy_->error(std::string("Server socket error: ") + std::strerror(errno));
		return code_error;
	}

	if (connect(fd_server, server_info->ai_addr, server_info->ai_addrlen) == -1)
	{
		if (logger_proxy_)
			logger_proxy_->error(std::string("Server connect error: ") + std::strerror(errno));
		close(fd_server);
		return code_error;
	}

	freeaddrinfo(server_info);

	if (!set_nonblocking(fd_server))
	{
		close(fd_server);
		return code_error;
	}

	return fd_server;
}

std::pair<int, int> xk7::Proxy::create_client(int fd_epoll, int fd_listener)
{
	epoll_event	ev;
	int	fd_client{0};
	int	fd_server{0};

	fd_client = create_client_conn(fd_listener);
	if (fd_client == code_error)
		return { code_error, code_error};

	if (logger_proxy_)
		logger_proxy_->debug(std::string("create client connection: ") + std::to_string(fd_client));

	fd_server = create_server_conn();
	if (fd_server == code_error)
		return { code_error, code_error};

	if (logger_proxy_)
		logger_proxy_->debug(std::string("create server connection: ") + std::to_string(fd_server));

	// Ивент на ожидание пакетов с клиента
	ev = create_event(fd_client, fd_server, opt_client_to_server, EPOLLIN | EPOLLET);
	if (epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_client, &ev) == -1)
	{
		if (logger_proxy_)
			logger_proxy_->error(std::string("EPoll ctl error: ") + std::strerror(errno));
		close(fd_client);
		close(fd_server);
		return { code_error, code_error };
	}

	// Ивент на ожидание ответных пакетов с сервера
	ev = create_event(fd_server, fd_client, opt_server_to_client, EPOLLIN | EPOLLET);
	if (epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_server, &ev) == -1)
	{
		if (logger_proxy_)
			logger_proxy_->error(std::string("EPoll ctl error: ") + std::strerror(errno));
		close(fd_client);
		close(fd_server);
		return { code_error, code_error };
	}

	return { fd_client, fd_server };
}

std::int64_t xk7::Proxy::write_data(int fd_socket, std::string_view buff)
{
	std::size_t	total_send{0};
	ssize_t		bytes_send{0};

	while (total_send < buff.size())
	{
		auto raw_ptr = reinterpret_cast<const void *>(buff.data() + total_send);
		auto size_buff = buff.size() - total_send;

		bytes_send = write(fd_socket, raw_ptr, size_buff);
		if (bytes_send <= 0)
			return bytes_send;
		
		total_send += bytes_send;
	}

	return total_send;
}

bool xk7::Proxy::handle_transfer_data(epoll_event& ev) // int fd_epoll, 
{
	char		buffer[max_buffer_size];
	auto [fd_src, fd_dest] = get_fds(ev);

	auto count_read_bytes = read(static_cast<int>(fd_src), buffer, max_buffer_size);
	
	if (count_read_bytes <= 0)
	{
		if (logger_proxy_)
			logger_proxy_->debug(std::string("closed connection: ") + std::to_string(fd_src));
		close(fd_src);
		if (logger_proxy_)
			logger_proxy_->debug(std::string("closed connection: ") + std::to_string(fd_dest));
		close(fd_dest);

		if (count_read_bytes == -1)
		{
			if (logger_proxy_)
				logger_proxy_->error(std::string("read error: ") + std::strerror(errno));
			return false;
		}
		return true;
	}

	auto count_write_bytes = write_data(static_cast<int>(fd_dest), std::string_view(buffer, count_read_bytes));

	if (count_write_bytes <= 0)
	{
		if (logger_proxy_)
			logger_proxy_->debug(std::string("closed connection: ") + std::to_string(fd_src));
		close(fd_src);
		if (logger_proxy_)
			logger_proxy_->debug(std::string("closed connection: ") + std::to_string(fd_dest));
		close(fd_dest);

		if (count_write_bytes == -1)
		{
			if (logger_proxy_)
				logger_proxy_->error(std::string("write error: ") + std::strerror(errno));
			return false;
		}
		return true;
	}

	if (logger_proxy_)
		logger_proxy_->debug(std::to_string(fd_src) + " -> "
			+ std::to_string(fd_dest) + "(" + std::to_string(count_write_bytes) + " bytes)");

	if (get_option(ev) == opt_client_to_server)
	{
		auto [success, count_bytes, query] = parse(std::string_view(buffer, count_read_bytes));
		if (success)
		{
			if (logger_query_)
				logger_query_->info(query);
			if (count_bytes != query.size() + 1 && logger_proxy_)
				logger_proxy_->warning(std::string("broken package (message size: ") + std::to_string(query.size()) + 
						"byte count: " + std::to_string(count_bytes));
		}
	}

	return true;
}