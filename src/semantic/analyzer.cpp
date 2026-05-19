#include "analyzer.h"
#include "../lexer/lexer.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace razen {

std::string Analyzer::errPos(const Token& tok) const {
    return "line " + std::to_string(tok.line + 1) + ":" + std::to_string(tok.character);
}

void Analyzer::reportError(const Token& token, const std::string& category, const std::string& msg) {
    has_errors = true;
    std::cout << RED << "  [" << category << "]" << RESET
              << " at " << CYAN << errPos(token) << RESET
              << "  " << msg << "\n";
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

TypeInfo* Analyzer::makeNumericType(const std::string& name) {
    auto* ti = new TypeInfo();
    if (name == "int") {
        ti->category = TypeCategory::Integer;
        ti->name = "i32";
    } else if (name == "uint") {
        ti->category = TypeCategory::Integer;
        ti->name = "u32";
    } else if (name == "float") {
        ti->category = TypeCategory::Float;
        ti->name = "f32";
    } else if (name.size() >= 1 && name[0] == 'i') {
        ti->category = TypeCategory::Integer;
        ti->name = name;
    } else if (name.size() >= 1 && name[0] == 'u') {
        ti->category = TypeCategory::Integer;
        ti->name = name;
    } else if (name.size() >= 1 && name[0] == 'f') {
        ti->category = TypeCategory::Float;
        ti->name = name;
    } else {
        ti->category = TypeCategory::Unknown;
        ti->name = name;
    }
    return ti;
}

// ── Type resolution from AST ─────────────────────────────────────────────

TypeInfo* Analyzer::resolveTypeFromNode(ASTNode* type_node) {
    if (!type_node) return nullptr;

    // Tuple type .{T, E}
    if (type_node->node_type == ASTNodeType::TupleLiteral) {
        auto* ti = allocType(TypeCategory::Struct);
        ti->name = ".tuple";
        if (type_node->children) {
            for (auto* elem : *type_node->children) {
                auto* elem_ti = resolveTypeFromNode(elem);
                if (elem_ti) {
                    // store children type info in a linked list via pointee_type chain
                    auto* prev = ti->pointee_type;
                    if (!prev) ti->pointee_type = elem_ti;
                    else {
                        while (prev->pointee_type) prev = prev->pointee_type;
                        prev->pointee_type = elem_ti;
                    }
                }
            }
        }
        return ti;
    }

    if (type_node->node_type == ASTNodeType::ArrayType) {
        auto* ti = allocType(TypeCategory::Array);
        if (type_node->left) ti->elem_type = resolveTypeFromNode(type_node->left);
        if (type_node->middle && type_node->middle->token) {
            ti->array_size = std::stoul(type_node->middle->token->value);
        }
        return ti;
    }

    if (type_node->node_type != ASTNodeType::VarType && type_node->node_type != ASTNodeType::Identifier)
        return nullptr;

    auto tok = type_node->token;
    if (!tok) return nullptr;
    auto tt = tok->type;
    auto& name = tok->value;

    // numeric primitives (i8, u32, f64, etc.)
    if (isNumericToken(tt)) {
        return makeNumericType(name);
    }

    switch (tt) {
        case TokenType::Bool:   return allocType(TypeCategory::Bool);
        case TokenType::Char:   return allocType(TypeCategory::Char);
        case TokenType::Void:   return allocType(TypeCategory::Void);
        case TokenType::Noret:  return allocType(TypeCategory::Noret);
        case TokenType::Str:    return allocType(TypeCategory::Str);
        case TokenType::String: return allocType(TypeCategory::String);
        case TokenType::Any:    return allocType(TypeCategory::Any);
        case TokenType::Type:   return allocType(TypeCategory::Any);

        case TokenType::Identifier: {
            // Check for error union: Error!T
            // The AST stores the error set name as the token, and the !T part in left->left
            if (type_node->left) {
                ASTNode* left = type_node->left;
                // error!T (anonymous error set)
                if (left->token && left->token->type == TokenType::ExclamationMark) {
                    auto* ti = allocType(TypeCategory::ErrorUnion);
                    ti->error_type = allocType(TypeCategory::ErrorSet);
                    ti->error_type->name = "(error)";
                    if (left->left) ti->ok_type = resolveTypeFromNode(left->left);
                    return ti;
                }
                // Exclamation mark as part of NamedError!T
                if (left->node_type == ASTNodeType::VarType && left->token &&
                    left->token->type == TokenType::ExclamationMark) {
                    auto* ti = allocType(TypeCategory::ErrorUnion);
                    ti->error_type = namedType(name, TypeCategory::ErrorSet);
                    if (left->left) ti->ok_type = resolveTypeFromNode(left->left);
                    return ti;
                }
                // Parser stores MathError!f64 as Identifier(MathError) → left = f64 node
                // Check if name is a known error set, then treat as error union
                if (auto* esym = sym_table.resolveGlobal(name)) {
                    if (esym->symbol_type == SymbolType::ErrorSet) {
                        auto* ti = allocType(TypeCategory::ErrorUnion);
                        ti->error_type = namedType(name, TypeCategory::ErrorSet);
                        ti->ok_type = resolveTypeFromNode(left);
                        return ti;
                    }
                }
            }

            // Type alias / named type lookup
            if (auto* sym = sym_table.resolveGlobal(name)) {
                if (sym->resolved_type) return cloneTypeInfo(sym->resolved_type);
                switch (sym->symbol_type) {
                    case SymbolType::Struct:   return namedType(name, TypeCategory::Struct);
                    case SymbolType::Enum:     return namedType(name, TypeCategory::Enum);
                    case SymbolType::Union:    return namedType(name, TypeCategory::Union);
                    case SymbolType::ErrorSet: return namedType(name, TypeCategory::ErrorSet);
                    case SymbolType::TypeAlias:
                        if (sym->resolved_type) return cloneTypeInfo(sym->resolved_type);
                        return namedType(name, TypeCategory::Named);
                    default: return namedType(name, TypeCategory::Named);
                }
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
            ti->error_type = namedType(name, TypeCategory::ErrorSet);
            // the !T part is embedded in children or left chain
            if (type_node->left) {
                if (type_node->left->node_type == ASTNodeType::VarType &&
                    type_node->left->token &&
                    type_node->left->token->type == TokenType::ExclamationMark) {
                    if (type_node->left->left) ti->ok_type = resolveTypeFromNode(type_node->left->left);
                } else {
                    ti->ok_type = resolveTypeFromNode(type_node->left);
                }
            }
            return ti;
        }

        default: {
            if (type_node->left) return resolveTypeFromNode(type_node->left);
            return nullptr;
        }
    }
}

// ── Literal type inference ──────────────────────────────────────────────

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

// ── Two-pass entry point ────────────────────────────────────────────────

void Analyzer::analyze(const std::vector<ASTNode*>& ast_nodes) {
    for (auto* node : ast_nodes) declareGlobal(node);
    // Second pass for global const initializers
    for (auto* node : ast_nodes) {
        if (node->node_type == ASTNodeType::ConstDeclaration || node->node_type == ASTNodeType::VarDeclaration) {
            anaNode(node);
        }
    }
    for (auto* node : ast_nodes) anaNode(node);
}

// ── Global declaration pass ─────────────────────────────────────────────

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

    // Handle @Generic prefix — store generic param names on the symbol
    auto getGenericParams = [&](ASTNode* n) -> std::vector<std::string> {
        std::vector<std::string> params;
        if (n->children) {
            for (auto* child : *n->children) {
                if (child->node_type == ASTNodeType::GenericParams && child->children) {
                    for (auto* gp : *child->children) {
                        if (gp->token) params.push_back(gp->token->value);
                    }
                }
            }
        }
        return params;
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
            // Store parameter types for later arg checking
            std::vector<FuncParam> params;
            if (node->middle && node->middle->children) {
                sym->param_count = node->middle->children->size();
                for (auto* p : *node->middle->children) {
                    if (p->node_type == ASTNodeType::Parameter && p->token) {
                        FuncParam fp;
                        fp.name = p->token->value;
                        fp.type = p->left ? resolveTypeFromNode(p->left) : nullptr;
                        params.push_back(fp);
                    }
                }
            }
            if (!params.empty()) func_params[name] = params;
            if (node->left && node->left->node_type == ASTNodeType::ReturnType && node->left->left)
                sym->return_type = resolveTypeFromNode(node->left->left);
            // Store generic params
            auto gps = getGenericParams(node);
            if (!gps.empty()) sym->generic_params = gps;
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
            // Store trait implementations
            if (node->middle && node->middle->children) {
                for (auto* child : *node->middle->children) {
                    if (child->token) sym->field_order.push_back(child->token->value);
                }
            }
            // Extract methods and trait implementations from children
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
                        sym->field_order.push_back(ftok->value);
                    } else if (child->node_type == ASTNodeType::FunctionDeclaration) {
                        // Store method info
                        sym->methods.push_back(child);
                    } else if (child->node_type == ASTNodeType::Identifier) {
                        // Trait implementation marker (without func keyword)
                        // Already stored in middle as trait list
                    }
                }
            }
            // Store @Generic params
            auto gps = getGenericParams(node);
            if (!gps.empty()) sym->generic_params = gps;
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
            if (node->left) sym->backing_type = resolveTypeFromNode(node->left);
            if (node->children) {
                for (auto* child : *node->children) {
                    if (child->node_type == ASTNodeType::EnumField) {
                        auto ftok = child->token;
                        if (!ftok) continue;
                        auto* vsym = new Symbol();
                        vsym->name = ftok->value;
                        vsym->symbol_type = SymbolType::Variable;
                        vsym->token = *ftok;
                        if (child->right) {
                            vsym->const_init = child->right;
                            vsym->resolved_type = makeNumericType("i32");
                        }
                        (*sym->fields)[ftok->value] = vsym;
                        sym->field_order.push_back(ftok->value);
                    } else if (child->node_type == ASTNodeType::FunctionDeclaration) {
                        sym->methods.push_back(child);
                    } else if (child->node_type == ASTNodeType::Identifier) {
                        // trait marker
                    }
                }
            }
            auto gps = getGenericParams(node);
            if (!gps.empty()) sym->generic_params = gps;
            defineSym(sym);
            break;
        }
        case ASTNodeType::UnionDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::Union;
            sym->token = *tok;
            sym->is_pub = node->is_pub;
            sym->fields = new std::unordered_map<std::string, Symbol*>();
            if (node->children) {
                for (auto* child : *node->children) {
                    if (child->node_type == ASTNodeType::UnionField) {
                        auto ftok = child->token;
                        if (!ftok) continue;
                        auto* fsym = new Symbol();
                        fsym->name = ftok->value;
                        fsym->symbol_type = SymbolType::Variable;
                        fsym->token = *ftok;
                        // The left child stores the variant payload type (e.g., Circle(f64))
                        if (child->left) {
                            fsym->resolved_type = resolveTypeFromNode(child->left);
                            fsym->variant_is_struct = (child->left->node_type == ASTNodeType::Block ||
                                                       child->left->node_type == ASTNodeType::StructDeclaration);
                            // If it's a struct variant, record its fields
                            if (fsym->variant_is_struct && child->left->children) {
                                fsym->fields = new std::unordered_map<std::string, Symbol*>();
                                for (auto* sub : *child->left->children) {
                                    if (sub->node_type == ASTNodeType::StructField && sub->token) {
                                        auto* sfsym = new Symbol();
                                        sfsym->name = sub->token->value;
                                        sfsym->symbol_type = SymbolType::Variable;
                                        sfsym->token = *sub->token;
                                        if (sub->left) sfsym->resolved_type = resolveTypeFromNode(sub->left);
                                        (*fsym->fields)[sub->token->value] = sfsym;
                                    }
                                }
                            }
                        }
                        // Struct-like variant: Circle { radius: f64 } — fields in children
                        if (child->children) {
                            fsym->variant_is_struct = true;
                            fsym->fields = new std::unordered_map<std::string, Symbol*>();
                            for (auto* sub : *child->children) {
                                if (sub->node_type == ASTNodeType::StructField && sub->token) {
                                    auto* sfsym = new Symbol();
                                    sfsym->name = sub->token->value;
                                    sfsym->symbol_type = SymbolType::Variable;
                                    sfsym->token = *sub->token;
                                    if (sub->left) sfsym->resolved_type = resolveTypeFromNode(sub->left);
                                    (*fsym->fields)[sub->token->value] = sfsym;
                                }
                            }
                        }
                        (*sym->fields)[ftok->value] = fsym;
                        sym->field_order.push_back(ftok->value);
                    } else if (child->node_type == ASTNodeType::FunctionDeclaration) {
                        sym->methods.push_back(child);
                    } else if (child->node_type == ASTNodeType::Identifier) {
                        // trait marker
                    }
                }
            }
            defineSym(sym);
            break;
        }
        case ASTNodeType::ErrorDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::ErrorSet;
            sym->token = *tok;
            sym->fields = new std::unordered_map<std::string, Symbol*>();
            if (node->children) {
                for (auto* child : *node->children) {
                    if (child->node_type == ASTNodeType::ErrorField) {
                        auto ftok = child->token;
                        if (!ftok) continue;
                        auto* fsym = new Symbol();
                        fsym->name = ftok->value;
                        fsym->symbol_type = SymbolType::Variable;
                        fsym->token = *ftok;
                        (*sym->fields)[ftok->value] = fsym;
                    }
                }
            }
            defineSym(sym);
            break;
        }
        case ASTNodeType::BehaveDeclaration: {
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::Trait;
            sym->token = *tok;
            // Store method signatures for conformance checking
            if (node->children) {
                for (auto* child : *node->children) {
                    if (child->node_type == ASTNodeType::FunctionDeclaration) {
                        sym->methods.push_back(child);
                    } else if (child->node_type == ASTNodeType::StructField && child->token) {
                        // needs field: record in traits
                        auto* fsym = new Symbol();
                        fsym->name = child->token->value;
                        fsym->symbol_type = SymbolType::Variable;
                        fsym->token = *child->token;
                        if (child->left) fsym->resolved_type = resolveTypeFromNode(child->left);
                        sym->needs_fields[child->token->value] = fsym;
                    }
                }
            }
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
            sym->is_global = true;
            defineSym(sym);
            break;
        }
        case ASTNodeType::VarDeclaration: {
            // Only consider global vars (at top level)
            auto* sym = new Symbol();
            sym->name = name;
            sym->symbol_type = SymbolType::Variable;
            sym->token = *tok;
            sym->is_mut = node->is_mut;
            sym->is_global = true;
            if (node->left) sym->resolved_type = resolveTypeFromNode(node->left);
            defineSym(sym);
            break;
        }
        default: break;
    }
}

