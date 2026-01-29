#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include "Location.hpp"

class Location;

struct ErrorPage
{
    int code;
    std::string path;
    
    ErrorPage();
};

class Config
{
public:
    Config();
    Config(int port, const std::string& server_name, const std::string& host,
           const std::string& root, int client_max_body_size, const std::string& index);
    ~Config();
    
    // Getters
    int getPort() const;
    std::string getServerName() const;
    std::vector<std::string> getServerNames() const;
    std::string getHost() const;
    std::string getRoot() const;
    std::string getCgiBinPath() const;
    int getClientMaxBodySize() const;
    std::string getIndex() const;
    ErrorPage getErrorPage() const;
    const std::vector<Location>& getLocations() const;
	std::vector<Location> getLocations();
    bool getSessionEnabled() const;
    
    // Setters
    void setPort(int port);
    void setServerName(const std::string& server_name);
    void addServerName(const std::string& server_name);
    void setHost(const std::string& host);
    void setRoot(const std::string& root);
    void setCgiBinPath(const std::string& path);
    void setClientMaxBodySize(int size);
    void setIndex(const std::string& index);
    void setErrorPage(const ErrorPage& error_page);
    void addLocation(const Location& location);
    void setSessionEnabled(bool enabled);
	void logDebug() const;

private:
    int _port;
    std::string _server_name;
    std::vector<std::string> _server_names;
    std::string _host;
    std::string _root;
    std::string _cgi_bin_path;
    int _client_max_body_size;
    std::string _index;
    ErrorPage _error_page;
    std::vector<Location> _locations;
    bool _session_enabled;

};

#endif
