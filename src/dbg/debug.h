#pragma once
#include <vector>
#include <string>
#include "../lexer/lexer.h"
#include "../ast/node.h"

namespace razen {

void printTokens(const std::vector<Token>& token_list);
void printAST(const std::vector<ASTNode*>& ast_nodes);
void printNode(const ASTNode* n, size_t depth);

// convert TokenType to string
const char* tokenTypeName(TokenType tt);

// save IR to tests/ directory
void writeIR(const std::string& label, const std::string& ir, bool success);

} // namespace razen
