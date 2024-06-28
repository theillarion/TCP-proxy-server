#pragma once

#include <cstdint>
#include <utility>
#include <sys/epoll.h>

namespace xk7
{
	// Опция - данные пришли с клиента и должны быть отправлены серверу
	constexpr bool opt_client_to_server = false;
	// Опция - данные пришли с сервера и должны быть отправлены клиенту
	// Сервер не отправляет клиенту SQL запросы, а значит, такие пакеты можно сразу пропускать
	constexpr bool opt_server_to_client = true;

	// Маска с установлеными битами с 1 по 31 бит
	constexpr std::uint64_t mask_31bit = 0x7FFFFFFF;
	// Маска с установленным 64 битом (последний бит)
	constexpr std::uint64_t mask_63th_bit = 0x8000000000000000;
}

namespace xk7
{
	epoll_event create_event(std::uint32_t fd_src, std::uint32_t fd_dest, bool option, int events);

	// Изъятие первого fd (источника)
	std::uint32_t get_fd_src(const epoll_event& ev);
	// Изъятие второго fd (назначение)
	std::uint32_t get_fd_dest(const epoll_event& ev);
	// Изъятие обоих fd
	std::pair<std::uint32_t, std::uint32_t> get_fds(const epoll_event& ev);
	// Изъятие опции
	bool get_option(const epoll_event& ev);
}