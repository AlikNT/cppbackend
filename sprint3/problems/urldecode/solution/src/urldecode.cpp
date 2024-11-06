#include "urldecode.h"

#include <charconv>
#include <stdexcept>

std::string UrlDecode(std::string_view str) {
    std::string result;
    char hex_char[3];
    hex_char[2] = '\0';

    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%') {
            // Проверяем, что достаточно символов для корректной %-последовательности
            if (i + 2 >= str.length() || !std::isxdigit(str[i + 1]) || !std::isxdigit(str[i + 2])) {
                throw std::invalid_argument("Invalid %-sequence in URL");
            }
            // Считываем два следующих символа и преобразуем их в символ
            hex_char[0] = str[i + 1];
            hex_char[1] = str[i + 2];
            result += static_cast<char>(std::strtol(hex_char, nullptr, 16));
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}
