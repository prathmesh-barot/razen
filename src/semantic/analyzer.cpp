#include "analyzer.h"
#include "../lexer/lexer.h"
#include <iostream>
#include <sstream>

namespace razen {

// ── position helper ─────────────────────────────────────────────────────────

std::string Analyzer::errPos(const Token& tok) const {
    return "line " + std::to_string(tok.line + 1) + ":" + std::to_string(tok.character);
}

// ── error reporting ─────────────────────────────────────────────────────────

void Analyzer::reportError(const Token& token, const std::string& category, const std::string& msg) {
    has_errors = true;
    std::cout << RED << "  [" << category << "]" << RESET
              << " at " << CYAN << errPos(token) << RESET
              << "  " << msg << "\n";
}

// ── type helpers ────────────────────────────────────────────────────────────

TypeInfo* Analyzer::allocType(TypeCategory cat) {
    auto* ti = new TypeInfo();
    ti->category = cat;
    return ti;
}

TypeInfo* Analyzer::namedType(const std::string& name, TypeCategory cat) {
    auto* ti = new TypeInfo();
    ti->category = cat;
    ti->name = name;
    return ti;
}

// ── two-pass entry point ────────────────────────────────────────────────────

void Analyzer::analyze(const std::vector<ASTNode*>& ast_nodes) {
    for (auto* node : ast_nodes) declareGlobal(node);
    for (auto* node : ast_nodes) anaNode(node);
}

// ── type resolution ─────────────────────────────────────────────────────────

TypeInfo* Analyzer::resolveTypeFromNode(ASTNode* type_node) {
    if (!type_node || type_node->node_type != ASTNodeType::VarType) return nullptr;
    auto tok = type_node->token;
    if (!tok) return nullptr;
    auto tt = tok->type;
    auto& name = tok->value;

    // numeric primitives (i8, u32, f64, etc.)
    if (isNumericToken(tt)) {
        auto* ti = new TypeInfo(primitiveFromToken(tt, name));
        return ti;
    }

    switch (tt) {
        case TokenType::Bool:   return allocType(TypeCategory::Bool);
        case TokenType::Char:   return allocType(TypeCategory::Char);
        case TokenType::Void:   return allocType(TypeCategory::Void);
        case TokenType::Noret:  return allocType(TypeCategory::Noret);
        case TokenType::Str:    return allocType(TypeCategory::Str);
        case TokenType::String: return allocType(TypeCategory::String);
        case TokenType::Any:    return allocType(TypeCategory::Any);

        // named types — look up in global scope; also handle Error!T pattern
        case TokenType::Identifier: {
            // check for named error union: FileError!str
            if (type_node->left && type_node->left->node_type == ASTNodeType::VarType &&
                type_node->left->token && type_node->left->token->type == TokenType::ExclamationMark)
            {
                auto* ti = allocType(TypeCategory::ErrorUnion);
                auto* excl = type_node->left;
                if (excl->left) ti->ok_type = resolveTypeFromNode(excl->left);
                ti->error_type = namedType(tok->value, TypeCategory::ErrorSet);
                return ti;
            }
            if (auto* sym = sym_table.resolveGlobal(name)) {
                if (sym->resolved_type) return sym->resolved_type;
                if (sym->symbol_type == SymbolType::Struct)   return namedType(name, TypeCategory::Struct);
                if (sym->symbol_type == SymbolType::Enum)     return namedType(name, TypeCategory::Enum);
                if (sym->symbol_type == SymbolType::Union)    return namedType(name, TypeCategory::Union);
                if (sym->symbol_type == SymbolType::ErrorSet) return namedType(name, TypeCategory::ErrorSet);
            }
            return namedType(name, TypeCategory::Named);
        }

        // pointer: *T
        case TokenType::Star: {
            auto* ti = allocType(TypeCategory::Pointer);
            if (type_node->left) ti->pointee_type = resolveTypeFromNode(type_node->left);
            return ti;
        }

        // optional: ?T
        case TokenType::QuestionMark: {
            auto* ti = allocType(TypeCategory::Optional);
            if (type_node->left) ti->elem_type = resolveTypeFromNode(type_node->left);
            return ti;
        }

        // failable: !T
        case TokenType::ExclamationMark: {
            auto* ti = allocType(TypeCategory::Failable);
            if (type_node->left) ti->elem_type = resolveTypeFromNode(type_node->left);
            return ti;
        }

        // error union: Error!T — tok.value is the error set name
        case TokenType::Error: {
            auto* ti = allocType(TypeCategory::ErrorUnion);
            // the !T part is the left child
            if (type_node->left && type_node->left->node_type == ASTNodeType::VarType &&
                type_node->left->token && type_node->left->token->type == TokenType::ExclamationMark)
            {
                auto* excl = type_node->left;
                if (excl->left) ti->ok_type = resolveTypeFromNode(excl->left);
            } else if (type_node->left) {
                ti->ok_type = resolveTypeFromNode(type_node->left);
            }
            ti->error_type = namedType(tok->value, TypeCategory::ErrorSet);
            return ti;
        }

        default: {
            if (type_node->left) return resolveTypeFromNode(type_node->left);
            return nullptr;
        }
    }
}

// ── literal type inference ──────────────────────────────────────────────────

TypeInfo* Analyzer::inferTypeFromLiteral(ASTNode* node) {
    switch (node->node_type) {
        case ASTNodeType::IntegerLiteral: {
            auto* ti = allocType(TypeCategory::Integer);
            ti->name = "i32";
            return ti;
        }
        case ASTNodeType::FloatLiteral: {
            auto* ti = allocType(TypeCategory::Float);
            ti->name = "f64";
            return ti;
        }
        case ASTNodeType::BoolLiteral:   return allocType(TypeCategory::Bool);
        case ASTNodeType::CharLiteral:   return allocType(TypeCategory::Char);
        case ASTNodeType::StringLiteral: return allocType(TypeCategory::Str);
        default: return nullptr;
    }
}

// ── global declaration pass ─────────────────────────────────────────────────

void Analyzer::declareGlobal(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return;
    auto& name = tok->value;

    auto defineSym = [&](Symbol* sym) -> bool {
        if (!sym_table.defineGlobal(sym)) {
            reportError(*tok, "DeclError",
                "'" + name + "' is already declared in this scope.");
            return false;
        }
        return true;
    };

    switch (node->node_type) {
        case ASTNodeType::FunctionDeclaration:
        case ASTNodeType::ExtDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::Function;
            sym->token = *tok;
            sym->is_pub = node->is_pub;
            sym->is_async = node->is_async;
            sym->is_const = node->is_const;
            if (node->middle && node->middle->children)
                sym->param_count = node->middle->children->size();
            if (node->left && node->left->node_type == ASTNodeType::ReturnType && node->left->left)
                sym->return_type = resolveTypeFromNode(node->left->left);
            defineSym(sym);
            break;
        }
        case ASTNodeType::StructDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::Struct;
            sym->token = *tok;
            sym->is_pub = node->is_pub;
            sym->fields = new std::unordered_map<std::string, Symbol*>();
            if (node->children) {
                for (auto* child : *node->children) {
                    if (child->node_type == ASTNodeType::StructField) {
                        auto ftok = child->token;
                        if (!ftok) continue;
                        auto* fsym = new Symbol();
                        fsym->name = ftok->value;
                        fsym->symbol_type = SymbolType::Variable;
                        fsym->token = *ftok;
                        if (child->left) fsym->resolved_type = resolveTypeFromNode(child->left);
                        (*sym->fields)[ftok->value] = fsym;
                    }
                }
            }
            defineSym(sym);
            break;
        }
        case ASTNodeType::EnumDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::Enum;
            sym->token = *tok;
            sym->is_pub = node->is_pub;
            sym->fields = new std::unordered_map<std::string, Symbol*>();
            if (node->children) {
                for (auto* child : *node->children) {
                    if (child->node_type == ASTNodeType::EnumField) {
                        auto ftok = child->token;
                        if (!ftok) continue;
                        auto* vsym = new Symbol();
                        vsym->name = ftok->value;
                        vsym->symbol_type = SymbolType::Variable;
                        vsym->token = *ftok;
                        (*sym->fields)[ftok->value] = vsym;
                    }
                }
            }
            defineSym(sym);
            break;
        }
        case ASTNodeType::UnionDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::Union;
            sym->token = *tok;
            sym->is_pub = node->is_pub;
            defineSym(sym);
            break;
        }
        case ASTNodeType::ErrorDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::ErrorSet;
            sym->token = *tok;
            defineSym(sym);
            break;
        }
        case ASTNodeType::BehaveDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::Trait;
            sym->token = *tok;
            defineSym(sym);
            break;
        }
        case ASTNodeType::TypeAliasDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::TypeAlias;
            sym->token = *tok;
            if (node->left) sym->resolved_type = resolveTypeFromNode(node->left);
            defineSym(sym);
            break;
        }
        case ASTNodeType::ModuleDeclaration:
        case ASTNodeType::UseDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::Module;
            sym->token = *tok;
            sym_table.defineGlobal(sym);
            break;
        }
        case ASTNodeType::ConstDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::Variable;
            sym->token = *tok;
            sym->is_mut = node->is_mut;
            sym->is_const = true;
            if (node->left) sym->resolved_type = resolveTypeFromNode(node->left);
            defineSym(sym);
            break;
        }
        default: break;
    }
}

