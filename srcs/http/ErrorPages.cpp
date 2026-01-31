#include "http/ErrorPages.hpp"

#include <fstream>
#include <sstream>

std::string ErrorPages::readFile(const std::string &path)
{
    std::ifstream f(path.c_str(), std::ios::in | std::ios::binary);
    if (!f)
        return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string ErrorPages::getPage(int status) const
{
    if (!enabled_)
        return "";

    std::map<int, std::string>::iterator it = cache_.find(status);
    if (it != cache_.end())
        return it->second;

    std::map<int, std::string>::const_iterator ov = overrides_.find(status);
    if (ov != overrides_.end())
    {
        std::string html = readFile(ov->second);
        cache_[status] = html;
        return html;
    }

    if (base_dir_.empty())
        return "";

    std::ostringstream path;
    path << base_dir_;
    if (!base_dir_.empty() && base_dir_[base_dir_.size() - 1] != '/')
        path << "/";
    path << status << ".html";

    std::string html = readFile(path.str());
    if (html.empty())
    {
        std::string def = base_dir_;
        if (!def.empty() && def[def.size() - 1] != '/')
            def += "/";
        def += "default.html";
        html = readFile(def);
    }

    cache_[status] = html;
    return html;
}

void ErrorPages::setOverride(int status, const std::string &path)
{
    overrides_[status] = path;
    cache_.erase(status);
}
