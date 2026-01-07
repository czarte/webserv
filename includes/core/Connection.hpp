#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <cstddef>
#include <string>

#include "http/Request.hpp"

struct Connection
{
    enum State
	{
		READING_HEADERS,
		READING_BODY,
		WRITING
	};

    int fd;
    std::string in_buf;
    std::string out_buf;
    Request * request;
    size_t config_index;
    bool keep_alive;
    size_t last_activity_ms;
    size_t header_start_ms;
    State state;
    size_t body_bytes_read;
    size_t body_bytes_expected;

    Connection();
    explicit Connection(int f);
};


#endif