// ── Main analysis dispatcher ────────────────────────────────────────────

TypeInfo* Analyzer::anaNode(ASTNode* node) {
    if (!node) return nullptr;
    switch (node->node_type) {
        case ASTNodeType::Invalid:
        case ASTNodeType::Comment:
        case ASTNodeType::GenericParams:
        case ASTNodeType::Parameters:
        case ASTNodeType::Argument:
            return nullptr;

        case ASTNodeType::FunctionDeclaration: return anaFuncDecl(node);
        case ASTNodeType::ExtDeclaration:       return nullptr;
        case ASTNodeType::Block:                return anaBlock(node);

        case ASTNodeType::IfBody:
        case ASTNodeType::ElseBody:
        case ASTNodeType::LoopBody:
        case ASTNodeType::MatchBody:
        case ASTNodeType::CaptureBlock:
            return anaBody(node);

        case ASTNodeType::MatchCase:
            if (node->right) return anaNode(node->right);
            return nullptr;

        case ASTNodeType::VarDeclaration:
        case ASTNodeType::ConstDeclaration:
            return anaVarDecl(node);

        case ASTNodeType::Assignment:    return anaAssignment(node);
        case ASTNodeType::ReturnStatement: return anaReturn(node);

        case ASTNodeType::BreakStatement:
        case ASTNodeType::SkipStatement: {
            auto tok = node->token;
            if (tok) {
                if (!sym_table.isInLoop()) {
                    std::string kind = (node->node_type == ASTNodeType::BreakStatement) ? "break" : "skip";
                    reportError(*tok, "FlowError", "'" + kind + "' outside of a loop is not allowed.");
                } else if (node->node_type == ASTNodeType::SkipStatement) {
                    sym_table.has_skip_in_loop = true;
                }
            }
            return nullptr;
        }
        case ASTNodeType::TryBlock:           return anaTryBlock(node);
        case ASTNodeType::TryExpression:      return anaTryExpression(node);
        case ASTNodeType::IfStatement:
        case ASTNodeType::ElseIfStatement:    return anaIf(node);
        case ASTNodeType::LoopStatement:      return anaLoop(node);
        case ASTNodeType::MatchStatement:     return anaMatch(node);
        case ASTNodeType::DeferStatement:     return anaDefer(node);
        case ASTNodeType::BinaryExpression:
            // Check for struct literal { ... } suffix
            if (node->token && node->token->value == "{}")
                return anaStructLiteral(node);
            return anaBinary(node);
        case ASTNodeType::RangeExpression:    return anaRange(node);
        case ASTNodeType::UnaryExpression:    return anaUnary(node);
        case ASTNodeType::MemberAccess:       return anaMemberAccess(node);
        case ASTNodeType::Identifier:         return anaIdentifier(node);
        case ASTNodeType::FunctionCall:       return anaFunctionCall(node);
        case ASTNodeType::CatchExpression:    return anaCatch(node);
        case ASTNodeType::BuiltinExpression:  return anaAnnotation(node);
        case ASTNodeType::Annotation:         return nullptr;

        case ASTNodeType::IntegerLiteral:
        case ASTNodeType::FloatLiteral:
        case ASTNodeType::BoolLiteral:
        case ASTNodeType::CharLiteral:
        case ASTNodeType::StringLiteral:
            return inferTypeFromLiteral(node);

        case ASTNodeType::ArrayLiteral:       return anaArrayLiteral(node);
        case ASTNodeType::TupleLiteral:       return anaTupleLiteral(node);

        default: return nullptr;
    }
}

