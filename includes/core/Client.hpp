#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <cstddef>
#include <string>

#include "http/Request.hpp"
#include "config/Location.hpp"

struct Client
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
    Request request;
    size_t config_index;
    bool keep_alive;
    size_t last_activity_ms;
    size_t header_start_ms;
    State state;
	size_t body_bytes_read;
	size_t body_bytes_expected;
	bool chunked;
	size_t chunk_bytes_remaining;
	bool chunk_reading_trailer;
	bool cgi_request;           // New: indicates if this is a CGI request
	std::string cgi_script_path; // New: full path to the CGI script
	std::string cgi_bin_path;   // New: CGI bin directory
	std::string cgi_path_info;   // New: PATH_INFO for CGI
	const Location* location;    // New: pointer to matched location

    Client();
    explicit Client(int f);

	void resetCgiInfo()
	{
		cgi_request = false;
		cgi_script_path.clear();
		cgi_bin_path.clear();
		cgi_path_info.clear();
		location = NULL;
	}
};

void logClient(const Client& conn);


#endif
