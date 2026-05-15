#include "analyzer.h"
#include "../lexer/lexer.h"
#include <iostream>
#include <cstring>

namespace razen {

void Analyzer::reportError(const Token& token, const std::string& msg) {
    has_errors = true;
    std::cout << RED << "Semantic Error" << RESET << " at line " << (token.line + 1) << ": " << msg << "\n";
}

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

void Analyzer::analyze(const std::vector<ASTNode*>& ast_nodes) {
    for (auto* node : ast_nodes) declareGlobal(node);
    for (auto* node : ast_nodes) anaNode(node);
}

TypeInfo* Analyzer::resolveTypeFromNode(ASTNode* type_node) {
    if (!type_node || type_node->node_type != ASTNodeType::VarType) return nullptr;
    auto tok = type_node->token;
    if (!tok) return nullptr;
    auto tt = tok->type;
    auto& name = tok->value;

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
        case TokenType::Identifier: {
            if (auto* sym = sym_table.resolveGlobal(name)) {
                if (sym->resolved_type) return sym->resolved_type;
                if (sym->symbol_type == SymbolType::Struct)   return namedType(name, TypeCategory::Struct);
                if (sym->symbol_type == SymbolType::Enum)     return namedType(name, TypeCategory::Enum);
                if (sym->symbol_type == SymbolType::Union)    return namedType(name, TypeCategory::Union);
                if (sym->symbol_type == SymbolType::ErrorSet) return namedType(name, TypeCategory::ErrorSet);
            }
            return namedType(name, TypeCategory::Named);
        }
        case TokenType::Star: {
            auto* ti = allocType(TypeCategory::Pointer);
            if (type_node->left) ti->pointee_type = resolveTypeFromNode(type_node->left);
            return ti;
        }
        case TokenType::QuestionMark: {
            auto* ti = allocType(TypeCategory::Optional);
            if (type_node->left) ti->elem_type = resolveTypeFromNode(type_node->left);
            return ti;
        }
        case TokenType::ExclamationMark: {
            auto* ti = allocType(TypeCategory::Failable);
            if (type_node->left) ti->elem_type = resolveTypeFromNode(type_node->left);
            return ti;
        }
        case TokenType::Error: {
            auto* ti = allocType(TypeCategory::ErrorUnion);
            if (type_node->left) ti->ok_type = resolveTypeFromNode(type_node->left);
            return ti;
        }
        default: {
            if (type_node->left) return resolveTypeFromNode(type_node->left);
            return nullptr;
        }
    }
}

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

void Analyzer::declareGlobal(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return;
    auto& name = tok->value;

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
            if (node->middle) {
                auto* params_node = node->middle;
                if (params_node->children) sym->param_count = params_node->children->size();
            }
            if (node->left) {
                auto* ret_type_node = node->left;
                if (ret_type_node->node_type == ASTNodeType::ReturnType && ret_type_node->left) {
                    sym->return_type = resolveTypeFromNode(ret_type_node->left);
                }
            }
            if (!sym_table.defineGlobal(sym))
                reportError(*tok, "'" + name + "' is already declared in this scope.");
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
            if (!sym_table.defineGlobal(sym))
                reportError(*tok, "Struct '" + name + "' is already declared.");
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
            if (!sym_table.defineGlobal(sym))
                reportError(*tok, "Enum '" + name + "' is already declared.");
            break;
        }
        case ASTNodeType::UnionDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::Union;
            sym->token = *tok;
            sym->is_pub = node->is_pub;
            if (!sym_table.defineGlobal(sym))
                reportError(*tok, "Union '" + name + "' is already declared.");
            break;
        }
        case ASTNodeType::ErrorDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::ErrorSet;
            sym->token = *tok;
            if (!sym_table.defineGlobal(sym))
                reportError(*tok, "Error set '" + name + "' is already declared.");
            break;
        }
        case ASTNodeType::BehaveDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::Trait;
            sym->token = *tok;
            if (!sym_table.defineGlobal(sym))
                reportError(*tok, "Behaviour '" + name + "' is already declared.");
            break;
        }
        case ASTNodeType::TypeAliasDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::TypeAlias;
            sym->token = *tok;
            if (node->left) sym->resolved_type = resolveTypeFromNode(node->left);
            if (!sym_table.defineGlobal(sym))
                reportError(*tok, "Type alias '" + name + "' is already declared.");
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
            if (!sym_table.defineGlobal(sym))
                reportError(*tok, "Constant '" + name + "' is already declared.");
            break;
        }
        default: break;
    }
}

