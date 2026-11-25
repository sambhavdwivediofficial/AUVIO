#include "Config.h"
#include "Logger.h"

#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace auvio {

Config& Config::instance() {
    static Config instance;
    return instance;
}

std::string Config::defaultFilePath() {
#ifdef _WIN32
    const char* appData = std::getenv("APPDATA");
    const std::filesystem::path base = appData ? std::filesystem::path(appData)
                                                 : std::filesystem::path(".");
    return (base / "AUVIO" / "config.ini").string();
#else
    const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
    std::filesystem::path base;
    if (xdgConfig != nullptr && xdgConfig[0] != '\0') {
        base = std::filesystem::path(xdgConfig);
    } else {
        const char* home = std::getenv("HOME");
        base = std::filesystem::path(home ? home : ".") / ".config";
    }
    return (base / "AUVIO" / "config.ini").string();
#endif
}

bool Config::load(const std::string& filePath) {
    const std::string path = filePath.empty() ? defaultFilePath() : filePath;

    std::ifstream in(path);
    if (!in.is_open()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_loadedFromPath = path;
        AUVIO_LOG_INFO << "No existing config at " << path << ", using defaults.";
        return true;
    }

    std::unordered_map<std::string, std::string> parsed;
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(in, line)) {
        ++lineNumber;

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        const auto firstNonSpace = line.find_first_not_of(" \t");
        if (firstNonSpace == std::string::npos) continue;
        if (line[firstNonSpace] == '#' || line[firstNonSpace] == ';') continue;

        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            AUVIO_LOG_WARNING << "Ignoring malformed config line " << lineNumber << " in " << path;
            continue;
        }

        auto trim = [](std::string s) {
            const auto start = s.find_first_not_of(" \t");
            const auto end   = s.find_last_not_of(" \t");
            return (start == std::string::npos) ? std::string{} : s.substr(start, end - start + 1);
        };

        const std::string key   = trim(line.substr(0, eq));
        const std::string value = trim(line.substr(eq + 1));

        if (!key.empty()) {
            parsed[key] = value;
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_values = std::move(parsed);
    m_loadedFromPath = path;
    AUVIO_LOG_INFO << "Loaded " << m_values.size() << " setting(s) from " << path;
    return true;
}

bool Config::save(const std::string& filePath) const {
    std::string path;
    std::unordered_map<std::string, std::string> snapshot;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        path = !filePath.empty() ? filePath
             : !m_loadedFromPath.empty() ? m_loadedFromPath
             : defaultFilePath();
        snapshot = m_values;
    }

    try {
        const std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (const std::exception& ex) {
        AUVIO_LOG_ERROR << "Could not create config directory for " << path << ": " << ex.what();
        return false;
    }

    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        AUVIO_LOG_ERROR << "Could not open config file for writing: " << path;
        return false;
    }

    out << "# AUVIO configuration -- edits here take effect on next launch\n";
    for (const auto& [key, value] : snapshot) {
        out << key << "=" << value << "\n";
    }

    AUVIO_LOG_INFO << "Saved " << snapshot.size() << " setting(s) to " << path;
    return true;
}

bool Config::has(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_values.find(key) != m_values.end();
}

void Config::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values.erase(key);
}

}
