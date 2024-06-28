#include "logger/alogger.hpp"

#include <cstdint>
#include <string_view>

xk7::ALogger::~ALogger() = default;

namespace
{
	using func_type = bool(xk7::ALogger::*)(std::string_view);
	constexpr func_type functions[]
	{
		&xk7::ALogger::info,
		&xk7::ALogger::debug,
		&xk7::ALogger::warning,
		&xk7::ALogger::error
	};

	inline constexpr std::int8_t count_functions = sizeof(functions) / sizeof(*functions);
}

bool xk7::ALogger::log(TypeLog type, std::string_view str)
{
	static_assert(count_functions == count_types_log, "internal error");

	return (this->*functions[static_cast<std::uint8_t>(type)])(str);
}
