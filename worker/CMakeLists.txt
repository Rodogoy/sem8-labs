cmake_minimum_required(VERSION 3.2)
project(Md5_Worker)

find_package(OpenSSL REQUIRED)

message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_executable(worker_app 
	server.cpp registration.cpp MD5cracker.cpp main.cpp handler_crack_task.cpp crack_task.cpp config.cpp
	main.hpp json.hpp httplib.h)

target_include_directories(worker_app PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(worker_app PRIVATE OpenSSL::SSL OpenSSL::Crypto)