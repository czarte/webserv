#ifndef HTTP_ERROR_PAGES_HPP
#define HTTP_ERROR_PAGES_HPP

#include <map>
#include <string>

class ErrorPages
{
public:
    ErrorPages() : enabled_(true) {}

    void setBaseDir(const std::string &dir) { base_dir_ = dir; }
    void setEnabled(bool on) { enabled_ = on; }
    void setOverride(int status, const std::string &path);

    std::string getPage(int status) const;

private:
    std::string base_dir_;
    bool enabled_;
    mutable std::map<int, std::string> cache_;
    std::map<int, std::string> overrides_;

    static std::string readFile(const std::string &path);
};

#endif
