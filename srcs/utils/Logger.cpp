#include "utils/Logger.hpp"
#include <ctime>
#include <cstdlib>
#include <iomanip>

namespace
{
	LogLevel parseLogLevel(const char* value)
	{
		if (!value || !*value)
			return LOG_DEBUG;
		std::string v(value);
		for (size_t i = 0; i < v.size(); ++i)
			v[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(v[i])));
		if (v == "debug" || v == "0")
			return LOG_DEBUG;
		if (v == "info" || v == "1")
			return LOG_INFO;
		if (v == "warn" || v == "warning" || v == "2")
			return LOG_WARN;
		if (v == "error" || v == "3")
			return LOG_ERROR;
		if (v == "none" || v == "4")
			return LOG_NONE;
		return LOG_DEBUG;
	}
}

Logger::Logger() : _level(parseLogLevel(std::getenv("WEBSERV_LOG")))
{
}

Logger::~Logger()
{
}

Logger& Logger::getInstance()
{
	static Logger instance;
	return instance;
}

void Logger::setLevel(LogLevel level)
{
	_level = level;
}

LogLevel Logger::getLevel() const
{
	return _level;
}

void Logger::debug(const std::string& msg)
{
	log(LOG_DEBUG, msg);
}

void Logger::info(const std::string& msg)
{
	log(LOG_INFO, msg);
}

void Logger::warn(const std::string& msg)
{
	log(LOG_WARN, msg);
}

void Logger::error(const std::string& msg)
{
	log(LOG_ERROR, msg);
}

void Logger::Debug(const std::string& msg)
{
	getInstance().debug(msg);
}

void Logger::Info(const std::string& msg)
{
	getInstance().info(msg);
}

void Logger::Warn(const std::string& msg)
{
	getInstance().warn(msg);
}

void Logger::Error(const std::string& msg)
{
	getInstance().error(msg);
}

void Logger::log(LogLevel level, const std::string& msg)
{
	if (level < _level)
		return;

	std::ostream& out = std::cerr;
	out << getTimestamp() << " [" << getLevelString(level) << "] " << msg << std::endl;
}

std::string Logger::getLevelString(LogLevel level) const
{
	switch (level)
	{
		case LOG_DEBUG: return "DEBUG";
		case LOG_INFO:  return "INFO ";
		case LOG_WARN:  return "WARN ";
		case LOG_ERROR: return "ERROR";
		default:        return "UNKNOWN";
	}
}

std::string Logger::getTimestamp() const
{
	std::time_t now = std::time(NULL);
	std::tm* tm_info = std::localtime(&now);

	std::ostringstream oss;
	oss << std::setfill('0')
		<< std::setw(4) << (tm_info->tm_year + 1900) << "-"
		<< std::setw(2) << (tm_info->tm_mon + 1) << "-"
		<< std::setw(2) << tm_info->tm_mday << " "
		<< std::setw(2) << tm_info->tm_hour << ":"
		<< std::setw(2) << tm_info->tm_min << ":"
		<< std::setw(2) << tm_info->tm_sec;
	return oss.str();
}

// LogStream implementation
LogStream::LogStream(LogLevel level) : _level(level)
{
}

LogStream::~LogStream()
{
	switch (_level)
	{
		case LOG_DEBUG: Logger::Debug(_stream.str()); break;
		case LOG_INFO:  Logger::Info(_stream.str());  break;
		case LOG_WARN:  Logger::Warn(_stream.str());  break;
		case LOG_ERROR: Logger::Error(_stream.str()); break;
		default: break;
	}
}
