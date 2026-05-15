#include "parser.h"
#include "../ast/ast_data.h"
#include "../ast/ast_utils.h"
#include "../ast/token_utils.h"
#include "../ast/expression.h"
#include "../ast/type_parser.h"
#include "../lexer/lexer.h"
#include <iostream>
#include <cstring>

namespace razen {

static constexpr size_t MAX_LOOP = 100000;

// forward declarations
static void processGlobalToken(ASTData& d);
static void processStatement(ASTData& d, ASTNode* body);
static ASTNode* parseVarDecl(ASTData& d, bool is_mut);
static ASTNode* parseIdentifierStatement(ASTData& d, bool is_mut);
static ASTNode* parseConst(ASTData& d);
static ASTNode* parseAssignment(ASTData& d);
static ASTNode* parseFuncDecl(ASTData& d, bool is_pub);
static ASTNode* parseParams(ASTData& d);
static ASTNode* parseBlock(ASTData& d);
static ASTNode* parseCallNode(ASTData& d, const Token& name_tok);
static ASTNode* parseReturn(ASTData& d);
static ASTNode* parseIf(ASTData& d);
static ASTNode* parseLoop(ASTData& d);
static ASTNode* parseTryStatement(ASTData& d);
static ASTNode* parseDefer(ASTData& d);
static ASTNode* parseMatch(ASTData& d);
static ASTNode* parseStruct(ASTData& d, bool is_pub);
static ASTNode* parseEnum(ASTData& d, bool is_pub);
static ASTNode* parseUnion(ASTData& d, bool is_pub);
static ASTNode* parseErrorMap(ASTData& d);
static ASTNode* parseTypeAlias(ASTData& d, bool is_pub);
static ASTNode* parseModule(ASTData& d);
static ASTNode* parseUse(ASTData& d);
static ASTNode* parseBehave(ASTData& d);
static ASTNode* parseExt(ASTData& d);
static ASTNode* finishInferred(ASTData& d, const Token& name_tok, bool is_mut);
static ASTNode* finishExplicit(ASTData& d, const Token& name_tok, bool is_mut, bool is_global);
static void consumeSemi(ASTData& d);
static ASTNode* parseTypeNodeWrapper(ASTData& d);

// ── consume a ';' if present ───────────────────────────────────────────────

static void consumeSemi(ASTData& d) {
    std::optional<Token> t = d.peekToken(0);
    if (t && t->type == TokenType::Semicolon) d.advance();
}

// ── entry point ────────────────────────────────────────────────────────────

std::vector<ASTNode*> buildAST(const std::vector<Token>& token_list, std::string_view source) {
    (void)source;

    std::cout << "\t" << GREY << "Formatting AST" << RESET << "\t\t\t";

    ASTData ast_data;
    ast_data.ast_nodes = new std::vector<ASTNode*>();
    ast_data.token_list = &token_list;
    ast_data.token_index = 0;

    size_t count = token_list.size();
    size_t loop_guard = 0;

    while (ast_data.token_index < count) {
        loop_guard++;
        if (loop_guard >= MAX_LOOP) throw AstError("Infinite while loop");

        size_t before = ast_data.token_index;

        try {
            processGlobalToken(ast_data);
        } catch (const AstError& err) {
            std::cout << RED << "Error: " << err.what() << RESET << "\n";
            if (ast_data.error_detail) {
                std::cout << "\t" << GREY << "Detail:" << RESET << "  " << *ast_data.error_detail << "\n";
            }
            if (ast_data.error_token) {
                std::cout << "\t" << GREY << "Near:" << RESET << "    '"
                          << ast_data.error_token->value << "' ("
                          << static_cast<int>(ast_data.error_token->type) << ")\n";
            }
            throw;
        }

        if (before == ast_data.token_index) ast_data.token_index++;
    }

    std::cout << CYAN << "Done" << RESET << "\n";
    return *ast_data.ast_nodes;
}

// ── top-level dispatcher ────────────────────────────────────────────────────

static void processGlobalToken(ASTData& d) {
    d.error_function = "processGlobalToken";
    Token tok = d.getToken();

    switch (tok.type) {
        case TokenType::Comment:
        case TokenType::EndComment:
        case TokenType::EOF_:
            d.advance();
            break;

        case TokenType::At: {
            // annotation: parse it, then attach to next declaration
            ASTNode* annotation = parsePrimary(d);
            if (!annotation) {
                d.setError("Expected annotation name after @", tok);
                throw AstError("Unexpected type");
            }
            if (d.hasMore()) {
                size_t sub_node = d.ast_nodes->size();
                processGlobalToken(d);
                if (d.ast_nodes->size() > sub_node) {
                    ASTNode* parsed_n = (*d.ast_nodes)[d.ast_nodes->size() - 1];
                    // store annotation in children (don't overwrite middle—it holds params for func)
                    appendChild(parsed_n, annotation);
                    // if this is @Generic(T), extract generic param names
                    if (annotation->token && annotation->token->value == "Generic" && annotation->children) {
                        ASTNode* gp = createDefaultAstNode();
                        gp->node_type = ASTNodeType::GenericParams;
                        gp->children = createChildList();
                        // transfer identifiers from annotation to GenericParams, then clear annotation
                        for (auto* arg : *annotation->children) {
                            if (arg->node_type == ASTNodeType::Identifier)
                                gp->children->push_back(arg);
                        }
                        annotation->children->clear();
                        if (gp->children->size() > 0)
                            appendChild(parsed_n, gp);
                    }
                }
            }
            break;
        }

        case TokenType::Pub: {
            d.advance();
            Token nxt = d.getToken();
            switch (nxt.type) {
                case TokenType::Func: {
                    ASTNode* n = parseFuncDecl(d, true);
                    d.ast_nodes->push_back(n);
                    break;
                }
                case TokenType::Async: {
                    d.advance();
                    ASTNode* n = parseFuncDecl(d, true);
                    n->is_async = true;
                    d.ast_nodes->push_back(n);
                    break;
                }
                case TokenType::Struct: {
                    ASTNode* n = parseStruct(d, true);
                    d.ast_nodes->push_back(n);
                    break;
                }
                case TokenType::Enum: {
                    ASTNode* n = parseEnum(d, true);
                    d.ast_nodes->push_back(n);
                    break;
                }
                case TokenType::Union: {
                    ASTNode* n = parseUnion(d, true);
                    d.ast_nodes->push_back(n);
                    break;
                }
                case TokenType::Type: {
                    ASTNode* n = parseTypeAlias(d, true);
                    d.ast_nodes->push_back(n);
                    break;
                }
                case TokenType::Behave: {
                    ASTNode* n = parseBehave(d);
                    n->is_pub = true;
                    d.ast_nodes->push_back(n);
                    break;
                }
                case TokenType::Ext: {
                    ASTNode* n = parseExt(d);
                    n->is_pub = true;
                    d.ast_nodes->push_back(n);
                    break;
                }
                case TokenType::Const: {
                    d.advance();
                    // distinguish const func from const var
                    if (d.hasMore()) {
                        Token ct = d.getToken();
                        if (ct.type == TokenType::Func) {
                            ASTNode* n = parseFuncDecl(d, true);
                            n->is_const = true;
                            d.ast_nodes->push_back(n);
                        } else {
                            ASTNode* n = parseVarDecl(d, false);
                            n->node_type = ASTNodeType::ConstDeclaration;
                            n->is_const = true;
                            n->is_pub = true;
                            d.ast_nodes->push_back(n);
                        }
                    }
                    break;
                }
                case TokenType::Mod: {
                    ASTNode* n = parseModule(d);
                    n->is_pub = true;
                    d.ast_nodes->push_back(n);
                    break;
                }
                case TokenType::Use: {
                    ASTNode* n = parseUse(d);
                    n->is_pub = true;
                    d.ast_nodes->push_back(n);
                    break;
                }
                default: {
                    d.setError("Expected declaration after 'pub'", nxt);
                    throw AstError("Unexpected type");
                }
            }
            break;
        }

        case TokenType::Struct: {
            ASTNode* n = parseStruct(d, false);
            d.ast_nodes->push_back(n);
            break;
        }
        case TokenType::Enum: {
            ASTNode* n = parseEnum(d, false);
            d.ast_nodes->push_back(n);
            break;
        }
        case TokenType::Union: {
            ASTNode* n = parseUnion(d, false);
            d.ast_nodes->push_back(n);
            break;
        }
        case TokenType::Type: {
            ASTNode* n = parseTypeAlias(d, false);
            d.ast_nodes->push_back(n);
            break;
        }
        case TokenType::Error: {
            ASTNode* n = parseErrorMap(d);
            d.ast_nodes->push_back(n);
            break;
        }
        case TokenType::Mod: {
            ASTNode* n = parseModule(d);
            d.ast_nodes->push_back(n);
            break;
        }
        case TokenType::Use: {
            ASTNode* n = parseUse(d);
            d.ast_nodes->push_back(n);
            break;
        }
        case TokenType::Behave: {
            ASTNode* n = parseBehave(d);
            d.ast_nodes->push_back(n);
            break;
        }
        case TokenType::Ext: {
            ASTNode* n = parseExt(d);
            d.ast_nodes->push_back(n);
            break;
        }

        case TokenType::Async: {
            d.advance();
            ASTNode* n = parseFuncDecl(d, false);
            n->is_async = true;
            d.ast_nodes->push_back(n);
            break;
        }

        case TokenType::Func: {
            ASTNode* n = parseFuncDecl(d, false);
            d.ast_nodes->push_back(n);
            break;
        }

        case TokenType::Const: {
            ASTNode* n = parseConst(d);
            d.ast_nodes->push_back(n);
            break;
        }

        case TokenType::Mut: {
            ASTNode* n = parseVarDecl(d, true);
            d.ast_nodes->push_back(n);
            break;
        }

        case TokenType::Identifier: {
            ASTNode* n = parseIdentifierStatement(d, false);
            d.ast_nodes->push_back(n);
            break;
        }

        default: {
            d.setError("Unrecognised token at top level", tok);
            d.advance();
            break;
        }
    }
}

// ── statement dispatcher (inside function bodies / blocks) ─────────────────

static void processStatement(ASTData& d, ASTNode* body) {
    d.error_function = "processStatement";
    Token tok = d.getToken();

    switch (tok.type) {
        case TokenType::Comment:
        case TokenType::EndComment:
        case TokenType::EOF_:
            d.advance();
            break;
        case TokenType::RightBrace:
            return;

        case TokenType::Ret: {
            ASTNode* n = parseReturn(d);
            appendChild(body, n);
            break;
        }
        case TokenType::If: {
            ASTNode* n = parseIf(d);
            appendChild(body, n);
            break;
        }
        case TokenType::Match: {
            ASTNode* n = parseMatch(d);
            appendChild(body, n);
            break;
        }
        case TokenType::Defer: {
            ASTNode* n = parseDefer(d);
            appendChild(body, n);
            break;
        }
        case TokenType::Loop: {
            ASTNode* n = parseLoop(d);
            appendChild(body, n);
            break;
        }
        case TokenType::Break: {
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::BreakStatement;
            n->token = tok;
            d.advance();
            consumeSemi(d);
            appendChild(body, n);
            break;
        }
        case TokenType::Skip: {
            ASTNode* n = createDefaultAstNode();
            n->node_type = ASTNodeType::SkipStatement;
            n->token = tok;
            d.advance();
            consumeSemi(d);
            appendChild(body, n);
            break;
        }
        case TokenType::Const: {
            ASTNode* n = parseConst(d);
            appendChild(body, n);
            break;
        }
        case TokenType::Mut: {
            ASTNode* n = parseVarDecl(d, true);
            appendChild(body, n);
            break;
        }
        case TokenType::Identifier: {
            ASTNode* n = parseIdentifierStatement(d, false);
            appendChild(body, n);
            break;
        }
        case TokenType::Try: {
            ASTNode* n = parseTryStatement(d);
            appendChild(body, n);
            break;
        }
        default: {
            d.advance();
            break;
        }
    }
}

// ── parseConst ─────────────────────────────────────────────────────────────

static ASTNode* parseConst(ASTData& d) {
    d.error_function = "parseConst";
    d.advance();

    if (!d.hasMore()) throw AstError("Unexpected type");

    Token nxt = d.getToken();
    if (nxt.type == TokenType::Func) {
        ASTNode* n = parseFuncDecl(d, false);
        n->is_const = true;
        return n;
    }

    bool is_mut = false;
    if (nxt.type == TokenType::Mut) {
        is_mut = true;
        d.advance();
    }

    ASTNode* n = parseVarDecl(d, is_mut);
    n->node_type = ASTNodeType::ConstDeclaration;
    n->is_const = true;
    return n;
}

// ── parseVarDecl ────────────────────────────────────────────────────────────

static ASTNode* parseVarDecl(ASTData& d, bool caller_mut) {
    d.error_function = "parseVarDecl";

    bool is_mut = caller_mut;
    if (d.hasMore()) {
        Token cur = d.getToken();
        if (cur.type == TokenType::Mut) {
            is_mut = true;
            d.advance();
        }
    }

    Token name_tok = d.getToken();
    if (name_tok.type != TokenType::Identifier) {
        d.setError("Expected variable name (identifier)", name_tok);
        throw AstError("Unexpected type");
    }
    d.advance();

    Token sep = d.getToken();

    if (sep.type == TokenType::ColonEquals) {
        d.advance();
        return finishInferred(d, name_tok, is_mut);
    }

    if (sep.type == TokenType::Colon) {
        d.advance();
        return finishExplicit(d, name_tok, is_mut, false);
    }

    d.setError("Expected ':' or ':=' in variable declaration", sep);
    throw AstError("Unexpected type");
}

// ── finishInferred ──────────────────────────────────────────────────────────

static ASTNode* finishInferred(ASTData& d, const Token& name_tok, bool is_mut) {
    ASTNode* value = nullptr;
    std::optional<Token> peek = d.peekToken(0);
    if (peek && peek->type == TokenType::Try) {
        value = parseTryStatement(d);
    } else {
        value = parseBinaryExpr(d, 0);
        consumeSemi(d);
    }

    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::VarDeclaration;
    n->token = name_tok;
    n->right = value;
    n->is_mut = is_mut;
    return n;
}

// ── finishExplicit ──────────────────────────────────────────────────────────

static ASTNode* finishExplicit(ASTData& d, const Token& name_tok, bool is_mut, bool is_global) {
    ASTNode* type_node = parseTypeNodeWrapper(d);

    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::VarDeclaration;
    n->token = name_tok;
    n->left = type_node;
    n->is_mut = is_mut;
    n->is_global = is_global;

    if (d.hasMore()) {
        Token eq_or_semi = d.getToken();
        if (eq_or_semi.type == TokenType::Equals) {
            d.advance();
            std::optional<Token> peek = d.peekToken(0);
            if (peek && peek->type == TokenType::Try) {
                n->right = parseTryStatement(d);
            } else {
                n->right = parseBinaryExpr(d, 0);
                consumeSemi(d);
            }
        } else if (eq_or_semi.type == TokenType::Semicolon) {
            d.advance();
        }
    }

    return n;
}

// ── parseIdentifierStatement ────────────────────────────────────────────────

static ASTNode* parseIdentifierStatement(ASTData& d, bool is_mut) {
    Token name_tok = d.getToken();
    std::optional<Token> nxt = d.peekToken(1);

    if (!nxt.has_value()) {
        d.advance();
        ASTNode* n = createDefaultAstNode();
        n->node_type = ASTNodeType::Identifier;
        n->token = name_tok;
        return n;
    }

    TokenType nxt_tt = nxt->type;

    // name : type …  explicit declaration
    if (nxt_tt == TokenType::Colon) {
        return parseVarDecl(d, is_mut);
    }
    // name := expr  inferred declaration
    if (nxt_tt == TokenType::ColonEquals) {
        return parseVarDecl(d, is_mut);
    }
    // name = expr; or name += expr; etc.
    if (isAssignmentOperator(nxt_tt)) {
        return parseAssignment(d);
    }
    // name(args…) function call
    if (nxt_tt == TokenType::LeftParen) {
        d.advance();
        ASTNode* node = parseCallNode(d, name_tok);
        consumeSemi(d);
        return node;
    }

    // parse as binary expression (e.g. c.value += 1)
    ASTNode* expr = parseBinaryExpr(d, 0);

    if (d.hasMore()) {
        Token maybe_op = d.getToken();
        if (isAssignmentOperator(maybe_op.type)) {
            d.advance();
            ASTNode* rhs = parseBinaryExpr(d, 0);
            consumeSemi(d);
            ASTNode* assign = createDefaultAstNode();
            assign->node_type = ASTNodeType::Assignment;
            assign->token = maybe_op;
            assign->left = expr;
            assign->right = rhs;
            return assign;
        }
    }

    consumeSemi(d);
    if (expr) return expr;
    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::Identifier;
    n->token = name_tok;
    return n;
}

// ── parseAssignment ─────────────────────────────────────────────────────────

static ASTNode* parseAssignment(ASTData& d) {
    d.error_function = "parseAssignment";

    Token name_tok = d.getToken();
    d.advance();
    Token op_tok = d.getToken();
    d.advance();

    ASTNode* value = parseBinaryExpr(d, 0);
    consumeSemi(d);

    ASTNode* id = createDefaultAstNode();
    id->node_type = ASTNodeType::Identifier;
    id->token = name_tok;

    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::Assignment;
    n->token = op_tok;
    n->left = id;
    n->right = value;
    return n;
}

// ── parseFuncDecl ───────────────────────────────────────────────────────────

static ASTNode* parseFuncDecl(ASTData& d, bool is_pub) {
    d.error_function = "parseFuncDecl";
    d.advance(); // eat 'func'

    Token name_tok = d.getToken();
    if (name_tok.type != TokenType::Identifier) {
        d.setError("Expected function name after 'func'", name_tok);
        throw AstError("Unexpected type");
    }
    d.advance();

    // check for ~> rename: func new_name ~> Trait.original(args) -> ret { body }
    std::optional<Token> tilde_check = d.peekToken(0);
    if (tilde_check && tilde_check->type == TokenType::TildeArrow) {
        d.advance(); // consume ~>
        // read the trait.method identifier chain
        while (d.hasMore()) {
            Token pt = d.getToken();
            if (pt.type == TokenType::Identifier) {
                d.advance();
            } else if (pt.type == TokenType::Dot) {
                d.advance();
            } else {
                break;
            }
        }
    }

    Token lp = d.getToken();
    if (lp.type != TokenType::LeftParen) {
        d.setError("Expected '(' after function name", lp);
        throw AstError("Unexpected type");
    }
    d.advance();

    ASTNode* params = parseParams(d);

    // optional ->
    if (d.hasMore()) {
        Token maybe_arrow = d.getToken();
        if (maybe_arrow.type == TokenType::Arrow) {
            d.advance();
        }
    }

    ASTNode* ret_node = createDefaultAstNode();
    ret_node->node_type = ASTNodeType::ReturnType;
    if (d.hasMore()) {
        Token t1 = d.getToken();
        if (t1.type != TokenType::LeftBrace) {
            ret_node->left = parseTypeNodeWrapper(d);
        }
    }

    ASTNode* body = nullptr;
    if (d.hasMore()) {
        Token maybe_lb = d.getToken();
        if (maybe_lb.type == TokenType::LeftBrace) {
            body = parseBlock(d);
        }
    }

    ASTNode* func = createDefaultAstNode();
    func->node_type = ASTNodeType::FunctionDeclaration;
    func->token = name_tok;
    func->left = ret_node;
    func->middle = params;
    func->right = body;
    func->is_pub = is_pub;
    return func;
}

// ── parseParams ─────────────────────────────────────────────────────────────

static ASTNode* parseParams(ASTData& d) {
    d.error_function = "parseParams";

    ASTNode* params = createDefaultAstNode();
    params->node_type = ASTNodeType::Parameters;
    params->children = createChildList();

    size_t guard = 0;
    while (d.hasMore()) {
        guard++;
        if (guard >= MAX_LOOP) throw AstError("Infinite while loop");

        Token cur = d.getToken();
        if (cur.type == TokenType::RightParen) {
            d.advance();
            break;
        }
        if (cur.type == TokenType::Comma) {
            d.advance();
            continue;
        }
        if (cur.type == TokenType::Semicolon) {
            d.advance();
            continue;
        }
        if (cur.type == TokenType::DotDotDot) {
            d.advance();
            ASTNode* p = createDefaultAstNode();
            p->node_type = ASTNodeType::Parameter;
            p->token = cur;
            params->children->push_back(p);
            continue;
        }

        bool param_is_mut = false;
        bool param_is_const = false;
        Token name_tok = cur;

        if (cur.type == TokenType::Mut) {
            param_is_mut = true;
            d.advance();
            name_tok = d.getToken();
        } else if (cur.type == TokenType::Const) {
            param_is_const = true;
            d.advance();
            name_tok = d.getToken();
        }

        if (name_tok.type != TokenType::Identifier) {
            d.setError("Expected parameter name", name_tok);
            throw AstError("Unexpected type");
        }
        d.advance();

        Token colon = d.getToken();
        if (colon.type != TokenType::Colon) {
            d.setError("Expected ':' after parameter name", colon);
            throw AstError("Unexpected type");
        }
        d.advance();

        ASTNode* type_node = parseTypeNodeWrapper(d);

        ASTNode* param = createDefaultAstNode();
        param->node_type = ASTNodeType::Parameter;
        param->token = name_tok;
        param->left = type_node;
        param->is_mut = param_is_mut;
        param->is_const = param_is_const;

        params->children->push_back(param);

        if (d.hasMore()) {
            Token maybe_comma = d.getToken();
            if (maybe_comma.type == TokenType::Comma) d.advance();
        }
    }

    return params;
}

// ── parseBlock ──────────────────────────────────────────────────────────────

static ASTNode* parseBlock(ASTData& d) {
    d.error_function = "parseBlock";

    if (d.hasMore()) {
        Token b = d.getToken();
        if (b.type == TokenType::LeftBrace) d.advance();
    }

    ASTNode* block = createDefaultAstNode();
    block->node_type = ASTNodeType::Block;

    size_t guard = 0;
    while (d.hasMore()) {
        guard++;
        if (guard >= MAX_LOOP) throw AstError("Infinite while loop");

        Token cur = d.getToken();
        if (cur.type == TokenType::RightBrace) {
            d.advance();
            break;
        }

        processStatement(d, block);
    }

    return block;
}

// ── parseCallNode ───────────────────────────────────────────────────────────

static ASTNode* parseCallNode(ASTData& d, const Token& name_tok) {
    ASTNode* call = createDefaultAstNode();
    call->node_type = ASTNodeType::FunctionCall;
    call->token = name_tok;
    call->children = createChildList();

    d.advance(); // eat '('

    size_t guard = 0;
    while (d.hasMore()) {
        guard++;
        if (guard >= MAX_LOOP) throw AstError("Infinite while loop");

        Token cur = d.getToken();
        if (cur.type == TokenType::RightParen) {
            d.advance();
            break;
        }

        ASTNode* arg_expr = parseBinaryExpr(d, 0);
        if (arg_expr) {
            ASTNode* arg = createDefaultAstNode();
            arg->node_type = ASTNodeType::Argument;
            arg->left = arg_expr;
            call->children->push_back(arg);
        }

        if (d.hasMore()) {
            Token mc = d.getToken();
            if (mc.type == TokenType::Comma) d.advance();
        }
    }

    return call;
}

// ── parseReturn ─────────────────────────────────────────────────────────────

static ASTNode* parseReturn(ASTData& d) {
    d.error_function = "parseReturn";
    Token ret_tok = d.getToken();
    d.advance();

    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::ReturnStatement;
    n->token = ret_tok;

    if (d.hasMore()) {
        Token nxt = d.getToken();
        if (nxt.type != TokenType::Semicolon && nxt.type != TokenType::RightBrace) {
            n->left = parseBinaryExpr(d, 0);
        }
    }
    consumeSemi(d);
    return n;
}

// ── parseIf ─────────────────────────────────────────────────────────────────

static ASTNode* parseIf(ASTData& d) {
    d.error_function = "parseIf";
    Token if_tok = d.getToken();
    d.advance();

    ASTNode* cond = parseBinaryExpr(d, 0);

    ASTNode* if_body = parseBlock(d);
    if_body->node_type = ASTNodeType::IfBody;

    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::IfStatement;
    n->token = if_tok;
    n->left = cond;
    n->middle = if_body;

    if (d.hasMore()) {
        Token maybe_else = d.getToken();
        if (maybe_else.type == TokenType::Else) {
            d.advance();
            // check for else if
            if (d.hasMore()) {
                std::optional<Token> peek = d.peekToken(0);
                if (peek && peek->type == TokenType::If) {
                    ASTNode* else_if = parseIf(d);
                    else_if->node_type = ASTNodeType::ElseIfStatement;
                    n->right = else_if;
                } else {
                    ASTNode* else_body = parseBlock(d);
                    else_body->node_type = ASTNodeType::ElseBody;
                    n->right = else_body;
                }
            }
        }
    }

    return n;
}

// ── parseLoop ───────────────────────────────────────────────────────────────

static ASTNode* parseLoop(ASTData& d) {
    d.error_function = "parseLoop";
    Token tok = d.getToken();
    d.advance();

    ASTNode* loop_st = createDefaultAstNode();
    loop_st->node_type = ASTNodeType::LoopStatement;
    loop_st->token = tok;

    std::optional<Token> peek = d.peekToken(0);
    if (peek && peek->type != TokenType::LeftBrace) {
        // check for |item| { pattern
        bool found_pipe = false;
        size_t pipe_idx = 0;
        for (size_t i = 0; d.token_index + i < d.token_list->size(); ++i) {
            const Token& t = (*d.token_list)[d.token_index + i];
            if (t.type == TokenType::LeftBrace) break;
            if (t.type == TokenType::Or) {
                size_t idx = d.token_index + i;
                if (idx + 3 < d.token_list->size()) {
                    if ((*d.token_list)[idx + 1].type == TokenType::Identifier &&
                        (*d.token_list)[idx + 2].type == TokenType::Or &&
                        (*d.token_list)[idx + 3].type == TokenType::LeftBrace)
                    {
                        found_pipe = true;
                        pipe_idx = idx;
                        break;
                    }
                }
            }
        }

        if (found_pipe) {
            const_cast<Token&>((*d.token_list)[pipe_idx]).type = TokenType::Semicolon;
        }
        loop_st->left = parseBinaryExpr(d, 0);
        if (found_pipe) {
            const_cast<Token&>((*d.token_list)[pipe_idx]).type = TokenType::Or;
        }

        std::optional<Token> nx = d.peekToken(0);
        if (nx && nx->type == TokenType::Or) {
            d.advance(); // |
            Token item = d.getToken();
            if (item.type == TokenType::Identifier) {
                ASTNode* item_n = createDefaultAstNode();
                item_n->node_type = ASTNodeType::Identifier;
                item_n->token = item;
                loop_st->middle = item_n;
                d.advance();

                std::optional<Token> nx2 = d.peekToken(0);
                if (nx2 && nx2->type == TokenType::Or) d.advance();
            } else {
                d.advance();
            }
        }
    }

    Token lb = d.getToken();
    if (lb.type != TokenType::LeftBrace) {
        d.setError("Expected '{' for loop body", lb);
        throw AstError("Unexpected type");
    }

    ASTNode* b = parseBlock(d);

    ASTNode* body_node = createDefaultAstNode();
    body_node->node_type = ASTNodeType::LoopBody;
    body_node->children = b->children;
    b->children = nullptr; // transfer ownership
    loop_st->right = body_node;

    return loop_st;
}

// ── parseTryStatement ────────────────────────────────────────────────────────

static ASTNode* parseTryStatement(ASTData& d) {
    d.error_function = "parseTryStatement";
    Token try_tok = d.getToken();
    d.advance();

    // check for block try: try { ... } catch (err) { ... }
    std::optional<Token> peek = d.peekToken(0);
    if (peek && peek->type == TokenType::LeftBrace) {
        ASTNode* try_body = parseBlock(d);

        ASTNode* try_node = createDefaultAstNode();
        try_node->node_type = ASTNodeType::TryBlock;
        try_node->token = try_tok;
        try_node->left = try_body;

        if (d.hasMore()) {
            Token maybe_catch = d.getToken();
            if (maybe_catch.type == TokenType::Catch) {
                d.advance();

                // optional (err) or |err| capture
                if (d.hasMore()) {
                    Token open = d.getToken();
                    if (open.type == TokenType::LeftParen || open.type == TokenType::Or) {
                        d.advance();
                        Token e_tok = d.getToken();
                        if (e_tok.type == TokenType::Identifier) {
                            d.advance();
                        }
                        Token close = d.getToken();
                        if (close.type == TokenType::RightParen || close.type == TokenType::Or) {
                            d.advance();
                        }
                    }
                }

                ASTNode* catch_node = createDefaultAstNode();
                catch_node->node_type = ASTNodeType::CatchExpression;
                catch_node->token = maybe_catch;

                if (d.hasMore()) {
                    Token pt = d.getToken();
                    if (pt.type == TokenType::LeftBrace) {
                        catch_node->left = parseBlock(d);
                    } else {
                        catch_node->left = parseBinaryExpr(d, 0);
                    }
                }
                try_node->right = catch_node;
            }
        }
        return try_node;
    }

    // expression try: try expr catch |err| { ... }
    ASTNode* expr = parseBinaryExpr(d, 0);

    ASTNode* try_node = createDefaultAstNode();
    try_node->node_type = ASTNodeType::TryExpression;
    try_node->token = try_tok;
    try_node->left = expr;

    if (d.hasMore()) {
        Token maybe_catch = d.getToken();
        if (maybe_catch.type == TokenType::Catch) {
            d.advance();

            // optional |e| capture
            if (d.hasMore()) {
                Token pipe = d.getToken();
                if (pipe.type == TokenType::Or) {
                    d.advance();
                    Token e_tok = d.getToken();
                    d.advance();
                    Token pipe2 = d.getToken();
                    if (pipe2.type == TokenType::Or) d.advance();
                    (void)e_tok;
                }
            }

            ASTNode* catch_node = createDefaultAstNode();
            catch_node->node_type = ASTNodeType::CatchExpression;
            catch_node->token = maybe_catch;

            if (d.hasMore()) {
                Token peek_tok = d.getToken();
                if (peek_tok.type == TokenType::LeftBrace) {
                    catch_node->left = parseBlock(d);
                } else {
                    catch_node->left = parseBinaryExpr(d, 0);
                }
            }
            try_node->right = catch_node;
        }
    }

    consumeSemi(d);
    return try_node;
}

// ── parseDefer ──────────────────────────────────────────────────────────────

static ASTNode* parseDefer(ASTData& d) {
    d.error_function = "parseDefer";
    Token tok = d.getToken();
    d.advance();

    ASTNode* stmt = createDefaultAstNode();
    stmt->node_type = ASTNodeType::DeferStatement;
    stmt->token = tok;

    ASTNode* s2 = createDefaultAstNode();
    s2->node_type = ASTNodeType::Block;
    s2->children = createChildList();

    Token peek_tok = d.getToken();
    if (peek_tok.type == TokenType::LeftBrace) {
        stmt->left = parseBlock(d);
    } else {
        processStatement(d, s2);
        stmt->left = s2;
    }
    return stmt;
}

// ── parseMatch ──────────────────────────────────────────────────────────────

static ASTNode* parseMatch(ASTData& d) {
    d.error_function = "parseMatch";
    Token tok = d.getToken();
    d.advance();

    ASTNode* expr = parseBinaryExpr(d, 0);

    ASTNode* match_node = createDefaultAstNode();
    match_node->node_type = ASTNodeType::MatchStatement;
    match_node->token = tok;
    match_node->left = expr;
    match_node->children = createChildList();

    Token lb = d.getToken();
    if (lb.type == TokenType::LeftBrace) d.advance();

    size_t guard = 0;
    while (d.hasMore()) {
        guard++;
        if (guard >= MAX_LOOP) throw AstError("Infinite while loop");

        Token cur = d.getToken();
        if (cur.type == TokenType::RightBrace) {
            d.advance();
            break;
        }

        ASTNode* case_node = createDefaultAstNode();
        case_node->node_type = ASTNodeType::MatchCase;

        if (cur.type == TokenType::Else) {
            case_node->token = cur;
            d.advance();
            // consume => arrow for else case too
            std::optional<Token> peek_arr = d.peekToken(0);
            if (peek_arr && (peek_arr->type == TokenType::Arrow || peek_arr->value == "=>")) {
                d.advance();
            }
        } else {
            ASTNode* c_expr = parseBinaryExpr(d, 0);
            case_node->left = c_expr;
            // struct destructuring pattern: Expr.Binary { left, right, op }
            if (d.hasMore()) {
                Token maybe_brace = d.getToken();
                if (maybe_brace.type == TokenType::LeftBrace) {
                    case_node->middle = parseBlock(d);
                }
            }
        }

        Token arr = d.getToken();
        if (arr.type == TokenType::Arrow || arr.value == "=>") {
            d.advance();
        } else if (arr.type == TokenType::Equals &&
                   d.peekToken(1) && d.peekToken(1)->type == TokenType::GreaterThan) {
            d.advance();
            d.advance();
        }

        Token b_peek = d.getToken();
        ASTNode* body_n = createDefaultAstNode();
        body_n->node_type = ASTNodeType::MatchBody;
        if (b_peek.type == TokenType::LeftBrace) {
            body_n->left = parseBlock(d);
        } else {
            ASTNode* blk = createDefaultAstNode();
            blk->node_type = ASTNodeType::Block;
            blk->children = createChildList();
            processStatement(d, blk);
            body_n->left = blk;
        }
        case_node->right = body_n;

        match_node->children->push_back(case_node);

        Token comma = d.getToken();
        if (comma.type == TokenType::Comma) {
            d.advance();
        }
    }

    return match_node;
}

// ── parseTypeNodeWrapper ────────────────────────────────────────────────────

static ASTNode* parseTypeNodeWrapper(ASTData& d) {
    return parseTypeNode(d);
}

// ── parseStruct ─────────────────────────────────────────────────────────────

static ASTNode* parseStruct(ASTData& d, bool is_pub) {
    d.error_function = "parseStruct";
    d.advance(); // eat 'struct'

    Token name_tok = d.getToken();
    if (name_tok.type != TokenType::Identifier && !isVarType(name_tok.type)) {
        d.setError("Expected struct name", name_tok);
        throw AstError("Unexpected type");
    }
    d.advance();

    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::StructDeclaration;
    n->token = name_tok;
    n->is_pub = is_pub;
    n->children = createChildList();

    // Traits: ~> TraitName
    if (d.hasMore()) {
        Token maybe_tilde = d.getToken();
        if (maybe_tilde.type == TokenType::TildeArrow) {
            d.advance();
            while (d.hasMore()) {
                Token trait_name = d.getToken();
                if (trait_name.type == TokenType::Identifier) {
                    ASTNode* trait_node = createDefaultAstNode();
                    trait_node->node_type = ASTNodeType::Identifier;
                    trait_node->token = trait_name;
                    n->children->push_back(trait_node);
                    d.advance();
                } else {
                    break;
                }
                Token comma = d.getToken();
                if (comma.type == TokenType::Comma) {
                    d.advance();
                } else {
                    break;
                }
            }
        }
    }

    Token lb = d.getToken();
    if (lb.type != TokenType::LeftBrace) {
        d.setError("Expected '{' for struct body", lb);
        throw AstError("Unexpected type");
    }
    d.advance();

    size_t guard = 0;
    while (d.hasMore()) {
        guard++;
        if (guard >= MAX_LOOP) throw AstError("Infinite while loop");
        Token cur = d.getToken();
        if (cur.type == TokenType::RightBrace) {
            d.advance();
            break;
        }

        if (cur.type == TokenType::Func) {
            ASTNode* m = parseFuncDecl(d, false);
            n->children->push_back(m);
        } else if (cur.type == TokenType::Identifier) {
            std::optional<Token> nxt = d.peekToken(1);
            if (nxt && nxt->type == TokenType::Colon) {
                d.advance();
                d.advance();
                ASTNode* type_node = parseTypeNodeWrapper(d);

                ASTNode* field = createDefaultAstNode();
                field->node_type = ASTNodeType::StructField;
                field->token = cur;
                field->left = type_node;

                if (d.hasMore()) {
                    Token maybe_eq = d.getToken();
                    if (maybe_eq.type == TokenType::Equals) {
                        d.advance();
                        field->right = parseBinaryExpr(d, 0);
                    }
                }

                n->children->push_back(field);
                if (d.hasMore()) {
                    Token comma = d.getToken();
                    if (comma.type == TokenType::Comma) d.advance();
                }
            } else if (nxt && nxt->type == TokenType::LeftParen) {
                d.token_index--;
                ASTNode* m = parseFuncDecl(d, false);
                n->children->push_back(m);
            } else {
                d.advance();
            }
        } else {
            d.advance();
        }
    }
    return n;
}

// ── parseEnum ───────────────────────────────────────────────────────────────

static ASTNode* parseEnum(ASTData& d, bool is_pub) {
    d.error_function = "parseEnum";
    d.advance();

    Token name_tok = d.getToken();
    if (name_tok.type != TokenType::Identifier && !isVarType(name_tok.type)) {
        throw AstError("Unexpected type");
    }
    d.advance();

    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::EnumDeclaration;
    n->token = name_tok;
    n->is_pub = is_pub;
    n->children = createChildList();

    // optional backing type: enum Flags: u8 { ... }
    if (d.hasMore()) {
        Token maybe_colon = d.getToken();
        if (maybe_colon.type == TokenType::Colon) {
            d.advance();
            n->left = parseTypeNodeWrapper(d);
        }
    }

    // Traits: ~> TraitName
    if (d.hasMore()) {
        Token maybe_tilde = d.getToken();
        if (maybe_tilde.type == TokenType::TildeArrow) {
            d.advance();
            while (d.hasMore()) {
                Token trait_name = d.getToken();
                if (trait_name.type == TokenType::Identifier) {
                    ASTNode* trait_node = createDefaultAstNode();
                    trait_node->node_type = ASTNodeType::Identifier;
                    trait_node->token = trait_name;
                    n->children->push_back(trait_node);
                    d.advance();
                } else {
                    break;
                }
                Token comma = d.getToken();
                if (comma.type == TokenType::Comma) {
                    d.advance();
                } else {
                    break;
                }
            }
        }
    }

    Token lb = d.getToken();
    if (lb.type != TokenType::LeftBrace) throw AstError("Unexpected type");
    d.advance();

    size_t guard = 0;
    while (d.hasMore()) {
        guard++;
        if (guard >= MAX_LOOP) throw AstError("Infinite while loop");
        Token cur = d.getToken();
        if (cur.type == TokenType::RightBrace) {
            d.advance();
            break;
        }

        if (cur.type == TokenType::Func) {
            ASTNode* m = parseFuncDecl(d, false);
            n->children->push_back(m);
        } else if (cur.type == TokenType::Identifier) {
            ASTNode* field = createDefaultAstNode();
            field->node_type = ASTNodeType::EnumField;
            field->token = cur;
            d.advance();

            std::optional<Token> nxt = d.peekToken(0);
            if (nxt && nxt->type == TokenType::Equals) {
                d.advance();
                ASTNode* c_expr = parseBinaryExpr(d, 0);
                field->right = c_expr;
            } else if (nxt && nxt->type == TokenType::LeftParen) {
                d.token_index--;
                ASTNode* m = parseFuncDecl(d, false);
                n->children->push_back(m);
                // field already added? no, pushed m instead
                // skip field push below
                Token comma = d.getToken();
                if (comma.type == TokenType::Comma) d.advance();
                continue;
            }

            n->children->push_back(field);
            Token comma = d.getToken();
            if (comma.type == TokenType::Comma) d.advance();
        } else {
            d.advance();
        }
    }
    return n;
}

// ── parseUnion ──────────────────────────────────────────────────────────────

static ASTNode* parseUnion(ASTData& d, bool is_pub) {
    d.error_function = "parseUnion";
    d.advance();

    Token name_tok = d.getToken();
    if (name_tok.type != TokenType::Identifier && !isVarType(name_tok.type)) {
        throw AstError("Unexpected type");
    }
    d.advance();

    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::UnionDeclaration;
    n->token = name_tok;
    n->is_pub = is_pub;
    n->children = createChildList();

    // Traits: ~> TraitName
    if (d.hasMore()) {
        Token maybe_tilde = d.getToken();
        if (maybe_tilde.type == TokenType::TildeArrow) {
            d.advance();
            while (d.hasMore()) {
                Token trait_name = d.getToken();
                if (trait_name.type == TokenType::Identifier) {
                    ASTNode* trait_node = createDefaultAstNode();
                    trait_node->node_type = ASTNodeType::Identifier;
                    trait_node->token = trait_name;
                    n->children->push_back(trait_node);
                    d.advance();
                }
                Token comma = d.getToken();
                if (comma.type == TokenType::Comma) {
                    d.advance();
                } else {
                    break;
                }
            }
        }
    }

    Token lb = d.getToken();
    if (lb.type != TokenType::LeftBrace) throw AstError("Unexpected type");
    d.advance();

    size_t guard = 0;
    while (d.hasMore()) {
        guard++;
        if (guard >= MAX_LOOP) throw AstError("Infinite while loop");
        Token cur = d.getToken();
        if (cur.type == TokenType::RightBrace) {
            d.advance();
            break;
        }

        if (cur.type == TokenType::Identifier) {
            ASTNode* field = createDefaultAstNode();
            field->node_type = ASTNodeType::UnionField;
            field->token = cur;
            d.advance();

            std::optional<Token> nxt = d.peekToken(0);
            if (nxt && nxt->type == TokenType::LeftParen) {
                d.advance();
                ASTNode* tp_n = parseTypeNodeWrapper(d);
                field->left = tp_n;
                if (d.hasMore()) {
                    Token rp = d.getToken();
                    if (rp.type == TokenType::RightParen) d.advance();
                }
            } else if (nxt && nxt->type == TokenType::Colon) {
                d.advance();
                field->left = parseTypeNodeWrapper(d);
            } else if (nxt && nxt->type == TokenType::LeftBrace) {
                d.advance();
                field->children = createChildList();
                size_t sg = 0;
                while (d.hasMore()) {
                    sg++;
                    if (sg >= MAX_LOOP) break;
                    Token sc = d.getToken();
                    if (sc.type == TokenType::RightBrace) {
                        d.advance();
                        break;
                    }
                    if (sc.type == TokenType::Comma) {
                        d.advance();
                        continue;
                    }
                    if (sc.type == TokenType::Identifier) {
                        std::optional<Token> peek2 = d.peekToken(1);
                        if (peek2 && peek2->type == TokenType::Colon) {
                            d.advance();
                            d.advance();
                            ASTNode* ft = parseTypeNodeWrapper(d);
                            ASTNode* sf = createDefaultAstNode();
                            sf->node_type = ASTNodeType::StructField;
                            sf->token = sc;
                            sf->left = ft;
                            field->children->push_back(sf);
                            if (d.hasMore()) {
                                Token c2 = d.getToken();
                                if (c2.type == TokenType::Comma) d.advance();
                            }
                        } else {
                            d.advance();
                        }
                    } else {
                        d.advance();
                    }
                }
            }

            n->children->push_back(field);
            if (d.hasMore()) {
                Token comma = d.getToken();
                if (comma.type == TokenType::Comma) d.advance();
            }
        } else if (cur.type == TokenType::Func) {
            ASTNode* m = parseFuncDecl(d, false);
            n->children->push_back(m);
        } else {
            d.advance();
        }
    }
    return n;
}

// ── parseErrorMap ───────────────────────────────────────────────────────────

static ASTNode* parseErrorMap(ASTData& d) {
    d.error_function = "parseErrorMap";
    d.advance();
    Token name_tok = d.getToken();
    if (name_tok.type != TokenType::Identifier && !isVarType(name_tok.type)) {
        throw AstError("Unexpected type");
    }
    d.advance();
    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::ErrorDeclaration;
    n->token = name_tok;
    n->children = createChildList();

    Token lb = d.getToken();
    if (lb.type == TokenType::LeftBrace) d.advance();

    size_t guard = 0;
    while (d.hasMore()) {
        guard++;
        if (guard >= MAX_LOOP) throw AstError("Infinite while loop");
        Token cur = d.getToken();
        if (cur.type == TokenType::RightBrace) {
            d.advance();
            break;
        }
        if (cur.type == TokenType::Identifier) {
            ASTNode* err_field = createDefaultAstNode();
            err_field->node_type = ASTNodeType::ErrorField;
            err_field->token = cur;
            n->children->push_back(err_field);
            d.advance();
            Token comma = d.getToken();
            if (comma.type == TokenType::Comma) d.advance();
        } else {
            d.advance();
        }
    }
    return n;
}

// ── parseTypeAlias ──────────────────────────────────────────────────────────

static ASTNode* parseTypeAlias(ASTData& d, bool is_pub) {
    d.error_function = "parseTypeAlias";
    d.advance();
    Token name_tok = d.getToken();
    d.advance();
    Token eq = d.getToken();
    if (eq.type == TokenType::Equals) d.advance();
    ASTNode* tn = parseTypeNodeWrapper(d);

    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::TypeAliasDeclaration;
    n->token = name_tok;
    n->left = tn;
    n->is_pub = is_pub;
    return n;
}

// ── parseModule ─────────────────────────────────────────────────────────────

static ASTNode* parseModule(ASTData& d) {
    d.error_function = "parseModule";
    d.advance();
    Token name_tok = d.getToken();
    d.advance();
    consumeSemi(d);

    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::ModuleDeclaration;
    n->token = name_tok;
    return n;
}

// ── parseUse ────────────────────────────────────────────────────────────────

static ASTNode* parseUse(ASTData& d) {
    d.error_function = "parseUse";
    d.advance();

    std::string full_path;
    size_t guard = 0;
    while (d.hasMore()) {
        guard++;
        if (guard >= MAX_LOOP) break;
        Token cur = d.getToken();
        if (cur.type == TokenType::Semicolon ||
            cur.type == TokenType::RightBrace ||
            isKeyword(cur.type))
        {
            break;
        }
        if (cur.type == TokenType::Dot) {
            full_path.push_back('.');
            d.advance();
            continue;
        }
        if (cur.type == TokenType::Identifier) {
            full_path += cur.value;
            d.advance();
            continue;
        }
        break;
    }
    consumeSemi(d);

    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::UseDeclaration;
    n->token = Token{TokenType::Identifier, full_path, 0, 0};
    return n;
}

// ── parseBehave ─────────────────────────────────────────────────────────────

static ASTNode* parseBehave(ASTData& d) {
    d.error_function = "parseBehave";
    d.advance();
    Token name_tok = d.getToken();
    d.advance();

    ASTNode* n = createDefaultAstNode();
    n->node_type = ASTNodeType::BehaveDeclaration;
    n->token = name_tok;
    n->children = createChildList();

    // optional ~> ParentTrait (behaviour inheritance)
    if (d.hasMore()) {
        Token maybe_tilde = d.getToken();
        if (maybe_tilde.type == TokenType::TildeArrow) {
            d.advance();
            while (d.hasMore()) {
                Token trait_name = d.getToken();
                if (trait_name.type == TokenType::Identifier) {
                    ASTNode* inherit = createDefaultAstNode();
                    inherit->node_type = ASTNodeType::Identifier;
                    inherit->token = trait_name;
                    n->children->push_back(inherit);
                    d.advance();
                } else {
                    break;
                }
                Token comma = d.getToken();
                if (comma.type == TokenType::Comma) {
                    d.advance();
                } else {
                    break;
                }
            }
        }
    }

    Token lb = d.getToken();
    if (lb.type == TokenType::LeftBrace) d.advance();

    size_t guard = 0;
    while (d.hasMore()) {
        guard++;
        if (guard >= MAX_LOOP) throw AstError("Infinite while loop");
        Token cur = d.getToken();
        if (cur.type == TokenType::RightBrace) {
            d.advance();
            break;
        }

        if (cur.type == TokenType::Func) {
            ASTNode* m = parseFuncDecl(d, false);
            n->children->push_back(m);
        } else if (cur.type == TokenType::Needs) {
            d.advance();
            Token field_name = d.getToken();
            d.advance();
            Token col = d.getToken();
            if (col.type == TokenType::Colon) d.advance();
            ASTNode* type_node = parseTypeNodeWrapper(d);

            ASTNode* req_n = createDefaultAstNode();
            req_n->node_type = ASTNodeType::StructField;
            req_n->token = field_name;
            req_n->left = type_node;
            n->children->push_back(req_n);
        } else if (cur.type == TokenType::Identifier) {
            std::optional<Token> nx = d.peekToken(1);
            if (nx && nx->type == TokenType::LeftParen) {
                d.token_index--;
                ASTNode* m = parseFuncDecl(d, false);
                n->children->push_back(m);
            } else {
                d.advance();
            }
        } else {
            d.advance();
        }
    }
    return n;
}

// ── parseExt ────────────────────────────────────────────────────────────────

static ASTNode* parseExt(ASTData& d) {
    d.error_function = "parseExt";
    d.advance();
    Token ext_tok = d.getToken();
    if (ext_tok.type == TokenType::Func) {
        ASTNode* m = parseFuncDecl(d, false);
        m->node_type = ASTNodeType::ExtDeclaration;
        return m;
    }
    if (ext_tok.type == TokenType::Struct) {
        ASTNode* m = parseStruct(d, false);
        m->node_type = ASTNodeType::ExtDeclaration;
        return m;
    }
    if (ext_tok.type == TokenType::Enum) {
        ASTNode* m = parseEnum(d, false);
        m->node_type = ASTNodeType::ExtDeclaration;
        return m;
    }
    if (ext_tok.type == TokenType::Union) {
        ASTNode* m = parseUnion(d, false);
        m->node_type = ASTNodeType::ExtDeclaration;
        return m;
    }
    d.setError("Expected func/struct/enum/union after ext", ext_tok);
    throw AstError("Unexpected type");
}

} // namespace razen