TypeInfo* Analyzer::anaNode(ASTNode* node) {
    if (!node) return nullptr;
    switch (node->node_type) {
        case ASTNodeType::Invalid:
        case ASTNodeType::Comment:
            return nullptr;
        case ASTNodeType::FunctionDeclaration:
            return anaFuncDecl(node);
        case ASTNodeType::ExtDeclaration:
            return nullptr;
        case ASTNodeType::Block:
            return anaBlock(node);
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
        case ASTNodeType::Assignment:
            return anaAssignment(node);
        case ASTNodeType::ReturnStatement: {
            auto tok = node->token;
            if (!tok) return nullptr;
            if (tok->value == "break") {
                if (!sym_table.isInLoop())
                    reportError(*tok, "'break' outside of a loop is not allowed.");
                return nullptr;
            }
            if (tok->value == "skip") {
                if (!sym_table.isInLoop())
                    reportError(*tok, "'skip' outside of a loop is not allowed.");
                return nullptr;
            }
            return anaReturn(node);
        }
        case ASTNodeType::IfStatement:
            return anaIf(node);
        case ASTNodeType::LoopStatement:
            return anaLoop(node);
        case ASTNodeType::MatchStatement:
            return anaMatch(node);
        case ASTNodeType::DeferStatement:
            if (node->left) anaNode(node->left);
            return nullptr;
        case ASTNodeType::TryExpression: {
            if (node->left) anaNode(node->left);
            if (node->right) {
                auto* catch_node = node->right;
                if (catch_node->node_type == ASTNodeType::CatchExpression) {
                    if (catch_node->left) anaNode(catch_node->left);
                }
            }
            return nullptr;
        }
        case ASTNodeType::BinaryExpression:
            return anaBinary(node);
        case ASTNodeType::UnaryExpression:
            return anaUnary(node);
        case ASTNodeType::MemberAccess:
            return anaMemberAccess(node);
        case ASTNodeType::Identifier:
            return anaIdentifier(node);
        case ASTNodeType::FunctionCall:
            return anaFunctionCall(node);
        case ASTNodeType::CatchExpression:
            if (node->left) return anaNode(node->left);
            return nullptr;
        case ASTNodeType::IntegerLiteral:
        case ASTNodeType::FloatLiteral:
        case ASTNodeType::BoolLiteral:
        case ASTNodeType::CharLiteral:
        case ASTNodeType::StringLiteral:
            return inferTypeFromLiteral(node);
        case ASTNodeType::BuiltinExpression:
            return nullptr;
        case ASTNodeType::ArrayLiteral: {
            if (node->children) {
                for (auto* elem : *node->children) anaNode(elem);
            }
            return nullptr;
        }
        default:
            return nullptr;
    }
}

TypeInfo* Analyzer::anaFuncDecl(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto& name = tok->value;

    TypeInfo* ret_type = nullptr;
    if (node->left) {
        auto* ret_type_node = node->left;
        if (ret_type_node->node_type == ASTNodeType::ReturnType && ret_type_node->left) {
            ret_type = resolveTypeFromNode(ret_type_node->left);
        }
    }

    current_return_type = ret_type;
    current_func_name = name;

    auto* func_scope = new Scope();
    sym_table.pushScope(func_scope);

    if (node->middle) {
        auto* params_node = node->middle;
        if (params_node->children) {
            for (auto* param : *params_node->children) {
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
                        reportError(*ptok, "Parameter '" + ptok->value + "' is already declared.");
                }
            }
        }
    }

    bool has_explicit_return = false;
    if (node->right) {
        auto* body = node->right;
        if (body->node_type == ASTNodeType::Block) {
            auto* body_type = anaBlock(body);
            if (body_type) has_explicit_return = true;
        }
    }

    if (ret_type && !has_explicit_return) {
        if (ret_type->category != TypeCategory::Void && ret_type->category != TypeCategory::Noret) {
            reportError(*tok, "Function '" + name + "' expects return type but may not return a value.");
        }
    }

    sym_table.popScope();
    current_return_type = nullptr;
    current_func_name.clear();
    return ret_type;
}

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
        if (node->left->node_type == ASTNodeType::Block) {
            last_type = anaBlock(node->left);
        } else {
            last_type = anaNode(node->left);
        }
    }
    sym_table.popScope();
    return last_type;
}

