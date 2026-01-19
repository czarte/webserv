#include "config/Route.hpp"
#include "utils/Logger.hpp"

Location matchLocation(const Config &config, const std::string &target)
{
	config.logDebug();
    std::vector<Location> locations = config.getLocations();
    const Location best;
    size_t best_len = 0;
    for (size_t i = 0; i < locations.size(); ++i)
    {
        const std::string path = locations[i].getPath();
		LOG_DBG << "LOG_DBG matchLocation path: " << path << " target: " << target << " compare: " << target.compare(0, path.size(), path);
        if (path.empty())
            continue;
        bool prefix_match = (target.compare(0, path.size(), path) == 0);
        bool boundary_ok = (target.size() == path.size() || target[path.size()] == '/');
        if (prefix_match && boundary_ok && path.size() >= best_len)
        {
			LOG_DBG << "LOG_DBG matchLocation: " << locations[i].getCgiBinPath();
			return locations[i];
        }
    }
	return best;
}