// ── main analysis dispatcher ────────────────────────────────────────────────

TypeInfo* Analyzer::anaNode(ASTNode* node) {
    if (!node) return nullptr;
    switch (node->node_type) {
        case ASTNodeType::Invalid:
        case ASTNodeType::Comment:
        case ASTNodeType::Annotation:
        case ASTNodeType::GenericParams:
            return nullptr;

        case ASTNodeType::FunctionDeclaration: return anaFuncDecl(node);
        case ASTNodeType::ExtDeclaration:       return nullptr;
        case ASTNodeType::Block:                return anaBlock(node);

        case ASTNodeType::IfBody:
        case ASTNodeType::ElseBody:
        case ASTNodeType::LoopBody:
        case ASTNodeType::MatchBody:
            return anaBody(node);

        case ASTNodeType::MatchCase:
            if (node->right) return anaNode(node->right);
            return nullptr;

        case ASTNodeType::VarDeclaration:
        case ASTNodeType::ConstDeclaration:
            return anaVarDecl(node);

        case ASTNodeType::Assignment:    return anaAssignment(node);
        case ASTNodeType::ReturnStatement: return anaReturn(node);

        case ASTNodeType::BreakStatement: {
            auto tok = node->token;
            if (tok && !sym_table.isInLoop())
                reportError(*tok, "FlowError", "'break' outside of a loop is not allowed.");
            return nullptr;
        }
        case ASTNodeType::SkipStatement: {
            auto tok = node->token;
            if (tok && !sym_table.isInLoop())
                reportError(*tok, "FlowError", "'skip' outside of a loop is not allowed.");
            return nullptr;
        }
        case ASTNodeType::TryBlock: {
            if (node->left) anaNode(node->left);
            if (node->right && node->right->node_type == ASTNodeType::CatchExpression)
                if (node->right->left) anaNode(node->right->left);
            return nullptr;
        }
        case ASTNodeType::IfStatement:
        case ASTNodeType::ElseIfStatement: return anaIf(node);
        case ASTNodeType::LoopStatement:   return anaLoop(node);
        case ASTNodeType::MatchStatement:  return anaMatch(node);
        case ASTNodeType::DeferStatement:
            if (node->left) anaNode(node->left);
            return nullptr;
        case ASTNodeType::TryExpression: {
            if (node->left) anaNode(node->left);
            if (node->right && node->right->node_type == ASTNodeType::CatchExpression)
                if (node->right->left) anaNode(node->right->left);
            return nullptr;
        }
        case ASTNodeType::BinaryExpression:  return anaBinary(node);
        case ASTNodeType::RangeExpression:
            if (node->left) anaNode(node->left);
            if (node->right) anaNode(node->right);
            return allocType(TypeCategory::Struct); // range is a struct-like value
        case ASTNodeType::UnaryExpression:    return anaUnary(node);
        case ASTNodeType::MemberAccess:       return anaMemberAccess(node);
        case ASTNodeType::Identifier:         return anaIdentifier(node);
        case ASTNodeType::FunctionCall:       return anaFunctionCall(node);
        case ASTNodeType::CatchExpression:
            if (node->left) return anaNode(node->left);
            return nullptr;

        case ASTNodeType::IntegerLiteral:
        case ASTNodeType::FloatLiteral:
        case ASTNodeType::BoolLiteral:
        case ASTNodeType::CharLiteral:
        case ASTNodeType::StringLiteral:
            return inferTypeFromLiteral(node);

        case ASTNodeType::BuiltinExpression: return nullptr;

        case ASTNodeType::ArrayLiteral: {
            if (node->children)
                for (auto* elem : *node->children) anaNode(elem);
            return nullptr;
        }
        default: return nullptr;
    }
}

