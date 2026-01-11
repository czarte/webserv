#include "utils/Logger.hpp"
#include <ctime>
#include <iomanip>

Logger::Logger() : _level(LOG_DEBUG)
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

	std::ostream& out = (level >= LOG_ERROR) ? std::cerr : std::cout;
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
