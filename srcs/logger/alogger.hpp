#pragma once

#include <cstdint>
#include <string_view>

namespace xk7
{
	enum class TypeLog
	{
		Info,
		Debug,
		Warning,
		Error,
		// CriticalError	// unsupported
	};

	inline constexpr std::uint8_t count_types_log = static_cast<std::uint8_t>(TypeLog::Error) + 1; 

	class ALogger
	{
	public:
		virtual bool info(std::string_view str) = 0;
		virtual bool debug(std::string_view str) = 0;
		virtual bool warning(std::string_view str) = 0;
		virtual bool error(std::string_view str) = 0;
		virtual bool flush() = 0;
		virtual ~ALogger() = 0;

		virtual bool log(TypeLog type, std::string_view str);
	};
} // namespace xk7
