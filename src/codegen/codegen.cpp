#include "codegen.h"
#include "../ast/ast_utils.h"
#include "../ast/token_utils.h"
#include <iostream>
#include <cstring>

namespace razen {
namespace codegen {

// ── Phase 6: Global dispatch ────────────────────────────────────────────────

void Codegen::generate(const std::vector<ASTNode*>& ast_nodes) {
    ir.emitPreamble(source_name);
    ir.emitLibcDecls();

    for (auto* node : ast_nodes) {
        genNode(node);
    }
}

void Codegen::genNode(ASTNode* node) {
    if (!node) return;
    // skip comments and annotations in codegen
    if (node->node_type == ASTNodeType::Comment ||
        node->node_type == ASTNodeType::Annotation ||
        node->node_type == ASTNodeType::GenericParams)
        return;

    switch (node->node_type) {
        case ASTNodeType::FunctionDeclaration:
            genFuncDecl(node);
            break;
        case ASTNodeType::ExtDeclaration:
            ir.emitLine("declare " + (node->left ? typeToLLVM(node->left->left) : "i32") +
                        " @" + (node->token ? node->token->value : "extern") + "(" +
                        (node->middle && node->middle->children ? "..." : "i32") + ")");
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
        case ASTNodeType::IfStatement:
        case ASTNodeType::ElseIfStatement:
            genIf(node);
            break;
        case ASTNodeType::LoopStatement:
            genLoop(node);
            break;
        case ASTNodeType::Assignment:
            genAssign(node);
            break;
        case ASTNodeType::MatchStatement:
            genMatch(node);
            break;
        case ASTNodeType::BreakStatement:
            ir.emitLine("br label %" + ir.label(".loop.end"));
            break;
        case ASTNodeType::SkipStatement:
            ir.emitLine("br label %" + ir.label(".loop.continue"));
            break;
        case ASTNodeType::DeferStatement:
            if (node->left) deferred_stmts.push_back(node->left);
            break;
        case ASTNodeType::TryBlock: {
            ir.emitLine("; try {");
            if (node->left) genNode(node->left);
            ir.emitLine("; }");
            if (node->right && node->right->node_type == ASTNodeType::CatchExpression) {
                ir.emitLine("; catch {");
                if (node->right->left) genNode(node->right->left);
                ir.emitLine("; }");
            }
            break;
        }
        case ASTNodeType::TryExpression:
            if (node->left) genNode(node->left);
            if (node->right) genNode(node->right);
            break;
        case ASTNodeType::CatchExpression:
            if (node->left) genNode(node->left);
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
        case ASTNodeType::MatchCase:
            if (node->right) genNode(node->right);
            break;
        case ASTNodeType::ArrayLiteral:
        case ASTNodeType::TupleLiteral:
        case ASTNodeType::RangeExpression:
        case ASTNodeType::BuiltinExpression:
            break;
        case ASTNodeType::StructDeclaration: {
            if (!node->token) break;
            auto& sname = node->token->value;
            std::vector<std::string> field_names;
            std::string struct_def = "%" + sname + " = type { ";
            bool first_field = true;
            if (node->children) {
                for (auto* child : *node->children) {
                    if (child->node_type == ASTNodeType::StructField) {
                        if (child->token) field_names.push_back(child->token->value);
                        if (!first_field) struct_def += ", ";
                        struct_def += child->left ? typeToLLVM(child->left) : "i32";
                        first_field = false;
                    }
                }
            }
            struct_def += " }";
            struct_types[sname] = field_names;
            ir.emitLine(struct_def);
            break;
        }
        case ASTNodeType::EnumDeclaration:
            if (node->token)
                ir.emitLine("; enum " + node->token->value + " (backing: i32)");
            break;
        case ASTNodeType::UnionDeclaration:
            if (node->token)
                ir.emitLine("; union " + node->token->value + " = { i32 tag, [N x i8] payload }");
            break;
        case ASTNodeType::ErrorDeclaration:
        case ASTNodeType::TypeAliasDeclaration:
        case ASTNodeType::ModuleDeclaration:
        case ASTNodeType::UseDeclaration:
        case ASTNodeType::BehaveDeclaration:
            break; // emitted as comments/stubs in later phases
        default:
            break;
    }
}

// ── Phase 8: Function codegen ───────────────────────────────────────────────

void Codegen::genFuncDecl(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    std::string ret_type = "void";
    if (node->left && node->left->node_type == ASTNodeType::ReturnType && node->left->left)
        ret_type = typeToLLVM(node->left->left);
    current_ret_type = ret_type;

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

    // store parameters to allocas
    if (node->middle && node->middle->children) {
        for (auto* param : *node->middle->children) {
            if (param->node_type == ASTNodeType::Parameter && param->token) {
                auto& pname = param->token->value;
                std::string ptype = param->left ? typeToLLVM(param->left) : "i32";
                locals[pname] = "%" + pname + ".addr";
                local_types[pname] = ptype;
                ir.emitLine("%" + pname + ".addr = alloca " + ptype);
                ir.emitLine("store " + ptype + " %" + pname + ", " + ptype + "* %" + pname + ".addr");
            }
        }
    }

    has_return_emitted = false;
    deferred_stmts.clear();
    if (node->right && node->right->node_type == ASTNodeType::Block)
        genBlock(node->right);

    // flush any remaining deferred statements
    emitDeferred();

    // ensure a terminator exists
    if (!has_return_emitted && ret_type != "void") {
        auto t = ir.tmp("%retval");
        ir.emitLine(t + " = alloca " + ret_type);
        ir.emitLine("ret " + ret_type + " 0");
    } else if (!has_return_emitted) {
        ir.emitLine("ret void");
    }

    ir.indent_str = "";
    ir.emitLine("}");
    ir.emitLine("");

    locals.clear();
    local_types.clear();
    current_ret_type = "void";
    deferred_stmts.clear();
}

// ── Phase 7: Variable declarations ──────────────────────────────────────────

void Codegen::genVarDecl(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;
    std::string llvm_type = node->left ? typeToLLVM(node->left) : "i32";

    // register local variable
    std::string reg = "%" + name + ".addr";
    locals[name] = reg;
    local_types[name] = llvm_type;

    ir.emitLine(reg + " = alloca " + llvm_type);

    if (node->right) {
        auto val = genExpr(node->right);
        // extract type prefix from val (e.g. "i32 42" → type is "i32")
        std::string val_type = llvm_type;
        auto space = val.find(' ');
        if (space != std::string::npos) val_type = val.substr(0, space);
        ir.emitLine("store " + val + ", " + val_type + "* " + reg);
    }
}

// ── Block ───────────────────────────────────────────────────────────────────

void Codegen::genBlock(ASTNode* node) {
    if (!node) return;
    if (node->children) {
        for (auto* child : *node->children) {
            genNode(child);
        }
    }
}

// ── Return ──────────────────────────────────────────────────────────────────

void Codegen::genReturn(ASTNode* node) {
    if (!node) return;
    emitDeferred();
    has_return_emitted = true;
    if (node->left) {
        auto val = genExpr(node->left);
        auto space = val.find(' ');
        std::string ty = space != std::string::npos ? val.substr(0, space) : "i32";
        ir.emitLine("ret " + val);
    } else {
        ir.emitLine("ret void");
    }
}

// ── If/else ─────────────────────────────────────────────────────────────────

void Codegen::genIf(ASTNode* node) {
    if (!node) return;
    auto cond_reg = node->left ? genExpr(node->left) : "i1 0";
    auto label_then = ir.label(".if.then");
    auto label_else = ir.label(".if.else");
    auto label_end = ir.label(".if.end");

    ir.emitLine("br " + cond_reg + ", label %" + label_then + ", label %" + label_else);
    ir.emitLine("");
    ir.emitLine(label_then + ":");
    if (node->middle) genNode(node->middle); // if body
    if (!has_return_emitted) ir.emitLine("br label %" + label_end);
    ir.emitLine("");

    ir.emitLine(label_else + ":");
    if (node->right) {
        if (node->right->node_type == ASTNodeType::ElseIfStatement)
            genIf(node->right); // else if chain
        else
            genNode(node->right); // else body
    }
    if (!has_return_emitted) ir.emitLine("br label %" + label_end);
    ir.emitLine("");

    ir.emitLine(label_end + ":");
}

// ── Loop ────────────────────────────────────────────────────────────────────

void Codegen::genLoop(ASTNode* node) {
    auto label_cond = ir.label(".loop.cond");
    auto label_body = ir.label(".loop.body");
    auto label_end = ir.label(".loop.end");
    auto label_cont = ir.label(".loop.continue");

    ir.emitLine("br label %" + label_cond);
    ir.emitLine("");
    ir.emitLine(label_cond + ":");
    if (node->left) {
        auto cond = genExpr(node->left);
        ir.emitLine("br " + cond + ", label %" + label_body + ", label %" + label_end);
    } else {
        ir.emitLine("br label %" + label_body);
    }
    ir.emitLine("");
    ir.emitLine(label_body + ":");
    if (node->right) genNode(node->right);
    ir.emitLine("br label %" + label_cont);
    ir.emitLine("");
    ir.emitLine(label_cont + ":");
    ir.emitLine("br label %" + label_cond);
    ir.emitLine("");
    ir.emitLine(label_end + ":");
}

// ── Assignment ───────────────────────────────────────────────────────────────

void Codegen::genAssign(ASTNode* node) {
    if (!node || !node->left || !node->token) return;
    auto* lhs = node->left;
    if (lhs->node_type == ASTNodeType::Identifier && lhs->token) {
        auto& name = lhs->token->value;
        auto it = locals.find(name);
        if (it != locals.end()) {
            auto tok_type = node->token->type;
            std::string val_type = local_types[name];

            if (tok_type == TokenType::PlusEquals ||
                tok_type == TokenType::MinusEquals ||
                tok_type == TokenType::StarEquals ||
                tok_type == TokenType::SlashEquals ||
                tok_type == TokenType::PercentEquals)
            {
                // load current value
                auto load = ir.tmp("%load");
                ir.emitLine(load + " = load " + val_type + ", " + val_type + "* " + it->second);
                // evaluate right-hand side
                auto rhs = node->right ? genExpr(node->right) : val_type + " 0";
                std::string rt, rr;
                auto sp = rhs.find(' ');
                if (sp != std::string::npos) { rt = rhs.substr(0, sp); rr = rhs.substr(sp + 1); }
                else { rt = val_type; rr = rhs; }
                // perform operation
                auto bin = ir.tmp("%bin");
                switch (tok_type) {
                    case TokenType::PlusEquals:
                        if (val_type == "double" || val_type == "float" || val_type == "fp128" || val_type == "half")
                            ir.emitLine(bin + " = fadd " + val_type + " " + load + ", " + rr);
                        else
                            ir.emitLine(bin + " = add " + val_type + " " + load + ", " + rr);
                        break;
                    case TokenType::MinusEquals:
                        if (val_type == "double" || val_type == "float" || val_type == "fp128" || val_type == "half")
                            ir.emitLine(bin + " = fsub " + val_type + " " + load + ", " + rr);
                        else
                            ir.emitLine(bin + " = sub " + val_type + " " + load + ", " + rr);
                        break;
                    case TokenType::StarEquals:
                        if (val_type == "double" || val_type == "float" || val_type == "fp128" || val_type == "half")
                            ir.emitLine(bin + " = fmul " + val_type + " " + load + ", " + rr);
                        else
                            ir.emitLine(bin + " = mul " + val_type + " " + load + ", " + rr);
                        break;
                    case TokenType::SlashEquals:
                        if (val_type == "double" || val_type == "float" || val_type == "fp128" || val_type == "half")
                            ir.emitLine(bin + " = fdiv " + val_type + " " + load + ", " + rr);
                        else
                            ir.emitLine(bin + " = sdiv " + val_type + " " + load + ", " + rr);
                        break;
                    case TokenType::PercentEquals:
                        ir.emitLine(bin + " = srem " + val_type + " " + load + ", " + rr);
                        break;
                    default: break;
                }
                ir.emitLine("store " + val_type + " " + bin + ", " + val_type + "* " + it->second);
            } else {
                auto val = node->right ? genExpr(node->right) : val_type + " 0";
                ir.emitLine("store " + val + ", " + val_type + "* " + it->second);
            }
        }
    }
}

// ── Match ───────────────────────────────────────────────────────────────────

void Codegen::genMatch(ASTNode* node) {
    if (!node) return;
    auto match_val = node->left ? genExpr(node->left) : "i32 0";
    auto label_end = ir.label(".match.end");

    auto extractReg = [](const std::string& s) -> std::string {
        auto sp = s.find(' ');
        if (sp != std::string::npos) return s.substr(sp + 1);
        return s;
    };

    std::string match_reg = extractReg(match_val);

    if (!node->children || node->children->empty()) {
        ir.emitLine("br label %" + label_end);
        ir.emitLine("");
        ir.emitLine(label_end + ":");
        return;
    }

    // Separate patterned cases from default case
    std::vector<ASTNode*> pattern_cases;
    ASTNode* else_case = nullptr;
    for (auto* case_node : *node->children) {
        if (case_node->left)
            pattern_cases.push_back(case_node);
        else
            else_case = case_node;
    }

    if (pattern_cases.empty()) {
        if (else_case && else_case->right) genNode(else_case->right);
        ir.emitLine("br label %" + label_end);
        ir.emitLine("");
        ir.emitLine(label_end + ":");
        return;
    }

    // Branch to first pattern check
    auto first_check = ir.label(".match.check");
    ir.emitLine("br label %" + first_check);
    ir.emitLine("");

    std::string current_label = first_check;

    for (size_t i = 0; i < pattern_cases.size(); i++) {
        auto* case_node = pattern_cases[i];

        ir.emitLine(current_label + ":");

        auto case_val = genExpr(case_node->left);
        std::string case_reg = extractReg(case_val);
        auto case_label = ir.label(".match.case");

        bool is_last = (i == pattern_cases.size() - 1);
        std::string fallthrough;
        if (is_last && else_case)
            fallthrough = ir.label(".match.else");
        else if (is_last)
            fallthrough = label_end;
        else
            fallthrough = ir.label(".match.next");

        auto ic = ir.tmp("%mcmp");
        ir.emitLine(ic + " = icmp eq i32 " + match_reg + ", " + case_reg);
        ir.emitLine("br i1 " + ic + ", label %" + case_label + ", label %" + fallthrough);
        ir.emitLine("");
        ir.emitLine(case_label + ":");
        if (case_node->right) genNode(case_node->right);
        ir.emitLine("br label %" + label_end);
        ir.emitLine("");

        current_label = fallthrough;
    }

    if (else_case) {
        ir.emitLine(current_label + ":");
        if (else_case->right) genNode(else_case->right);
        ir.emitLine("br label %" + label_end);
        ir.emitLine("");
    }

    ir.emitLine(label_end + ":");
}

// ── Expression dispatch ─────────────────────────────────────────────────────

std::string Codegen::genExpr(ASTNode* node) {
    if (!node) return "i32 0";
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

// ── Phase 9: Literal codegen ────────────────────────────────────────────────

std::string Codegen::genLiteral(ASTNode* node) {
    if (!node || !node->token) return "i32 0";
    auto& val = node->token->value;
    switch (node->node_type) {
        case ASTNodeType::IntegerLiteral:
            return "i32 " + val;
        case ASTNodeType::FloatLiteral:
            return "double " + val;
        case ASTNodeType::BoolLiteral:
            return "i1 " + std::string(val == "true" ? "1" : "0");
        case ASTNodeType::CharLiteral:
            return "i8 " + std::to_string(val.empty() ? 0 : (unsigned char)val[0]);
        case ASTNodeType::StringLiteral: {
            auto gv = ir.strName();
            // escape special chars for LLVM IR string constant
            std::string escaped;
            for (char c : val) {
                if (c == '\\') escaped += "\\5C";
                else if (c == '"') escaped += "\\22";
                else if (c == '\n') escaped += "\\0A";
                else if (c == '\t') escaped += "\\09";
                else if (c == '\r') escaped += "\\0D";
                else if (c >= 32 && c < 127) escaped += c;
                else { char buf[8]; std::snprintf(buf, 8, "\\%02X", (unsigned char)c); escaped += buf; }
            }
            ir.emitLine(gv + " = private unnamed_addr constant [" +
                        std::to_string(val.size() + 1) + " x i8] c\"" + escaped + "\\00\"");
            auto t = ir.tmp("%gep");
            ir.emitLine(t + " = getelementptr inbounds ([" + std::to_string(val.size() + 1) +
                        " x i8], [" + std::to_string(val.size() + 1) + " x i8]* " + gv +
                        ", i32 0, i32 0)");
            return "i8* " + t;
        }
        default:
            return "i32 0";
    }
}

// ── Phase 10: Identifier ────────────────────────────────────────────────────

std::string Codegen::genIdentifier(ASTNode* node) {
    if (!node || !node->token) return "i32 0";
    auto& name = node->token->value;
    auto it = locals.find(name);
    if (it != locals.end()) {
        std::string ty = local_types[name];
        auto t = ir.tmp("%load");
        ir.emitLine(t + " = load " + ty + ", " + ty + "* " + it->second);
        return ty + " " + t;
    }
    // use named value directly (parameter or global)
    std::string ty = "i32";
    return ty + " %" + name;
}

// ── Phase 10: Binary operations ─────────────────────────────────────────────

std::string Codegen::genBinary(ASTNode* node) {
    if (!node) return "i32 0";
    auto lhs = node->left ? genExpr(node->left) : "i32 0";
    auto rhs = node->right ? genExpr(node->right) : "i32 0";
    auto tok = node->token;
    if (!tok) return lhs;

    // extract type and register from operands
    auto parseVal = [](const std::string& s, std::string& ty, std::string& reg) {
        auto sp = s.find(' ');
        if (sp != std::string::npos) {
            ty = s.substr(0, sp);
            reg = s.substr(sp + 1);
        } else {
            ty = "i32";
            reg = s;
        }
    };

    std::string lt, lr, rt, rr;
    parseVal(lhs, lt, lr);
    parseVal(rhs, rt, rr);
    std::string res_type = lt; // result type follows left operand
    auto t = ir.tmp("%bin");

    switch (tok->type) {
        // Arithmetic
        case TokenType::Plus:
            if (res_type == "double" || res_type == "float" || res_type == "fp128" || res_type == "half")
                ir.emitLine(t + " = fadd " + res_type + " " + lr + ", " + rr);
            else
                ir.emitLine(t + " = add " + res_type + " " + lr + ", " + rr);
            return res_type + " " + t;
        case TokenType::Minus:
            if (res_type == "double" || res_type == "float" || res_type == "fp128" || res_type == "half")
                ir.emitLine(t + " = fsub " + res_type + " " + lr + ", " + rr);
            else
                ir.emitLine(t + " = sub " + res_type + " " + lr + ", " + rr);
            return res_type + " " + t;
        case TokenType::Star:
            if (res_type == "double" || res_type == "float" || res_type == "fp128" || res_type == "half")
                ir.emitLine(t + " = fmul " + res_type + " " + lr + ", " + rr);
            else
                ir.emitLine(t + " = mul " + res_type + " " + lr + ", " + rr);
            return res_type + " " + t;
        case TokenType::Slash:
            if (res_type == "double" || res_type == "float" || res_type == "fp128" || res_type == "half")
                ir.emitLine(t + " = fdiv " + res_type + " " + lr + ", " + rr);
            else
                ir.emitLine(t + " = sdiv " + res_type + " " + lr + ", " + rr);
            return res_type + " " + t;
        case TokenType::Percent:
            ir.emitLine(t + " = srem " + res_type + " " + lr + ", " + rr);
            return res_type + " " + t;

        // Comparison (icmp produces i1, need zext to i32)
        case TokenType::EqualsEquals: {
            auto ic = ir.tmp("%ic");
            ir.emitLine(ic + " = icmp eq " + res_type + " " + lr + ", " + rr);
            auto z = ir.tmp("%zext");
            ir.emitLine(z + " = zext i1 " + ic + " to i32");
            return "i32 " + z;
        }
        case TokenType::NotEquals: {
            auto ic = ir.tmp("%ic");
            ir.emitLine(ic + " = icmp ne " + res_type + " " + lr + ", " + rr);
            auto z = ir.tmp("%zext");
            ir.emitLine(z + " = zext i1 " + ic + " to i32");
            return "i32 " + z;
        }
        case TokenType::LessThan: {
            auto ic = ir.tmp("%ic");
            if (res_type == "double" || res_type == "float" || res_type == "fp128" || res_type == "half")
                ir.emitLine(ic + " = fcmp olt " + res_type + " " + lr + ", " + rr);
            else
                ir.emitLine(ic + " = icmp slt " + res_type + " " + lr + ", " + rr);
            auto z = ir.tmp("%zext");
            ir.emitLine(z + " = zext i1 " + ic + " to i32");
            return "i32 " + z;
        }
        case TokenType::LessThanEquals: {
            auto ic = ir.tmp("%ic");
            if (res_type == "double" || res_type == "float" || res_type == "fp128" || res_type == "half")
                ir.emitLine(ic + " = fcmp ole " + res_type + " " + lr + ", " + rr);
            else
                ir.emitLine(ic + " = icmp sle " + res_type + " " + lr + ", " + rr);
            auto z = ir.tmp("%zext");
            ir.emitLine(z + " = zext i1 " + ic + " to i32");
            return "i32 " + z;
        }
        case TokenType::GreaterThan: {
            auto ic = ir.tmp("%ic");
            if (res_type == "double" || res_type == "float" || res_type == "fp128" || res_type == "half")
                ir.emitLine(ic + " = fcmp ogt " + res_type + " " + lr + ", " + rr);
            else
                ir.emitLine(ic + " = icmp sgt " + res_type + " " + lr + ", " + rr);
            auto z = ir.tmp("%zext");
            ir.emitLine(z + " = zext i1 " + ic + " to i32");
            return "i32 " + z;
        }
        case TokenType::GreaterThanEquals: {
            auto ic = ir.tmp("%ic");
            if (res_type == "double" || res_type == "float" || res_type == "fp128" || res_type == "half")
                ir.emitLine(ic + " = fcmp oge " + res_type + " " + lr + ", " + rr);
            else
                ir.emitLine(ic + " = icmp sge " + res_type + " " + lr + ", " + rr);
            auto z = ir.tmp("%zext");
            ir.emitLine(z + " = zext i1 " + ic + " to i32");
            return "i32 " + z;
        }

        // Logical (i1)
        case TokenType::AndAnd:
            ir.emitLine(t + " = and i1 " + lr + ", " + rr);
            return "i1 " + t;
        case TokenType::OrOr:
            ir.emitLine(t + " = or i1 " + lr + ", " + rr);
            return "i1 " + t;

        // Bitwise
        case TokenType::And:
            ir.emitLine(t + " = and " + res_type + " " + lr + ", " + rr);
            return res_type + " " + t;
        case TokenType::Or:
            ir.emitLine(t + " = or " + res_type + " " + lr + ", " + rr);
            return res_type + " " + t;
        case TokenType::Caret:
            ir.emitLine(t + " = xor " + res_type + " " + lr + ", " + rr);
            return res_type + " " + t;
        case TokenType::ShiftLeft:
            ir.emitLine(t + " = shl " + res_type + " " + lr + ", " + rr);
            return res_type + " " + t;
        case TokenType::ShiftRight:
            ir.emitLine(t + " = ashr " + res_type + " " + lr + ", " + rr);
            return res_type + " " + t;

        default:
            return lhs;
    }
}

// ── Unary operations ────────────────────────────────────────────────────────

std::string Codegen::genUnary(ASTNode* node) {
    if (!node || !node->token) return "i32 0";
    std::string op = node->token->value;
    auto inner = node->left ? genExpr(node->left) : "i32 0";

    std::string ty, reg;
    auto sp = inner.find(' ');
    if (sp != std::string::npos) { ty = inner.substr(0, sp); reg = inner.substr(sp + 1); }
    else { ty = "i32"; reg = inner; }

    auto t = ir.tmp("%un");

    if (op == "-") {
        if (ty == "double" || ty == "float" || ty == "fp128" || ty == "half") {
            ir.emitLine(t + " = fneg " + ty + " " + reg);
        } else {
            auto zero = ir.tmp("%zero");
            ir.emitLine(zero + " = add " + ty + " 0, 0"); // or use a constant
            ir.emitLine(t + " = sub " + ty + " 0, " + reg);
        }
        return ty + " " + t;
    }
    if (op == "!") {
        auto one = ir.tmp("%one");
        ir.emitLine(one + " = add " + ty + " 0, 0"); // zero
        ir.emitLine(t + " = sub " + ty + " 1, " + reg); // 1 - x
        return ty + " " + t;
    }
    if (op == "~") {
        ir.emitLine(t + " = xor " + ty + " " + reg + ", -1");
        return ty + " " + t;
    }
    if (op == "&") {
        // address-of: the alloca register is already the address
        auto it = node->left && node->left->token ? locals.find(node->left->token->value) : locals.end();
        if (it != locals.end()) {
            return "ptr " + it->second;
        }
        return "ptr undef";
    }
    if (op == ".*") {
        // dereference
        ir.emitLine(t + " = load i32, i32* " + reg);
        return "i32 " + t;
    }
    return inner;
}

// ── Function calls ──────────────────────────────────────────────────────────

std::string Codegen::genCall(ASTNode* node) {
    if (!node || !node->token) return "i32 0";
    auto& name = node->token->value;
    std::string args;
    if (node->children) {
        for (size_t i = 0; i < node->children->size(); i++) {
            if (i > 0) args += ", ";
            auto* arg = (*node->children)[i];
            if (arg->left) args += genExpr(arg->left);
        }
    }

    std::string ret_type = "i32";
    if (name == "print" || name == "println") ret_type = "i32";
    else if (name == "exit" || name == "assert" || name == "panic") ret_type = "void";
    else if (name == "clock_ms" || name == "clock_ns") ret_type = "i64";

    if (ret_type == "void") {
        ir.emitLine("call void @" + name + "(" + args + ")");
        return "void undef";
    } else {
        auto t = ir.tmp("%call");
        ir.emitLine(t + " = call " + ret_type + " @" + name + "(" + args + ")");
        return ret_type + " " + t;
    }
}

// ── Member access ───────────────────────────────────────────────────────────

std::string Codegen::genMemberAccess(ASTNode* node) {
    if (!node) return "i32 0";
    if (node->left && node->left->node_type == ASTNodeType::Identifier && node->left->token) {
        auto& var_name = node->left->token->value;
        auto it = locals.find(var_name);
        if (it != locals.end()) {
            auto& llvm_type = local_types[var_name];
            std::string struct_name = llvm_type;
            if (!struct_name.empty() && struct_name[0] == '%')
                struct_name = struct_name.substr(1);

            std::string field_name;
            if (node->right && node->right->token)
                field_name = node->right->token->value;

            int field_index = 0;
            auto sit = struct_types.find(struct_name);
            if (sit != struct_types.end()) {
                for (size_t i = 0; i < sit->second.size(); i++) {
                    if (sit->second[i] == field_name) { field_index = i; break; }
                }
            }

            auto gep = ir.tmp("%gep");
            ir.emitLine(gep + " = getelementptr inbounds " + llvm_type + ", " +
                        llvm_type + "* " + it->second + ", i32 0, i32 " +
                        std::to_string(field_index));

            auto load = ir.tmp("%load");
            ir.emitLine(load + " = load i32, i32* " + gep);
            return "i32 " + load;
        }
    }
    return "i32 undef";
}

// ── Expression type helper ───────────────────────────────────────────────────

std::string Codegen::exprType(ASTNode* node) {
    if (!node) return "i32";
    switch (node->node_type) {
        case ASTNodeType::IntegerLiteral: return "i32";
        case ASTNodeType::FloatLiteral:   return "double";
        case ASTNodeType::BoolLiteral:    return "i1";
        case ASTNodeType::CharLiteral:    return "i8";
        case ASTNodeType::StringLiteral:  return "i8*";
        default: return "i32";
    }
}

// ── Phase 15: Deferred statement flush ──────────────────────────────────────

void Codegen::emitDeferred() {
    for (auto it = deferred_stmts.rbegin(); it != deferred_stmts.rend(); ++it) {
        genNode(*it);
    }
    deferred_stmts.clear();
}

} // namespace codegen
} // namespace razen
