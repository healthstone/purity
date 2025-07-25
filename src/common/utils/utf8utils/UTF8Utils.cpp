#include "UTF8Utils.hpp"

namespace UTF8Utils {

    // Проверка UTF-8 по базовым правилам
    bool is_valid_utf8(const std::string &str) {
        const unsigned char *bytes = reinterpret_cast<const unsigned char *>(str.c_str());
        size_t len = str.size();
        size_t i = 0;

        while (i < len) {
            unsigned char c = bytes[i];

            if (c <= 0x7F) {
                // ASCII 0xxxxxxx
                i += 1;
            } else if ((c & 0xE0) == 0xC0) {
                // 2-byte sequence 110xxxxx 10xxxxxx
                if (i + 1 >= len) return false;
                if ((bytes[i + 1] & 0xC0) != 0x80) return false;
                if (c < 0xC2) return false; // overlong encoding check
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                // 3-byte sequence 1110xxxx 10xxxxxx 10xxxxxx
                if (i + 2 >= len) return false;
                if ((bytes[i + 1] & 0xC0) != 0x80) return false;
                if ((bytes[i + 2] & 0xC0) != 0x80) return false;
                if (c == 0xE0 && bytes[i + 1] < 0xA0) return false; // overlong encoding
                if (c == 0xED && bytes[i + 1] >= 0xA0) return false; // surrogate halves
                i += 3;
            } else if ((c & 0xF8) == 0xF0) {
                // 4-byte sequence 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                if (i + 3 >= len) return false;
                if ((bytes[i + 1] & 0xC0) != 0x80) return false;
                if ((bytes[i + 2] & 0xC0) != 0x80) return false;
                if ((bytes[i + 3] & 0xC0) != 0x80) return false;
                if (c == 0xF0 && bytes[i + 1] < 0x90) return false; // overlong
                if (c > 0xF4) return false; // > U+10FFFF
                i += 4;
            } else {
                return false;
            }
        }
        return true;
    }

    // Простое lowercase для ASCII, остальные символы остаются без изменений
    std::string to_lowercase(const std::string &str) {
        std::string result;
        result.reserve(str.size());

        for (size_t i = 0; i < str.size();) {
            unsigned char c = str[i];
            if (c <= 0x7F) {
                // ASCII
                result.push_back(std::tolower(c));
                ++i;
            } else {
                // UTF-8 multibyte - просто копируем, т.к. без внешних библиотек нет удобного lower
                // Можно улучшить, если нужна поддержка Unicode case folding с ICU
                size_t length = 1;
                if ((c & 0xE0) == 0xC0) length = 2;
                else if ((c & 0xF0) == 0xE0) length = 3;
                else if ((c & 0xF8) == 0xF0) length = 4;

                result.append(str.substr(i, length));
                i += length;
            }
        }

        return result;
    }

    std::string to_uppercase(const std::string &str) {
        std::string result;
        result.reserve(str.size());

        for (size_t i = 0; i < str.size();) {
            unsigned char c = str[i];
            if (c <= 0x7F) {
                result.push_back(std::toupper(c));
                ++i;
            } else {
                size_t length = 1;
                if ((c & 0xE0) == 0xC0) length = 2;
                else if ((c & 0xF0) == 0xE0) length = 3;
                else if ((c & 0xF8) == 0xF0) length = 4;

                result.append(str.substr(i, length));
                i += length;
            }
        }

        return result;
    }

} // namespace UTF8Utils
