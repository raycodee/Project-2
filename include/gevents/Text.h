#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace gevents {

std::vector<std::string> tokenize(std::string_view text);
std::string normalizeKeyword(std::string_view text);
std::string normalizeCategory(std::string_view text);

}
