#pragma once

#include <stdexcept>
#include <string>
#include <utility>

inline void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Function>
void requireInvalid(Function&& function, const std::string& message) {
    try {
        std::forward<Function>(function)();
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error(message);
}
