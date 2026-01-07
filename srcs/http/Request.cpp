#include "http/Request.hpp"

Request::Request()
    : method()
    , method_enum(METHOD_UNKNOWN)
    , target()
    , version()
    , headers()
    , content_length(0)
    , has_body(false)
    , body()
{
}
