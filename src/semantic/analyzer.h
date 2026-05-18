#pragma once
#include <unordered_set>
#include <string_view>
#include <vector>
#include "../ast/node.h"
#include "symbol_table.h"
#include "type_info.h"

namespace razen {

struct Analyzer {
    bool has_errors = false;
    TypeInfo* current_return_type = nullptr;
    std::string current_func_name;
    std::vector<std::string> current_generic_params;
    std::unordered_set<std::string> undeclared_whitelist;
    SymbolTable sym_table;

    // Track per-function parameter types for arg checking
    struct FuncParam { std::string name; TypeInfo* type; };
    std::unordered_map<std::string, std::vector<FuncParam>> func_params;

    Analyzer(Scope* /*gs*/, Scope* /*cs*/, std::unordered_set<std::string>&& whitelist, SymbolTable st)
        : undeclared_whitelist(std::move(whitelist)), sym_table(std::move(st)) {}

    void reportError(const Token& token, const std::string& category, const std::string& msg);
    void analyze(const std::vector<ASTNode*>& ast_nodes);

private:
    // ── Type system ──
    TypeInfo* resolveTypeFromNode(ASTNode* type_node);
    TypeInfo* makeNumericType(const std::string& name);
    TypeInfo* inferTypeFromLiteral(ASTNode* node);
    TypeInfo* allocType(TypeCategory cat);
    TypeInfo* namedType(const std::string& name, TypeCategory cat);

    // ── Pass 1: declarations ──
    void declareGlobal(ASTNode* node);

    // ── Pass 2: analysis ──
    TypeInfo* anaNode(ASTNode* node);
    TypeInfo* anaFuncDecl(ASTNode* node);
    TypeInfo* anaBlock(ASTNode* node);
    TypeInfo* anaBody(ASTNode* node);
    TypeInfo* anaVarDecl(ASTNode* node);
    TypeInfo* anaAssignment(ASTNode* node);
    TypeInfo* anaReturn(ASTNode* node);
    TypeInfo* anaIf(ASTNode* node);
    TypeInfo* anaLoop(ASTNode* node);
    TypeInfo* anaMatch(ASTNode* node);
    TypeInfo* anaBinary(ASTNode* node);
    TypeInfo* anaUnary(ASTNode* node);
    TypeInfo* anaMemberAccess(ASTNode* node);
    TypeInfo* anaIdentifier(ASTNode* node);
    TypeInfo* anaFunctionCall(ASTNode* node);
    TypeInfo* anaTryBlock(ASTNode* node);
    TypeInfo* anaTryExpression(ASTNode* node);
    TypeInfo* anaDefer(ASTNode* node);
    TypeInfo* anaArrayLiteral(ASTNode* node);
    TypeInfo* anaTupleLiteral(ASTNode* node);
    TypeInfo* anaAnnotation(ASTNode* node);
    TypeInfo* anaRange(ASTNode* node);
    TypeInfo* anaCatch(ASTNode* node);
    TypeInfo* anaMatchCasePattern(ASTNode* pattern, TypeInfo* match_type);
    TypeInfo* anaStructLiteral(ASTNode* node);

    // ── Type compatibility ──
    bool typesCompatible(const TypeInfo* expected, const TypeInfo* actual, std::string* why = nullptr);
    bool typesExactMatch(const TypeInfo* a, const TypeInfo* b);

    // ── Helpers ──
    std::string errPos(const Token& tok) const;
    bool hasControlFlowTerminator(ASTNode* node);
    TypeInfo* tryNormalizeInteger(TypeInfo* t, TypeInfo* desired);
};

} // namespace razen