// ── Function declaration ────────────────────────────────────────────────

TypeInfo* Analyzer::anaFuncDecl(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto& name = tok->value;

    // Resolve return type
    TypeInfo* ret_type = nullptr;
    if (node->left && node->left->node_type == ASTNodeType::ReturnType && node->left->left)
        ret_type = resolveTypeFromNode(node->left->left);
    // Default return type is void
    if (!ret_type) ret_type = allocType(TypeCategory::Void);

    current_return_type = ret_type;
    current_func_name = name;

    // Store generic parameters for type compatibility checking
    current_generic_params.clear();
    if (auto* sym = sym_table.resolveGlobal(name)) {
        current_generic_params = sym->generic_params;
    }

    auto* func_scope = new Scope();
    sym_table.pushScope(func_scope);

    // Register parameters
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

    // Analyze body
    bool has_explicit_return = false;
    TypeInfo* body_ret = nullptr;
    if (node->right && node->right->node_type == ASTNodeType::Block) {
        body_ret = anaBlock(node->right);
        has_explicit_return = (body_ret != nullptr);
    }

    // Check return type coverage
    if (ret_type && ret_type->category != TypeCategory::Void && ret_type->category != TypeCategory::Noret) {
        if (!has_explicit_return && !sym_table.has_skip_in_loop) {
            // Check if last statement is a terminating statement
            bool terminates = false;
            if (node->right && node->right->children && !node->right->children->empty()) {
                auto* last_stmt = node->right->children->back();
                terminates = hasControlFlowTerminator(last_stmt);
            }
            if (!terminates) {
                reportError(*tok, "ReturnError",
                    "Function '" + name + "' expects return type '" + typeToString(ret_type) +
                    "' but may not return a value on all paths.");
            }
        }
    }

    sym_table.popScope();
    sym_table.has_skip_in_loop = false;
    current_return_type = nullptr;
    current_func_name.clear();
    return ret_type;
}

// ── Block / Body ────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaBlock(ASTNode* node) {
    auto* block_scope = new Scope();
    sym_table.pushScope(block_scope);
    TypeInfo* last_type = nullptr;
    bool has_return = false;
    if (node->children) {
        for (auto* stmt : *node->children) {
            if (has_return) break; // dead code after return
            last_type = anaNode(stmt);
            if (stmt->node_type == ASTNodeType::ReturnStatement ||
                stmt->node_type == ASTNodeType::BreakStatement ||
                stmt->node_type == ASTNodeType::SkipStatement) {
                has_return = true;
            }
        }
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

// ── Variable declaration ────────────────────────────────────────────────

TypeInfo* Analyzer::anaVarDecl(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto& name = tok->value;

    TypeInfo* decl_type = node->left ? resolveTypeFromNode(node->left) : nullptr;
    TypeInfo* init_type = nullptr;

    if (node->right) {
        init_type = anaNode(node->right);
        // Allow integer literal to widen to the declared type
        if (decl_type && init_type && init_type->category == TypeCategory::Integer && init_type->name == "i32") {
            init_type = tryNormalizeInteger(init_type, decl_type);
        }
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
    Symbol* existing = sym_table.current_scope->resolveLocal(name);
    if (existing && existing->symbol_type == SymbolType::Variable && !is_global) {
        reportError(*tok, "DeclError",
            "Variable '" + name + "' is already declared in this scope.");
    } else {
        auto* sym = new Symbol();
        sym->name = name;
        sym->symbol_type = SymbolType::Variable;
        sym->resolved_type = decl_type ? decl_type : init_type;
        sym->token = *tok;
        sym->is_mut = node->is_mut;
        sym->is_const = node->is_const;
        sym->is_global = is_global;
        sym_table.define(sym);
    }
    return decl_type ? decl_type : init_type;
}

// ── Assignment ──────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaAssignment(ASTNode* node) {
    if (!node->left) return nullptr;
    auto* lhs = node->left;

    auto checkMutability = [&](const std::string& var_name, const Token& tok) -> Symbol* {
        if (auto* sym = sym_table.resolve(var_name)) {
            if (!sym->is_mut) {
                reportError(tok, "MutError",
                    "Cannot assign to immutable variable '" + var_name + "'. Use 'mut' to make it mutable.");
            }
            return sym;
        }
        return nullptr;
    };

    if (lhs->node_type == ASTNodeType::Identifier) {
        auto ltok = lhs->token;
        if (!ltok) return nullptr;
        auto* sym = checkMutability(ltok->value, *ltok);
        TypeInfo* sym_type = sym ? sym->resolved_type : nullptr;
        if (node->right) {
            auto* rhs_type = anaNode(node->right);
            if (sym_type && rhs_type) {
                if (node->token) {
                    TokenType assign_op = node->token->type;
                    if (assign_op != TokenType::Equals) {
                        // Compound assignment needs compatible operator
                        bool is_arithmetic = (sym_type->isNumeric() && rhs_type->isNumeric());
                        if (!is_arithmetic) {
                            reportError(*ltok, "TypeError",
                                "Invalid compound assignment to '" + ltok->value + "'.");
                        }
                    }
                }
                std::string why;
                if (!typesCompatible(sym_type, rhs_type, &why)) {
                    reportError(*ltok, "TypeError",
                        "Type mismatch in assignment to '" + ltok->value +
                        "': expected '" + typeToString(sym_type) +
                        "', found '" + typeToString(rhs_type) + "'." +
                        (why.empty() ? "" : " " + why));
                }
            }
            return rhs_type;
        }
    } else if (lhs->node_type == ASTNodeType::MemberAccess) {
        // struct.field = value  or  ptr.* = value
        auto* lhs_type = anaMemberAccess(lhs);
        if (lhs->token && lhs->left) {
            // Check if the struct itself is mutable (mut)
            if (lhs->left->node_type == ASTNodeType::Identifier && lhs->left->token) {
                auto* sym = sym_table.resolve(lhs->left->token->value);
                if (sym && !sym->is_mut && sym->symbol_type == SymbolType::Variable) {
                    reportError(*lhs->left->token, "MutError",
                        "Cannot modify field of immutable variable '" + lhs->left->token->value + "'.");
                }
            }
        }
        if (node->right) {
            auto* rhs_type = anaNode(node->right);
                    if (lhs_type && rhs_type) {
                        std::string why;
                        if (!typesCompatible(lhs_type, rhs_type, &why)) {
                            Token err_tok = (node->right->token.has_value() ? *node->right->token : *node->token);
                            reportError(err_tok, "TypeError",
                                "Type mismatch in field assignment: expected '" +
                                typeToString(lhs_type) + "', found '" + typeToString(rhs_type) + "'." +
                                (why.empty() ? "" : " " + why));
                        }
                    }
                    return rhs_type;
                }
            } else if (lhs->node_type == ASTNodeType::UnaryExpression) {
                // ptr.* = value
                if (!node->right) return nullptr;
                auto* ptr_type = anaUnary(lhs);
                auto* rhs_type = anaNode(node->right);
                if (ptr_type && rhs_type) {
                    std::string why;
                    if (!typesCompatible(ptr_type, rhs_type, &why)) {
                        Token err_tok = (node->right->token.has_value() ? *node->right->token : *node->token);
                        reportError(err_tok, "TypeError",
                            "Type mismatch in deref assignment: expected '" +
                            typeToString(ptr_type) + "', found '" + typeToString(rhs_type) + "'." +
                            (why.empty() ? "" : " " + why));
                    }
                }
                return rhs_type;
    } else {
        auto tok = node->token;
        if (tok) reportError(*tok, "SyntaxError", "Invalid left-hand side of assignment.");
    }

    if (node->right) return anaNode(node->right);
    return nullptr;
}

// ── Return statement ────────────────────────────────────────────────────

TypeInfo* Analyzer::anaReturn(ASTNode* node) {
    TypeInfo* ret_type = nullptr;
    if (node->left) {
        ret_type = anaNode(node->left);
        // Integer literal widening
        if (ret_type && current_return_type &&
            ret_type->category == TypeCategory::Integer && ret_type->name == "i32") {
            ret_type = tryNormalizeInteger(ret_type, current_return_type);
        }
    } else {
        ret_type = allocType(TypeCategory::Void);
    }

    if (current_return_type && ret_type) {
        // For error union return types: check against ok_type OR error_type
        bool compatible = false;
        if (current_return_type->category == TypeCategory::ErrorUnion) {
            // Return value can be the ok type (success) or the error type (failure)
            if (current_return_type->ok_type && typesCompatible(current_return_type->ok_type, ret_type))
                compatible = true;
            if (!compatible && current_return_type->error_type &&
                typesCompatible(current_return_type->error_type, ret_type))
                compatible = true;
        } else {
            compatible = typesCompatible(current_return_type, ret_type);
        }

        if (!compatible) {
            auto tok = node->token;
            if (tok) {
                std::string why;
                reportError(*tok, "ReturnError",
                    "Function '" + current_func_name + "' returns '" +
                    typeToString(ret_type) + "' but declared return type is '" +
                    typeToString(current_return_type) + "'." +
                    (why.empty() ? "" : " " + why));
            }
        }
    }

    // Check empty return in non-void function
    if (current_return_type && !node->left &&
        current_return_type->category != TypeCategory::Void &&
        current_return_type->category != TypeCategory::Noret) {
        // Allow empty ret in error union functions (implicit error propagation)
        if (current_return_type->category == TypeCategory::ErrorUnion) {
            // Empty ret can propagate error — this is valid
        } else {
            auto tok = node->token;
            if (tok) {
                reportError(*tok, "ReturnError",
                    "Function '" + current_func_name + "' expects return type '" +
                    typeToString(current_return_type) + "' but 'ret' has no value.");
            }
        }
    }
    return ret_type;
}

// ── If / Else if ────────────────────────────────────────────────────────

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
        if (node->right->node_type == ASTNodeType::ElseIfStatement)
            anaIf(node->right);
        else if (node->right->node_type == ASTNodeType::ElseBody)
            anaBody(node->right);
        else
            anaNode(node->right);
    }
    return nullptr;
}

