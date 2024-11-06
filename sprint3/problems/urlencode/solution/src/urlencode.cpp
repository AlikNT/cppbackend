#include "urlencode.h"

std::string UrlEncode(std::string_view str) {
    std::ostringstream encoded;
    encoded << std::hex << std::uppercase;  // Устанавливаем шестнадцатеричный формат и верхний регистр

    for (char ch : str) {
        if (ch == ' ') {
            encoded << '+';
        } else if (ch < 32 || ch >= 128 || std::string("!#$&'()*+,/:;=?@[]").find(ch) != std::string::npos) {
            encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(ch));
        } else {
            encoded << ch;
        }
    }

    return encoded.str();
}

