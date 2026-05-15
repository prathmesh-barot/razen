#pragma once
#include <unordered_set>
#include "../ast/node.h"
#include "symbol_table.h"
#include "type_info.h"

namespace razen {

struct Analyzer {
    bool has_errors = false;
    TypeInfo* current_return_type = nullptr;
    std::string current_func_name;
    std::unordered_set<std::string> undeclared_whitelist;
    SymbolTable sym_table;

    Analyzer(Scope* gs, Scope* cs, std::unordered_set<std::string>&& whitelist, SymbolTable st)
        : undeclared_whitelist(std::move(whitelist)), sym_table(std::move(st)) {}

    void reportError(const Token& token, const std::string& msg);
    void analyze(const std::vector<ASTNode*>& ast_nodes);

private:
    TypeInfo* resolveTypeFromNode(ASTNode* type_node);
    TypeInfo* inferTypeFromLiteral(ASTNode* node);
    TypeInfo* allocType(TypeCategory cat);
    TypeInfo* namedType(const std::string& name, TypeCategory cat);
    void declareGlobal(ASTNode* node);
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
    bool typesCompatible(const TypeInfo* expected, const TypeInfo* actual);
};

} // namespace razen