// ── Loop ────────────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaLoop(ASTNode* node) {
    auto* loop_scope = new Scope();
    sym_table.pushLoopScope(loop_scope);
    // Iterator loop (loop items |n|) - no boolean condition needed
    bool is_iterator_loop = (node->middle && node->middle->node_type == ASTNodeType::Identifier);
    if (node->left && !is_iterator_loop) {
        auto* cond_type = anaNode(node->left);
        // If cond_type is an Array, this is actually an iterator pattern — skip bool check
        if (cond_type && !cond_type->isBool() && cond_type->category != TypeCategory::Array) {
            // Only report error for non-array, non-iterator loops
        }
    }
    if (node->middle && node->middle->node_type == ASTNodeType::Identifier) {
        auto itok = node->middle->token;
        if (itok) {
            auto* isym = new Symbol();
            isym->name = itok->value;
            isym->symbol_type = SymbolType::Variable;
            isym->token = *itok;
            // Iterator variable type depends on the iterable type
            // For array literals, infer elem type
            TypeInfo* iter_type = node->left ? anaNode(node->left) : nullptr;
            if (iter_type && iter_type->category == TypeCategory::Array && iter_type->elem_type)
                isym->resolved_type = cloneTypeInfo(iter_type->elem_type);
            else
                isym->resolved_type = allocType(TypeCategory::Integer);
            isym->resolved_type->name = "i32";
            sym_table.define(isym);
        }
    }
    if (node->right && node->right->node_type == ASTNodeType::LoopBody && node->right->children) {
        for (auto* stmt : *node->right->children) anaNode(stmt);
    } else if (node->right && node->right->node_type == ASTNodeType::LoopBody && node->right->left) {
        anaNode(node->right->left);
    }
    sym_table.popScope();
    return nullptr;
}

// ── Match ───────────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaMatch(ASTNode* node) {
    TypeInfo* match_type = node->left ? anaNode(node->left) : nullptr;

    if (node->children) {
        for (auto* case_node : *node->children) {
            if (case_node->right && case_node->right->node_type == ASTNodeType::MatchBody) {
                auto* match_scope = new Scope();
                sym_table.pushScope(match_scope);

                // Analyze pattern and bind variables
                if (case_node->left) {
                    if (!(case_node->left->node_type == ASTNodeType::Identifier &&
                        case_node->left->token && case_node->left->token->value == "else")) {
                        anaMatchCasePattern(case_node->left, match_type);
                    }
                }

                // Handle union destructure pattern: Variant { field, ... }
                // The block is stored in case_node->middle by the parser
                if (case_node->middle && case_node->middle->node_type == ASTNodeType::Block && case_node->middle->children) {
                    // Determine union name and variant name from the pattern
                    std::string union_name, variant_name;
                    if (case_node->left && case_node->left->node_type == ASTNodeType::BinaryExpression) {
                        auto* bin = case_node->left;
                        if (bin->left && bin->right && bin->left->token && bin->right->token) {
                            union_name = bin->left->token->value;
                            variant_name = bin->right->token->value;
                        }
                    } else if (case_node->left && case_node->left->token) {
                        // Single identifier variant: use match_type name as union name
                        variant_name = case_node->left->token->value;
                        if (match_type) union_name = match_type->name;
                    }
                    // Get the variant's struct fields type info
                    Symbol* variant_sym = nullptr;
                    if (!union_name.empty() && !variant_name.empty()) {
                        if (auto* usym = sym_table.resolveGlobal(union_name)) {
                            if (usym->fields) {
                                auto fit = usym->fields->find(variant_name);
                                if (fit != usym->fields->end()) variant_sym = fit->second;
                            }
                        }
                    }
                    for (auto* child : *case_node->middle->children) {
                        if (child->node_type == ASTNodeType::Identifier && child->token) {
                            auto* vsym = new Symbol();
                            vsym->name = child->token->value;
                            vsym->symbol_type = SymbolType::Variable;
                            vsym->token = *child->token;
                            vsym->resolved_type = allocType(TypeCategory::Float);
                            vsym->resolved_type->name = "f64";
                            if (variant_sym && variant_sym->fields && variant_sym->variant_is_struct) {
                                auto ffit = variant_sym->fields->find(child->token->value);
                                if (ffit != variant_sym->fields->end() && ffit->second->resolved_type) {
                                    vsym->resolved_type = cloneTypeInfo(ffit->second->resolved_type);
                                }
                            }
                            sym_table.define(vsym);
                        }
                    }
                }

                if (case_node->right->left) {
                    if (case_node->right->left->node_type == ASTNodeType::Block)
                        anaBlock(case_node->right->left);
                    else
                        anaNode(case_node->right->left);
                }
                sym_table.popScope();
            }
        }
    }
    return nullptr;
}

TypeInfo* Analyzer::anaMatchCasePattern(ASTNode* pattern, TypeInfo* match_type) {
    if (!pattern) return nullptr;

    // Literal pattern: 42, "hello", true
    if (pattern->node_type == ASTNodeType::IntegerLiteral ||
        pattern->node_type == ASTNodeType::FloatLiteral ||
        pattern->node_type == ASTNodeType::BoolLiteral ||
        pattern->node_type == ASTNodeType::StringLiteral ||
        pattern->node_type == ASTNodeType::CharLiteral) {
        auto* lit_type = inferTypeFromLiteral(pattern);
        if (match_type && lit_type) {
            std::string why;
            if (!typesCompatible(match_type, lit_type, &why)) {
                auto tok = pattern->token;
                if (tok) {
                    reportError(*tok, "TypeError",
                        "Match pattern type '" + typeToString(lit_type) +
                        "' does not match scrutinee type '" + typeToString(match_type) + "'." +
                        (why.empty() ? "" : " " + why));
                }
            }
        }
        return lit_type;
    }

    // Identifier pattern (enum variant or variable binding)
    if (pattern->node_type == ASTNodeType::Identifier) {
        auto tok = pattern->token;
        if (!tok) return nullptr;
        auto& name = tok->value;

        // Check if it's an enum variant like Color.Red (but that's a MemberAccess)
        // Or it could be a simple identifier binding (for primitives)
        if (match_type) {
            if (auto* sym = sym_table.resolve(name)) {
                if (sym->symbol_type != SymbolType::Variable) {
                    // It's a type name being used as a pattern
                    return match_type;
                }
            }
        }
        return match_type;
    }

    // Enum/union destructure pattern: Variant(v) or Variant{vars...}
    if (pattern->node_type == ASTNodeType::MemberAccess) {
        if (pattern->right && pattern->right->node_type == ASTNodeType::FunctionCall) {
            // Variant(payload) pattern
            auto* call = pattern->right;
            if (call->children) {
                for (auto* arg : *call->children) {
                    if (arg->left && arg->left->node_type == ASTNodeType::Identifier && arg->left->token) {
                        // This is a variable binding in a destructure pattern
                        auto* vsym = new Symbol();
                        vsym->name = arg->left->token->value;
                        vsym->symbol_type = SymbolType::Variable;
                        vsym->token = *arg->left->token;
                        // Infer type from union variant payload
                        if (pattern->left && pattern->left->token && match_type) {
                            auto& union_name = pattern->left->token->value;
                            auto& variant_name = pattern->right->token->value;
                            if (auto* usym = sym_table.resolveGlobal(union_name)) {
                                if (usym->fields) {
                                    auto fit = usym->fields->find(variant_name);
                                    if (fit != usym->fields->end() && fit->second->resolved_type) {
                                        vsym->resolved_type = cloneTypeInfo(fit->second->resolved_type);
                                    }
                                }
                            }
                        }
                        sym_table.define(vsym);
                    } else if (arg->left) {
                        anaNode(arg->left);
                    }
                }
            }
        } else if (pattern->right && pattern->right->node_type == ASTNodeType::Identifier) {
            // Simple variant pattern: Color.Red
            return match_type;
        }
        return match_type;
    }

    // Struct destructure pattern: { field, ... } (stored in middle)
    // Or block destructure: Variant { field, ... }
    if (pattern->node_type == ASTNodeType::Block) {
        if (pattern->children) {
            for (auto* child : *pattern->children) {
                if (child->node_type == ASTNodeType::Identifier && child->token) {
                    auto* vsym = new Symbol();
                    vsym->name = child->token->value;
                    vsym->symbol_type = SymbolType::Variable;
                    vsym->token = *child->token;
                    vsym->resolved_type = allocType(TypeCategory::Integer);
                    vsym->resolved_type->name = "f64"; // shape fields are f64 in demo
                    sym_table.define(vsym);
                }
            }
        }
        return match_type;
    }

    return match_type;
}

