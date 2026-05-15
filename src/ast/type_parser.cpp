#include "type_parser.h"
#include "ast_utils.h"
#include "token_utils.h"
#include "../lexer/lexer.h"
#include <cstring>

namespace razen {

ASTNode* parseTypeNode(ASTData& d) {
    ASTNode* type_node = createDefaultAstNode();
    type_node->node_type = ASTNodeType::VarType;

    Token tok = d.getToken();
    d.advance();

    // modifiers that wrap another type
    if (tok.type == TokenType::Star ||
        tok.type == TokenType::And ||
        tok.type == TokenType::ExclamationMark ||
        tok.type == TokenType::QuestionMark)
    {
        ASTNode* inner = parseTypeNode(d);
        type_node->token = tok;
        type_node->left = inner;
        return type_node;
    }

    // mut in type position
    if (tok.type == TokenType::Mut) {
        ASTNode* inner = parseTypeNode(d);
        type_node->token = tok;
        type_node->is_mut = true;
        type_node->left = inner;
        return type_node;
    }

    // [ T ] or [ T ; N ]
    if (tok.type == TokenType::LeftBracket) {
        ASTNode* inner = parseTypeNode(d);
        if (d.hasMore()) {
            Token nt = d.getToken();
            if (nt.type == TokenType::Semicolon) {
                d.advance();
                Token size_tok = d.getToken();
                d.advance();
                ASTNode* size_node = createDefaultAstNode();
                size_node->node_type = ASTNodeType::IntegerLiteral;
                size_node->token = size_tok;
                type_node->middle = size_node;
            }
        }
        Token rb = d.getToken();
        if (rb.type == TokenType::RightBracket) d.advance();

        type_node->node_type = ASTNodeType::ArrayType;
        type_node->token = tok;
        type_node->left = inner;
        return type_node;
    }

    // vec[T], map{K,V}, set{T}
    if (tok.type == TokenType::Identifier &&
        (tok.value == "vec" || tok.value == "map" || tok.value == "set"))
    {
        Token open = d.getToken();
        if (open.type == TokenType::LeftBracket || open.type == TokenType::LeftBrace) d.advance();
        ASTNode* inner = parseTypeNode(d);
        type_node->left = inner;

        Token after = d.getToken();
        if (after.type == TokenType::Comma) {
            d.advance();
            ASTNode* inner2 = parseTypeNode(d);
            type_node->middle = inner2;
        }

        Token close = d.getToken();
        if (close.type == TokenType::RightBracket || close.type == TokenType::RightBrace) d.advance();

        type_node->token = tok;
        return type_node;
    }

    // Error!T (named error union)
    if (tok.type == TokenType::Identifier) {
        if (d.hasMore()) {
            Token maybe_excl = d.getToken();
            if (maybe_excl.type == TokenType::ExclamationMark) {
                d.advance();
                ASTNode* inner = parseTypeNode(d);
                type_node->token = tok;
                type_node->left = inner;
                return type_node;
            }
        }
        // plain identifier (user-defined type)
        type_node->token = tok;
        return type_node;
    }

    // error !T (bare anonymous error)
    if (tok.type == TokenType::Error) {
        Token excl = d.getToken();
        if (excl.type == TokenType::ExclamationMark) d.advance();
        ASTNode* inner = parseTypeNode(d);
        type_node->token = tok;
        type_node->left = inner;
        return type_node;
    }

    // @ builtins
    if (tok.type == TokenType::At) {
        Token name_tok = d.getToken();
        d.advance();
        type_node->token = name_tok;

        if (d.hasMore()) {
            Token lp = d.getToken();
            if (lp.type == TokenType::LeftParen) {
                d.advance();
                size_t depth = 1;
                size_t guard = 0;
                while (d.hasMore() && depth > 0) {
                    guard++;
                    if (guard > 1000) break;
                    Token t = d.getToken();
                    d.advance();
                    if (t.type == TokenType::LeftParen) depth++;
                    if (t.type == TokenType::RightParen) depth--;
                }
            }
        }
        return type_node;
    }

    // primitive/built-in type keywords
    if (isVarType(tok.type)) {
        if (d.hasMore()) {
            Token maybe_excl = d.getToken();
            if (maybe_excl.type == TokenType::ExclamationMark) {
                d.advance();
                ASTNode* inner = parseTypeNode(d);
                type_node->token = tok;
                type_node->left = inner;
                return type_node;
            }
        }
        type_node->token = tok;
        return type_node;
    }

    // DotDot / DotDotDot for variadic params
    if (tok.type == TokenType::DotDotDot) {
        type_node->token = tok;
        return type_node;
    }

    d.setError("Expected a type", tok);
    throw AstError("Unexpected type");
}

} // namespace razen
