#ifndef RESPONSE_BUILDER_HPP
#define RESPONSE_BUILDER_HPP

#include <map>
#include <string>

class ErrorPages;
#include "core/Client.hpp"
#include "config/Location.hpp"

std::string buildResponse(int status, const std::string &body, bool keep_alive,
                          const std::string &content_type);
std::string buildResponse(int status, const std::string &body, bool keep_alive,
                          const std::string &content_type, bool include_body);
std::string buildResponse(int status, const std::string &body, bool keep_alive,
                          const std::string &content_type, bool include_body,
                          const std::map<std::string, std::string> &extra_headers);
std::string buildErrorResponse(int status, bool keep_alive, std::string message);
std::string buildErrorResponse(int status, bool keep_alive, std::string message,
                               const ErrorPages *pages);
std::string buildErrorResponse(int status, bool keep_alive, std::string message,
                               const ErrorPages *pages, bool include_body);
std::string contentTypeForPath(const std::string &path);
bool determineCgiRequest(Client &conn, const Location *loc, const std::string &uri);
std::string getFileExtension(const std::string &path);
std::string buildCgiResponse(const std::string &cgi_headers, const std::string &body,
							 bool keep_alive);
std::string buildCgiResponse(const std::string &cgi_headers, const std::string &body,
							 bool keep_alive, bool include_body);

#endif // RESPONSE_BUILDER_HPP