// ── function declaration ────────────────────────────────────────────────────

TypeInfo* Analyzer::anaFuncDecl(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto& name = tok->value;

    TypeInfo* ret_type = nullptr;
    if (node->left && node->left->node_type == ASTNodeType::ReturnType && node->left->left)
        ret_type = resolveTypeFromNode(node->left->left);

    current_return_type = ret_type;
    current_func_name = name;

    auto* func_scope = new Scope();
    sym_table.pushScope(func_scope);

    // register parameters
    if (node->middle && node->middle->children) {
        for (auto* param : *node->middle->children) {
            if (param->node_type == ASTNodeType::Parameter) {
                auto ptok = param->token;
                if (!ptok) continue;
                auto* psym = new Symbol();
                psym->name = ptok->value;
                psym->symbol_type = SymbolType::Variable;
                psym->token = *ptok;
                psym->is_mut = param->is_mut;
                if (param->left) psym->resolved_type = resolveTypeFromNode(param->left);
                if (!sym_table.define(psym))
                    reportError(*ptok, "DeclError",
                        "Parameter '" + ptok->value + "' is already declared in function '" + name + "'.");
            }
        }
    }

    // analyze body
    bool has_explicit_return = false;
    if (node->right && node->right->node_type == ASTNodeType::Block) {
        auto* body_type = anaBlock(node->right);
        if (body_type) has_explicit_return = true;
    }

    if (ret_type && !has_explicit_return) {
        if (ret_type->category != TypeCategory::Void && ret_type->category != TypeCategory::Noret) {
            reportError(*tok, "ReturnError",
                "Function '" + name + "' expects return type '" + typeToString(ret_type) +
                "' but may not return a value on all paths.");
        }
    }

    sym_table.popScope();
    current_return_type = nullptr;
    current_func_name.clear();
    return ret_type;
}

