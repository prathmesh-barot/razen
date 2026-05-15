#pragma once
#include <vector>
#include "../ast/node.h"
#include "../semantic/analyzer.h"
#include "ir.h"

namespace razen {
namespace codegen {

struct Codegen {
    IRBuilder ir;
    std::string source_name;

    Codegen(const std::string& name = "main.rz") : source_name(name) {}

    void generate(const std::vector<ASTNode*>& ast_nodes);
    std::string getIR() { return ir.getIR(); }

private:
    void genNode(ASTNode* node);
    void genFuncDecl(ASTNode* node);
    void genVarDecl(ASTNode* node);
    void genBlock(ASTNode* node);
    void genReturn(ASTNode* node);
    std::string genExpr(ASTNode* node);
    std::string genLiteral(ASTNode* node);
    std::string genIdentifier(ASTNode* node);
    std::string genBinary(ASTNode* node);
    std::string genUnary(ASTNode* node);
    std::string genCall(ASTNode* node);
    std::string genMemberAccess(ASTNode* node);
};

} // namespace codegen
} // namespace razen
