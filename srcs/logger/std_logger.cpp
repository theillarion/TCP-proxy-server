#include "logger/std_logger.hpp"
#include <string_view>

xk7::StdLogger::StdLogger(std::ostream& out, bool print_type_log)
	:	out_{ out }, print_type_log_{ print_type_log }
{

}

xk7::StdLogger::~StdLogger()
{

}

bool xk7::StdLogger::info(std::string_view str)
{
	return (out_ << (print_type_log_ ? "[INFO]: " : "") << str << std::endl).good();
}

bool xk7::StdLogger::debug(std::string_view str)
{
	return (out_ << (print_type_log_ ? "[DEBUG]: " : "") << str << std::endl).good();
}

bool xk7::StdLogger::warning(std::string_view str)
{
	return (out_ << (print_type_log_ ? "[WARNING]: " : "") << str << std::endl).good();
}

bool xk7::StdLogger::error(std::string_view str)
{
	return (out_ << (print_type_log_ ? "[ERROR]: " : "") << str << std::endl).good();
}

bool xk7::StdLogger::flush()
{
	return out_.flush().good();
}