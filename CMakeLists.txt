cmake_minimum_required(VERSION 3.31)
project(webserv)

set(CMAKE_CXX_STANDARD 98)

add_executable(webserv
        srcs/WebservMaster.cpp
        includes/WebservMaster.h
        srcs/main.cpp)
