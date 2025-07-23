#pragma once

#include <string>
#include <cstdint>

namespace NetUtils {

    inline std::string uint32_to_ip_be(uint32_t ip_be) {
        uint8_t a = (ip_be >> 24) & 0xFF;
        uint8_t b = (ip_be >> 16) & 0xFF;
        uint8_t c = (ip_be >> 8) & 0xFF;
        uint8_t d = ip_be & 0xFF;

        return std::to_string(a) + "." +
               std::to_string(b) + "." +
               std::to_string(c) + "." +
               std::to_string(d);
    }

    inline std::string uint32_to_ip_le(uint32_t ip_le) {
        uint8_t a = ip_le & 0xFF;
        uint8_t b = (ip_le >> 8) & 0xFF;
        uint8_t c = (ip_le >> 16) & 0xFF;
        uint8_t d = (ip_le >> 24) & 0xFF;

        return std::to_string(a) + "." +
               std::to_string(b) + "." +
               std::to_string(c) + "." +
               std::to_string(d);
    }

} // namespace NetUtils
