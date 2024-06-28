#pragma once

#include "logger/alogger.hpp"

#include <ostream>
#include <string_view>

namespace xk7
{
	class StdLogger : public ALogger
	{
	public:
		explicit StdLogger(std::ostream& out, bool print_type_log = true);
		~StdLogger() override;

		StdLogger(const StdLogger&) = delete;
		StdLogger(StdLogger&&) noexcept = default;

		StdLogger& operator=(const StdLogger&) = delete;
		StdLogger& operator=(StdLogger&&) noexcept = default;

		bool info(std::string_view str) override;
		bool debug(std::string_view str) override;
		bool warning(std::string_view str) override;
		bool error(std::string_view str) override;
		bool flush() override;

	protected:
		std::ostream& out_;
		bool print_type_log_;
	};
} // namespace xk7
