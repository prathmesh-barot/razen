#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "../lexer/lexer.h"
#include "type_info.h"
#include "../ast/node.h"

namespace razen {

enum class SymbolType {
    Variable, Function, Struct, Enum, Union, Trait, ErrorSet, Module, TypeAlias
};

struct Symbol {
    std::string name;
    SymbolType symbol_type;
    TypeInfo* resolved_type = nullptr;
    bool is_mut = false;
    size_t param_count = 0;
    TypeInfo* return_type = nullptr;
    Token token;
    bool is_pub = false;
    std::unordered_map<std::string, Symbol*>* fields = nullptr;
    bool is_async = false;
    bool is_const = false;
    bool is_global = false;

    // Generic parameters (e.g. @Generic(T) stores {"T"})
    std::vector<std::string> generic_params;

    // Field order for structs/enums/unions
    std::vector<std::string> field_order;

    // Methods (FunctionDeclaration nodes) for structs/enums/unions
    std::vector<ASTNode*> methods;

    // Behaviour needs fields
    std::unordered_map<std::string, Symbol*> needs_fields;

    // Enum backing type
    TypeInfo* backing_type = nullptr;

    // Constant initializer expression
    ASTNode* const_init = nullptr;

    // Union variant struct fields
    bool variant_is_struct = false;

    ~Symbol() {
        delete fields;
        for (auto& p : needs_fields) delete p.second;
    }
};

struct Scope {
    std::unordered_map<std::string, Symbol*> symbols;
    Scope* parent = nullptr;
    bool is_loop = false;

    bool define(Symbol* sym) {
        if (symbols.count(sym->name)) return false;
        symbols[sym->name] = sym;
        return true;
    }

    Symbol* resolve(const std::string& name) const {
        auto it = symbols.find(name);
        if (it != symbols.end()) return it->second;
        if (parent) return parent->resolve(name);
        return nullptr;
    }

    Symbol* resolveLocal(const std::string& name) const {
        auto it = symbols.find(name);
        if (it != symbols.end()) return it->second;
        return nullptr;
    }

    bool isDefinedInCurrentScope(const std::string& name) const {
        return symbols.count(name) > 0;
    }

    bool inLoop() const {
        if (is_loop) return true;
        if (parent) return parent->inLoop();
        return false;
    }
};

struct SymbolTable {
    Scope* global_scope;
    Scope* current_scope;
    size_t loop_depth = 0;
    bool has_skip_in_loop = false;

    SymbolTable(Scope* gs, Scope* cs) : global_scope(gs), current_scope(cs) {}

    void pushScope(Scope* new_scope) {
        new_scope->parent = current_scope;
        current_scope = new_scope;
    }

    void pushLoopScope(Scope* new_scope) {
        new_scope->parent = current_scope;
        new_scope->is_loop = true;
        current_scope = new_scope;
        loop_depth++;
    }

    void popScope() {
        if (current_scope->parent) {
            if (current_scope->is_loop) loop_depth--;
            current_scope = current_scope->parent;
        }
    }

    bool define(Symbol* sym) { return current_scope->define(sym); }
    bool defineGlobal(Symbol* sym) { return global_scope->define(sym); }
    Symbol* resolve(const std::string& name) const { return current_scope->resolve(name); }
    Symbol* resolveGlobal(const std::string& name) const { return global_scope->resolve(name); }
    bool isInLoop() const { return loop_depth > 0; }
};

} // namespace razen