// ── block / body ────────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaBlock(ASTNode* node) {
    auto* block_scope = new Scope();
    sym_table.pushScope(block_scope);
    TypeInfo* last_type = nullptr;
    if (node->children) {
        for (auto* stmt : *node->children) last_type = anaNode(stmt);
    } else if (node->left) {
        last_type = anaNode(node->left);
    }
    sym_table.popScope();
    return last_type;
}

TypeInfo* Analyzer::anaBody(ASTNode* node) {
    auto* body_scope = new Scope();
    sym_table.pushScope(body_scope);
    TypeInfo* last_type = nullptr;
    if (node->children) {
        for (auto* stmt : *node->children) last_type = anaNode(stmt);
    } else if (node->left) {
        if (node->left->node_type == ASTNodeType::Block)
            last_type = anaBlock(node->left);
        else
            last_type = anaNode(node->left);
    }
    sym_table.popScope();
    return last_type;
}

// ── variable declaration ────────────────────────────────────────────────────

TypeInfo* Analyzer::anaVarDecl(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto& name = tok->value;

    TypeInfo* decl_type = node->left ? resolveTypeFromNode(node->left) : nullptr;

    // check initializer type against declared type
    if (node->right) {
        auto* init_type = anaNode(node->right);
        if (decl_type && init_type) {
            std::string why;
            if (!typesCompatible(decl_type, init_type, &why)) {
                reportError(*tok, "TypeError",
                    "Type mismatch initializing variable '" + name +
                    "': expected '" + typeToString(decl_type) +
                    "', found '" + typeToString(init_type) + "'." +
                    (why.empty() ? "" : " " + why));
            }
        } else if (decl_type && !init_type) {
            reportError(*tok, "TypeError",
                "Cannot infer initializer type for variable '" + name + "'.");
        }
    }

    bool is_global = (sym_table.current_scope == sym_table.global_scope);
    if (!is_global && sym_table.current_scope->isDefinedInCurrentScope(name)) {
        reportError(*tok, "DeclError",
            "Variable '" + name + "' is already declared in this scope.");
    } else {
        auto* sym = new Symbol();
        sym->name = name;
        sym->symbol_type = SymbolType::Variable;
        sym->resolved_type = decl_type;
        sym->token = *tok;
        sym->is_mut = node->is_mut;
        sym->is_const = node->is_const;
        sym_table.define(sym);
    }
    return decl_type;
}

