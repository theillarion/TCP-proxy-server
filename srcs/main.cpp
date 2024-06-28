#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include "logger/alogger.hpp"
#include "logger/std_logger.hpp"
#include "proxy/proxy.hpp"

int main(int argc, char** argv)
{
	if (argc != 5)
	{
		std::cerr << "wrong number of arguments" << std::endl;
		return 1;
	}
	std::vector<xk7::ALogger*> loggers;

	std::ofstream file("query.log", std::ios_base::binary | std::ios_base::app);

	if (!file)
	{
		std::cerr << "File cannot be created" << std::endl;
		return 1;
	}

	auto logger_query = std::make_shared<xk7::StdLogger>(file, false);		
	auto logger_info = std::make_shared<xk7::StdLogger>(std::clog);

	xk7::Proxy proxy(logger_query, logger_info, argv[3], argv[4]);

	proxy.bind(argv[1], argv[2]);
	proxy.run();

	file.close();;

	return 0;
}