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
    , state(READING)
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
    , state(READING)
{
}