// ── assignment ──────────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaAssignment(ASTNode* node) {
    if (!node->left) return nullptr;
    auto* lhs = node->left;

    if (lhs->node_type == ASTNodeType::Identifier) {
        auto ltok = lhs->token;
        if (!ltok) return nullptr;
        if (auto* sym = sym_table.resolve(ltok->value)) {
            if (!sym->is_mut && !sym->is_const) {
                reportError(*ltok, "MutError",
                    "Cannot assign to immutable variable '" + ltok->value + "'. Use 'mut' to make it mutable.");
            }
            if (node->right) {
                auto* rhs_type = anaNode(node->right);
                if (sym->resolved_type && rhs_type) {
                    std::string why;
                    if (!typesCompatible(sym->resolved_type, rhs_type, &why)) {
                        reportError(*ltok, "TypeError",
                            "Type mismatch in assignment to '" + ltok->value +
                            "': expected '" + typeToString(sym->resolved_type) +
                            "', found '" + typeToString(rhs_type) + "'." +
                            (why.empty() ? "" : " " + why));
                    }
                }
            }
        } else if (undeclared_whitelist.count(ltok->value) == 0) {
            reportError(*ltok, "NameError",
                "Use of undeclared identifier '" + ltok->value + "'.");
        }
    } else if (lhs->node_type == ASTNodeType::MemberAccess) {
        anaMemberAccess(lhs);
    } else if (lhs->node_type == ASTNodeType::UnaryExpression) {
        auto tok = lhs->token;
        if (tok && tok->value != ".*")
            reportError(*tok, "SyntaxError", "Invalid left-hand side of assignment.");
    }
    if (node->right) return anaNode(node->right);
    return nullptr;
}

