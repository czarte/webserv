#include "core/Client.hpp"
#include "utils/Logger.hpp"

Client::Client()
    : fd(-1)
    , in_buf()
    , out_buf()
    , request()
    , config_index(0)
    , keep_alive(false)
    , last_activity_ms(0)
    , header_start_ms(0)
    , state(READING_HEADERS)
    , body_bytes_read(0)
    , body_bytes_expected(0)
	, chunked(false)
	, chunk_bytes_remaining(0)
	, chunk_reading_trailer(false)
	, cgi_request(false)
	, cgi_script_path()
	, cgi_bin_path()
	, cgi_path_info()
	, location(NULL)

{
}

Client::Client(int f)
    : fd(f)
    , in_buf()
    , out_buf()
    , request()
    , config_index(0)
    , keep_alive(false)
    , last_activity_ms(0)
    , header_start_ms(0)
    , state(READING_HEADERS)
    , body_bytes_read(0)
    , body_bytes_expected(0)
	, chunked(false)
	, chunk_bytes_remaining(0)
	, chunk_reading_trailer(false)
{
}

void logClient(const Client& conn)
{
	LOG_DBG << "=== Client Debug ==="
			<< " fd=" << conn.fd
			<< " state=" << (conn.state == Client::READING_HEADERS ? "READING_HEADERS" :
							 conn.state == Client::READING_BODY ? "READING_BODY" : "WRITING")
			<< " config_index=" << conn.config_index
			<< " keep_alive=" << (conn.keep_alive ? "true" : "false")
			<< " last_activity_ms=" << conn.last_activity_ms
			<< " header_start_ms=" << conn.header_start_ms
			<< " body_bytes_read=" << conn.body_bytes_read
			<< " body_bytes_expected=" << conn.body_bytes_expected
			<< " chunked=" << (conn.chunked ? "true" : "false")
			<< " chunk_bytes_remaining=" << conn.chunk_bytes_remaining
			<< " chunk_reading_trailer=" << (conn.chunk_reading_trailer ? "true" : "false")
			<< " cgi_request=" << (conn.cgi_request ? "true" : "false")
			<< " cgi_script_path=" << conn.cgi_script_path
			<< " cgi_bin_path=" << conn.cgi_bin_path
			<< " cgi_path_info=" << conn.cgi_path_info
			<< " in_buf.size=" << conn.in_buf.size()
			<< " out_buf.size=" << conn.out_buf.size();
}
