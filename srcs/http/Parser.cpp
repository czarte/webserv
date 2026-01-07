#include "http/Parser.hpp"

#include <cctype>
#include <sstream>
#include <cstdlib>
#include <cerrno>
#include <limits>

namespace
{
    std::string trim(const std::string &s)
    {
        std::string::size_type a = 0;
        while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a])))
            ++a;

        std::string::size_type b = s.size();
        while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])))
            --b;

        return s.substr(a, b - a);
    }

    std::string toLower(const std::string &s)
    {
        std::string out = s;
        for (size_t i = 0; i < out.size(); ++i)
            out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
        return out;
    }

    HttpMethod parseMethod(const std::string &m)
    {
        if (m == "GET")
            return METHOD_GET;
        if (m == "POST")
            return METHOD_POST;
        if (m == "PUT")
            return METHOD_PUT;
        if (m == "DELETE")
            return METHOD_DELETE;
        return METHOD_UNKNOWN;
    }

bool split3(const std::string &line, std::string &a, std::string &b, std::string &c)
{
    std::string::size_type i = 0;
    while (i < line.size() && line[i] == ' ')
        ++i;
    std::string::size_type j = line.find(' ', i);
    if (j == std::string::npos)
        return false;
    a = line.substr(i, j - i);

    while (j < line.size() && line[j] == ' ')
        ++j;
    i = j;
    j = line.find(' ', i);
    if (j == std::string::npos)
        return false;
    b = line.substr(i, j - i);

    while (j < line.size() && line[j] == ' ')
        ++j;
    c = line.substr(j);
    return !a.empty() && !b.empty() && !c.empty();
}
}

Parser::Parser()
{
}

bool Parser::hasCompleteHeaders(const std::string &buf) const
{
    return buf.find("\r\n\r\n") != std::string::npos;
}

int Parser::parseRequestLine(const std::string &line, Request &out) const
{
    std::string method, target, version;
    if (!split3(line, method, target, version))
        return 400;

    out.method = method;
    out.method_enum = parseMethod(method);
    out.target = target;
    out.version = version;

    if (version != "HTTP/1.1" && version != "HTTP/1.0")
        return 400;

    if (out.method_enum == METHOD_UNKNOWN)
        return 405;

    return 200;
}

Parser::Result Parser::parseOne(std::string &in_buf, Request &req, int &status, std::string &err) const
{
    req = Request();
    status = 200;
    err.clear();

    std::string::size_type end = in_buf.find("\r\n\r\n");
    if (end == std::string::npos)
        return NEED_MORE;

    std::string block = in_buf.substr(0, end + 4);
    in_buf.erase(0, end + 4);

    std::string::size_type line_end = block.find("\r\n");
    if (line_end == std::string::npos)
    {
        err = "no CRLF in request line";
        status = 400;
        return PARSE_ERROR;
    }

    std::string request_line = block.substr(0, line_end);
    status = parseRequestLine(request_line, req);
    if (status == 405)
        return PARSED_OK;
    if (status != 200)
    {
        err = "bad request line";
        return PARSE_ERROR;
    }

    std::string::size_type pos = line_end + 2;
    while (pos < block.size())
    {
        std::string::size_type next = block.find("\r\n", pos);
        if (next == std::string::npos)
            break;

        if (next == pos)
            break;

        std::string line = block.substr(pos, next - pos);
        pos = next + 2;

        std::string::size_type colon = line.find(':');
        if (colon == std::string::npos)
        {
            err = "header without colon";
            status = 400;
            return PARSE_ERROR;
        }

        std::string key = toLower(trim(line.substr(0, colon)));
        std::string val = trim(line.substr(colon + 1));
        if (key.empty())
        {
            err = "empty header key";
            status = 400;
            return PARSE_ERROR;
        }

        req.headers[key] = val;
    }
     // validate headers fields : check all bad cases in the header fields
    std::map<std::string, std::string>::iterator it = req.headers.find("content-length");
    if (it != req.headers.end())
    {
        const std::string &val = it->second;
        errno = 0;
        char *end = 0;
        unsigned long long len = std::strtoull(val.c_str(), &end, 10);
        while (end && *end && std::isspace(static_cast<unsigned char>(*end)))
            ++end;
        if (errno != 0 || !end || *end != '\0' || len > std::numeric_limits<size_t>::max())
        {
            err = "invalid content-length";
            status = 400;
            return PARSE_ERROR;
        }
        req.content_length = static_cast<size_t>(len);
        req.has_body = (req.content_length > 0);
    }

    return PARSED_OK;
}
