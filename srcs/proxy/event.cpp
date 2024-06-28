#include "proxy/event.hpp"

epoll_event xk7::create_event(std::uint32_t fd_src, std::uint32_t fd_dest, bool option, int events)
{
	// Предполагается, что int не больше 4 байт,
	// иначе необходимо хранить fd_src, fd_dest и option
	// в днамической памяти, в ev.data.ptr
	static_assert(sizeof(int) <= 4);

	epoll_event ev{};

	// Первые 32 бита под fd_src
	ev.data.u64 |= fd_src;
	// Следующие 31 бит под fd_dest
	// (31 бит т.к. fd является типом int, а значит его положительная часть ограничена 31 битами)
	ev.data.u64 |= (static_cast<std::uint64_t>(fd_dest) << 32);
	// Последний бит под option
	ev.data.u64 |= (static_cast<std::uint64_t>(option) << 63);

	ev.events = events;

	return ev;
}

std::uint32_t xk7::get_fd_src(const epoll_event& ev)
{
	return ev.data.u64 & mask_31bit;
}

std::uint32_t xk7::get_fd_dest(const epoll_event& ev)
{
	return (ev.data.u64 >> 32) & mask_31bit;
}

std::pair<std::uint32_t, std::uint32_t> xk7::get_fds(const epoll_event& ev)
{
	return { get_fd_src(ev), get_fd_dest(ev) };
}

bool xk7::get_option(const epoll_event& ev)
{
	return ev.data.u64 & mask_63th_bit;
}