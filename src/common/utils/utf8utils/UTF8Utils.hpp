#pragma once

#include <string>
#include <stdexcept>
#include <cctype>

namespace UTF8Utils {

// Проверка, что строка валидный UTF-8 (базовая реализация)
    bool is_valid_utf8(const std::string& str);

// Преобразование строки UTF-8 в lowercase (работает корректно для ASCII)
    std::string to_lowercase(const std::string& str);

} // namespace UTF8Utils
