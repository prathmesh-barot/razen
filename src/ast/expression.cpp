#include "expression.h"
#include "ast_utils.h"
#include "token_utils.h"
#include "type_parser.h"
#include "../lexer/lexer.h"
#include <cstring>

namespace razen {

static constexpr size_t MAX_LOOP = 10000;

ASTNode* parsePrimary(ASTData& d) {
    if (!d.hasMore()) return nullptr;

    Token tok = d.getToken();

    switch (tok.type) {
        // integer literal
        case TokenType::IntegerValue: {
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::IntegerLiteral;
            n->token = tok;
            d.advance();
            return n;
        }

        // float/decimal literal
        case TokenType::DecimalValue: {
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::FloatLiteral;
            n->token = tok;
            d.advance();
            return n;
        }

        // true/false
        case TokenType::True:
        case TokenType::False: {
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::BoolLiteral;
            n->token = tok;
            d.advance();
            return n;
        }

        // char literal
        case TokenType::CharValue: {
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::CharLiteral;
            n->token = tok;
            d.advance();
            return n;
        }

        // string literal
        case TokenType::StringValue: {
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::StringLiteral;
            n->token = tok;
            d.advance();
            return n;
        }

        // try expression
        case TokenType::Try: {
            d.advance();
            ASTNode* operand = parsePrimary(d);
            if (!operand) {
                d.setError("Expected expression after 'try'", tok);
                throw AstError("Unexpected type");
            }
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::TryExpression;
            n->token = tok;
            n->left = operand;
            return n;
        }

        // array literal [ 1, 2, 3 ]
        case TokenType::LeftBracket: {
            d.advance();
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::ArrayLiteral;
            n->token = tok;
            n->children = createChildList();

            while (d.hasMore()) {
                Token cur = d.getToken();
                if (cur.type == TokenType::RightBracket) {
                    d.advance();
                    break;
                }
                ASTNode* elem = parseBinaryExpr(d, 0);
                if (elem) {
                    n->children->push_back(elem);
                }
                if (d.hasMore()) {
                    Token comma = d.getToken();
                    if (comma.type == TokenType::Comma) d.advance();
                }
            }
            return n;
        }

        // annotation / builtin @name
        case TokenType::At: {
            d.advance();
            Token name_tok = d.getToken();
            d.advance();

            // @as(Type, value)
            if (name_tok.value == "as") {
                Token lp = d.getToken();
                if (lp.type == TokenType::LeftParen) {
                    d.advance();
                    ASTNode* target_type = parseTypeNode(d);

                    Token comma = d.getToken();
                    if (comma.type == TokenType::Comma) d.advance();

                    ASTNode* value_expr = parseBinaryExpr(d, 0);

                    Token rp = d.getToken();
                    if (rp.type == TokenType::RightParen) d.advance();

                    ASTNode* as_n = createDefaultAstNode();
                    as_n->node_type = ASTNodeType::BuiltinExpression;
                    as_n->token = name_tok;
                    as_n->left = target_type;
                    as_n->right = value_expr;
                    if (!as_n->right) throw AstError("Unexpected type");
                    return as_n;
                }
            }

            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::Annotation;
            n->token = name_tok;

            // @Name(args)
            std::optional<Token> next = d.peekToken(0);
            if (next && next->type == TokenType::LeftParen) {
                d.advance();
                size_t depth = 1;
                size_t guard2 = 0;
                while (d.hasMore() && depth > 0) {
                    guard2++;
                    if (guard2 > 1000) break;
                    Token t = d.getToken();
                    d.advance();
                    if (t.type == TokenType::LeftParen) depth++;
                    if (t.type == TokenType::RightParen) depth--;
                }
                // check if @Name() is followed by a func call
                std::optional<Token> after = d.peekToken(0);
                if (after && after->type == TokenType::LeftParen) {
                    return parseFunctionCallNode(d, name_tok);
                }
            } else if (next && next->type == TokenType::LeftParen) {
                return parseFunctionCallNode(d, name_tok);
            }
            return n;
        }

        // identifier or function call: name / name(args)
        case TokenType::Identifier: {
            d.advance();
            std::optional<Token> next = d.peekToken(0);
            if (next && next->type == TokenType::LeftParen) {
                return parseFunctionCallNode(d, tok);
            }
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::Identifier;
            n->token = tok;
            return n;
        }

        // unary minus: -expr
        case TokenType::Minus: {
            d.advance();
            ASTNode* operand = parsePrimary(d);
            if (!operand) {
                d.setError("Expected expression after '-'", tok);
                throw AstError("Unexpected type");
            }
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::UnaryExpression;
            n->token = tok;
            n->left = operand;
            return n;
        }

        // logical not: !expr
        case TokenType::ExclamationMark: {
            d.advance();
            ASTNode* operand = parsePrimary(d);
            if (!operand) {
                d.setError("Expected expression after '!'", tok);
                throw AstError("Unexpected type");
            }
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::UnaryExpression;
            n->token = tok;
            n->left = operand;
            return n;
        }

        // &x address-of
        case TokenType::And: {
            d.advance();
            ASTNode* operand = parsePrimary(d);
            if (!operand) return nullptr;
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::UnaryExpression;
            n->token = tok;
            n->left = operand;
            return n;
        }

        // |e| block/closure
        case TokenType::Or: {
            d.advance();
            Token cap_tok = d.getToken();
            if (cap_tok.type != TokenType::Identifier) {
                d.setError("Expected identifier in capture block", cap_tok);
                throw AstError("Unexpected type");
            }
            d.advance();
            Token end_or = d.getToken();
            if (end_or.type != TokenType::Or) {
                d.setError("Expected closing '|' for capture block", end_or);
                throw AstError("Unexpected type");
            }
            d.advance();

            ASTNode* body_expr = nullptr;
            std::optional<Token> maybe_brac = d.peekToken(0);
            if (maybe_brac && maybe_brac->type == TokenType::LeftBrace) {
                d.advance();
                ASTNode* b_n = createDefaultAstNode();
                b_n->node_type = ASTNodeType::Block;
                b_n->children = createChildList();
                while (d.hasMore()) {
                    Token cur_t = d.getToken();
                    if (cur_t.type == TokenType::RightBrace) {
                        d.advance();
                        break;
                    }
                    if (cur_t.type == TokenType::Ret) {
                        d.advance();
                        ASTNode* ret_expr = parseBinaryExpr(d, 0);
                        ASTNode* ret_n = createDefaultAstNode();
                        ret_n->node_type = ASTNodeType::ReturnStatement;
                        ret_n->token = cur_t;
                        ret_n->right = ret_expr;
                        b_n->children->push_back(ret_n);
                    } else {
                        ASTNode* expr_n = parseBinaryExpr(d, 0);
                        if (expr_n) b_n->children->push_back(expr_n);
                    }
                }
                body_expr = b_n;
            } else {
                body_expr = parseBinaryExpr(d, 0);
            }

            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::MatchBody;
            n->token = cap_tok;
            n->left = body_expr;
            return n;
        }

        // grouped expression: ( expr )
        case TokenType::LeftParen: {
            d.advance();
            ASTNode* inner = parseBinaryExpr(d, 0);
            if (d.hasMore()) {
                Token close = d.getToken();
                if (close.type == TokenType::RightParen) {
                    d.advance();
                }
            }
            return inner;
        }

        // .{expr, ...} tuple literal
        case TokenType::Dot: {
            d.advance();
            Token next_tok = d.getToken();
            if (next_tok.type == TokenType::LeftBrace) {
                d.advance();
                ASTNode* n = createDefaultAstNode();
                n->node_type = ASTNodeType::ArrayLiteral;
                n->token = tok;
                n->children = createChildList();
                size_t guard_t = 0;
                while (d.hasMore()) {
                    guard_t++;
                    if (guard_t > 200) break;
                    Token cur = d.getToken();
                    if (cur.type == TokenType::RightBrace) {
                        d.advance();
                        break;
                    }
                    ASTNode* elem = parseBinaryExpr(d, 0);
                    if (elem) {
                        n->children->push_back(elem);
                    }
                    if (d.hasMore()) {
                        Token comma = d.getToken();
                        if (comma.type == TokenType::Comma) d.advance();
                    }
                }
                return n;
            }
            // plain dot without brace — back up
            d.token_index--;
            return nullptr;
        }

        default:
            return nullptr;
    }
}

static constexpr size_t MAX_EXPR_LOOP = 10000;

ASTNode* parseBinaryExpr(ASTData& d, size_t min_prec) {
    ASTNode* left = parsePrimary(d);

    size_t loop_guard = 0;
    while (d.hasMore()) {
        loop_guard++;
        if (loop_guard >= MAX_EXPR_LOOP) throw AstError("Infinite while loop");

        Token op_tok = (*d.token_list)[d.token_index];

        if (!isBinaryOperator(op_tok.type)) break;

        size_t prec = getPrecedence(op_tok.type);
        if (prec < min_prec) break;

        d.advance();

        // ptr.* dereference
        if (op_tok.type == TokenType::Dot) {
            std::optional<Token> peek_star = d.peekToken(0);
            if (peek_star && peek_star->type == TokenType::Star) {
                d.advance();
                ASTNode* deref = createDefaultAstNode();
                deref->node_type = ASTNodeType::UnaryExpression;
                Token star_tok = *peek_star;
                star_tok.value = ".*";
                deref->token = star_tok;
                deref->left = left;
                left = deref;
                continue;
            }
        }

        ASTNode* right = parseBinaryExpr(d, prec + 1);

        if (!right) {
            d.setError("Expected expression after operator", op_tok);
            throw AstError("Unexpected type");
        }

        ASTNode* bin = createDefaultAstNode();
        if (op_tok.type == TokenType::Dot) {
            bin->node_type = ASTNodeType::MemberAccess;
        } else {
            bin->node_type = ASTNodeType::BinaryExpression;
        }
        bin->token = op_tok;
        bin->left = left;
        bin->right = right;

        left = bin;
    }

    return left;
}

ASTNode* parseFunctionCallNode(ASTData& d, const Token& name_tok) {
    ASTNode* call = createDefaultAstNode();
    call->node_type = ASTNodeType::FunctionCall;
    call->token = name_tok;
    call->children = createChildList();

    d.advance(); // eat '('

    size_t loop_guard = 0;
    while (d.hasMore()) {
        loop_guard++;
        if (loop_guard >= MAX_EXPR_LOOP) throw AstError("Infinite while loop");

        Token cur = d.getToken();
        if (cur.type == TokenType::RightParen) {
            d.advance();
            break;
        }

        ASTNode* arg_expr = parseBinaryExpr(d, 0);
        if (!arg_expr) break;

        ASTNode* arg_node = createDefaultAstNode();
        arg_node->node_type = ASTNodeType::Argument;
        arg_node->left = arg_expr;

        call->children->push_back(arg_node);

        if (d.hasMore()) {
            Token sep = d.getToken();
            if (sep.type == TokenType::Comma) {
                d.advance();
            }
        }
    }

    return call;
}

} // namespace razen
