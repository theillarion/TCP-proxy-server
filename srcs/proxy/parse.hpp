#pragma once

#include <cstdint>
#include <string_view>
#include <tuple>

namespace xk7
{
	std::tuple<bool, std::uint64_t, std::string_view> parse(std::string_view raw_data);
} // namespace xk7