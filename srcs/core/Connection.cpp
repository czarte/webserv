#include "core/Connection.hpp"

Connection::Connection()
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
    , body_bytes_expected(0),
	  cgi_request(false),
	  cgi_script_path(),
	  cgi_bin_path(),
	  cgi_path_info(),
	  location(NULL)

{
}

Connection::Connection(int f)
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
{
}
