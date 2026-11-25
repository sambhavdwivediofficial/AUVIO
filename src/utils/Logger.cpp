#include "Logger.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
#endif

namespace auvio {

namespace {

void enableAnsiOnWindowsConsole() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return;

    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

} // namespace

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    enableAnsiOnWindowsConsole();
}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(m_streamMutex);
    if (m_fileStream.is_open()) {
        m_fileStream.flush();
        m_fileStream.close();
    }
}

void Logger::setMinLevel(LogLevel level) noexcept {
    m_minLevel.store(level, std::memory_order_relaxed);
}

LogLevel Logger::minLevel() const noexcept {
    return m_minLevel.load(std::memory_order_relaxed);
}

void Logger::enableConsoleOutput(bool enabled) noexcept {
    m_consoleOutput.store(enabled, std::memory_order_relaxed);
}

bool Logger::setLogFile(const std::string& filePath) {
    try {
        std::filesystem::path path(filePath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
    } catch (const std::exception&) {
    }

    std::lock_guard<std::mutex> lock(m_streamMutex);
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
    m_fileStream.open(filePath, std::ios::out | std::ios::app);
    return m_fileStream.is_open();
}

void Logger::log(LogLevel level, const std::string& message, const char* file, int line) {
    if (level < m_minLevel.load(std::memory_order_relaxed)) {
        return;
    }

    const std::string timestamp = timestampNow();
    const std::string label = levelToLabel(level);
    const std::string location = shortFileName(file) + ":" + std::to_string(line);

    std::lock_guard<std::mutex> lock(m_streamMutex);

    if (m_consoleOutput.load(std::memory_order_relaxed)) {
        const char* color = levelToColor(level);
        std::ostream& out = (level >= LogLevel::Error) ? std::cerr : std::cout;
        out << color << "[" << timestamp << "] [" << label << "] "
            << "[" << location << "] " << message << "\033[0m" << '\n';
    }

    if (m_fileStream.is_open()) {
        m_fileStream << "[" << timestamp << "] [" << label << "] "
                     << "[" << location << "] " << message << '\n';
        m_fileStream.flush();
    }
}

std::string Logger::levelToLabel(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO ";
        case LogLevel::Warning:  return "WARN ";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRIT ";
    }
    return "?????";
}

const char* Logger::levelToColor(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:    return "\033[90m";
        case LogLevel::Info:     return "\033[37m";
        case LogLevel::Warning:  return "\033[33m";
        case LogLevel::Error:    return "\033[31m";
        case LogLevel::Critical: return "\033[1;41m";
    }
    return "\033[0m";
}

std::string Logger::timestampNow() {
    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto nowTimeT = system_clock::to_time_t(now);
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &nowTimeT);
#else
    localtime_r(&nowTimeT, &tmBuf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::shortFileName(const char* filePath) {
    const std::string path(filePath);
    const auto slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

} // namespace auvio