TypeInfo* Analyzer::anaVarDecl(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto& name = tok->value;

    TypeInfo* decl_type = nullptr;
    if (node->left) decl_type = resolveTypeFromNode(node->left);

    if (node->right) {
        auto* init_type = anaNode(node->right);
        if (decl_type && init_type) {
            if (!typesCompatible(decl_type, init_type)) {
                reportError(*tok, "Type mismatch initializing variable '" + name + "'.");
            }
        }
    }

    bool is_global = (sym_table.current_scope == sym_table.global_scope);
    if (sym_table.current_scope->isDefinedInCurrentScope(name)) {
        if (!is_global) {
            reportError(*tok, "Variable '" + name + "' is already declared in this scope.");
        }
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

TypeInfo* Analyzer::anaAssignment(ASTNode* node) {
    if (node->left) {
        auto* lhs = node->left;
        if (lhs->node_type == ASTNodeType::Identifier) {
            auto ltok = lhs->token;
            if (!ltok) return nullptr;
            if (auto* sym = sym_table.resolve(ltok->value)) {
                if (!sym->is_mut && !sym->is_const) {
                    reportError(*ltok, "Cannot assign to immutable variable '" + ltok->value + "'. Use 'mut' to make it mutable.");
                }
                if (node->right) {
                    auto* rhs_type = anaNode(node->right);
                    if (sym->resolved_type && rhs_type) {
                        if (!typesCompatible(sym->resolved_type, rhs_type)) {
                            reportError(*ltok, "Type mismatch in assignment to '" + ltok->value + "'.");
                        }
                    }
                }
            } else if (undeclared_whitelist.count(ltok->value) == 0) {
                reportError(*ltok, "Use of undeclared identifier '" + ltok->value + "'.");
            }
        } else if (lhs->node_type == ASTNodeType::MemberAccess) {
            anaMemberAccess(lhs);
        } else if (lhs->node_type == ASTNodeType::UnaryExpression) {
            auto tok = lhs->token;
            if (tok && tok->value != ".*") {
                reportError(*tok, "Invalid left-hand side of assignment.");
            }
        }
        if (node->right) return anaNode(node->right);
    }
    return nullptr;
}

TypeInfo* Analyzer::anaReturn(ASTNode* node) {
    TypeInfo* ret_type = nullptr;
    if (node->left) {
        ret_type = anaNode(node->left);
    } else {
        ret_type = allocType(TypeCategory::Void);
    }

    if (current_return_type) {
        if (ret_type) {
            if (!typesCompatible(current_return_type, ret_type)) {
                auto tok = node->token;
                if (tok) reportError(*tok, "Function returns a different type than declared.");
            }
        }
    }
    return ret_type;
}

TypeInfo* Analyzer::anaIf(ASTNode* node) {
    if (node->left) {
        auto* cond_type = anaNode(node->left);
        if (cond_type) {
            if (!cond_type->isBool()) {
                auto tok = node->left->token;
                if (tok) reportError(*tok, "'if' condition must be a boolean.");
            }
        }
    }
    if (node->middle) anaBody(node->middle);
    if (node->right) anaBody(node->right);
    return nullptr;
}

TypeInfo* Analyzer::anaLoop(ASTNode* node) {
    if (node->left) {
        auto* cond_type = anaNode(node->left);
        if (cond_type) {
            if (!cond_type->isBool()) {
                auto tok = node->left->token;
                if (tok) reportError(*tok, "'loop' condition must be a boolean.");
            }
        }
    }
    auto* loop_scope = new Scope();
    sym_table.pushLoopScope(loop_scope);
    if (node->middle) {
        if (node->middle->node_type == ASTNodeType::Identifier) {
            auto itok = node->middle->token;
            if (itok) {
                auto* isym = new Symbol();
                isym->name = itok->value;
                isym->symbol_type = SymbolType::Variable;
                isym->token = *itok;
                sym_table.define(isym);
            }
        }
    }
    if (node->right) {
        if (node->right->node_type == ASTNodeType::LoopBody) {
            if (node->right->children) {
                for (auto* stmt : *node->right->children) anaNode(stmt);
            }
        }
    }
    sym_table.popScope();
    return nullptr;
}

TypeInfo* Analyzer::anaMatch(ASTNode* node) {
    if (node->left) anaNode(node->left);
    if (node->children) {
        for (auto* case_node : *node->children) {
            if (case_node->left) anaNode(case_node->left);
            if (case_node->right) {
                if (case_node->right->node_type == ASTNodeType::MatchBody) {
                    auto* match_scope = new Scope();
                    sym_table.pushScope(match_scope);
                    if (case_node->right->left) anaNode(case_node->right->left);
                    sym_table.popScope();
                }
            }
        }
    }
    return nullptr;
}

TypeInfo* Analyzer::anaBinary(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto op_type = tok->type;
    auto* left_type = node->left ? anaNode(node->left) : nullptr;
    auto* right_type = node->right ? anaNode(node->right) : nullptr;
    if (!left_type || !right_type) return nullptr;

    switch (op_type) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
            if (!left_type->isNumeric() || !right_type->isNumeric()) {
                reportError(*tok, "Arithmetic operator requires numeric operands.");
                return nullptr;
            }
            return left_type;
        case TokenType::EqualsEquals:
        case TokenType::NotEquals:
            if (!typesCompatible(left_type, right_type)) {
                reportError(*tok, "Cannot compare values of different types.");
                return nullptr;
            }
            return allocType(TypeCategory::Bool);
        case TokenType::LessThan:
        case TokenType::LessThanEquals:
        case TokenType::GreaterThan:
        case TokenType::GreaterThanEquals:
            if (!left_type->isNumeric() || !right_type->isNumeric()) {
                reportError(*tok, "Comparison operator requires numeric operands.");
                return nullptr;
            }
            return allocType(TypeCategory::Bool);
        case TokenType::AndAnd:
        case TokenType::OrOr:
            if (!left_type->isBool() || !right_type->isBool()) {
                reportError(*tok, "Logical operator requires boolean operands.");
                return nullptr;
            }
            return left_type;
        case TokenType::And:
        case TokenType::Or:
        case TokenType::Caret:
        case TokenType::ShiftLeft:
        case TokenType::ShiftRight:
            if (!left_type->isInteger() || !right_type->isInteger()) {
                reportError(*tok, "Bitwise operator requires integer operands.");
                return nullptr;
            }
            return left_type;
        default:
            return left_type;
    }
}

TypeInfo* Analyzer::anaUnary(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto* inner_type = node->left ? anaNode(node->left) : nullptr;
    if (!inner_type) return nullptr;

    if (tok->value == "-") {
        if (!inner_type->isNumeric()) {
            reportError(*tok, "Unary minus requires numeric operand.");
            return nullptr;
        }
        return inner_type;
    }
    if (tok->value == "!") {
        if (!inner_type->canBeBool()) {
            reportError(*tok, "Logical not requires boolean operand.");
            return nullptr;
        }
        return inner_type;
    }
    if (tok->value == "&") {
        auto* ptr_ti = allocType(TypeCategory::Pointer);
        ptr_ti->pointee_type = inner_type;
        return ptr_ti;
    }
    if (tok->value == ".*") {
        if (inner_type->category != TypeCategory::Pointer) {
            reportError(*tok, "Dereference requires pointer operand.");
            return nullptr;
        }
        return inner_type->pointee_type;
    }
    return inner_type;
}

TypeInfo* Analyzer::anaMemberAccess(ASTNode* node) {
    auto* left_type = node->left ? anaNode(node->left) : nullptr;
    if (!left_type) return nullptr;
    if (node->right) {
        auto* right_node = node->right;
        if (right_node->node_type == ASTNodeType::FunctionCall) {
            return anaNode(right_node);
        }
        if (right_node->token) {
            auto& field_name = right_node->token->value;
            if (left_type->category == TypeCategory::Struct || left_type->category == TypeCategory::Named) {
                auto& type_name = left_type->name;
                if (auto* struct_sym = sym_table.resolveGlobal(type_name)) {
                    if (struct_sym->fields) {
                        auto it = struct_sym->fields->find(field_name);
                        if (it != struct_sym->fields->end())
                            return it->second->resolved_type;
                        reportError(*right_node->token, "Struct '" + type_name + "' has no field '" + field_name + "'.");
                        return nullptr;
                    }
                }
            }
            if (left_type->category == TypeCategory::Enum) {
                return left_type;
            }
        }
    }
    return left_type;
}

TypeInfo* Analyzer::anaIdentifier(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto& name = tok->value;

    if (undeclared_whitelist.count(name)) {
        return allocType(TypeCategory::Any);
    }

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

    reportError(*tok, "Use of undeclared identifier '" + name + "'.");
    return nullptr;
}

TypeInfo* Analyzer::anaFunctionCall(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto& name = tok->value;

    size_t arg_count = node->children ? node->children->size() : 0;

    if (auto* sym = sym_table.resolve(name)) {
        if (sym->symbol_type == SymbolType::Function) {
            if (arg_count != sym->param_count) {
                reportError(*tok, "Function '" + name + "' expects " + std::to_string(sym->param_count) + " argument(s) but got " + std::to_string(arg_count) + ".");
            }
            if (node->children) {
                for (auto* arg : *node->children) {
                    if (arg->left) anaNode(arg->left);
                }
            }
            if (sym->return_type) return sym->return_type;
        }
    } else if (undeclared_whitelist.count(name) == 0) {
        reportError(*tok, "Call to undeclared function '" + name + "'.");
    }

    if (node->children) {
        for (auto* arg : *node->children) {
            if (arg->left) anaNode(arg->left);
        }
    }
    return nullptr;
}

bool Analyzer::typesCompatible(const TypeInfo* expected, const TypeInfo* actual) {
    if (expected->category == TypeCategory::Any) return true;
    if (expected->category == actual->category) {
        if (expected->category == TypeCategory::Integer) return true;
        if (expected->category == TypeCategory::Float) return true;
        if (expected->category == TypeCategory::Pointer) {
            if (expected->pointee_type) {
                if (actual->pointee_type) return typesCompatible(expected->pointee_type, actual->pointee_type);
                return false;
            }
            return actual->pointee_type == nullptr;
        }
        return true;
    }
    return false;
}

} // namespace razen
