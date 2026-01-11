#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <map>
#include "core/Connection.hpp"

class CgiHandler
{
public:
    CgiHandler();
    ~CgiHandler();

    // Main method to handle CGI request
    std::string handleRequest(const Connection& connection, const std::string& scriptPath);

    // Configuration methods
	std::string getScriptName(const std::string& scriptPath);
    void setPythonInterpreter(const std::string& path);
    void setPhpInterpreter(const std::string& path);
    void setDocumentRoot(const std::string& root);
    void setServerName(const std::string& name);
    void setServerPort(int port);

    // Check if the handler had an error
    bool hasError() const;
    std::string getErrorMessage() const;

private:
    std::string _pythonInterpreter;
    std::string _phpInterpreter;
    std::string _documentRoot;
    std::string _serverName;
    int _serverPort;
    bool _hasError;
    std::string _errorMessage;

    // Helper methods
    std::map<std::string, std::string> buildCgiEnvironment(const Request& request, const std::string& scriptPath);
    std::string getInterpreterPath(CGIMethod method);
    std::string extractQueryString(const std::string& target);
    std::string extractPathInfo(const std::string& target);
    void parseUrlParameters(const std::string& queryString, std::map<std::string, std::string>& env);
};

#endif
