#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <vector>
#include <atomic>
#include <sstream>
#include <random>

namespace chatroom {

namespace utils {

inline uint64_t getCurrentTimestampMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

inline uint64_t generateUniqueId() {
    static std::atomic<uint64_t> counter{0};
    auto now = getCurrentTimestampMs();
    return (now << 20) | (counter.fetch_add(1) & 0xFFFFF);
}

inline std::string generateSecureToken();
inline std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

inline std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        tokens.push_back(item);
    }
    return tokens;
}

inline bool isNullOrEmpty(const std::string& str) {
    return str.empty() || trim(str).empty();
}

} // namespace utils

} // namespace chatroom
