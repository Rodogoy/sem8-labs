cmake_minimum_required(VERSION 3.2)
project(Md5_Manager)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

include_directories(${BOOST_INCLUDE_DIRS})

add_executable(manager_app 
	main.cpp crack_hash_handler.cpp dispatcher.cpp models.cpp queue.cpp result_handler.cpp rr_balancer.cpp server.cpp status_handler.cpp worker_register_handler.cpp
	main.hpp httplib.h json.hpp)