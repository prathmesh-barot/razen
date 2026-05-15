#pragma once
#include <vector>
#include <unordered_map>
#include "../ast/node.h"
#include "../semantic/analyzer.h"
#include "ir.h"

namespace razen {
namespace codegen {

struct Codegen {
    IRBuilder ir;
    std::string source_name;

    // local variable map: name → alloca register
    std::unordered_map<std::string, std::string> locals;
    std::unordered_map<std::string, std::string> local_types;
    std::string current_ret_type;
    bool has_return_emitted = false;

    std::vector<ASTNode*> deferred_stmts;
    std::unordered_map<std::string, std::vector<std::string>> struct_types;

    // Enum data
    std::unordered_map<std::string, std::unordered_map<std::string, int>> enum_values;
    std::unordered_map<std::string, std::string> enum_backing_types;

    // Union data
    struct UnionFieldInfo {
        std::string name;
        int tag;
        std::string payload_type;
        int payload_size;
    };
    std::unordered_map<std::string, std::vector<UnionFieldInfo>> union_variants;
    std::unordered_map<std::string, int> union_payload_sizes;

    // Struct methods + field type info
    std::unordered_map<std::string, std::vector<ASTNode*>> struct_methods;
    std::string current_struct_name;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> struct_field_types;

    // Error handling
    std::unordered_map<std::string, std::unordered_map<std::string, int>> error_values;
    std::string current_catch_label;

    Codegen(const std::string& name = "main.rz") : source_name(name) {}

    void generate(const std::vector<ASTNode*>& ast_nodes);
    std::string getIR() { return ir.getIR(); }

private:
    void genNode(ASTNode* node);
    void genFuncDecl(ASTNode* node);
    void genVarDecl(ASTNode* node);
    void genBlock(ASTNode* node);
    void genReturn(ASTNode* node);
    void genIf(ASTNode* node);
    void genLoop(ASTNode* node);
    void genAssign(ASTNode* node);
    void genMatch(ASTNode* node);
    std::string genExpr(ASTNode* node);
    std::string genLiteral(ASTNode* node);
    std::string genIdentifier(ASTNode* node);
    std::string genBinary(ASTNode* node);
    std::string genUnary(ASTNode* node);
    std::string genCall(ASTNode* node);
    std::string genMemberAccess(ASTNode* node);
    std::string exprType(ASTNode* node);
    void emitDeferred();

    // Phase 17-20 additions
    void genEnumDecl(ASTNode* node);
    void genUnionDecl(ASTNode* node);
    void genErrorDecl(ASTNode* node);
    std::string genTryExpression(ASTNode* node);
    std::string genArrayLiteral(ASTNode* node);
    std::string genTupleLiteral(ASTNode* node);
    std::string genRangeLiteral(ASTNode* node);
    bool isErrorVariant(ASTNode* node, std::string* out_set, int* out_code);
    bool isEnumName(const std::string& name);
    bool isUnionName(const std::string& name);
    bool isErrorSetName(const std::string& name);
    std::string genEnumMemberAccess(ASTNode* node);
    std::string genUnionConstruct(ASTNode* node);
    std::string resolveType(ASTNode* type_node);
    int getLLVMTypeSize(const std::string& type_str);
};

} // namespace codegen
} // namespace razen
