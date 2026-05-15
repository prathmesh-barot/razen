#pragma once
#include <stdexcept>
#include <string>

namespace razen {

struct LexerError : public std::runtime_error {
    explicit LexerError(const std::string& msg)
        : std::runtime_error(msg) {}
};

} // namespace razen
