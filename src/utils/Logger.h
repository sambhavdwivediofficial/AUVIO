#pragma once

#include <string>
#include <mutex>
#include <atomic>
#include <fstream>
#include <sstream>

namespace auvio {

enum class LogLevel {
    Debug = 0,
    Info,
    Warning,
    Error,
    Critical
};

class Logger {
public:
    static Logger& instance();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // Messages below this level are dropped before any locking/formatting
    // happens, so raising this in release builds costs nothing extra.
    void setMinLevel(LogLevel level) noexcept;
    LogLevel minLevel() const noexcept;

    void enableConsoleOutput(bool enabled) noexcept;

    // Mirrors all subsequent log output to a file as well. Creates parent
    // directories if needed. Returns false if the file could not be opened.
    bool setLogFile(const std::string& filePath);

    // Low-level entry point used by LogStream. Prefer the AUVIO_LOG_*
    // macros in normal code instead of calling this directly.
    void log(LogLevel level, const std::string& message, const char* file, int line);

private:
    Logger();
    ~Logger();

    static std::string levelToLabel(LogLevel level);
    static const char* levelToColor(LogLevel level);
    static std::string timestampNow();
    static std::string shortFileName(const char* filePath);

    std::atomic<LogLevel> m_minLevel{LogLevel::Debug};
    std::atomic<bool> m_consoleOutput{true};

    std::mutex m_streamMutex;   // guards m_fileStream + console writes
    std::ofstream m_fileStream;
};

class LogStream {
public:
    LogStream(LogLevel level, const char* file, int line)
        : m_level(level), m_file(file), m_line(line) {}

    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;

    ~LogStream() {
        Logger::instance().log(m_level, m_buffer.str(), m_file, m_line);
    }

    template <typename T>
    LogStream& operator<<(const T& value) {
        m_buffer << value;
        return *this;
    }

private:
    std::ostringstream m_buffer;
    LogLevel m_level;
    const char* m_file;
    int m_line;
};

} // namespace auvio

#define AUVIO_LOG_DEBUG    ::auvio::LogStream(::auvio::LogLevel::Debug,    __FILE__, __LINE__)
#define AUVIO_LOG_INFO     ::auvio::LogStream(::auvio::LogLevel::Info,     __FILE__, __LINE__)
#define AUVIO_LOG_WARNING  ::auvio::LogStream(::auvio::LogLevel::Warning,  __FILE__, __LINE__)
#define AUVIO_LOG_ERROR    ::auvio::LogStream(::auvio::LogLevel::Error,    __FILE__, __LINE__)
#define AUVIO_LOG_CRITICAL ::auvio::LogStream(::auvio::LogLevel::Critical, __FILE__, __LINE__)
