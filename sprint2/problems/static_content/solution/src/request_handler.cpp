#include <boost/algorithm/string.hpp>

#include "request_handler.h"

namespace http_handler {

std::string RequestHandler::GetMimeType(const std::string &extension) {
    auto it = mime_types_.find(boost::algorithm::to_lower_copy(extension));
    if (it != mime_types_.end()) {
        return it->second;
    }
    return "application/octet-stream";  // MIME тип по умолчанию
}

std::string RequestHandler::UrlDecode(const std::string &url) {
    std::string result;
    char hex_char[3];
    hex_char[2] = '\0';

    for (size_t i = 0; i < url.length(); ++i) {
        if (url[i] == '%' && i + 2 < url.length()) {
            hex_char[0] = url[i + 1];
            hex_char[1] = url[i + 2];
            result += static_cast<char>(std::strtol(hex_char, nullptr, 16));
            i += 2;
        } else if (url[i] == '+') {
            result += ' ';
        } else {
            result += url[i];
        }
    }

    return result;
}

bool RequestHandler::IsSubPath(fs::path path, fs::path base) {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

}  // namespace http_handler
