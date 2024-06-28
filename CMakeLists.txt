cmake_minimum_required(VERSION 3.16)
project(proxy)

set(CMAKE_CXX_STANDARD_REQUIRED 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCE_FILES
	srcs/logger/alogger.cpp
	srcs/logger/std_logger.cpp
	srcs/proxy/event.cpp
	srcs/proxy/parse.cpp
	srcs/proxy/proxy.cpp
	srcs/main.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE srcs)