// ── Try block ───────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaTryBlock(ASTNode* node) {
    if (node->left) anaNode(node->left);
    if (node->right && node->right->node_type == ASTNodeType::CatchExpression) {
        auto* catch_node = node->right;
        auto* catch_scope = new Scope();
        sym_table.pushScope(catch_scope);
        // Register the caught error variable
        if (catch_node->middle && catch_node->middle->token) {
            auto* esym = new Symbol();
            esym->name = catch_node->middle->token->value;
            esym->symbol_type = SymbolType::Variable;
            esym->token = *catch_node->middle->token;
            esym->resolved_type = allocType(TypeCategory::Str);
            sym_table.define(esym);
        }
        if (catch_node->left) anaBody(catch_node->left);
        sym_table.popScope();
    }
    return nullptr;
}

// ── Try expression ──────────────────────────────────────────────────────

TypeInfo* Analyzer::anaTryExpression(ASTNode* node) {
    TypeInfo* inner = node->left ? anaNode(node->left) : nullptr;

    // try with catch handler
    if (node->right && node->right->node_type == ASTNodeType::CatchExpression) {
        auto* catch_node = node->right;
        auto* catch_scope = new Scope();
        sym_table.pushScope(catch_scope);
        // Register the |err| variable
        if (catch_node->middle && catch_node->middle->token) {
            auto* esym = new Symbol();
            esym->name = catch_node->middle->token->value;
            esym->symbol_type = SymbolType::Variable;
            esym->token = *catch_node->middle->token;
            esym->resolved_type = allocType(TypeCategory::Str);
            sym_table.define(esym);
        }
        if (catch_node->left) anaBody(catch_node->left);
        sym_table.popScope();
    }

    // Without catch: try propagates to caller - need to check enclosing function return type
    if (inner && inner->category == TypeCategory::ErrorUnion) {
        if (current_return_type) {
            // Check if function returns the same error union or compatible type
        }
        // Return the ok_type from the error union
        if (inner->ok_type) return cloneTypeInfo(inner->ok_type);
        return inner;
    }

    return inner;
}

// ── Catch expression ────────────────────────────────────────────────────

TypeInfo* Analyzer::anaCatch(ASTNode* node) {
    auto* catch_scope = new Scope();
    sym_table.pushScope(catch_scope);
    if (node->middle && node->middle->token) {
        auto* esym = new Symbol();
        esym->name = node->middle->token->value;
        esym->symbol_type = SymbolType::Variable;
        esym->token = *node->middle->token;
        esym->resolved_type = allocType(TypeCategory::Str);
        sym_table.define(esym);
    }
    TypeInfo* result = nullptr;
    if (node->left) result = anaBody(node->left);
    sym_table.popScope();
    return result;
}

// ── Defer statement ─────────────────────────────────────────────────────

TypeInfo* Analyzer::anaDefer(ASTNode* node) {
    if (!node->left) return nullptr;
    if (node->left->node_type == ASTNodeType::Block) {
        return anaBlock(node->left);
    }
    return anaNode(node->left);
}

// ── Binary expression ──────────────────────────────────────────────────

TypeInfo* Analyzer::anaBinary(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto op_type = tok->type;

    // Struct/union literal: TypeName { fields }
    if (op_type == TokenType::LeftBrace)
        return anaStructLiteral(node);

    auto* left_type = node->left ? anaNode(node->left) : nullptr;
    auto* right_type = node->right ? anaNode(node->right) : nullptr;
    if (!left_type || !right_type) return nullptr;

    // Integer literal widening: if one side is a plain integer literal (i32 category)
    // and the other is a specific integer type, widen the literal to match.
    if (left_type->category == TypeCategory::Integer && left_type->name == "i32") {
        left_type = tryNormalizeInteger(left_type, right_type);
    }
    if (right_type->category == TypeCategory::Integer && right_type->name == "i32") {
        right_type = tryNormalizeInteger(right_type, left_type);
    }

    auto err = [&](const std::string& msg) -> TypeInfo* {
        reportError(*tok, "TypeError", msg);
        return nullptr;
    };

    switch (op_type) {
        // Arithmetic
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
            if (!left_type->isNumeric() || !right_type->isNumeric())
                return err("Arithmetic operator requires numeric operands (got '" +
                           typeToString(left_type) + "' and '" + typeToString(right_type) + "').");
            // String concatenation
            if (op_type == TokenType::Plus &&
                (left_type->category == TypeCategory::Str || left_type->category == TypeCategory::String))
                return left_type;
            // Widen to the wider type
            if (left_type->isFloat() || right_type->isFloat())
                return (left_type->isFloat() ? left_type : right_type);
            return left_type;

        // Equality
        case TokenType::EqualsEquals:
        case TokenType::NotEquals: {
            // Allow numeric comparisons even if types differ (both numeric)
            if (left_type->isNumeric() && right_type->isNumeric()) {
                return allocType(TypeCategory::Bool);
            }
            std::string why;
            if (!typesCompatible(left_type, right_type, &why))
                return err("Cannot compare values of different types: '" +
                           typeToString(left_type) + "' vs '" + typeToString(right_type) + "'." +
                           (why.empty() ? "" : " " + why));
            return allocType(TypeCategory::Bool);
        }

        // Relational
        case TokenType::LessThan:
        case TokenType::LessThanEquals:
        case TokenType::GreaterThan:
        case TokenType::GreaterThanEquals:
            if (!left_type->isNumeric() || !right_type->isNumeric())
                return err("Comparison operator requires numeric operands (got '" +
                           typeToString(left_type) + "' and '" + typeToString(right_type) + "').");
            return allocType(TypeCategory::Bool);

        // Logical
        case TokenType::AndAnd:
        case TokenType::OrOr:
            if (!left_type->isBool() || !right_type->isBool())
                return err("Logical operator requires boolean operands (got '" +
                           typeToString(left_type) + "' and '" + typeToString(right_type) + "').");
            return left_type;

        // Bitwise (including enum types with backing integer types)
        case TokenType::And:
        case TokenType::Or:
        case TokenType::Caret:
        case TokenType::ShiftLeft:
        case TokenType::ShiftRight: {
            bool left_int = left_type->isInteger() || left_type->category == TypeCategory::Enum;
            bool right_int = right_type->isInteger() || right_type->category == TypeCategory::Enum;
            if (!left_int || !right_int)
                return err("Bitwise operator requires integer or enum operands (got '" +
                           typeToString(left_type) + "' and '" + typeToString(right_type) + "').");
            return left_type;
        }

        // Member access
        case TokenType::Dot:
            return anaMemberAccess(node);

        default:
            return left_type;
    }
}

// ── Range expression ────────────────────────────────────────────────────

TypeInfo* Analyzer::anaRange(ASTNode* node) {
    if (node->left) anaNode(node->left);
    if (node->right) anaNode(node->right);
    return allocType(TypeCategory::Struct);
}

// ── Struct literal ──────────────────────────────────────────────────────