// ── return statement ────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaReturn(ASTNode* node) {
    TypeInfo* ret_type = nullptr;
    if (node->left) {
        ret_type = anaNode(node->left);
    } else {
        ret_type = allocType(TypeCategory::Void);
    }

    if (current_return_type && ret_type) {
        std::string why;
        if (!typesCompatible(current_return_type, ret_type, &why)) {
            auto tok = node->token;
            if (tok) {
                reportError(*tok, "ReturnError",
                    "Function '" + current_func_name + "' returns '" +
                    typeToString(ret_type) + "' but declared return type is '" +
                    typeToString(current_return_type) + "'." +
                    (why.empty() ? "" : " " + why));
            }
        }
    }
    return ret_type;
}

// ── if / else if ────────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaIf(ASTNode* node) {
    if (node->left) {
        auto* cond_type = anaNode(node->left);
        if (cond_type && !cond_type->isBool()) {
            auto tok = node->left->token;
            if (tok) {
                reportError(*tok, "TypeError",
                    "'if' condition must be a boolean, found '" + typeToString(cond_type) + "'.");
            }
        }
    }
    if (node->middle) anaBody(node->middle);
    if (node->right) {
        // right could be ElseBody or ElseIfStatement
        if (node->right->node_type == ASTNodeType::ElseIfStatement)
            anaIf(node->right);
        else
            anaBody(node->right);
    }
    return nullptr;
}

// ── loop ────────────────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaLoop(ASTNode* node) {
    if (node->left) {
        auto* cond_type = anaNode(node->left);
        if (cond_type && !cond_type->isBool()) {
            auto tok = node->left->token;
            if (tok) {
                reportError(*tok, "TypeError",
                    "'loop' condition must be a boolean, found '" + typeToString(cond_type) + "'.");
            }
        }
    }
    auto* loop_scope = new Scope();
    sym_table.pushLoopScope(loop_scope);
    if (node->middle && node->middle->node_type == ASTNodeType::Identifier) {
        auto itok = node->middle->token;
        if (itok) {
            auto* isym = new Symbol();
            isym->name = itok->value;
            isym->symbol_type = SymbolType::Variable;
            isym->token = *itok;
            sym_table.define(isym);
        }
    }
    if (node->right && node->right->node_type == ASTNodeType::LoopBody && node->right->children) {
        for (auto* stmt : *node->right->children) anaNode(stmt);
    }
    sym_table.popScope();
    return nullptr;
}

// ── match ───────────────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaMatch(ASTNode* node) {
    if (node->left) anaNode(node->left);
    if (node->children) {
        for (auto* case_node : *node->children) {
            if (case_node->left && case_node->left->node_type != ASTNodeType::Comment)
                anaNode(case_node->left);
            if (case_node->right && case_node->right->node_type == ASTNodeType::MatchBody) {
                auto* match_scope = new Scope();
                sym_table.pushScope(match_scope);
                if (case_node->right->left) anaNode(case_node->right->left);
                sym_table.popScope();
            }
        }
    }
    return nullptr;
}

// ── binary expression ───────────────────────────────────────────────────────

TypeInfo* Analyzer::anaBinary(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto op_type = tok->type;
    auto* left_type = node->left ? anaNode(node->left) : nullptr;
    auto* right_type = node->right ? anaNode(node->right) : nullptr;
    if (!left_type || !right_type) return nullptr;

    auto err = [&](const std::string& msg) -> TypeInfo* {
        reportError(*tok, "TypeError", msg);
        return nullptr;
    };

    switch (op_type) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
            if (!left_type->isNumeric() || !right_type->isNumeric())
                return err("Arithmetic operator requires numeric operands (got '" +
                           typeToString(left_type) + "' and '" + typeToString(right_type) + "').");
            return left_type;

        case TokenType::EqualsEquals:
        case TokenType::NotEquals: {
            std::string why;
            if (!typesCompatible(left_type, right_type, &why))
                return err("Cannot compare values of different types: '" +
                           typeToString(left_type) + "' vs '" + typeToString(right_type) + "'." +
                           (why.empty() ? "" : " " + why));
            return allocType(TypeCategory::Bool);
        }
        case TokenType::LessThan:
        case TokenType::LessThanEquals:
        case TokenType::GreaterThan:
        case TokenType::GreaterThanEquals:
            if (!left_type->isNumeric() || !right_type->isNumeric())
                return err("Comparison operator requires numeric operands (got '" +
                           typeToString(left_type) + "' and '" + typeToString(right_type) + "').");
            return allocType(TypeCategory::Bool);

        case TokenType::AndAnd:
        case TokenType::OrOr:
            if (!left_type->isBool() || !right_type->isBool())
                return err("Logical operator requires boolean operands (got '" +
                           typeToString(left_type) + "' and '" + typeToString(right_type) + "').");
            return left_type;

        case TokenType::And:
        case TokenType::Or:
        case TokenType::Caret:
        case TokenType::ShiftLeft:
        case TokenType::ShiftRight:
            if (!left_type->isInteger() || !right_type->isInteger())
                return err("Bitwise operator requires integer operands (got '" +
                           typeToString(left_type) + "' and '" + typeToString(right_type) + "').");
            return left_type;

        default:
            return left_type;
    }
}

