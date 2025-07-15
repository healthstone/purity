#pragma once

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <optional>

namespace TimeUtils {

    inline std::chrono::system_clock::time_point parse_pg_timestamp(const std::string& timestamp) {
        std::tm t = {};
        std::istringstream ss(timestamp);
        ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
        if (ss.fail()) {
            throw std::runtime_error("Failed to parse timestamp: " + timestamp);
        }
        return std::chrono::system_clock::from_time_t(std::mktime(&t));
    }

    inline std::optional<std::chrono::system_clock::time_point> parse_pg_timestamp_optional(const std::string& timestamp) {
        if (timestamp.empty()) return std::nullopt;
        try {
            return parse_pg_timestamp(timestamp);
        } catch (...) {
            return std::nullopt;
        }
    }

} // namespace TimeUtils
