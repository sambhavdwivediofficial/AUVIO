#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <sstream>

namespace auvio {

class Config {
public:
    static Config& instance();

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    bool load(const std::string& filePath = {});
    bool save(const std::string& filePath = {}) const;

    static std::string defaultFilePath();

    template <typename T>
    T get(const std::string& key, const T& defaultValue) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_values.find(key);
        if (it == m_values.end()) {
            return defaultValue;
        }
        return fromString<T>(it->second, defaultValue);
    }

    template <typename T>
    void set(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_values[key] = toString<T>(value);
    }

    bool has(const std::string& key) const;
    void remove(const std::string& key);

private:
    Config() = default;

    template <typename T>
    static std::string toString(const T& value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    template <typename T>
    static T fromString(const std::string& text, const T& defaultValue) {
        std::istringstream iss(text);
        T value{};
        if (iss >> value) {
            return value;
        }
        return defaultValue;
    }

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::string> m_values;
    std::string m_loadedFromPath;
};

template <>
inline std::string Config::toString<bool>(const bool& value) {
    return value ? "true" : "false";
}

template <>
inline bool Config::fromString<bool>(const std::string& text, const bool& defaultValue) {
    if (text == "true" || text == "1")  return true;
    if (text == "false" || text == "0") return false;
    return defaultValue;
}

template <>
inline std::string Config::fromString<std::string>(const std::string& text, const std::string&) {
    return text;
}

}