// ── unary expression ────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaUnary(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto* inner_type = node->left ? anaNode(node->left) : nullptr;
    if (!inner_type) return nullptr;

    if (tok->value == "-") {
        if (!inner_type->isNumeric()) {
            reportError(*tok, "TypeError",
                "Unary minus requires numeric operand, found '" + typeToString(inner_type) + "'.");
            return nullptr;
        }
        return inner_type;
    }
    if (tok->value == "!") {
        if (!inner_type->canBeBool()) {
            reportError(*tok, "TypeError",
                "Logical not requires boolean operand, found '" + typeToString(inner_type) + "'.");
            return nullptr;
        }
        return inner_type;
    }
    if (tok->value == "~") {
        if (!inner_type->isInteger()) {
            reportError(*tok, "TypeError",
                "Bitwise not requires integer operand, found '" + typeToString(inner_type) + "'.");
            return nullptr;
        }
        return inner_type;
    }
    if (tok->value == "&") {
        auto* ptr_ti = allocType(TypeCategory::Pointer);
        ptr_ti->pointee_type = inner_type;
        ptr_ti->name = "*" + inner_type->name;
        return ptr_ti;
    }
    if (tok->value == ".*") {
        if (inner_type->category != TypeCategory::Pointer) {
            reportError(*tok, "TypeError",
                "Dereference requires pointer operand, found '" + typeToString(inner_type) + "'.");
            return nullptr;
        }
        return inner_type->pointee_type;
    }
    return inner_type;
}

// ── member access ───────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaMemberAccess(ASTNode* node) {
    auto* left_type = node->left ? anaNode(node->left) : nullptr;
    if (!left_type) return nullptr;

    if (node->right) {
        auto* right_node = node->right;
        if (right_node->node_type == ASTNodeType::FunctionCall)
            return anaNode(right_node);

        if (right_node->token) {
            auto& field_name = right_node->token->value;

            // struct field access
            if (left_type->category == TypeCategory::Struct || left_type->category == TypeCategory::Named) {
                auto& type_name = left_type->name;
                if (auto* struct_sym = sym_table.resolveGlobal(type_name)) {
                    if (struct_sym->fields) {
                        auto it = struct_sym->fields->find(field_name);
                        if (it != struct_sym->fields->end())
                            return it->second->resolved_type;
                        reportError(*right_node->token, "FieldError",
                            "Struct '" + type_name + "' has no field '" + field_name + "'.");
                        return nullptr;
                    }
                }
            }

            // enum variant access (e.g. Color.Red)
            if (left_type->category == TypeCategory::Enum) {
                return left_type;
            }
        }
    }
    return left_type;
}