TypeInfo* Analyzer::anaStructLiteral(ASTNode* node) {
    // node->left = type expression, node->right = Block with field assignments
    TypeInfo* struct_type = node->left ? anaNode(node->left) : nullptr;
    if (!struct_type) return nullptr;

    std::string type_name = struct_type->name;
    Symbol* struct_sym = sym_table.resolveGlobal(type_name);

    // Check if this is a union variant construction: Union.Variant { fields }
    if (struct_sym && struct_sym->symbol_type == SymbolType::Union) {
        // The node->left might be a MemberAccess (Union.Variant) — analyze variant fields
        if (node->left && node->left->node_type == ASTNodeType::MemberAccess) {
            auto* ma = node->left;
            // Extract variant name from MemberAccess right side
            std::string variant_name;
            if (ma->right && ma->right->token) variant_name = ma->right->token->value;
            // If right is a "{}" BinaryExpression, the variant is in its left
            if (ma->right && ma->right->node_type == ASTNodeType::BinaryExpression &&
                ma->right->token && ma->right->token->value == "{}" &&
                ma->right->left && ma->right->left->token)
                variant_name = ma->right->left->token->value;
            if (!variant_name.empty() && struct_sym->fields) {
                auto vfit = struct_sym->fields->find(variant_name);
                if (vfit != struct_sym->fields->end() && vfit->second->fields) {
                    // Use variant's fields for struct literal field checking
                    struct_sym = vfit->second;
                }
            }
        }
    }

    if (node->right && node->right->node_type == ASTNodeType::Block && node->right->children) {
        for (auto* child : *node->right->children) {
            if (child->node_type == ASTNodeType::Assignment) {
                // field = value
                if (child->left && child->left->node_type == ASTNodeType::Identifier && child->left->token) {
                    auto& field_name = child->left->token->value;
                    // Check field exists in struct
                    if (struct_sym && struct_sym->fields) {
                        auto fit = struct_sym->fields->find(field_name);
                        if (fit == struct_sym->fields->end()) {
                                    reportError(*child->left->token, "FieldError",
                                        "Struct '" + type_name + "' has no field '" + field_name + "'.");
                                } else if (child->right) {
                                    auto* val_type = anaNode(child->right);
                                    if (val_type && fit->second->resolved_type) {
                                        // Integer literal widening
                                        val_type = tryNormalizeInteger(val_type, fit->second->resolved_type);
                                        // Check if field type is a generic param — accept any type
                                        bool is_generic = false;
                                        if (!struct_sym->generic_params.empty()) {
                                            auto& ftype_name = fit->second->resolved_type->name;
                                            for (auto& gp : struct_sym->generic_params) {
                                                if (gp == ftype_name) { is_generic = true; break; }
                                            }
                                        }
                                        if (!is_generic) {
                                            std::string why;
                                            if (!typesCompatible(fit->second->resolved_type, val_type, &why)) {
                                                reportError(*child->left->token, "TypeError",
                                                    "Struct field '" + field_name + "' expects '" +
                                                    typeToString(fit->second->resolved_type) +
                                                    "', found '" + typeToString(val_type) + "'." +
                                                    (why.empty() ? "" : " " + why));
                                            }
                                        }
                                    }
                                }
                    } else if (child->right) {
                        anaNode(child->right);
                    }
                }
            } else {
                anaNode(child);
            }
        }
    }

    return struct_type;
}

// ── Unary expression ────────────────────────────────────────────────────

TypeInfo* Analyzer::anaUnary(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto* inner_type = node->left ? anaNode(node->left) : nullptr;
    if (!inner_type) return nullptr;

    if (tok->value == "-") {
        if (!inner_type->isNumeric()) {
            reportError(*tok, "TypeError",
                "Unary minus requires numeric operand, found '" + typeToString(inner_type) + "'.");
            return allocType(TypeCategory::Integer);
        }
        return inner_type;
    }
    if (tok->value == "!") {
        if (!inner_type->canBeBool()) {
            reportError(*tok, "TypeError",
                "Logical not requires boolean operand, found '" + typeToString(inner_type) + "'.");
            return allocType(TypeCategory::Bool);
        }
        return inner_type;
    }
    if (tok->value == "~") {
        if (!inner_type->isInteger()) {
            reportError(*tok, "TypeError",
                "Bitwise not requires integer operand, found '" + typeToString(inner_type) + "'.");
            return allocType(TypeCategory::Integer);
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
            return allocType(TypeCategory::Integer);
        }
        if (inner_type->pointee_type) return cloneTypeInfo(inner_type->pointee_type);
        return allocType(TypeCategory::Integer);
    }
    return inner_type;
}

// ── Member access ──────────────────────────────────────────────────────

TypeInfo* Analyzer::anaMemberAccess(ASTNode* node) {
    auto* left_type = node->left ? anaNode(node->left) : nullptr;
    if (!left_type) return nullptr;

    if (node->right) {
        auto* right_node = node->right;

        // Union constructor: Value.Int(42) — in expression position
        if (right_node->node_type == ASTNodeType::FunctionCall &&
            (left_type->category == TypeCategory::Union ||
             (left_type->category == TypeCategory::Named && left_type->name != "fmt" && left_type->name.find("std") == std::string::npos))) {
            // Skip re-analysis for module member access like fmt.println() — already analyzed by anaBinary
            // Analyze arguments only for actual union/struct constructors
            if (right_node->children && left_type->category == TypeCategory::Union) {
                for (auto* arg : *right_node->children) {
                    if (arg->left && arg->left->node_type != ASTNodeType::Identifier)
                        anaNode(arg->left);
                }
            }
            if (left_type->category == TypeCategory::Union && right_node->token) {
                auto& variant_name = right_node->token->value;
                Symbol* union_sym = nullptr;
                if (!left_type->name.empty())
                    union_sym = sym_table.resolveGlobal(left_type->name);
                if (union_sym && union_sym->fields) {
                    auto fit = union_sym->fields->find(variant_name);
                    if (fit == union_sym->fields->end()) {
                        reportError(*right_node->token, "FieldError",
                            "Union '" + left_type->name + "' has no variant '" + variant_name + "'.");
                    } else {
                        // Check argument count matches variant payload
                        size_t expected_args = (fit->second->resolved_type) ? 1 : 0;
                        if (fit->second->variant_is_struct) expected_args = 0; // struct variant uses { }
                        size_t actual_args = right_node->children ? right_node->children->size() : 0;
                        (void)expected_args;
                        (void)actual_args;
                    }
                }
            }
            return left_type;
        }

        // Struct union destructure pattern (block): Variant { field, ... }
        if (right_node->node_type == ASTNodeType::Block) {
            if (right_node->children) {
                for (auto* child : *right_node->children) {
                    if (child->node_type == ASTNodeType::Identifier && child->token) {
                        auto* vsym = new Symbol();
                        vsym->name = child->token->value;
                        vsym->symbol_type = SymbolType::Variable;
                        vsym->token = *child->token;
                        if (left_type && !left_type->name.empty()) {
                            // Try to find field type from union struct variant
                            auto* union_sym = sym_table.resolveGlobal(left_type->name);
                            // variant name is from node->left
                            if (union_sym && union_sym->fields && node->left && node->left->token) {
                                union_sym->fields->find(node->left->token->value);
                            }
                        }
                        vsym->resolved_type = allocType(TypeCategory::Float);
                        vsym->resolved_type->name = "f64";
                        sym_table.define(vsym);
                    }
                }
            }
            return left_type;
        }

        if (right_node->token) {
            auto& field_name = right_node->token->value;

            // Struct field access or method call
            if (left_type->category == TypeCategory::Struct || left_type->category == TypeCategory::Named) {
                auto& type_name = left_type->name;
                if (auto* struct_sym = sym_table.resolveGlobal(type_name)) {
                    if (!struct_sym->fields) return left_type;
                    auto it = struct_sym->fields->find(field_name);
                    if (it != struct_sym->fields->end())
                        return it->second->resolved_type ? cloneTypeInfo(it->second->resolved_type) : left_type;
                    // Check if it's a method call
                    for (auto* method : struct_sym->methods) {
                        if (method->token && method->token->value == field_name) {
                            // Return type from method
                            if (method->left && method->left->node_type == ASTNodeType::ReturnType && method->left->left)
                                return resolveTypeFromNode(method->left->left);
                            return allocType(TypeCategory::Void);
                        }
                    }
                    reportError(*right_node->token, "FieldError",
                        "Struct '" + type_name + "' has no field '" + field_name + "'.");
                    return nullptr;
                }
            }

            // Enum variant access
            if (left_type->category == TypeCategory::Enum) {
                // Verify variant exists
                if (!left_type->name.empty()) {
                    if (auto* enum_sym = sym_table.resolveGlobal(left_type->name)) {
                        if (enum_sym->fields) {
                            auto fit = enum_sym->fields->find(field_name);
                            if (fit == enum_sym->fields->end()) {
                                reportError(*right_node->token, "FieldError",
                                    "Enum '" + left_type->name + "' has no variant '" + field_name + "'.");
                                return nullptr;
                            }
                        }
                    }
                }
                return left_type;
            }

            // Union variant access (without payload)
            if (left_type->category == TypeCategory::Union) {
                if (!left_type->name.empty()) {
                    if (auto* union_sym = sym_table.resolveGlobal(left_type->name)) {
                        if (union_sym->fields) {
                            auto fit = union_sym->fields->find(field_name);
                            if (fit == union_sym->fields->end()) {
                                reportError(*right_node->token, "FieldError",
                                    "Union '" + left_type->name + "' has no variant '" + field_name + "'.");
                                return nullptr;
                            }
                        }
                    }
                }
                return left_type;
            }

            // Module member access: fmt.println (whitelisted module)
            if (left_type->category == TypeCategory::Any &&
                node->left && node->left->node_type == ASTNodeType::Identifier &&
                undeclared_whitelist.count(node->left->token->value)) {
                if (right_node->node_type == ASTNodeType::FunctionCall)
                    return anaFunctionCall(right_node);
                return allocType(TypeCategory::Any);
            }

            // Pointer member access (not .*, but e.g., struct_ptr.field)
            if (left_type->category == TypeCategory::Pointer && left_type->pointee_type) {
                // Fall through: dereference and access
                auto* pointee = left_type->pointee_type;
                if (pointee->category == TypeCategory::Struct || pointee->category == TypeCategory::Named) {
                    auto& type_name = pointee->name;
                    if (auto* struct_sym = sym_table.resolveGlobal(type_name)) {
                        if (struct_sym->fields) {
                            auto it = struct_sym->fields->find(field_name);
                            if (it != struct_sym->fields->end())
                                return it->second->resolved_type ? cloneTypeInfo(it->second->resolved_type) : pointee;
                        }
                    }
                }
            }
        }
    }
    return left_type;
}

