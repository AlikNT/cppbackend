#include "htmldecode.h"

std::string HtmlDecode(std::string_view str) {
    std::string result;
    std::unordered_map<std::string, char> entities = {
        {"lt", '<'},
        {"LT", '<'},
        {"gt", '>'},
        {"GT", '>'},
        {"amp", '&'},
        {"AMP", '&'},
        {"apos", '\''},
        {"APOS", '\''},
        {"quot", '"'},
        {"QUOT", '"'}
    };
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '&') {
            bool found = false;
            for (size_t j = 2; j <= 4; ++j) {
                std::string entity(str.substr(i + 1, j));
                auto it = entities.find(entity);
                if (it != entities.end()) {
                    result += it->second;
                    i += j;
                    if (str[i + 1] == ';') {
                        ++i;
                    }
                    found = true;
                    break;
                }
            }
            if (found) continue;
        }
        result += str[i];
    }
    return result;
}