// ── identifier ──────────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaIdentifier(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto& name = tok->value;

    if (undeclared_whitelist.count(name))
        return allocType(TypeCategory::Any);

    if (auto* sym = sym_table.resolve(name)) {
        if (sym->resolved_type) return sym->resolved_type;
        switch (sym->symbol_type) {
            case SymbolType::Function:
                if (sym->return_type) return sym->return_type;
                return allocType(TypeCategory::Function);
            case SymbolType::Struct:   return namedType(name, TypeCategory::Struct);
            case SymbolType::Enum:     return namedType(name, TypeCategory::Enum);
            case SymbolType::Union:    return namedType(name, TypeCategory::Union);
            case SymbolType::ErrorSet: return namedType(name, TypeCategory::ErrorSet);
            case SymbolType::TypeAlias:
                if (sym->resolved_type) return sym->resolved_type;
                return nullptr;
            default: return nullptr;
        }
    }

    reportError(*tok, "NameError",
        "Use of undeclared identifier '" + name + "'.");
    return nullptr;
}

// ── function call ───────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaFunctionCall(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto& name = tok->value;

    size_t arg_count = node->children ? node->children->size() : 0;

    if (auto* sym = sym_table.resolve(name)) {
        if (sym->symbol_type == SymbolType::Function) {
            // argument count check
            if (arg_count != sym->param_count) {
                reportError(*tok, "ArgError",
                    "Function '" + name + "' expects " + std::to_string(sym->param_count) +
                    " argument(s) but got " + std::to_string(arg_count) + ".");
            }

            // argument type checking against parameter types
            if (node->children && sym->param_count > 0 && sym->param_count == arg_count) {
                // need param type info from the function declaration
                // we stored the return type in the symbol but not the param types
                // for now, just analyze each arg expression
                for (auto* arg : *node->children) {
                    if (arg->left) anaNode(arg->left);
                }
            } else if (node->children) {
                for (auto* arg : *node->children) {
                    if (arg->left) anaNode(arg->left);
                }
            }

            if (sym->return_type) return sym->return_type;
        }
    } else if (undeclared_whitelist.count(name) == 0) {
        reportError(*tok, "NameError",
            "Call to undeclared function '" + name + "'.");
    }

    if (node->children) {
        for (auto* arg : *node->children) {
            if (arg->left) anaNode(arg->left);
        }
    }
    return nullptr;
}

// ── type compatibility ──────────────────────────────────────────────────────

bool Analyzer::typesCompatible(const TypeInfo* expected, const TypeInfo* actual, std::string* why) {
    if (!expected || !actual) return true;
    if (expected->category == TypeCategory::Any) return true;
    if (expected->category == TypeCategory::Unknown) return true;
    if (actual->category == TypeCategory::Unknown) return true;

    // same category
    if (expected->category == actual->category) {
        if (expected->category == TypeCategory::Integer) return true;
        if (expected->category == TypeCategory::Float) return true;

        // pointer compatibility
        if (expected->category == TypeCategory::Pointer) {
            if (expected->pointee_type && actual->pointee_type)
                return typesCompatible(expected->pointee_type, actual->pointee_type, why);
            return expected->pointee_type == actual->pointee_type;
        }

        // error union compatibility
        if (expected->category == TypeCategory::ErrorUnion) {
            if (expected->ok_type && actual->ok_type)
                return typesCompatible(expected->ok_type, actual->ok_type, why);
            return true;
        }

        // named types: compare by name
        if (expected->category == TypeCategory::Struct ||
            expected->category == TypeCategory::Enum ||
            expected->category == TypeCategory::Union ||
            expected->category == TypeCategory::ErrorSet ||
            expected->category == TypeCategory::Named)
        {
            if (expected->name == actual->name) return true;
            if (why) *why = "types have the same kind but different names";
            return false;
        }

        return true;
    }

    // error union compatible with its ok type (e.g. Error!i32 ~ i32 is not compatible)
    // this would need special handling — error union is NOT its inner type

    if (why) {
        *why = "expected '" + typeToString(expected) + "' but expression has type '" +
               typeToString(actual) + "'";
    }
    return false;
}

} // namespace razen