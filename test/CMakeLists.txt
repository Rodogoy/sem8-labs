cmake_minimum_required(VERSION 3.2)
project(Md5_Worker)

set(SFML_VERSION_REQUIRED "2.5.1")

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui CACHE PATH "Path to ImGui")
set(IMGUI_SFML_DIR ${CMAKE_SOURCE_DIR}/external/imgui-sfml CACHE PATH "Path to ImGui-SFML")

# Проверка наличия ImGui
if(NOT EXISTS ${IMGUI_DIR}/imgui.h)
    message(FATAL_ERROR "ImGui not found at ${IMGUI_DIR}")
endif()

find_package(OpenSSL REQUIRED)
find_package(SFML ${SFML_VERSION_REQUIRED} COMPONENTS graphics window system REQUIRED)

message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_executable(launcher_app 
	main.cpp
	json.hpp httplib.h)

target_include_directories(launcher_app PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(launcher_app PRIVATE OpenSSL::SSL OpenSSL::Crypto)