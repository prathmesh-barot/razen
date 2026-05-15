#pragma once
#include "node.h"
#include "ast_data.h"

namespace razen {

// parse the smallest unit of an expression
ASTNode* parsePrimary(ASTData& d);

// precedence-climbing binary expression parser
ASTNode* parseBinaryExpr(ASTData& d, size_t min_prec);

// function call: name_tok already consumed, current token is '('
ASTNode* parseFunctionCallNode(ASTData& d, const Token& name_tok);

} // namespace razen