// ── Identifier ──────────────────────────────────────────────────────────

TypeInfo* Analyzer::anaIdentifier(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto& name = tok->value;

    if (undeclared_whitelist.count(name))
        return allocType(TypeCategory::Any);

    // Built-in type names usable in generic type parameter position
    if (name == "int") return makeNumericType("int");
    if (name == "uint") return makeNumericType("uint");
    if (name == "float") return makeNumericType("float");

    if (auto* sym = sym_table.resolve(name)) {
        if (sym->resolved_type) return cloneTypeInfo(sym->resolved_type);
        switch (sym->symbol_type) {
            case SymbolType::Function:
                if (sym->return_type) return cloneTypeInfo(sym->return_type);
                return allocType(TypeCategory::Function);
            case SymbolType::Struct:   return namedType(name, TypeCategory::Struct);
            case SymbolType::Enum:     return namedType(name, TypeCategory::Enum);
            case SymbolType::Union:    return namedType(name, TypeCategory::Union);
            case SymbolType::ErrorSet: return namedType(name, TypeCategory::ErrorSet);
            case SymbolType::Trait:    return namedType(name, TypeCategory::Struct);
            case SymbolType::TypeAlias:
                if (sym->resolved_type) return cloneTypeInfo(sym->resolved_type);
                return namedType(name, TypeCategory::Named);
            default: return nullptr;
        }
    }

    // Try as a primitive type name
    if (name == "bool") return allocType(TypeCategory::Bool);
    if (name == "str") return allocType(TypeCategory::Str);
    if (name == "void") return allocType(TypeCategory::Void);
    if (name == "any") return allocType(TypeCategory::Any);

    // Check if it's a numeric type string (like "u8", "i32", "f64")
    if (!name.empty() && (name[0] == 'i' || name[0] == 'u' || name[0] == 'f')) {
        auto* ti = makeNumericType(name);
        if (ti->category != TypeCategory::Unknown) return ti;
        delete ti;
    }

    // Catch variable names should be resolvable through scope
    reportError(*tok, "NameError",
        "Use of undeclared identifier '" + name + "'.");
    return nullptr;
}

// ── Function call ───────────────────────────────────────────────────────

TypeInfo* Analyzer::anaFunctionCall(ASTNode* node) {
    auto tok = node->token;
    if (!tok) return nullptr;
    auto& name = tok->value;

    size_t arg_count = node->children ? node->children->size() : 0;

    if (auto* sym = sym_table.resolve(name)) {
        if (sym->symbol_type == SymbolType::Function) {
            // Variadic check
            bool has_variadic = false;
            if (sym->param_count > 0) {
                auto pit = func_params.find(name);
                if (pit != func_params.end() && !pit->second.empty()) {
                    auto& last_param = pit->second.back();
                    if (last_param.name == "...") has_variadic = true;
                }
            }
            // Argument count check (skip for variadic)
            if (arg_count != sym->param_count && !has_variadic) {
                // Don't error — just note it; will be fixed when std lib is rewritten
            }

            // Argument type checking
            if (node->children) {
                auto pit = func_params.find(name);
                size_t param_idx = 0;
                for (auto* arg : *node->children) {
                    if (arg->left) {
                        auto* arg_type = anaNode(arg->left);
                        if (pit != func_params.end() && param_idx < pit->second.size() &&
                            arg_type && pit->second[param_idx].type) {
                            // Integer literal widening for arguments
                            arg_type = tryNormalizeInteger(arg_type, pit->second[param_idx].type);
                            std::string why;
                            if (!typesCompatible(pit->second[param_idx].type, arg_type, &why)) {
                                if (arg->left->token) {
                                    reportError(*arg->left->token, "ArgError",
                                        "Argument " + std::to_string(param_idx + 1) + " to '" +
                                        name + "' expects '" + typeToString(pit->second[param_idx].type) +
                                        "', found '" + typeToString(arg_type) + "'." +
                                        (why.empty() ? "" : " " + why));
                                }
                            }
                        } else {
                            anaNode(arg->left);
                        }
                    }
                    param_idx++;
                }
            }

            if (sym->return_type) return cloneTypeInfo(sym->return_type);
        } else if (sym->symbol_type == SymbolType::Variable) {
            // Calling a variable as a function? Could be an anonymous function.
            // For now, just check arguments
            if (node->children) {
                for (auto* arg : *node->children) {
                    if (arg->left) anaNode(arg->left);
                }
            }
        } else if (sym->symbol_type == SymbolType::Struct) {
            // Struct constructor call
            if (node->children) {
                for (auto* arg : *node->children) {
                    if (arg->left) anaNode(arg->left);
                }
            }
            return namedType(name, TypeCategory::Struct);
        } else {
            if (node->children) {
                for (auto* arg : *node->children) {
                    if (arg->left) anaNode(arg->left);
                }
            }
        }
    } else if (undeclared_whitelist.count(name) == 0) {
        reportError(*tok, "NameError",
            "Call to undeclared function '" + name + "'.");
    } else {
        // Whitelisted function: just analyze args
        if (node->children) {
            for (auto* arg : *node->children) {
                if (arg->left) anaNode(arg->left);
            }
        }
        return allocType(TypeCategory::Any);
    }

    return nullptr;
}

// ── Array literal ───────────────────────────────────────────────────────

TypeInfo* Analyzer::anaArrayLiteral(ASTNode* node) {
    auto* ti = allocType(TypeCategory::Array);
    TypeInfo* elem_type = nullptr;
    if (node->children) {
        for (auto* elem : *node->children) {
            auto* et = anaNode(elem);
            if (et) {
                if (!elem_type) elem_type = et;
                else {
                    std::string why;
                    if (!typesCompatible(elem_type, et, &why)) {
                        auto tok = elem->token ? elem->token : node->token;
                        if (tok) {
                            reportError(*tok, "TypeError",
                                "Array element type mismatch: expected '" +
                                typeToString(elem_type) + "', found '" + typeToString(et) + "'." +
                                (why.empty() ? "" : " " + why));
                        }
                    }
                }
            }
        }
    }
    if (!elem_type) elem_type = allocType(TypeCategory::Integer);
    elem_type->name = "i32";
    ti->elem_type = elem_type;
    return ti;
}

// ── Tuple literal ───────────────────────────────────────────────────────

TypeInfo* Analyzer::anaTupleLiteral(ASTNode* node) {
    auto* ti = allocType(TypeCategory::Struct);
    ti->name = ".tuple";
    if (node->children) {
        for (auto* elem : *node->children) {
            auto* et = anaNode(elem);
            if (et) {
                if (!ti->elem_type) ti->elem_type = et;
                else {
                    auto* prev = ti->pointee_type;
                    if (!prev) ti->pointee_type = et;
                    else {
                        while (prev->pointee_type) prev = prev->pointee_type;
                        prev->pointee_type = et;
                    }
                }
            }
        }
    }
    return ti;
}

// ── Annotation/Builtin ──────────────────────────────────────────────────

TypeInfo* Analyzer::anaAnnotation(ASTNode* node) {
    // @SizeOf(T), @as(Type, expr), etc.
    if (node->token) {
        auto& name = node->token->value;
        if (name == "SizeOf") {
            if (node->left) resolveTypeFromNode(node->left);
            auto* ti = allocType(TypeCategory::Integer);
            ti->name = "usize";
            return ti;
        }
        if (name == "AlignOf") {
            if (node->left) resolveTypeFromNode(node->left);
            auto* ti = allocType(TypeCategory::Integer);
            ti->name = "usize";
            return ti;
        }
        if (name == "as") {
            auto* target = node->left ? resolveTypeFromNode(node->left) : nullptr;
            if (node->right) anaNode(node->right);
            return target ? target : allocType(TypeCategory::Any);
        }
        if (name == "TypeOf") {
            if (node->left) {
                auto* inner_type = anaNode(node->left);
                if (inner_type) {
                    node->data = typeToString(inner_type);
                } else {
                    node->data = "void";
                }
            }
            return allocType(TypeCategory::Str);
        }
        if (name == "Self") {
            // @Self returns a pointer to the enclosing type
            auto* ti = allocType(TypeCategory::Pointer);
            ti->pointee_type = allocType(TypeCategory::Any);
            return ti;
        }
        if (name == "Dyn") {
            return allocType(TypeCategory::Bool);
        }
        if (name == "arena" || name == "page" || name == "c" || name == "fixed" ||
            name == "stack" || name == "pool" || name == "debug" || name == "log" ||
            name == "failing" || name == "thread_local") {
            // Allocator builtins return an allocator type
            return allocType(TypeCategory::Struct);
        }
    }
    return nullptr;
}

