cmake_minimum_required(VERSION 3.31)
project(webserv)

set(CMAKE_CXX_STANDARD 98)

set(SRCS
        srcs/main.cpp
        srcs/core/Server.cpp
        srcs/core/EventLoop.cpp
        srcs/config/Config.cpp
        srcs/config/ConfigParser.cpp
        srcs/config/WebservMaster.cpp
        srcs/http/Parser.cpp
        srcs/http/Request.cpp
        srcs/http/Response.cpp
        srcs/io/Socket.cpp
        srcs/io/File.cpp
        srcs/io/NonBlocking.cpp
        srcs/cgi/CgiHandler.cpp
        srcs/cgi/CgiProcess.cpp
        srcs/utils/Log.cpp
        srcs/utils/String.cpp)

add_executable(webserv ${SRCS})
target_include_directories(webserv PRIVATE includes)
