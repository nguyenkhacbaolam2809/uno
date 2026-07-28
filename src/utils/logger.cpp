#include "logger.h"
#include <iostream>
#include <ctime>

Logger & Logger::instance()
{
    static Logger inst;
    return inst;
}

Logger::Logger() : m_minLevel(LogLevel::DEBUG), m_fileEnabled(false) {}

Logger::~Logger()
{
    if (m_file.is_open())
        m_file.close();
}

void Logger::setLevel(LogLevel minLevel) { m_minLevel = minLevel; }

void Logger::enableFileOutput(const std::string & path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) m_file.close();
    m_file.open(path, std::ios::app);
    m_fileEnabled = m_file.is_open();
}

void Logger::disableFileOutput()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) m_file.close();
    m_fileEnabled = false;
}

const char * Logger::levelName(LogLevel level)
{
    switch (level)
    {
        case LogLevel::TRACE:   return "TRACE";
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

void Logger::logv(LogLevel level, const char * file, int line, const char * fmt, va_list args)
{
    if (level < m_minLevel) return;

    char buf[4096];
    std::vsnprintf(buf, sizeof(buf), fmt, args);

    std::time_t t = std::time(nullptr);
    struct tm * lt = std::localtime(&t);
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", lt);

    std::lock_guard<std::mutex> lock(m_mutex);

    std::cerr << "[" << timeBuf << "][" << levelName(level) << "] "
              << file << ":" << line << " " << buf << std::endl;

    if (m_fileEnabled && m_file.is_open())
        m_file << "[" << timeBuf << "][" << levelName(level) << "] "
               << file << ":" << line << " " << buf << std::endl;
}

void Logger::log(LogLevel level, const char * file, int line, const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logv(level, file, line, fmt, args);
    va_end(args);
}
