cmake_minimum_required(VERSION 3.12)
project(Md5_Manager)

find_package(OpenSSL REQUIRED)

find_package(rabbitmq-c QUIET)
find_library(AMQPCPP_LIB amqpcpp)
find_path(AMQPCPP_INCLUDE_DIR amqpcpp.h)

find_library(AMQPCPP_TCP_LIB
    NAMES amqpcpp_tcp
    PATHS ${AMQPCPP_LIB_DIR}
    NO_DEFAULT_PATH
) 
    
find_path(EV_INCLUDE_DIR ev.h
    PATHS /usr/include /usr/local/include
)
find_library(EV_LIBRARY
    NAMES ev
    PATHS /usr/lib /usr/local/lib
)

find_package(bsoncxx REQUIRED)
find_package(mongocxx REQUIRED)
find_package(libmongocxx REQUIRED)

include_directories(
    /usr/local/include/mongocxx/v_noabi
    /usr/local/include/bsoncxx/v_noabi
    ${PROJECT_SOURCE_DIR}/include
    /usr/local/include
)

message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
message(STATUS "RabbitMQ-C found: ${rabbitmq-c_FOUND}")
message(STATUS "AMQP-CPP lib: ${AMQPCPP_LIB}")
message(STATUS "AMQP-CPP includes: ${AMQPCPP_INCLUDE_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(manager_app 
	main.cpp crack_hash_handler.cpp dispatcher.cpp models.cpp queue.cpp result_handler.cpp server.cpp status_handler.cpp inits.cpp
	handlers.hpp dispatcher.hpp models.hpp queue.hpp server.hpp json.hpp httplib.h)


target_include_directories(manager_app PRIVATE
    ${AMQPCPP_INCLUDE_DIR}
    ${RABBITMQ_C_INCLUDE_DIRS}
)

target_link_libraries(manager_app PRIVATE 
	OpenSSL::SSL 
	OpenSSL::Crypto 
	${AMQPCPP_LIB} 
    amqpcpp
    ssl
    /usr/local/lib/libmongocxx.so
    /usr/local/lib/libbsoncxx.so
    /usr/local/lib/libmongocxx.so._noabi
    /usr/local/lib/libmongocxx.so.3.8.0
    crypto
    rabbitmq
    ev)

if(NOT AMQPCPP_LIB)
    message(WARNING "AMQP-CPP main library not found at ${AMQPCPP_LIB_DIR}")
else()
    message(STATUS "Found AMQP-CPP main library: ${AMQPCPP_LIB}")
endif()

if(NOT AMQPCPP_TCP_LIB)
    message(WARNING "AMQP-CPP TCP library not found at ${AMQPCPP_LIB_DIR}")
else()
    message(STATUS "Found AMQP-CPP TCP library: ${AMQPCPP_TCP_LIB}")
endif()

if(EV_INCLUDE_DIR AND EV_LIBRARY)
    message(STATUS "Found libev: includes at ${EV_INCLUDE_DIR}, library at ${EV_LIBRARY}")
else()
    message(FATAL_ERROR "libev not found! Install libev-dev: 'sudo apt-get install libev-dev'")
endif()

if(AMQPCPP_LIB)
    target_link_libraries(manager_app PRIVATE ${AMQPCPP_LIB})
    message(STATUS "найден клиент RabbitMQ")
elseif(rabbitmq-c_FOUND)
    target_link_libraries(manager_app PRIVATE rabbitmq::rabbitmq)
else()
    message(FATAL_ERROR "Не найден клиент RabbitMQ")
endif()