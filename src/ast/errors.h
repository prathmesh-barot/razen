#pragma once
#include <stdexcept>
#include <string>

namespace razen {

struct AstError : public std::runtime_error {
    explicit AstError(const std::string& msg)
        : std::runtime_error(msg) {}
};

} // namespace razen
