#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>

class Location
{
public:
    Location();
    Location(const std::string& path);
    ~Location();
    
    // Getters
    std::string getPath() const;
    std::string getRoot() const;
	std::string getAlias() const;
    std::string getIndex() const;
    bool getAutoindex() const;
    std::vector<std::string> getAllowedMethods() const;
    std::string getRedirect() const;
    std::vector<std::string> getCgiPath() const;
    std::vector<std::string> getCgiExt() const;
    std::string getUploadPath() const;
	bool isCgiEnabled() const;  // New: check if CGI is enabled
	std::string getCgiBinPath() const;  // New: get the actual CGI bin path
    
    // Setters
    void setPath(const std::string& path);
    void setRoot(const std::string& root);
	void setAlias(const std::string& alias);
	void setIndex(const std::string& index);
    void setAutoindex(bool autoindex);
    void addAllowedMethod(const std::string& method);
    void setRedirect(const std::string& redirect);
    void addCgiPath(const std::string& cgi_path);
    void addCgiExt(const std::string& cgi_ext);
    void setUploadPath(const std::string& upload_path);
	void setCgiEnabled(bool enabled);

private:
    std::string _path;
    std::string _root;
	std::string _alias;
    std::string _index;
    bool _autoindex;
    std::vector<std::string> _allowed_methods;
    std::string _redirect;
    std::vector<std::string> _cgi_path;
    std::vector<std::string> _cgi_ext;
    std::string _upload_path;
	bool _cgi_enabled;
};

#endif