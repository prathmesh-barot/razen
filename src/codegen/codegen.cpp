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
    if (node->node_type == ASTNodeType::Comment ||
        node->node_type == ASTNodeType::Annotation ||
        node->node_type == ASTNodeType::GenericParams)
        return;

    switch (node->node_type) {
        case ASTNodeType::FunctionDeclaration:
            genFuncDecl(node);
            break;
        case ASTNodeType::ExtDeclaration: {
            std::string ret_type = "i32";
            if (node->left && node->left->node_type == ASTNodeType::ReturnType && node->left->left)
                ret_type = typeToLLVM(node->left->left);
            std::string name = node->token ? node->token->value : "extern";
            std::string params;
            if (node->middle && node->middle->children) {
                for (size_t i = 0; i < node->middle->children->size(); i++) {
                    if (i > 0) params += ", ";
                    auto* p = (*node->middle->children)[i];
                    if (p->node_type == ASTNodeType::Parameter) {
                        params += p->left ? typeToLLVM(p->left) : "i32";
                    }
                }
            }
            if (params.empty()) params = "void";
            ir.emitLine("declare " + ret_type + " @" + name + "(" + params + ")");
            break;
        }
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
            auto catch_label = ir.label(".try.catch");
            auto end_label = ir.label(".try.end");
            std::string prev_catch = current_catch_label;
            current_catch_label = catch_label;
            if (node->left) genNode(node->left);
            ir.emitLine("br label %" + end_label);
            ir.emitLine("");
            ir.emitLine(catch_label + ":");
            if (node->right && node->right->node_type == ASTNodeType::CatchExpression) {
                if (node->right->left) genNode(node->right->left);
            }
            ir.emitLine("br label %" + end_label);
            ir.emitLine("");
            ir.emitLine(end_label + ":");
            current_catch_label = prev_catch;
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
        case ASTNodeType::StructDeclaration: {
            if (!node->token) break;
            auto& sname = node->token->value;
            std::vector<std::string> field_names;
            std::string struct_def = "%" + sname + " = type { ";
            bool first_field = true;

            // Collect methods and field types
            struct_methods[sname].clear();
            struct_field_types[sname].clear();

            if (node->children) {
                for (auto* child : *node->children) {
                    if (child->node_type == ASTNodeType::StructField) {
                        if (child->token) field_names.push_back(child->token->value);
                        if (!first_field) struct_def += ", ";
                        std::string ftype = child->left ? typeToLLVM(child->left) : "i32";
                        struct_def += ftype;
                        first_field = false;
                        if (child->token)
                            struct_field_types[sname][child->token->value] = ftype;
                    } else if (child->node_type == ASTNodeType::FunctionDeclaration) {
                        struct_methods[sname].push_back(child);
                    }
                }
            }
            struct_def += " }";
            struct_types[sname] = field_names;
            ir.emitLine(struct_def);

            // Emit struct methods
            current_struct_name = sname;
            for (auto* method : struct_methods[sname]) {
                std::string orig_name = method->token ? method->token->value : "method";
                method->token->value = sname + "." + orig_name;
                genFuncDecl(method);
                method->token->value = orig_name;
            }
            current_struct_name.clear();
            break;
        }
        case ASTNodeType::EnumDeclaration:
            genEnumDecl(node);
            break;
        case ASTNodeType::UnionDeclaration:
            genUnionDecl(node);
            break;
        case ASTNodeType::ErrorDeclaration:
            genErrorDecl(node);
            break;
        case ASTNodeType::TypeAliasDeclaration:
        case ASTNodeType::ModuleDeclaration:
        case ASTNodeType::UseDeclaration:
        case ASTNodeType::BehaveDeclaration:
            break;
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
        ret_type = resolveType(node->left->left);
    current_ret_type = ret_type;

    std::string sig = "define " + ret_type + " @" + name + "(";
    if (node->middle && node->middle->children) {
        for (size_t i = 0; i < node->middle->children->size(); i++) {
            if (i > 0) sig += ", ";
            auto* param = (*node->middle->children)[i];
            if (param->node_type == ASTNodeType::Parameter) {
                std::string ptype = param->left ? resolveType(param->left) : "i32";
                std::string pname = param->token ? param->token->value : "arg" + std::to_string(i);
                sig += ptype + " %" + pname;
            }
        }
    }
    sig += ") {";
    ir.emitLine(sig);
    ir.indent_str = "    ";

    if (node->middle && node->middle->children) {
        bool is_method = !current_struct_name.empty();
        for (size_t i = 0; i < node->middle->children->size(); i++) {
            auto* param = (*node->middle->children)[i];
            if (param->node_type == ASTNodeType::Parameter && param->token) {
                auto& pname = param->token->value;
                std::string ptype = param->left ? resolveType(param->left) : "i32";

                if (is_method && (pname == "self" || i == 0)) {
                    locals[pname] = "%" + pname;
                    local_types[pname] = "%" + current_struct_name + "*";
                } else {
                    locals[pname] = "%" + pname + ".addr";
                    local_types[pname] = ptype;
                    ir.emitLine("%" + pname + ".addr = alloca " + ptype);
                    ir.emitLine("store " + ptype + " %" + pname + ", " + ptype + "* %" + pname + ".addr");
                }
            }
        }
    }

    has_return_emitted = false;
    deferred_stmts.clear();
    if (node->right && node->right->node_type == ASTNodeType::Block)
        genBlock(node->right);

    emitDeferred();

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

    if (node->right && node->right->node_type == ASTNodeType::TryExpression) {
        if (llvm_type.find("{ i1,") == 0) {
            llvm_type = "i32";
            size_t end = llvm_type.find_last_of(' ');
            if (end != std::string::npos) {
                std::string inner = llvm_type.substr(end + 1);
                if (inner.back() == '}') inner.pop_back();
                if (!inner.empty() && inner != "void") llvm_type = inner;
            }
        }
    }

    std::string reg = "%" + name + ".addr";
    locals[name] = reg;
    local_types[name] = llvm_type;

    ir.emitLine(reg + " = alloca " + llvm_type);

    if (node->right) {
        std::string val;
        if (node->right->node_type == ASTNodeType::TryExpression) {
            val = genTryExpression(node->right);
        } else {
            val = genExpr(node->right);
        }
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
        // Check if returning error variant (FileError.NotFound)
        std::string err_set;
        int err_code;
        if (isErrorVariant(node->left, &err_set, &err_code)) {
            // Construct error union: { i1 1, i32 code }
            auto tmp = ir.tmp("%err_ret");
            ir.emitLine(tmp + " = alloca " + current_ret_type);
            auto fg = ir.tmp("%efg");
            ir.emitLine(fg + " = getelementptr inbounds " + current_ret_type + ", " +
                        current_ret_type + "* " + tmp + ", i32 0, i32 0");
            ir.emitLine("store i1 1, i1* " + fg);
            auto cg = ir.tmp("%ecg");
            ir.emitLine(cg + " = getelementptr inbounds " + current_ret_type + ", " +
                        current_ret_type + "* " + tmp + ", i32 0, i32 1");
            ir.emitLine("store i32 " + std::to_string(err_code) + ", i32* " + cg);
            auto l = ir.tmp("%el");
            ir.emitLine(l + " = load " + current_ret_type + ", " + current_ret_type + "* " + tmp);
            ir.emitLine("ret " + current_ret_type + " " + l);
            return;
        }

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
    if (node->middle) genNode(node->middle);
    if (!has_return_emitted) ir.emitLine("br label %" + label_end);
    ir.emitLine("");

    ir.emitLine(label_else + ":");
    if (node->right) {
        if (node->right->node_type == ASTNodeType::ElseIfStatement)
            genIf(node->right);
        else
            genNode(node->right);
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
    auto tok_type = node->token->type;
    bool is_compound = (tok_type == TokenType::PlusEquals ||
                        tok_type == TokenType::MinusEquals ||
                        tok_type == TokenType::StarEquals ||
                        tok_type == TokenType::SlashEquals ||
                        tok_type == TokenType::PercentEquals);

    // Helper: emit store to address
    auto emitStore = [&](const std::string& ptr, const std::string& ptr_type, const std::string& val_str) {
        if (is_compound) {
            auto load = ir.tmp("%load");
            ir.emitLine(load + " = load " + ptr_type + ", " + ptr_type + "* " + ptr);
            auto rhs = genExpr(node->right);
            std::string rt, rr;
            auto sp = rhs.find(' ');
            if (sp != std::string::npos) { rt = rhs.substr(0, sp); rr = rhs.substr(sp + 1); }
            else { rt = ptr_type; rr = rhs; }
            auto bin = ir.tmp("%bin");
            switch (tok_type) {
                case TokenType::PlusEquals:
                    if (ptr_type == "double" || ptr_type == "float" || ptr_type == "fp128" || ptr_type == "half")
                        ir.emitLine(bin + " = fadd " + ptr_type + " " + load + ", " + rr);
                    else
                        ir.emitLine(bin + " = add " + ptr_type + " " + load + ", " + rr);
                    break;
                case TokenType::MinusEquals:
                    if (ptr_type == "double" || ptr_type == "float" || ptr_type == "fp128" || ptr_type == "half")
                        ir.emitLine(bin + " = fsub " + ptr_type + " " + load + ", " + rr);
                    else
                        ir.emitLine(bin + " = sub " + ptr_type + " " + load + ", " + rr);
                    break;
                case TokenType::StarEquals:
                    if (ptr_type == "double" || ptr_type == "float" || ptr_type == "fp128" || ptr_type == "half")
                        ir.emitLine(bin + " = fmul " + ptr_type + " " + load + ", " + rr);
                    else
                        ir.emitLine(bin + " = mul " + ptr_type + " " + load + ", " + rr);
                    break;
                case TokenType::SlashEquals:
                    if (ptr_type == "double" || ptr_type == "float" || ptr_type == "fp128" || ptr_type == "half")
                        ir.emitLine(bin + " = fdiv " + ptr_type + " " + load + ", " + rr);
                    else
                        ir.emitLine(bin + " = sdiv " + ptr_type + " " + load + ", " + rr);
                    break;
                case TokenType::PercentEquals:
                    ir.emitLine(bin + " = srem " + ptr_type + " " + load + ", " + rr);
                    break;
                default: break;
            }
            ir.emitLine("store " + ptr_type + " " + bin + ", " + ptr_type + "* " + ptr);
        } else {
            auto val = genExpr(node->right);
            ir.emitLine("store " + val + ", " + ptr_type + "* " + ptr);
        }
    };

    // Case 1: x = expr (simple identifier)
    if (lhs->node_type == ASTNodeType::Identifier && lhs->token) {
        auto& name = lhs->token->value;
        auto it = locals.find(name);
        if (it != locals.end()) {
            std::string val_type = local_types[name];
            if (is_compound) {
                emitStore(it->second, val_type, "");
            } else {
                auto val = node->right ? genExpr(node->right) : val_type + " 0";
                ir.emitLine("store " + val + ", " + val_type + "* " + it->second);
            }
        }
        return;
    }

    // Case 2: x.field = expr or x.field += expr (member access)
    if (lhs->node_type == ASTNodeType::MemberAccess) {
        // Evaluate the LHS to get the field pointer, then store
        // For member access as LHS, we need the GEP pointer
        auto* ma = lhs;
        if (ma->left && ma->left->node_type == ASTNodeType::Identifier && ma->left->token) {
            auto& var_name = ma->left->token->value;
            auto it = locals.find(var_name);
            if (it != locals.end()) {
                auto& llvm_type = local_types[var_name];
                std::string struct_name = llvm_type;
                if (!struct_name.empty() && struct_name[0] == '%')
                    struct_name = struct_name.substr(1);

                std::string field_name;
                if (ma->right && ma->right->token)
                    field_name = ma->right->token->value;

                int field_index = 0;
                auto sit = struct_types.find(struct_name);
                if (sit != struct_types.end()) {
                    for (size_t i = 0; i < sit->second.size(); i++) {
                        if (sit->second[i] == field_name) { field_index = i; break; }
                    }
                }

                std::string field_llvm_type = "i32";
                auto ftit = struct_field_types.find(struct_name);
                if (ftit != struct_field_types.end()) {
                    auto fit = ftit->second.find(field_name);
                    if (fit != ftit->second.end()) field_llvm_type = fit->second;
                }

                auto gep = ir.tmp("%lhs_gep");
                ir.emitLine(gep + " = getelementptr inbounds " + llvm_type + ", " +
                            llvm_type + "* " + it->second + ", i32 0, i32 " +
                            std::to_string(field_index));

                emitStore(gep, field_llvm_type, "");
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
    auto space = match_val.find(' ');
    std::string match_type = space != std::string::npos ? match_val.substr(0, space) : "i32";

    if (!node->children || node->children->empty()) {
        ir.emitLine("br label %" + label_end);
        ir.emitLine("");
        ir.emitLine(label_end + ":");
        return;
    }

    // Separate patterned cases from default case, detect union patterns
    std::vector<ASTNode*> pattern_cases;
    std::vector<std::pair<std::string, int>> union_case_info; // (union_name, tag)
    ASTNode* else_case = nullptr;
    bool has_union_cases = false;

    for (auto* case_node : *node->children) {
        if (!case_node->left) {
            else_case = case_node;
            continue;
        }
        // Check for union pattern: MemberAccess(Identifier("UnionName"), FunctionCall("Variant"))
        if (case_node->left->node_type == ASTNodeType::MemberAccess &&
            case_node->left->left && case_node->left->right &&
            case_node->left->right->node_type == ASTNodeType::FunctionCall &&
            case_node->left->left->token &&
            union_variants.count(case_node->left->left->token->value)) {
            has_union_cases = true;
            std::string uname = case_node->left->left->token->value;
            std::string vname = case_node->left->right->token->value;
            int tag = 0;
            for (auto& vi : union_variants[uname]) {
                if (vi.name == vname) { tag = vi.tag; break; }
            }
            union_case_info.push_back({uname, tag});
            pattern_cases.push_back(case_node);
        } else {
            union_case_info.push_back({"", -1});
            pattern_cases.push_back(case_node);
        }
    }

    if (pattern_cases.empty()) {
        if (else_case && else_case->right) genNode(else_case->right);
        ir.emitLine("br label %" + label_end);
        ir.emitLine("");
        ir.emitLine(label_end + ":");
        return;
    }

    // ── Union match path ──
    if (has_union_cases) {
        std::string uname;
        for (auto& uci : union_case_info) {
            if (!uci.first.empty()) { uname = uci.first; break; }
        }

        // Alloca the union value
        auto match_ptr = ir.tmp("%match.ptr");
        ir.emitLine(match_ptr + " = alloca " + match_type);
        ir.emitLine("store " + match_val + ", " + match_type + "* " + match_ptr);

        // Extract tag
        auto tag_gep = ir.tmp("%tag.gep");
        ir.emitLine(tag_gep + " = getelementptr inbounds " + match_type + ", " +
                    match_type + "* " + match_ptr + ", i32 0, i32 0");
        auto tag = ir.tmp("%tag");
        ir.emitLine(tag + " = load i32, i32* " + tag_gep);

        auto first_check = ir.label(".match.check");
        ir.emitLine("br label %" + first_check);
        ir.emitLine("");

        std::string current_label = first_check;

        for (size_t i = 0; i < pattern_cases.size(); i++) {
            auto* case_node = pattern_cases[i];
            auto& uci = union_case_info[i];
            int expected_tag = uci.second;

            ir.emitLine(current_label + ":");

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
            ir.emitLine(ic + " = icmp eq i32 " + tag + ", " + std::to_string(expected_tag));
            ir.emitLine("br i1 " + ic + ", label %" + case_label + ", label %" + fallthrough);
            ir.emitLine("");
            ir.emitLine(case_label + ":");

            // For union cases, extract payload and bind variable
            if (uci.first == uname && expected_tag >= 0) {
                auto& variants = union_variants[uname];
                if ((size_t)expected_tag < variants.size()) {
                    auto& vi = variants[expected_tag];
                    auto payload_gep = ir.tmp("%pgep");
                    int psize = union_payload_sizes[uname];
                    ir.emitLine(payload_gep + " = getelementptr inbounds " + match_type + ", " +
                                match_type + "* " + match_ptr + ", i32 0, i32 1");
                    auto pc = ir.tmp("%pcast");
                    ir.emitLine(pc + " = bitcast [" + std::to_string(psize) + " x i8]* " +
                                payload_gep + " to " + vi.payload_type + "*");

                    // Bind variable from match pattern: Value.Int(v)
                    if (case_node->left->right->children && !case_node->left->right->children->empty()) {
                        auto* arg = (*case_node->left->right->children)[0];
                        if (arg->left && arg->left->node_type == ASTNodeType::Identifier && arg->left->token) {
                            auto& vname = arg->left->token->value;
                            auto vaddr = ir.tmp("%" + vname + ".addr");
                            ir.emitLine(vaddr + " = alloca " + vi.payload_type);
                            auto pv = ir.tmp("%pv");
                            ir.emitLine(pv + " = load " + vi.payload_type + ", " +
                                        vi.payload_type + "* " + pc);
                            ir.emitLine("store " + vi.payload_type + " " + pv + ", " +
                                        vi.payload_type + "* " + vaddr);
                            locals[vname] = vaddr;
                            local_types[vname] = vi.payload_type;
                        }
                    }
                }
            }

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
        return;
    }

    // ── Simple (non-union) match path ──
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
        ir.emitLine(ic + " = icmp eq " + match_type + " " + match_reg + ", " + case_reg);
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
        case ASTNodeType::TryExpression:
            return genTryExpression(node);
        case ASTNodeType::ArrayLiteral:
            return genArrayLiteral(node);
        case ASTNodeType::TupleLiteral:
            return genTupleLiteral(node);
        case ASTNodeType::RangeExpression:
            return genRangeLiteral(node);
        default:
            return "i32 0";
    }
}

std::string Codegen::genArrayLiteral(ASTNode* node) {
    if (!node || !node->children || node->children->empty()) return "[0 x i32] zeroinitializer";
    std::string elem_type = "i32";
    auto first_val = genExpr((*node->children)[0]);
    auto sp = first_val.find(' ');
    if (sp != std::string::npos) elem_type = first_val.substr(0, sp);

    size_t n = node->children->size();
    std::string arr_type = "[" + std::to_string(n) + " x " + elem_type + "]";
    auto arr_reg = ir.tmp("%arr");
    ir.emitLine(arr_reg + " = alloca " + arr_type);

    for (size_t i = 0; i < n; i++) {
        auto* child = (*node->children)[i];
        // Re-evaluate each child for proper type
        auto val = genExpr(child);
        auto sp2 = val.find(' ');
        std::string val_type = sp2 != std::string::npos ? val.substr(0, sp2) : elem_type;
        std::string val_reg = sp2 != std::string::npos ? val.substr(sp2 + 1) : val;
        auto gep = ir.tmp("%gep");
        ir.emitLine(gep + " = getelementptr inbounds " + arr_type + ", " +
                    arr_type + "* " + arr_reg + ", i32 0, i32 " + std::to_string(i));
        ir.emitLine("store " + val_type + " " + val_reg + ", " + val_type + "* " + gep);
    }

    auto load = ir.tmp("%arrload");
    ir.emitLine(load + " = load " + arr_type + ", " + arr_type + "* " + arr_reg);
    return arr_type + " " + load;
}

std::string Codegen::genTupleLiteral(ASTNode* node) {
    if (!node || !node->children || node->children->empty()) return "{ i32 } zeroinitializer";

    std::string struct_type = "{";
    for (size_t i = 0; i < node->children->size(); i++) {
        if (i > 0) struct_type += ",";
        auto* child = (*node->children)[i];
        auto val = genExpr(child);
        auto sp = val.find(' ');
        std::string ty = sp != std::string::npos ? val.substr(0, sp) : "i32";
        struct_type += " " + ty;
    }
    struct_type += " }";

    auto reg = ir.tmp("%tup");
    ir.emitLine(reg + " = alloca " + struct_type);

    for (size_t i = 0; i < node->children->size(); i++) {
        auto* child = (*node->children)[i];
        auto val = genExpr(child);
        auto sp = val.find(' ');
        std::string ty = sp != std::string::npos ? val.substr(0, sp) : "i32";
        std::string reg_val = sp != std::string::npos ? val.substr(sp + 1) : val;
        auto gep = ir.tmp("%gep");
        ir.emitLine(gep + " = getelementptr inbounds " + struct_type + ", " +
                    struct_type + "* " + reg + ", i32 0, i32 " + std::to_string(i));
        ir.emitLine("store " + ty + " " + reg_val + ", " + ty + "* " + gep);
    }

    auto load = ir.tmp("%tupload");
    ir.emitLine(load + " = load " + struct_type + ", " + struct_type + "* " + reg);
    return struct_type + " " + load;
}

std::string Codegen::genRangeLiteral(ASTNode* node) {
    auto left = node->left ? genExpr(node->left) : "i32 0";
    auto right = node->right ? genExpr(node->right) : "i32 0";
    auto sp_l = left.find(' ');
    auto sp_r = right.find(' ');
    std::string lt = sp_l != std::string::npos ? left.substr(0, sp_l) : "i32";
    std::string lr = sp_l != std::string::npos ? left.substr(sp_l + 1) : left;
    std::string rt = sp_r != std::string::npos ? right.substr(0, sp_r) : "i32";
    std::string rr = sp_r != std::string::npos ? right.substr(sp_r + 1) : right;

    std::string range_type = "{ " + lt + ", " + rt + " }";
    auto reg = ir.tmp("%range");
    ir.emitLine(reg + " = alloca " + range_type);

    auto g0 = ir.tmp("%rg0");
    ir.emitLine(g0 + " = getelementptr inbounds " + range_type + ", " +
                range_type + "* " + reg + ", i32 0, i32 0");
    ir.emitLine("store " + lt + " " + lr + ", " + lt + "* " + g0);

    auto g1 = ir.tmp("%rg1");
    ir.emitLine(g1 + " = getelementptr inbounds " + range_type + ", " +
                range_type + "* " + reg + ", i32 0, i32 1");
    ir.emitLine("store " + rt + " " + rr + ", " + rt + "* " + g1);

    auto load = ir.tmp("%rload");
    ir.emitLine(load + " = load " + range_type + ", " + range_type + "* " + reg);
    return range_type + " " + load;
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
        // Check if this is a pointer-type variable (self param in method)
        if (ty.size() > 1 && ty.back() == '*') {
            // It's already a pointer, return as-is
            return ty + " " + it->second;
        }
        auto t = ir.tmp("%load");
        ir.emitLine(t + " = load " + ty + ", " + ty + "* " + it->second);
        return ty + " " + t;
    }
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
    std::string res_type = lt;
    auto t = ir.tmp("%bin");

    switch (tok->type) {
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

        case TokenType::AndAnd:
            ir.emitLine(t + " = and i1 " + lr + ", " + rr);
            return "i1 " + t;
        case TokenType::OrOr:
            ir.emitLine(t + " = or i1 " + lr + ", " + rr);
            return "i1 " + t;

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
            ir.emitLine(t + " = sub " + ty + " 0, " + reg);
        }
        return ty + " " + t;
    }
    if (op == "!") {
        ir.emitLine(t + " = sub " + ty + " 1, " + reg);
        return ty + " " + t;
    }
    if (op == "~") {
        ir.emitLine(t + " = xor " + ty + " " + reg + ", -1");
        return ty + " " + t;
    }
    if (op == "&") {
        auto it = node->left && node->left->token ? locals.find(node->left->token->value) : locals.end();
        if (it != locals.end()) {
            return "ptr " + it->second;
        }
        return "ptr undef";
    }
    if (op == ".*") {
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

    // ── Error variant: FileError.NotFound ──
    if (node->left && node->left->node_type == ASTNodeType::Identifier && node->left->token &&
        node->right && node->right->node_type == ASTNodeType::Identifier && node->right->token) {
        auto& left_name = node->left->token->value;
        auto& right_name = node->right->token->value;

        // Check enum constant: Dir.Up, Color.Red
        auto eit = enum_values.find(left_name);
        if (eit != enum_values.end()) {
            auto vit = eit->second.find(right_name);
            if (vit != eit->second.end()) {
                std::string bt = enum_backing_types.count(left_name) ? enum_backing_types[left_name] : "i32";
                return bt + " " + std::to_string(vit->second);
            }
        }

        // Check error variant: FileError.NotFound
        auto erit = error_values.find(left_name);
        if (erit != error_values.end()) {
            auto vit = erit->second.find(right_name);
            if (vit != erit->second.end()) {
                return "i32 " + std::to_string(vit->second);
            }
        }
    }

    // ── Union constructor: Value.Int(42) ──
    if (node->left && node->left->node_type == ASTNodeType::Identifier && node->left->token &&
        node->right && node->right->node_type == ASTNodeType::FunctionCall) {
        auto& uname = node->left->token->value;
        if (union_variants.count(uname)) {
            return genUnionConstruct(node);
        }
    }

    // ── Struct method call: c.increment() ──
    if (node->left && node->left->node_type == ASTNodeType::Identifier && node->left->token &&
        node->right && node->right->node_type == ASTNodeType::FunctionCall) {
        auto& var_name = node->left->token->value;
        auto it = locals.find(var_name);
        if (it != locals.end()) {
            std::string& llvm_type = local_types[var_name];
            std::string struct_name = llvm_type;
            if (!struct_name.empty() && struct_name[0] == '%')
                struct_name = struct_name.substr(1);
            if (struct_name.size() > 1 && struct_name.back() == '*')
                struct_name.pop_back();

            // Check if this struct has methods
            auto mit = struct_methods.find(struct_name);
            if (mit != struct_methods.end()) {
                auto& method_name = node->right->token->value;
                std::string mangled = struct_name + "." + method_name;

                // Build args: self + rest
                std::string args = llvm_type + "* " + it->second;
                if (node->right->children) {
                    for (auto* arg : *node->right->children) {
                        if (arg->left) args += ", " + genExpr(arg->left);
                    }
                }

                auto caller = ir.tmp("%call");
                ir.emitLine(caller + " = call void @" + mangled + "(" + args + ")");
                return "void undef";
            }
        }
    }

    // ── Struct field access: c.value ──
    if (node->left && node->left->node_type == ASTNodeType::Identifier && node->left->token) {
        auto& var_name = node->left->token->value;
        auto it = locals.find(var_name);
        if (it != locals.end()) {
            auto& llvm_type = local_types[var_name];
            std::string struct_name = llvm_type;

            // Handle pointer-to-struct (self param in methods)
            bool is_ptr = false;
            if (!struct_name.empty() && struct_name.back() == '*') {
                is_ptr = true;
                struct_name.pop_back();
                while (!struct_name.empty() && struct_name.back() == ' ') struct_name.pop_back();
            }
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

            // Get field type
            std::string field_llvm_type = "i32";
            auto ftit = struct_field_types.find(struct_name);
            if (ftit != struct_field_types.end()) {
                auto fit = ftit->second.find(field_name);
                if (fit != ftit->second.end()) field_llvm_type = fit->second;
            }

            std::string ptr = it->second;
            if (!is_ptr) {
                // Load the struct value first, then GEP
                auto load = ir.tmp("%sload");
                ir.emitLine(load + " = load " + llvm_type + ", " + llvm_type + "* " + ptr);
                auto alloc = ir.tmp("%salloc");
                ir.emitLine(alloc + " = alloca " + llvm_type);
                ir.emitLine("store " + llvm_type + " " + load + ", " + llvm_type + "* " + alloc);
                ptr = alloc;
            }

            auto gep = ir.tmp("%gep");
            ir.emitLine(gep + " = getelementptr inbounds " + llvm_type + ", " +
                        (is_ptr ? "" : llvm_type + "* ") + ptr + ", i32 0, i32 " +
                        std::to_string(field_index));

            auto load = ir.tmp("%load");
            ir.emitLine(load + " = load " + field_llvm_type + ", " + field_llvm_type + "* " + gep);
            return field_llvm_type + " " + load;
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

// ═══════════════════════════════════════════════════════════════════════════════
// Phase 17-20: Enum, Union, Error Handling, Struct Methods
// ═══════════════════════════════════════════════════════════════════════════════

// ── Enum declaration ─────────────────────────────────────────────────────────

void Codegen::genEnumDecl(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    std::string backing_type = "i32";
    if (node->left) backing_type = typeToLLVM(node->left);
    enum_backing_types[name] = backing_type;

    ir.emitLine("%" + name + " = type " + backing_type);

    int next_value = 0;
    if (node->children) {
        for (auto* child : *node->children) {
            if (child->node_type == ASTNodeType::EnumField && child->token) {
                int value = next_value;
                if (child->right && child->right->node_type == ASTNodeType::IntegerLiteral && child->right->token) {
                    value = std::stoi(child->right->token->value);
                    next_value = value + 1;
                } else {
                    next_value++;
                }
                enum_values[name][child->token->value] = value;
            }
        }
    }
}

// ── Union declaration ────────────────────────────────────────────────────────

void Codegen::genUnionDecl(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    int max_size = 0;
    std::vector<UnionFieldInfo> variants;

    if (node->children) {
        for (size_t i = 0; i < node->children->size(); i++) {
            auto* child = (*node->children)[i];
            if (child->node_type != ASTNodeType::UnionField) continue;

            UnionFieldInfo info;
            info.name = child->token ? child->token->value : "v" + std::to_string(i);
            info.tag = (int)i;

            if (child->left) {
                info.payload_type = typeToLLVM(child->left);
                info.payload_size = getLLVMTypeSize(info.payload_type);
            } else if (child->children && !child->children->empty()) {
                info.payload_type = "{";
                for (size_t j = 0; j < child->children->size(); j++) {
                    if (j > 0) info.payload_type += ", ";
                    auto* sf = (*child->children)[j];
                    info.payload_type += sf->left ? typeToLLVM(sf->left) : "i32";
                }
                info.payload_type += " }";
                info.payload_size = getLLVMTypeSize(info.payload_type);
            } else {
                info.payload_type = "void";
                info.payload_size = 0;
            }

            if (info.payload_size > max_size) max_size = info.payload_size;
            variants.push_back(info);
        }
    }

    union_variants[name] = variants;
    union_payload_sizes[name] = max_size;

    ir.emitLine("%" + name + " = type { i32, [" + std::to_string(max_size) + " x i8] }");
}

// ── Error set declaration ────────────────────────────────────────────────────

void Codegen::genErrorDecl(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    ir.emitLine("; error " + name);

    int code = 0;
    if (node->children) {
        for (auto* child : *node->children) {
            if (child->node_type == ASTNodeType::ErrorField && child->token) {
                error_values[name][child->token->value] = code++;
            }
        }
    }
}

// ── isEnumName, isUnionName, isErrorSetName helpers ─────────────────────────

bool Codegen::isEnumName(const std::string& name) {
    return enum_values.count(name) > 0;
}

bool Codegen::isUnionName(const std::string& name) {
    return union_variants.count(name) > 0;
}

bool Codegen::isErrorSetName(const std::string& name) {
    return error_values.count(name) > 0;
}

// ── Union constructor: Value.Int(42) ─────────────────────────────────────────

std::string Codegen::genUnionConstruct(ASTNode* node) {
    // node is MemberAccess with left=Identifier(union_name), right=FunctionCall(variant_name)
    std::string uname = node->left->token->value;
    ASTNode* call = node->right;
    std::string vname = call->token->value;

    UnionFieldInfo* info = nullptr;
    for (auto& v : union_variants[uname]) {
        if (v.name == vname) { info = &v; break; }
    }
    if (!info) return "i32 0";

    // Evaluate argument
    std::string arg_type = "i32";
    std::string arg_reg = "0";
    if (call->children && !call->children->empty()) {
        auto val = genExpr((*call->children)[0]->left);
        auto sp = val.find(' ');
        if (sp != std::string::npos) {
            arg_type = val.substr(0, sp);
            arg_reg = val.substr(sp + 1);
        } else {
            arg_reg = val;
        }
    }

    std::string union_type = "%" + uname;
    auto tmp = ir.tmp("%ucon");
    ir.emitLine(tmp + " = alloca " + union_type);
    int psize = union_payload_sizes[uname];

    // Store tag
    auto tg = ir.tmp("%utag");
    ir.emitLine(tg + " = getelementptr inbounds " + union_type + ", " +
                union_type + "* " + tmp + ", i32 0, i32 0");
    ir.emitLine("store i32 " + std::to_string(info->tag) + ", i32* " + tg);

    // Store payload
    auto pg = ir.tmp("%upg");
    ir.emitLine(pg + " = getelementptr inbounds " + union_type + ", " +
                union_type + "* " + tmp + ", i32 0, i32 1");
    if (info->payload_size > 0) {
        auto pc = ir.tmp("%upc");
        ir.emitLine(pc + " = bitcast [" + std::to_string(psize) + " x i8]* " + pg +
                    " to " + info->payload_type + "*");
        ir.emitLine("store " + info->payload_type + " " + arg_reg + ", " +
                    info->payload_type + "* " + pc);
    }

    auto l = ir.tmp("%uload");
    ir.emitLine(l + " = load " + union_type + ", " + union_type + "* " + tmp);
    return union_type + " " + l;
}

// ── Error variant check ──────────────────────────────────────────────────────

bool Codegen::isErrorVariant(ASTNode* node, std::string* out_set, int* out_code) {
    if (!node || node->node_type != ASTNodeType::MemberAccess) return false;
    if (!node->left || node->left->node_type != ASTNodeType::Identifier || !node->left->token) return false;
    if (!node->right || node->right->node_type != ASTNodeType::Identifier || !node->right->token) return false;

    auto& set_name = node->left->token->value;
    auto& var_name = node->right->token->value;

    auto sit = error_values.find(set_name);
    if (sit == error_values.end()) return false;
    auto vit = sit->second.find(var_name);
    if (vit == sit->second.end()) return false;

    if (out_set) *out_set = set_name;
    if (out_code) *out_code = vit->second;
    return true;
}

// ── Try expression codegen ───────────────────────────────────────────────────

std::string Codegen::genTryExpression(ASTNode* node) {
    if (!node || !node->left) return "i32 0";

    // Emit the expression (function call)
    auto call_result = genExpr(node->left);

    // Extract the result type
    auto sp = call_result.find(' ');
    std::string full_type = sp != std::string::npos ? call_result.substr(0, sp) : "i32";
    std::string call_reg = sp != std::string::npos ? call_result.substr(sp + 1) : call_result;

    // Determine inner type from { i1, T }
    std::string inner_type = "i32";
    if (full_type.find("{ i1,") == 0) {
        size_t end = full_type.find_last_of(' ');
        if (end != std::string::npos) {
            inner_type = full_type.substr(end + 1);
            if (inner_type.back() == '}') inner_type.pop_back();
            if (inner_type.empty() || inner_type == "void") inner_type = "i32";
        }
    } else {
        // Not an error union, just return the value
        return call_result;
    }

    bool has_catch = (node->right && node->right->node_type == ASTNodeType::CatchExpression);

    auto success_label = ir.label(".try.success");
    auto end_label = ir.label(".try.end");

    // Extract flag and value
    auto flag = ir.tmp("%tflag");
    ir.emitLine(flag + " = extractvalue " + full_type + " " + call_reg + ", 0");

    if (has_catch) {
        auto catch_label = ir.label(".try.catch");
        ir.emitLine("br i1 " + flag + ", label %" + catch_label + ", label %" + success_label);
        ir.emitLine("");
        ir.emitLine(success_label + ":");
        auto val = ir.tmp("%tval");
        ir.emitLine(val + " = extractvalue " + full_type + " " + call_reg + ", 1");
        ir.emitLine("br label %" + end_label);
        ir.emitLine("");
        ir.emitLine(catch_label + ":");
        if (node->right->left) genNode(node->right->left);
        ir.emitLine("br label %" + end_label);
        ir.emitLine("");
        ir.emitLine(end_label + ":");
        // Return the extracted value in success case (stored in %tval)
        return inner_type + " " + ir.tmp("%tval_phi_placeholder");
    } else {
        // Propagate to caller
        auto propagate_label = ir.label(".try.propagate");
        ir.emitLine("br i1 " + flag + ", label %" + propagate_label + ", label %" + success_label);
        ir.emitLine("");
        ir.emitLine(success_label + ":");
        auto val = ir.tmp("%tval");
        ir.emitLine(val + " = extractvalue " + full_type + " " + call_reg + ", 1");
        ir.emitLine("br label %" + end_label);
        ir.emitLine("");
        ir.emitLine(propagate_label + ":");
        ir.emitLine("ret " + full_type + " " + call_reg);
        has_return_emitted = true;
        ir.emitLine("");
        ir.emitLine(end_label + ":");
        return inner_type + " " + val;
    }
}

// ── resolveType: wraps typeToLLVM, resolves @Self ───────────────────────────

std::string Codegen::resolveType(ASTNode* type_node) {
    if (!type_node) return "void";
    if (type_node->node_type == ASTNodeType::VarType &&
        type_node->token && type_node->token->value == "Self") {
        return "%" + current_struct_name + "*";
    }
    return typeToLLVM(type_node);
}

// ── getLLVMTypeSize ──────────────────────────────────────────────────────────

int Codegen::getLLVMTypeSize(const std::string& type_str) {
    if (type_str == "i1" || type_str == "i8") return 1;
    if (type_str == "i16" || type_str == "half") return 2;
    if (type_str == "i32" || type_str == "float") return 4;
    if (type_str == "i64" || type_str == "double" || type_str == "ptr" ||
        type_str == "i8*" || type_str == "i32*") return 8;
    if (type_str == "i128" || type_str == "fp128") return 16;

    // Array: [N x T]
    if (type_str.size() > 2 && type_str[0] == '[') {
        auto xp = type_str.find(" x ");
        if (xp != std::string::npos) {
            int count = std::stoi(type_str.substr(1, xp - 1));
            std::string elem = type_str.substr(xp + 3, type_str.size() - xp - 4);
            return count * getLLVMTypeSize(elem);
        }
    }

    // Struct: { T1, T2, ... } — simple parser
    if (type_str.size() > 2 && type_str[0] == '{') {
        int total = 0;
        size_t depth = 0;
        std::string cur;
        for (size_t i = 1; i < type_str.size() - 1; i++) {
            char c = type_str[i];
            if (c == '{' || c == '[') depth++;
            else if (c == '}' || c == ']') depth--;
            else if (c == ',' && depth == 0) {
                while (!cur.empty() && cur[0] == ' ') cur = cur.substr(1);
                while (!cur.empty() && cur.back() == ' ') cur.pop_back();
                if (!cur.empty()) total += getLLVMTypeSize(cur);
                cur.clear();
                continue;
            }
            cur += c;
        }
        while (!cur.empty() && cur[0] == ' ') cur = cur.substr(1);
        while (!cur.empty() && cur.back() == ' ') cur.pop_back();
        if (!cur.empty()) total += getLLVMTypeSize(cur);
        return total;
    }

    // named struct %Name: estimate 64 bytes
    if (!type_str.empty() && type_str == "i8**") return 8;
    if (!type_str.empty() && type_str[0] == '%') return 64;

    return 8;
}

} // namespace codegen
} // namespace razen
