cmake_minimum_required(VERSION 3.16)
project(webserv LANGUAGES CXX)

# ============================================================================
# C++ Standard and Compiler Settings
# ============================================================================
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# Compiler Flags
# ============================================================================
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "-g3")

# Release flags
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# ============================================================================
# Source Files
# ============================================================================

# Core sources
set(CORE_SRCS
        srcs/core/server/ServerCore.cpp
        srcs/core/server/ServerPoll.cpp
        srcs/core/server/ServerClient.cpp
        srcs/core/server/ServerDispatch.cpp
        srcs/core/Fd.cpp
        srcs/core/Client.cpp
        srcs/core/EventLoop.cpp
)

# HTTP sources
set(HTTP_SRCS
        srcs/http/Parser.cpp
        srcs/http/Request.cpp
        srcs/http/Response.cpp
        srcs/http/ResponseBuilder.cpp
)

# Config sources
set(CONFIG_SRCS
        srcs/config/ConfigParser.cpp
        srcs/config/Config.cpp
        srcs/config/Location.cpp
        srcs/config/Route.cpp
        srcs/config/WebservMaster.cpp
)

# IO sources
set(IO_SRCS
        srcs/io/Socket.cpp
        srcs/io/File.cpp
        srcs/io/NonBlocking.cpp
        srcs/io/FileSystem.cpp
)

# CGI sources
set(CGI_SRCS
        srcs/cgi/CgiHandler.cpp
        srcs/cgi/CgiProcess.cpp
)

# Utility sources
set(UTIL_SRCS
        srcs/utils/Log.cpp
        srcs/utils/Logger.cpp
        srcs/utils/String.cpp
        srcs/utils/Time.cpp
        srcs/utils/Path.cpp
)

# All sources combined
set(ALL_SRCS
        srcs/main.cpp
        ${CORE_SRCS}
        ${HTTP_SRCS}
        ${CONFIG_SRCS}
        ${IO_SRCS}
        ${CGI_SRCS}
        ${UTIL_SRCS}
)

# ============================================================================
# Main Executable
# ============================================================================
add_executable(webserv ${ALL_SRCS})
target_include_directories(webserv PRIVATE ${CMAKE_SOURCE_DIR}/includes)

# ============================================================================
# Test Executables
# ============================================================================

# Config parser test
set(TEST_CONFIG_PARSER_SRCS
        tests/milestone4/test_config_parser.cpp
        ${CONFIG_SRCS}
        ${UTIL_SRCS}
        ${IO_SRCS}
)

add_executable(test_config_parser ${TEST_CONFIG_PARSER_SRCS})
target_include_directories(test_config_parser PRIVATE ${CMAKE_SOURCE_DIR}/includes)

# ============================================================================
# Custom Targets
# ============================================================================

# Sanitize build target (for AddressSanitizer)
add_custom_target(sanitize
        COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug
        -DCMAKE_CXX_FLAGS="-g3 -fsanitize=address -fno-omit-frame-pointer"
        ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} --build .
        COMMENT "Building with AddressSanitizer enabled"
)
