#pragma once
#include <stdexcept>
#include <string>

namespace razen {

struct SemanticError : public std::runtime_error {
    explicit SemanticError(const std::string& msg) : std::runtime_error(msg) {}
};

} // namespace razen
