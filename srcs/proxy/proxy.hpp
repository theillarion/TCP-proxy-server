#pragma once

#include "logger/alogger.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

#include <sys/epoll.h>
#include <sys/types.h>

namespace xk7
{
	
} // namespace xk7

namespace xk7
{
	class Proxy
	{
	public:
		explicit Proxy(std::shared_ptr<ALogger> logger_query, std::shared_ptr<ALogger> logger_proxy,
			std::string_view server_host = "127.0.0.1", std::string_view server_port = "5432");

		bool bind(std::string_view proxy_host, std::string_view proxy_port);

		bool run(std::function<bool()> exit_conditional = [](){ return false; });

	private:
		int create_proxy(std::string_view host, std::string_view port, std::size_t max_connection);

		bool set_nonblocking(int fd);

		int create_client_conn(int fd_listener);
		int create_server_conn();

		std::pair<int, int> create_client(int fd_epoll, int fd_listener);

		std::int64_t write_data(int fd_socket, std::string_view buff);

		bool handle_transfer_data(epoll_event& event);
		
	private:
		std::shared_ptr<ALogger> logger_query_;
		std::shared_ptr<ALogger> logger_proxy_;

		// Адрес и порт СУБД, к которой будет подключаться прокси
		std::string_view server_host_;
		std::string_view server_port_;

		static constexpr int code_error = -1;

		// Максимальное количество одновременно активных пользователей
		static constexpr std::size_t max_connections = 100;

		// Максимальный размер буфера
		static constexpr std::size_t max_buffer_size = 65536;

		// События, приходящие из epoll_wait.
		// Можно сделать динамический массив, если максимальное количество
		// одновременно активных пользователей неизвестно на этапе компиляции
		epoll_event events[max_connections];

		int fd_listener{0};
		int fd_epoll{0};
	};
} // namespace xk7