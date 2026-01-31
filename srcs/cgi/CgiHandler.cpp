#include "cgi/CgiHandler.hpp"
#include "cgi/CgiProcess.hpp"
#include "utils/Logger.hpp"
#include <sstream>
#include <cstdlib>
#include <unistd.h>

CgiHandler::CgiHandler()
    : _pythonInterpreter("/usr/bin/python3"),
      _phpInterpreter("/usr/bin/php"),
      _documentRoot("/var/www"),
      _serverName("localhost"),
      _serverPort(8080),
      _hasError(false)
{
}

CgiHandler::~CgiHandler()
{
}

std::string CgiHandler::handleRequest(const Client& connection, const std::string& scriptPath)
{
    _hasError = false;
    _errorMessage.clear();

	LOG_DBG << "cgi: handleRequest method=" << connection.request.method
			<< " script=" << scriptPath;

    // Create CGI process
    CgiProcess cgiProcess;

    // Set script path
    cgiProcess.setScript(scriptPath);

    // Determine and set interpreter based on CGI method
    std::string interpreterPath = getInterpreterPath(connection.request.cgi);
    if (interpreterPath.empty())
    {
        _hasError = true;
        _errorMessage = "Unknown or unsupported CGI method";
        return "Status: 500 Internal Server Error\r\n\r\nCGI Error: Unsupported script type";
    }
    cgiProcess.setInterpreter(interpreterPath);
    setUploadPath(connection.location->getUploadPath());

    // Build CGI environment variables
    std::map<std::string, std::string> cgiEnv = buildCgiEnvironment(connection.request, scriptPath);
    cgiProcess.setEnvironment(cgiEnv);

    // If POST request with body, set input data
    if (connection.request.method_enum == METHOD_POST)
    {
        if (connection.body_to_file && connection.body_tmp_fd >= 0)
        {
            lseek(connection.body_tmp_fd, 0, SEEK_SET);
            cgiProcess.setInputFd(connection.body_tmp_fd);
        }
        else if (!connection.request.body.empty())
        {
            cgiProcess.setInputData(connection.request.body);
        }
    }

    // Execute the CGI script
	logClient(connection);
    std::string output = cgiProcess.execute();

    if (cgiProcess.hasError())
    {
        _hasError = true;
        _errorMessage = cgiProcess.getErrorMessage();
        return "Status: 500 Internal Server Error\r\n\r\nCGI Error: " + _errorMessage;
    }

    // Check if output contains headers, if not add default
    if (output.find("Content-Type:") == std::string::npos &&
        output.find("content-type:") == std::string::npos && connection.request.cgi != PHP)
    {
       output = "Content-Type: text/html\r\n\r\n" + output;
    }

    return output;
}

void CgiHandler::setPythonInterpreter(const std::string& path)
{
    _pythonInterpreter = path;
}

void CgiHandler::setPhpInterpreter(const std::string& path)
{
    _phpInterpreter = path;
}

void CgiHandler::setDocumentRoot(const std::string& root)
{
    _documentRoot = root;
}

void CgiHandler::setServerName(const std::string& name)
{
    _serverName = name;
}

void CgiHandler::setServerPort(int port)
{
    _serverPort = port;
}

void CgiHandler::setUploadPath(const std::string &path)
{
    _upload_path = path;
}

bool CgiHandler::hasError() const
{
    return _hasError;
}

std::string CgiHandler::getErrorMessage() const
{
    return _errorMessage;
}

std::map<std::string, std::string> CgiHandler::buildCgiEnvironment(const Request& request, const std::string& scriptPath)
{
    std::map<std::string, std::string> env;

    // CGI/1.1 standard variables
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_SOFTWARE"] = "WebServ/1.0";
    env["SERVER_NAME"] = _serverName;
    env["SERVER_PROTOCOL"] = request.version;
    env["REDIRECT_STATUS"] = "200";

    // Server port
    std::stringstream portStr;
    portStr << _serverPort;
    env["SERVER_PORT"] = portStr.str();

    // Request method
    env["REQUEST_METHOD"] = request.method;

    //upload path
    env["UPLOAD_PATH"] = _upload_path;

    // Script information
    env["SCRIPT_NAME"] = getScriptName(scriptPath);
    env["SCRIPT_FILENAME"] = scriptPath;

    // Path and query information
    std::string queryString = extractQueryString(request.target);
    std::string pathInfo = extractPathInfo(request.target);

    env["REQUEST_URI"] = request.target;
    env["QUERY_STRING"] = request.query;
    env["PATH_INFO"] = pathInfo;
    env["PATH_TRANSLATED"] = _documentRoot + pathInfo;

    // Parse query parameters into environment
    if (!queryString.empty())
    {
        parseUrlParameters(queryString, env);
    }

    // Content type and length for POST requests
    if (request.method_enum == METHOD_POST)
    {
        std::map<std::string, std::string>::const_iterator it = request.headers.find("content-type");
        if (it != request.headers.end())
            env["CONTENT_TYPE"] = it->second;

        std::stringstream contentLength;
        contentLength << request.content_length;
        env["CONTENT_LENGTH"] = contentLength.str();
    }

    // Additional headers
    for (std::map<std::string, std::string>::const_iterator it = request.headers.begin();
         it != request.headers.end(); ++it)
    {
        std::string headerName = "HTTP_";
        for (size_t i = 0; i < it->first.length(); ++i)
        {
            if (it->first[i] == '-')
                headerName += '_';
            else
                headerName += toupper(it->first[i]);
        }
        env[headerName] = it->second;
    }

    std::map<std::string, std::string>::const_iterator it_auth = request.headers.find("authorization");
    if (it_auth != request.headers.end())
    {
        env["AUTH_TYPE"] = "Basic";
        env["REMOTE_USER"] = "Admin";
        env["REMOTE_IDENT"] = "Admin";
    }

    // Remote address (if available)
    env["REMOTE_ADDR"] = "127.0.0.1";
    env["REMOTE_HOST"] = "localhost";

    return env;
}

std::string CgiHandler::getInterpreterPath(CGIMethod method)
{
    switch (method)
    {
        case Python:
            return _pythonInterpreter;
        case PHP:
            return _phpInterpreter;
        case Shell:
            return _pythonInterpreter;
        case Static:
            return _pythonInterpreter;
        default:
            return "";
    }
}

std::string CgiHandler::extractQueryString(const std::string& target)
{
    size_t pos = target.find('?');
    if (pos != std::string::npos)
        return target.substr(pos + 1);
    return "";
}

std::string CgiHandler::extractPathInfo(const std::string& target)
{
    std::string path = target;
    size_t pos = path.find('?');
    if (pos != std::string::npos)
        path = path.substr(0, pos);
    return path;
}

std::string CgiHandler::getScriptName(const std::string& scriptPath)
{
    size_t pos = scriptPath.rfind('?');
    if (pos != std::string::npos)
        return scriptPath.substr(pos + 1);
    return scriptPath;
}

void CgiHandler::parseUrlParameters(const std::string& queryString, std::map<std::string, std::string>& env)
{
    std::stringstream ss(queryString);
    std::string param;

    while (std::getline(ss, param, '&'))
    {
        size_t equalPos = param.find('=');
        if (equalPos != std::string::npos)
        {
            std::string key = param.substr(0, equalPos);
            std::string value = param.substr(equalPos + 1);

            std::string envKey = "QUERY_";
            for (size_t i = 0; i < key.length(); ++i)
            {
                envKey += toupper(key[i]);
            }
            env[envKey] = value;
        }
    }
}