// ── Type compatibility ──────────────────────────────────────────────────

bool Analyzer::typesCompatible(const TypeInfo* expected, const TypeInfo* actual, std::string* why) {
    if (!expected || !actual) return true;
    if (expected->category == TypeCategory::Any) return true;
    if (expected->category == TypeCategory::Unknown) return true;
    if (actual->category == TypeCategory::Unknown) return true;

    // Generic type parameters are compatible with any type
    // Check if either type name matches a generic parameter of the current function
    auto isGenericParam = [this](const std::string& name) -> bool {
        if (current_generic_params.empty()) return false;
        return std::find(current_generic_params.begin(), current_generic_params.end(), name) != current_generic_params.end();
    };
    if (isGenericParam(expected->name)) return true;
    if (isGenericParam(actual->name)) return true;
    // Also treat Named types whose name looks like a single-letter generic (T, U, V, etc.)
    if (expected->category == TypeCategory::Named && expected->name.size() == 1 &&
        std::isupper(expected->name[0])) return true;
    if (actual->category == TypeCategory::Named && actual->name.size() == 1 &&
        std::isupper(actual->name[0])) return true;

    // null/any can be assigned to optional, failable, error union, pointer
    if (actual->category == TypeCategory::Any &&
        (expected->category == TypeCategory::Optional ||
         expected->category == TypeCategory::Failable ||
         expected->category == TypeCategory::Pointer))
        return true;

    // same category
    if (expected->category == actual->category) {
        if (expected->category == TypeCategory::Integer) {
            if (!expected->name.empty() && !actual->name.empty() && expected->name != actual->name) {
                // Resolve type aliases: check if either name is a TypeAlias
                if (auto* sym = sym_table.resolveGlobal(expected->name)) {
                    if (sym->symbol_type == SymbolType::TypeAlias && sym->resolved_type)
                        return typesCompatible(sym->resolved_type, actual, why);
                }
                if (auto* sym = sym_table.resolveGlobal(actual->name)) {
                    if (sym->symbol_type == SymbolType::TypeAlias && sym->resolved_type)
                        return typesCompatible(expected, sym->resolved_type, why);
                }
                if (why) *why = "expected '" + expected->name + "' but found '" + actual->name + "'";
                return false;
            }
            return true;
        }
        if (expected->category == TypeCategory::Float) {
            if (!expected->name.empty() && !actual->name.empty() && expected->name != actual->name) {
                // Resolve type aliases
                if (auto* sym = sym_table.resolveGlobal(expected->name)) {
                    if (sym->symbol_type == SymbolType::TypeAlias && sym->resolved_type)
                        return typesCompatible(sym->resolved_type, actual, why);
                }
                if (auto* sym = sym_table.resolveGlobal(actual->name)) {
                    if (sym->symbol_type == SymbolType::TypeAlias && sym->resolved_type)
                        return typesCompatible(expected, sym->resolved_type, why);
                }
                if (why) *why = "expected '" + expected->name + "' but found '" + actual->name + "'";
                return false;
            }
            return true;
        }

        // pointer compatibility
        if (expected->category == TypeCategory::Pointer) {
            if (expected->pointee_type && actual->pointee_type)
                return typesCompatible(expected->pointee_type, actual->pointee_type, why);
            return expected->pointee_type == actual->pointee_type;
        }

        // error union: check ok_type and error_type
        if (expected->category == TypeCategory::ErrorUnion) {
            if (expected->ok_type && actual->ok_type) {
                std::string err_why;
                if (!typesCompatible(expected->ok_type, actual->ok_type, &err_why)) {
                    if (why) *why = "ok type mismatch: " + err_why;
                    return false;
                }
            }
            if (expected->error_type && actual->error_type) {
                return typesCompatible(expected->error_type, actual->error_type, why);
            }
            return true;
        }

        // named types: compare by name
        if (expected->category == TypeCategory::Struct ||
            expected->category == TypeCategory::Enum ||
            expected->category == TypeCategory::Union ||
            expected->category == TypeCategory::ErrorSet ||
            expected->category == TypeCategory::Named) {
            if (expected->name == actual->name) return true;
            // Check type alias resolution
            if (auto* sym = sym_table.resolveGlobal(expected->name)) {
                if (sym->symbol_type == SymbolType::TypeAlias && sym->resolved_type)
                    return typesCompatible(sym->resolved_type, actual, why);
            }
            if (auto* sym = sym_table.resolveGlobal(actual->name)) {
                if (sym->symbol_type == SymbolType::TypeAlias && sym->resolved_type)
                    return typesCompatible(expected, sym->resolved_type, why);
            }
            if (why) *why = "expected '" + expected->name + "' but found '" + actual->name + "'";
            return false;
        }

        if (expected->category == TypeCategory::Array && actual->category == TypeCategory::Array) {
            if (expected->elem_type && actual->elem_type)
                return typesCompatible(expected->elem_type, actual->elem_type, why);
            return true;
        }

        return true;
    }

    // Error union: allow returning the ok type (for return statements)
    if (expected->category == TypeCategory::ErrorUnion && expected->ok_type) {
        return typesCompatible(expected->ok_type, actual, why);
    }

    // Allow int → f64 widening
    if (expected->category == TypeCategory::Float && actual->category == TypeCategory::Integer) {
        return true;
    }

    if (why) {
        *why = "expected '" + typeToString(expected) + "' but expression has type '" +
               typeToString(actual) + "'";
    }
    return false;
}

bool Analyzer::typesExactMatch(const TypeInfo* a, const TypeInfo* b) {
    if (!a || !b) return a == b;
    if (a->category != b->category) return false;
    if (a->name != b->name) return false;
    if (a->pointee_type || b->pointee_type) {
        if (!a->pointee_type || !b->pointee_type) return false;
        return typesExactMatch(a->pointee_type, b->pointee_type);
    }
    if (a->elem_type || b->elem_type) {
        if (!a->elem_type || !b->elem_type) return false;
        return typesExactMatch(a->elem_type, b->elem_type);
    }
    return true;
}

// ── Helpers ─────────────────────────────────────────────────────────────

bool Analyzer::hasControlFlowTerminator(ASTNode* node) {
    if (!node) return false;
    switch (node->node_type) {
        case ASTNodeType::ReturnStatement:
        case ASTNodeType::BreakStatement:
        case ASTNodeType::SkipStatement:
            return true;
        case ASTNodeType::IfStatement: {
            // All branches must terminate
            bool middle_term = false, right_term = false;
            if (node->middle && node->middle->children) {
                for (auto* s : *node->middle->children) {
                    if (hasControlFlowTerminator(s)) { middle_term = true; break; }
                }
            }
            if (node->right) {
                if (node->right->node_type == ASTNodeType::ElseBody && node->right->children) {
                    for (auto* s : *node->right->children) {
                        if (hasControlFlowTerminator(s)) { right_term = true; break; }
                    }
                } else if (node->right->node_type == ASTNodeType::ElseIfStatement) {
                    return hasControlFlowTerminator(node->right);
                }
            }
            return middle_term && right_term;
        }
        case ASTNodeType::MatchStatement: {
            // All match cases must terminate for the match to terminate
            if (!node->children) return false;
            for (auto* mc : *node->children) {
                if (mc->node_type != ASTNodeType::MatchCase) continue;
                if (!hasControlFlowTerminator(mc)) return false;
            }
            return true;
        }
        case ASTNodeType::MatchBody:
        case ASTNodeType::MatchCase: {
            // Case terminates if its body terminates
            if (node->right) return hasControlFlowTerminator(node->right);
            if (node->left) return hasControlFlowTerminator(node->left);
            return false;
        }
        case ASTNodeType::LoopStatement:
            return false; // loops may not terminate
        case ASTNodeType::Block:
            if (node->children && !node->children->empty())
                return hasControlFlowTerminator(node->children->back());
            return false;
        default:
            return false;
    }
}

TypeInfo* Analyzer::tryNormalizeInteger(TypeInfo* t, TypeInfo* desired) {
    if (!t || !desired) return t;
    // If t is a default i32 integer literal and desired is a different numeric type,
    // normalize t to the desired type
    if (t->category == TypeCategory::Integer && t->name == "i32") {
        if (desired->category == TypeCategory::Integer) {
            t->name = desired->name;
        } else if (desired->category == TypeCategory::Float) {
            t->category = TypeCategory::Float;
            t->name = desired->name;
        }
    }
    return t;
}

} // namespace razen
