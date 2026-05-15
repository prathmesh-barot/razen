#include "codegen.h"
#include "../ast/ast_utils.h"
#include "../ast/token_utils.h"
#include <iostream>

namespace razen {
namespace codegen {

void Codegen::generate(const std::vector<ASTNode*>& ast_nodes) {
    ir.emitPreamble(source_name);
    ir.emitLibcDecls();

    for (auto* node : ast_nodes) {
        genNode(node);
    }
}

void Codegen::genNode(ASTNode* node) {
    if (!node) return;
    switch (node->node_type) {
        case ASTNodeType::FunctionDeclaration:
            genFuncDecl(node);
            break;
        case ASTNodeType::ExtDeclaration:
            ir.emitLine("; extern function: " + (node->token ? node->token->value : "<anon>"));
            break;
        case ASTNodeType::VarDeclaration:
        case ASTNodeType::ConstDeclaration:
            genVarDecl(node);
            break;
        case ASTNodeType::Block:
            genBlock(node);
            break;
        case ASTNodeType::ReturnStatement:
            genReturn(node);
            break;
        case ASTNodeType::BinaryExpression:
            genBinary(node);
            break;
        case ASTNodeType::UnaryExpression:
            genUnary(node);
            break;
        case ASTNodeType::Identifier:
            genIdentifier(node);
            break;
        case ASTNodeType::FunctionCall:
            genCall(node);
            break;
        case ASTNodeType::MemberAccess:
            genMemberAccess(node);
            break;
        case ASTNodeType::IntegerLiteral:
        case ASTNodeType::FloatLiteral:
        case ASTNodeType::BoolLiteral:
        case ASTNodeType::CharLiteral:
        case ASTNodeType::StringLiteral:
            genLiteral(node);
            break;
        case ASTNodeType::IfStatement:
        case ASTNodeType::ElseIfStatement:
            ir.emitLine("; if statement");
            if (node->left) genNode(node->left);
            if (node->middle) genNode(node->middle);
            if (node->right) genNode(node->right);
            break;
        case ASTNodeType::LoopStatement:
            ir.emitLine("; loop statement");
            if (node->left) genNode(node->left);
            if (node->right) genNode(node->right);
            break;
        case ASTNodeType::Assignment:
            ir.emitLine("; assignment");
            if (node->left) genNode(node->left);
            if (node->right) genExpr(node->right);
            break;
        case ASTNodeType::BreakStatement:
            ir.emitLine("; break");
            break;
        case ASTNodeType::SkipStatement:
            ir.emitLine("; skip");
            break;
        case ASTNodeType::IfBody:
        case ASTNodeType::ElseBody:
        case ASTNodeType::LoopBody:
        case ASTNodeType::MatchBody:
            if (node->left && node->left->node_type == ASTNodeType::Block)
                genBlock(node->left);
            else if (node->children)
                for (auto* child : *node->children) genNode(child);
            break;
        case ASTNodeType::MatchStatement:
            ir.emitLine("; match statement");
            if (node->children)
                for (auto* child : *node->children) genNode(child);
            break;
        case ASTNodeType::MatchCase:
            ir.emitLine("; match case");
            if (node->right) genNode(node->right);
            break;
        case ASTNodeType::DeferStatement:
            ir.emitLine("; defer");
            if (node->left) genNode(node->left);
            break;
        case ASTNodeType::TryBlock:
        case ASTNodeType::TryExpression:
            ir.emitLine("; try");
            if (node->left) genNode(node->left);
            if (node->right) genNode(node->right);
            break;
        case ASTNodeType::CatchExpression:
            ir.emitLine("; catch");
            if (node->left) genNode(node->left);
            break;
        case ASTNodeType::ArrayLiteral:
        case ASTNodeType::TupleLiteral:
            ir.emitLine("; array/tuple literal");
            if (node->children)
                for (auto* elem : *node->children) genNode(elem);
            break;
        case ASTNodeType::RangeExpression:
            ir.emitLine("; range expression");
            break;
        case ASTNodeType::StructDeclaration:
            ir.emitLine("; struct: " + (node->token ? node->token->value : "<anon>"));
            break;
        case ASTNodeType::EnumDeclaration:
            ir.emitLine("; enum: " + (node->token ? node->token->value : "<anon>"));
            break;
        case ASTNodeType::UnionDeclaration:
            ir.emitLine("; union: " + (node->token ? node->token->value : "<anon>"));
            break;
        case ASTNodeType::ErrorDeclaration:
            ir.emitLine("; error set: " + (node->token ? node->token->value : "<anon>"));
            break;
        case ASTNodeType::ModuleDeclaration:
            ir.emitLine("; module: " + (node->token ? node->token->value : "<anon>"));
            break;
        case ASTNodeType::UseDeclaration:
            ir.emitLine("; use: " + (node->token ? node->token->value : "<anon>"));
            break;
        case ASTNodeType::TypeAliasDeclaration:
            ir.emitLine("; type alias: " + (node->token ? node->token->value : "<anon>"));
            break;
        case ASTNodeType::BehaveDeclaration:
            ir.emitLine("; behave: " + (node->token ? node->token->value : "<anon>"));
            break;
        case ASTNodeType::BuiltinExpression:
            ir.emitLine("; builtin expression");
            break;
        default:
            break;
    }
}

void Codegen::genFuncDecl(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    ir.emitLine("; function: " + name);

    std::string ret_type = "void";
    if (node->left && node->left->node_type == ASTNodeType::ReturnType && node->left->left)
        ret_type = typeToLLVM(node->left->left);

    std::string sig = "define " + ret_type + " @" + name + "(";

    if (node->middle && node->middle->children) {
        for (size_t i = 0; i < node->middle->children->size(); i++) {
            if (i > 0) sig += ", ";
            auto* param = (*node->middle->children)[i];
            if (param->node_type == ASTNodeType::Parameter) {
                std::string ptype = param->left ? typeToLLVM(param->left) : "i32";
                std::string pname = param->token ? param->token->value : "arg" + std::to_string(i);
                sig += ptype + " %" + pname;
            }
        }
    }
    sig += ") {";

    ir.emitLine(sig);
    ir.indent_str = "    ";

    if (node->right && node->right->node_type == ASTNodeType::Block)
        genBlock(node->right);

    ir.indent_str = "";
    ir.emitLine("}");
    ir.emitLine("");
}

void Codegen::genVarDecl(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;
    std::string type_str = node->left ? typeToLLVM(node->left) : "i32";
    ir.emitLine("; var: " + name + " : " + type_str);
    if (node->right) {
        auto val = genExpr(node->right);
        ir.emitLine(";   = " + val);
    }
}

void Codegen::genBlock(ASTNode* node) {
    if (!node) return;
    ir.emitLine("; block {");
    if (node->children) {
        for (auto* child : *node->children) {
            genNode(child);
        }
    }
    ir.emitLine("; }");
}

void Codegen::genReturn(ASTNode* node) {
    if (!node) return;
    if (node->left) {
        auto val = genExpr(node->left);
        ir.emitLine("ret " + val);
    } else {
        ir.emitLine("ret void");
    }
}

std::string Codegen::genExpr(ASTNode* node) {
    if (!node) return "void undef";
    switch (node->node_type) {
        case ASTNodeType::IntegerLiteral:
        case ASTNodeType::FloatLiteral:
        case ASTNodeType::BoolLiteral:
        case ASTNodeType::CharLiteral:
        case ASTNodeType::StringLiteral:
            return genLiteral(node);
        case ASTNodeType::Identifier:
            return genIdentifier(node);
        case ASTNodeType::BinaryExpression:
            return genBinary(node);
        case ASTNodeType::UnaryExpression:
            return genUnary(node);
        case ASTNodeType::FunctionCall:
            return genCall(node);
        case ASTNodeType::MemberAccess:
            return genMemberAccess(node);
        default:
            return "i32 0";
    }
}

std::string Codegen::genLiteral(ASTNode* node) {
    if (!node || !node->token) return "i32 0";
    auto& val = node->token->value;
    switch (node->node_type) {
        case ASTNodeType::IntegerLiteral:
            return "i32 " + val;
        case ASTNodeType::FloatLiteral:
            return "double " + val;
        case ASTNodeType::BoolLiteral:
            return std::string("i1 ") + (val == "true" ? "1" : "0");
        case ASTNodeType::CharLiteral:
            return std::string("i8 ") + std::to_string(val.empty() ? 0 : (unsigned char)val[0]);
        case ASTNodeType::StringLiteral: {
            auto gv = ir.strName();
            ir.emitLine(gv + " = private unnamed_addr constant [" +
                        std::to_string(val.size() + 1) + " x i8] c\"" + val + "\\00\"");
            return "i8* getelementptr inbounds ([" + std::to_string(val.size() + 1) +
                   " x i8], [" + std::to_string(val.size() + 1) + " x i8]* " + gv +
                   ", i32 0, i32 0)";
        }
        default:
            return "i32 0";
    }
}

std::string Codegen::genIdentifier(ASTNode* node) {
    if (!node || !node->token) return "i32 undef";
    auto& name = node->token->value;
    return "i32 %" + name;
}

std::string Codegen::genBinary(ASTNode* node) {
    if (!node) return "i32 0";
    auto lhs = node->left ? genExpr(node->left) : "i32 0";
    auto rhs = node->right ? genExpr(node->right) : "i32 0";
    std::string op = node->token ? node->token->value : "?";
    auto t = ir.tmp();
    ir.emitLine("; binary: " + lhs + " " + op + " " + rhs);
    ir.emitLine(t + " = add i32 0, 0  ; placeholder for " + op);
    return t;
}

std::string Codegen::genUnary(ASTNode* node) {
    if (!node) return "i32 0";
    auto inner = node->left ? genExpr(node->left) : "i32 0";
    std::string op = node->token ? node->token->value : "?";
    auto t = ir.tmp();
    ir.emitLine("; unary: " + op + " " + inner);
    ir.emitLine(t + " = add i32 0, 0  ; placeholder for unary " + op);
    return t;
}

std::string Codegen::genCall(ASTNode* node) {
    if (!node || !node->token) return "i32 0";
    auto& name = node->token->value;
    ir.emitLine("; call: " + name);
    auto t = ir.tmp();
    ir.emitLine(t + " = call i32 @" + name + "()  ; placeholder call");
    return t;
}

std::string Codegen::genMemberAccess(ASTNode* node) {
    if (!node) return "i32 0";
    ir.emitLine("; member access");
    if (node->left) genExpr(node->left);
    return ir.tmp();
}

} // namespace codegen
} // namespace razen
