#include "http/ResponseBuilder.hpp"
#include "http/ErrorPages.hpp"
#include "core/ServerInternal.hpp"
#include "utils/Logger.hpp"

#include <cctype>
#include <sstream>

namespace
{
std::string toLower(const std::string &s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

const char *statusMessage(int status)
{
    switch (status)
    {
    case 201: return "Created";
    case 200: return "OK";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 413: return "Payload Too Large";
    case 500: return "Internal Server Error";
    default:  return "Error";
    }
}

std::string replaceAll(std::string s, const std::string &from, const std::string &to)
{
    if (from.empty())
        return s;
    std::string::size_type pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos)
    {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::string htmlEscape(const std::string &in)
{
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i)
    {
        char c = in[i];
        if (c == '&')
            out += "&amp;";
        else if (c == '<')
            out += "&lt;";
        else if (c == '>')
            out += "&gt;";
        else if (c == '"')
            out += "&quot;";
        else if (c == '\'')
            out += "&#39;";
        else
            out += c;
    }
    return out;
}

std::string buildDefaultErrorHtml(int status, const std::string &msg)
{
    std::ostringstream b;
    b << "<!doctype html><html><head><meta charset=\"utf-8\">"
      << "<title>" << status << " " << statusMessage(status) << "</title>"
      << "</head><body>"
      << "<h1>" << status << " " << statusMessage(status) << "</h1>";
    if (!msg.empty())
        b << "<pre>" << htmlEscape(msg) << "</pre>";
    b << "</body></html>";
    return b.str();
}
}

std::string contentTypeForPath(const std::string &path)
{
    std::string::size_type dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "text/plain";
    std::string ext = toLower(path.substr(dot + 1));
    if (ext == "html" || ext == "htm")
        return "text/html";
    if (ext == "css")
        return "text/css";
    if (ext == "js")
        return "application/javascript";
    if (ext == "png")
        return "image/png";
    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "gif")
        return "image/gif";
    if (ext == "txt")
        return "text/plain";
    return "application/octet-stream";
}

std::string buildResponse(int status, const std::string &body, bool keep_alive,
                          const std::string &content_type)
{
    return buildResponse(status, body, keep_alive, content_type, true);
}

std::string buildResponse(int status, const std::string &body, bool keep_alive,
                          const std::string &content_type, bool include_body)
{
    std::map<std::string, std::string> empty;
    return buildResponse(status, body, keep_alive, content_type, include_body, empty);
}

std::string buildResponse(int status, const std::string &body, bool keep_alive,
                          const std::string &content_type, bool include_body,
                          const std::map<std::string, std::string> &extra_headers)
{
    std::ostringstream out;
    out << "HTTP/1.1 " << status << " " << statusMessage(status) << "\r\n";
    out << "Content-Length: " << body.size() << "\r\n";
    out << "Content-Type: " << content_type << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = extra_headers.begin();
         it != extra_headers.end(); ++it)
    {
        out << it->first << ": " << it->second << "\r\n";
    }
    out << "Connection: " << (keep_alive ? "keep-alive" : "close") << "\r\n";
    out << "\r\n";
    if (include_body)
        out << body;
    return out.str();
}

std::string buildErrorResponse(int status, bool keep_alive, std::string message)
{
    return buildErrorResponse(status, keep_alive, message, 0, true);
}

std::string buildErrorResponse(int status, bool keep_alive, std::string message,
                               const ErrorPages *pages)
{
    return buildErrorResponse(status, keep_alive, message, pages, true);
}

std::string buildErrorResponse(int status, bool keep_alive, std::string message,
                               const ErrorPages *pages, bool include_body)
{
    std::string html;
    if (pages)
    {
        html = pages->getPage(status);
        if (!html.empty())
        {
            std::ostringstream ss;
            ss << status;
            html = replaceAll(html, "{{STATUS}}", ss.str());
            html = replaceAll(html, "{{REASON}}", statusMessage(status));
            html = replaceAll(html, "{{MESSAGE}}", htmlEscape(message));
        }
    }

    if (html.empty())
        html = buildDefaultErrorHtml(status, message);

    return buildResponse(status, html, keep_alive, "text/html", include_body);
}

std::string buildCgiResponse(const std::string &cgi_headers, const std::string &body,
							 bool keep_alive)
{
	return buildCgiResponse(cgi_headers, body, keep_alive, true);
}

std::string buildCgiResponse(const std::string &cgi_headers, const std::string &body,
							 bool keep_alive, bool include_body)
{
	std::ostringstream response;

	// Start with HTTP status line
	// Check if CGI provided Status header
	std::string status_line = "HTTP/1.1 200 OK";
	size_t status_pos = cgi_headers.find("Status:");
	if (status_pos != std::string::npos)
	{
		size_t end_pos = cgi_headers.find("\n", status_pos);
		std::string status = cgi_headers.substr(status_pos + 7, end_pos - status_pos - 7);
		status = serverutil::trim(status);
		status_line = "HTTP/1.1 " + status;
	}

	response << status_line << "\r\n";

	// Add CGI headers (except Status which we already processed)
	std::istringstream header_stream(cgi_headers);
	std::string header_line;
	bool has_content_type = false;
	bool has_content_length = false;

	while (std::getline(header_stream, header_line))
	{
		header_line = serverutil::trim(header_line);
		if (header_line.empty())
			continue;

		// Skip Status header as we already processed it
		if (header_line.find("Status:") == 0)
			continue;

		if (header_line.find("Content-Type:") == 0 ||
			header_line.find("content-type:") == 0)
			has_content_type = true;

		if (header_line.find("Content-Length:") == 0 ||
			header_line.find("content-length:") == 0)
			has_content_length = true;

		response << header_line << "\r\n";
	}

	// Add default headers if not provided by CGI
	if (!has_content_type)
		response << "Content-Type: text/html\r\n";

	if (!has_content_length)
		response << "Content-Length: " << body.size() << "\r\n";

	// Add connection header
	response << "Connection: " << (keep_alive ? "keep-alive" : "close") << "\r\n";

	// End headers
	response << "\r\n";

	// Add body
	if (include_body)
		response << body;

	return response.str();
}

std::string getFileExtension(const std::string &path)
{
	size_t dot_pos = path.find_last_of('.');
	if (dot_pos == std::string::npos)
		return "";
	std::string fromdot = path.substr(dot_pos);
	size_t and_pos = fromdot.find_first_of('&');
	if (and_pos == std::string::npos)
		return fromdot;
	return fromdot.substr(0, and_pos);
}
