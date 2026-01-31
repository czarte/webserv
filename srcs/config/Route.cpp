#include "config/Route.hpp"
#include "utils/Logger.hpp"

Location matchLocation(const Config &config, const std::string &target)
{
	config.logDebug();
    const std::vector<Location>& locations = config.getLocations();
    bool found = false;
    Location best;
    size_t best_len = 0;
    for (size_t i = 0; i < locations.size(); ++i)
    {
        const std::string& path = locations[i].getPath();
		LOG_DBG << "LOG_DBG matchLocation path: " << path << " target: " << target << " compare: " << target.compare(0, path.size(), path);
        if (path.empty())
            continue;
        bool prefix_match = false;
        if (target.size() >= path.size())
            prefix_match = (target.compare(0, path.size(), path) == 0);
        if (!prefix_match && path.size() > 1 && path[path.size() - 1] == '/')
        {
            std::string path_no_slash = path.substr(0, path.size() - 1);
            if (target == path_no_slash)
                prefix_match = true;
        }
        bool boundary_ok = false;
        if (path == "/")
            boundary_ok = true;
        else if (!path.empty() && path[path.size() - 1] == '/')
        {
            boundary_ok = true;
            if (target.size() + 1 == path.size()
                && path.compare(0, target.size(), target) == 0)
            {
                boundary_ok = true;
            }
        }
        else
            boundary_ok = (target.size() == path.size()
                || (target.size() > path.size() && target[path.size()] == '/'));
        if (prefix_match && boundary_ok && path.size() >= best_len)
        {
			best = locations[i];
			best_len = path.size();
            found = true;
        }
    }
    if (!found)
    {
        LOG_DBG << "matchLocation: no match for target=" << target;
    }
	return best;
}
