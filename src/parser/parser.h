#pragma once
#include <vector>
#include <string_view>
#include "../lexer/token.h"
#include "../ast/node.h"

namespace razen {

std::vector<ASTNode*> buildAST(const std::vector<Token>& token_list, std::string_view source);

} // namespace razen
