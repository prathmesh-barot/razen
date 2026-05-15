#pragma once
#include <vector>
#include <optional>
#include <string>
#include "../lexer/lexer.h"
#include "node.h"
#include "errors.h"

namespace razen {

struct ASTData {
    std::vector<ASTNode*>* ast_nodes;
    size_t token_index = 0;
    const std::vector<Token>* token_list;

    // error info
    std::optional<std::string> error_detail;
    std::optional<Token> error_token;
    std::optional<std::string> error_function;

    // get current token without advancing
    Token getToken() {
        if (token_index >= token_list->size()) {
            throw AstError("Index out of range");
        }
        Token tok = (*token_list)[token_index];
        error_token = tok;
        return tok;
    }

    // advance and return new token
    Token getNextToken() {
        token_index += 1;
        if (token_index >= token_list->size()) {
            throw AstError("Index out of range");
        }
        Token tok = (*token_list)[token_index];
        error_token = tok;
        return tok;
    }

    // peek ahead without advancing
    std::optional<Token> peekToken(size_t offset) const {
        size_t idx = token_index + offset;
        if (idx >= token_list->size()) return std::nullopt;
        return (*token_list)[idx];
    }

    // advance by one
    void incrementIndex() {
        token_index += 1;
        if (token_index >= token_list->size()) {
            throw AstError("Index out of range");
        }
    }

    // advance without error on EOF
    void advance() {
        if (token_index < token_list->size()) {
            token_index += 1;
        }
    }

    void setError(const std::string& detail, const Token& tok) {
        error_detail = detail;
        error_token = tok;
    }

    bool hasMore() const {
        return token_index < token_list->size();
    }
};

} // namespace razen
