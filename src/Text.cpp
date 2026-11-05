#include "gevents/Text.h"

#include <stdexcept>

namespace gevents {

std::vector<std::string> tokenize(std::string_view text) {
    std::vector<std::string> tokens;
    std::string token;
    for (unsigned char c : text) {
        if (c >= 'A' && c <= 'Z') {
            token += static_cast<char>(c - 'A' + 'a');
        } else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            token += static_cast<char>(c);
        } else if (!token.empty()) {
            tokens.push_back(token);
            token.clear();
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string normalizeKeyword(std::string_view text) {
    auto tokens = tokenize(text);
    if (tokens.size() > 1) {
        throw std::invalid_argument("Expected one keyword; tokenize a query before lookup");
    }
    return tokens.empty() ? std::string{} : tokens.front();
}

std::string normalizeCategory(std::string_view text) {
    const auto tokens = tokenize(text);
    std::string category;
    for (const auto& token : tokens) {
        if (!category.empty()) {
            category += ' ';
        }
        category += token;
    }
    return category;
}

}
