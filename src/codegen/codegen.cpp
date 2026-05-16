#include "codegen.h"
#include "../ast/ast_utils.h"
#include "../ast/token_utils.h"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/Support/raw_ostream.h>
#include <iostream>

using namespace llvm;

namespace razen {
namespace codegen {

// ── Module setup & top-level dispatch ───────────────────────────────────────

void Codegen::generate(const std::vector<ASTNode*>& nodes) {
    auto& ctx = ir.ctx;
    auto& mod = ir.module;

    mod.setTargetTriple("x86_64-pc-linux-gnu");
    mod.setDataLayout("e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128");

    // Libc / runtime declarations (modern LLVM pattern with attributes)
    auto* int32 = Type::getInt32Ty(ctx);
    auto* i8ptr = PointerType::getUnqual(ctx);
    auto* voidTy = Type::getVoidTy(ctx);

    {
        auto callee = mod.getOrInsertFunction("printf", FunctionType::get(int32, {i8ptr}, true));
        if (auto* f = dyn_cast<Function>(callee.getCallee())) {
            f->setDoesNotThrow();
            f->addParamAttr(0, Attribute::ReadOnly);
        }
    }
    {
        auto callee = mod.getOrInsertFunction("puts", FunctionType::get(int32, {i8ptr}, false));
        if (auto* f = dyn_cast<Function>(callee.getCallee())) {
            f->setDoesNotThrow();
            f->addParamAttr(0, Attribute::ReadOnly);
        }
    }
    {
        auto callee = mod.getOrInsertFunction("exit", FunctionType::get(voidTy, {int32}, false));
        if (auto* f = dyn_cast<Function>(callee.getCallee())) {
            f->setDoesNotThrow();
        }
    }
    {
        auto callee = mod.getOrInsertFunction("abort", FunctionType::get(voidTy, true));
        if (auto* f = dyn_cast<Function>(callee.getCallee())) {
            f->setDoesNotThrow();
        }
    }

    // Pass 1: collect type declarations (structs, enums, unions, errors)
    for (auto* node : nodes) {
        if (!node) continue;
        switch (node->node_type) {
            case ASTNodeType::StructDeclaration:  ir.genStruct(node); break;
            case ASTNodeType::EnumDeclaration:    ir.genEnum(node);   break;
            case ASTNodeType::UnionDeclaration:   ir.genUnion(node);  break;
            case ASTNodeType::ErrorDeclaration:   ir.genError(node);  break;
            default: break;
        }
    }

    // Pass 2: emit function declarations (ext func) for forward references
    for (auto* node : nodes) {
        if (node && node->node_type == ASTNodeType::ExtDeclaration) {
            ir.genExt(node);
        }
    }

    // Pass 3: emit function definitions and globals
    for (auto* node : nodes) {
        if (!node) continue;
        switch (node->node_type) {
            case ASTNodeType::FunctionDeclaration:
                ir.genFunc(node);
                break;
            case ASTNodeType::VarDeclaration:
            case ASTNodeType::ConstDeclaration:
                try {
                    ir.genVar(node, node->node_type == ASTNodeType::ConstDeclaration);
                } catch (...) {
                    errs() << "Warning: failed to codegen variable " << (node->token ? node->token->value : "?") << "\n";
                }
                break;
            default: break;
        }
    }

    verifyModule(mod, &errs());
}

// ── IRGen top-level dispatch ────────────────────────────────────────────────

void IRGen::genTopLevel(ASTNode* node) {
    if (!node) return;
    switch (node->node_type) {
        case ASTNodeType::Comment:
        case ASTNodeType::Annotation:
        case ASTNodeType::GenericParams:
        case ASTNodeType::ModuleDeclaration:
        case ASTNodeType::UseDeclaration:
        case ASTNodeType::BehaveDeclaration:
        case ASTNodeType::TypeAliasDeclaration:
            return;
        case ASTNodeType::FunctionDeclaration:   genFunc(node); break;
        case ASTNodeType::ExtDeclaration:        genExt(node);  break;
        case ASTNodeType::VarDeclaration:        genVar(node, false); break;
        case ASTNodeType::ConstDeclaration:      genVar(node, true);  break;
        case ASTNodeType::StructDeclaration:     genStruct(node); break;
        case ASTNodeType::EnumDeclaration:       genEnum(node);   break;
        case ASTNodeType::UnionDeclaration:      genUnion(node);  break;
        case ASTNodeType::ErrorDeclaration:      genError(node);  break;
        default: break;
    }
}

// ── Struct codegen ──────────────────────────────────────────────────────────

void IRGen::genStruct(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    if (types.structs.count(name)) return;

    std::vector<Type*> field_types;
    std::vector<std::string> field_names;
    struct_methods[name].clear();

    if (node->children) {
        for (auto* child : *node->children) {
            if (child->node_type == ASTNodeType::StructField) {
                if (child->token) field_names.push_back(child->token->value);
                field_types.push_back(child->left ? razenType(child->left) : Type::getInt32Ty(ctx));
            } else if (child->node_type == ASTNodeType::FunctionDeclaration) {
                struct_methods[name].push_back(child);
            }
        }
    }

    auto* st = StructType::create(ctx, field_types, name);
    types.structs[name] = st;
    types.field_names[name] = field_names;
}

// ── Enum codegen ────────────────────────────────────────────────────────────

void IRGen::genEnum(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    Type* backing = Type::getInt32Ty(ctx);
    if (node->left) backing = razenType(node->left);
    enums[name].backing = backing;

    int next = 0;
    if (node->children) {
        for (auto* child : *node->children) {
            if (child->node_type == ASTNodeType::EnumField && child->token) {
                int val = next;
                if (child->right && child->right->node_type == ASTNodeType::IntegerLiteral && child->right->token) {
                    val = std::stoi(child->right->token->value);
                    next = val + 1;
                } else {
                    next++;
                }
                enums[name].values[child->token->value] = val;
            }
        }
    }
}

// ── Union codegen ───────────────────────────────────────────────────────────

void IRGen::genUnion(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    int max_payload = 0;
    std::vector<UnionVariant> variants;

    if (node->children) {
        for (size_t i = 0; i < node->children->size(); i++) {
            auto* child = (*node->children)[i];
            if (child->node_type != ASTNodeType::UnionField) continue;

            UnionVariant vi;
            vi.name = child->token ? child->token->value : "v" + std::to_string(i);
            vi.tag = (int)i;

            if (child->left) {
                vi.payload_type = razenType(child->left);
            } else if (child->children && !child->children->empty()) {
        std::vector<Type*> fields;
        for (auto* sf : *child->children) {
            fields.push_back(sf->left ? razenType(sf->left) : Type::getInt32Ty(ctx));
        }
        vi.payload_type = StructType::get(ctx, fields);
            } else {
                vi.payload_type = Type::getVoidTy(ctx);
            }

            int sz = (int)module.getDataLayout().getTypeAllocSize(vi.payload_type);

            if (sz > max_payload) max_payload = sz;
            variants.push_back(vi);
        }
    }

    unions[name] = {nullptr, max_payload, variants};

    auto* ll = StructType::create(ctx, {Type::getInt32Ty(ctx), ArrayType::get(Type::getInt8Ty(ctx), max_payload)}, name);
    types.structs[name] = ll;
    unions[name].ll_type = ll;
}

// ── Error set codegen ───────────────────────────────────────────────────────

void IRGen::genError(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;
    int code = 0;
    if (node->children) {
        for (auto* child : *node->children) {
            if (child->node_type == ASTNodeType::ErrorField && child->token) {
                errors[name][child->token->value] = code++;
            }
        }
    }
}

// ── External function declaration ───────────────────────────────────────────

void IRGen::genExt(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    Type* ret = Type::getInt32Ty(ctx);
    if (node->left && node->left->node_type == ASTNodeType::ReturnType && node->left->left)
        ret = resolveType(node->left->left);

    std::vector<Type*> params;
    if (node->middle && node->middle->children) {
        for (auto* p : *node->middle->children) {
            if (p->node_type == ASTNodeType::Parameter)
                params.push_back(p->left ? resolveType(p->left) : Type::getInt32Ty(ctx));
        }
    }

    auto* ft = FunctionType::get(ret, params, false);
    module.getOrInsertFunction(name, ft);
}

// ── Function codegen ────────────────────────────────────────────────────────

void IRGen::genFunc(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    Type* ret = Type::getVoidTy(ctx);
    if (node->left && node->left->node_type == ASTNodeType::ReturnType && node->left->left)
        ret = resolveType(node->left->left);
    current_ret_type = ret;

    std::vector<Type*> params;
    std::vector<std::string> param_names;
    if (node->middle && node->middle->children) {
        for (auto* p : *node->middle->children) {
            if (p->node_type == ASTNodeType::Parameter) {
                params.push_back(p->left ? resolveType(p->left) : Type::getInt32Ty(ctx));
                param_names.push_back(p->token ? p->token->value : "arg");
            }
        }
    }

    auto* ft = FunctionType::get(ret, params, false);
    auto* fn = Function::Create(ft, Function::ExternalLinkage, name, module);

    named_values.clear();
    named_types.clear();
    deferred.clear();
    has_return = false;
    current_func_name = name;

    auto* entry = BasicBlock::Create(ctx, "entry", fn);
    builder.SetInsertPoint(entry);

    unsigned idx = 0;
    bool is_method = !current_struct.empty();
    for (auto& arg : fn->args()) {
        if (idx < param_names.size()) {
            auto& pname = param_names[idx];
            if (is_method && (pname == "self" || idx == 0)) {
                named_values[pname] = builder.CreateAlloca(arg.getType(), nullptr, pname);
                named_types[pname] = arg.getType();
                builder.CreateStore(&arg, named_values[pname]);
            } else {
                createEntryBlockAlloca(fn, pname, arg.getType());
                builder.CreateStore(&arg, named_values[pname]);
            }
        }
        idx++;
    }

    if (node->right && node->right->node_type == ASTNodeType::Block)
        genBlock(node->right);

    emitDeferred();

    if (!has_return) {
        if (ret->isVoidTy())
            builder.CreateRetVoid();
        else
            builder.CreateRet(Constant::getNullValue(ret));
    }

    if (verifyFunction(*fn, &errs()))
        errs() << "Function verification failed: " << name << "\n";

    named_values.clear();
    named_types.clear();
    deferred.clear();
    current_ret_type = Type::getVoidTy(ctx);
}

// ── Variable declaration ────────────────────────────────────────────────────

void IRGen::genVar(ASTNode* node, bool is_const) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    Type* ty = node->left ? razenType(node->left) : nullptr;

    auto* current_fn = builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
    if (!current_fn) {
        if (!ty) ty = Type::getInt32Ty(ctx);
        auto* init_val = node->right ? genExpr(node->right) : nullptr;
        Constant* init_const = nullptr;
        if (init_val) {
            if (!ty) ty = init_val->getType();
            init_const = dyn_cast<Constant>(init_val);
        }
        if (!init_const) init_const = Constant::getNullValue(ty);
        new GlobalVariable(module, ty, is_const, GlobalValue::InternalLinkage, init_const, name);
        return;
    }

    if (node->right) {
        auto* init = genExpr(node->right);
        if (init) {
            if (!ty) ty = init->getType();
            if (!ty) ty = Type::getInt32Ty(ctx);
            createEntryBlockAlloca(current_fn, name, ty);
            builder.CreateStore(init, named_values[name]);
            return;
        }
    }

    if (!ty) ty = Type::getInt32Ty(ctx);
    createEntryBlockAlloca(current_fn, name, ty);

    if (!is_const && !node->right) {
        builder.CreateStore(Constant::getNullValue(ty), named_values[name]);
    }
}

// ── Block ───────────────────────────────────────────────────────────────────

void IRGen::genBlock(ASTNode* node) {
    if (!node) return;
    if (node->children) {
        for (auto* child : *node->children) {
            if (has_return) break;
            genNode(child);
        }
    }
    if (node->left && node->node_type != ASTNodeType::Block)   genNode(node->left);
    if (node->right && node->node_type != ASTNodeType::Block)  genNode(node->right);
    if (node->middle && node->node_type != ASTNodeType::Block) genNode(node->middle);
}

// ── Return ──────────────────────────────────────────────────────────────────

void IRGen::genReturn(ASTNode* node) {
    if (!node) return;
    emitDeferred();
    has_return = true;

    if (node->left) {
        auto* val = genExpr(node->left);
        if (val && current_ret_type) {
            if (val->getType() != current_ret_type) {
                if (isIntTy(val->getType()) && isIntTy(current_ret_type)) {
                    unsigned w1 = cast<IntegerType>(val->getType())->getBitWidth();
                    unsigned w2 = cast<IntegerType>(current_ret_type)->getBitWidth();
                    if (w1 < w2) val = builder.CreateSExt(val, current_ret_type);
                    else if (w1 > w2) val = builder.CreateTrunc(val, current_ret_type);
                } else {
                    val = Constant::getNullValue(current_ret_type);
                }
            }
            builder.CreateRet(val);
            return;
        }
    }
    builder.CreateRetVoid();
}

// ── If / else ───────────────────────────────────────────────────────────────

void IRGen::genIf(ASTNode* node) {
    if (!node) return;

    auto* fn = builder.GetInsertBlock()->getParent();
    auto* thenBB = BasicBlock::Create(ctx, "if.then", fn);
    auto* elseBB = BasicBlock::Create(ctx, "if.else", fn);
    auto* endBB  = BasicBlock::Create(ctx, "if.end", fn);

    auto* cond = node->left ? genExpr(node->left) : ConstantInt::getFalse(ctx);
    if (!cond) cond = ConstantInt::getFalse(ctx);
    if (cond->getType()->isIntegerTy() && cast<IntegerType>(cond->getType())->getBitWidth() != 1)
        cond = builder.CreateICmpNE(cond, ConstantInt::get(cond->getType(), 0), "if.cond");
    builder.CreateCondBr(cond, thenBB, elseBB);

    builder.SetInsertPoint(thenBB);
    bool then_ret = false;
    if (node->middle) {
        bool prev = has_return; has_return = false;
        genNode(node->middle);
        then_ret = has_return;
        has_return = prev;
    }
    if (!then_ret && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(endBB);

    builder.SetInsertPoint(elseBB);
    bool else_ret = false;
    if (node->right) {
        bool prev = has_return; has_return = false;
        if (node->right->node_type == ASTNodeType::ElseIfStatement)
            genIf(node->right);
        else
            genNode(node->right);
        else_ret = has_return;
        has_return = prev;
    }
    if (!else_ret && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(endBB);

    has_return = has_return || (then_ret && else_ret);
    builder.SetInsertPoint(endBB);
}

// ── Loop ────────────────────────────────────────────────────────────────────

void IRGen::genLoop(ASTNode* node) {
    auto* fn = builder.GetInsertBlock()->getParent();
    auto* condBB = BasicBlock::Create(ctx, "loop.cond", fn);
    auto* bodyBB = BasicBlock::Create(ctx, "loop.body", fn);
    auto* endBB  = BasicBlock::Create(ctx, "loop.end", fn);

    auto* prev_cont = loop_continue;
    auto* prev_end  = loop_end;
    loop_continue = condBB;
    loop_end = endBB;

    builder.CreateBr(condBB);
    builder.SetInsertPoint(condBB);

    if (node->left) {
        auto* cond = genExpr(node->left);
        if (!cond) cond = ConstantInt::getTrue(ctx);
        if (cond->getType()->isIntegerTy() && cast<IntegerType>(cond->getType())->getBitWidth() != 1)
            cond = builder.CreateICmpNE(cond, ConstantInt::get(cond->getType(), 0), "loop.cond");
        else if (!cond->getType()->isIntegerTy(1))
            cond = ConstantInt::getTrue(ctx);
        builder.CreateCondBr(cond, bodyBB, endBB);
    } else {
        builder.CreateBr(bodyBB);
    }

    builder.SetInsertPoint(bodyBB);
    if (node->right) genNode(node->right);
    if (!has_return && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(loop_continue);

    builder.SetInsertPoint(endBB);
    loop_continue = prev_cont;
    loop_end = prev_end;
}

// ── Assignment ──────────────────────────────────────────────────────────────

void IRGen::genAssign(ASTNode* node) {
    if (!node || !node->left || !node->token) return;
    auto* lhs = node->left;
    auto tok_type = node->token->type;

    bool compound = (tok_type == TokenType::PlusEquals ||
                     tok_type == TokenType::MinusEquals ||
                     tok_type == TokenType::StarEquals ||
                     tok_type == TokenType::SlashEquals ||
                     tok_type == TokenType::PercentEquals);

    auto doStore = [&](Value* ptr, Type* ty) {
        if (compound) {
            auto* loaded = builder.CreateLoad(ty, ptr, "load");
            auto* rhs = genExpr(node->right);
            if (!rhs) return;
            Value* result = nullptr;

            auto isFloat = [](Type* t) { return t->isHalfTy() || t->isFloatTy() || t->isDoubleTy() || t->isFP128Ty(); };
            bool fl = isFloat(ty);

            switch (tok_type) {
                case TokenType::PlusEquals:
                    result = fl ? builder.CreateFAdd(loaded, rhs) : builder.CreateAdd(loaded, rhs);
                    break;
                case TokenType::MinusEquals:
                    result = fl ? builder.CreateFSub(loaded, rhs) : builder.CreateSub(loaded, rhs);
                    break;
                case TokenType::StarEquals:
                    result = fl ? builder.CreateFMul(loaded, rhs) : builder.CreateMul(loaded, rhs);
                    break;
                case TokenType::SlashEquals:
                    result = fl ? builder.CreateFDiv(loaded, rhs) : builder.CreateSDiv(loaded, rhs);
                    break;
                case TokenType::PercentEquals:
                    result = builder.CreateSRem(loaded, rhs);
                    break;
                default: return;
            }
            builder.CreateStore(result, ptr);
        } else {
            auto* rhs = genExpr(node->right);
            if (rhs) builder.CreateStore(rhs, ptr);
        }
    };

    if (lhs->node_type == ASTNodeType::Identifier && lhs->token) {
        auto& name = lhs->token->value;
        auto it = named_values.find(name);
        if (it != named_values.end())
            doStore(it->second, named_types[name]);
        return;
    }

    if (lhs->node_type == ASTNodeType::MemberAccess) {
        if (lhs->left && lhs->left->node_type == ASTNodeType::Identifier && lhs->left->token) {
            auto& var = lhs->left->token->value;
            auto it = named_values.find(var);
            if (it != named_values.end()) {
                auto* st_ty = named_types[var];
                std::string sname;
                // name is known from context: struct name = var_name if we stored it, or from struct_methods
                for (auto& [sn, _] : types.structs) {
                    if (st_ty == types.structs[sn] ||
                        (st_ty->isPointerTy() && current_struct == sn)) {
                        sname = sn;
                        break;
                    }
                }

                std::string fname;
                if (lhs->right && lhs->right->token) fname = lhs->right->token->value;

                auto sit = types.structs.find(sname);
                if (sit == types.structs.end()) return;
                auto* st = sit->second;
                unsigned fidx = 0;
                auto fnit = types.field_names.find(sname);
                if (fnit != types.field_names.end()) {
                    auto& fnames = fnit->second;
                    for (unsigned i = 0; i < fnames.size(); i++) {
                        if (fnames[i] == fname) { fidx = i; break; }
                    }
                }

                Value* ptr = it->second;
                Type* base_ty = st;
                if (st_ty->isPointerTy()) {
                    // Load the pointer value and create alloca for it
                    auto* loaded_ptr = builder.CreateLoad(st_ty, ptr);
                    auto* tmp = builder.CreateAlloca(st);
                    builder.CreateStore(loaded_ptr, tmp);
                    ptr = tmp;
                    base_ty = st;
                }

                auto* gep = builder.CreateStructGEP(base_ty, ptr, fidx, "field");
                Type* fty = st->getElementType(fidx);

                doStore(gep, fty);
            }
        }
    }
}


// ── Match ───────────────────────────────────────────────────────────────────

void IRGen::genMatch(ASTNode* node) {
    if (!node) return;
    if (!node->children) return;
    if (node->children->empty()) return;

    auto* fn = builder.GetInsertBlock()->getParent();
    auto* endBB = BasicBlock::Create(ctx, "match.end", fn);

    auto* match_val = genExpr(node->left);
    if (!match_val) { builder.CreateBr(endBB); builder.SetInsertPoint(endBB); return; }

    Type* match_ty = match_val->getType();
    std::string match_sname;
    if (auto* st = dyn_cast<StructType>(match_ty))
        match_sname = st->getName().str();

    ASTNode* else_case = nullptr;
    std::vector<ASTNode*> cases;
    for (auto* c : *node->children) {
        if (!c->left) { else_case = c; continue; }
        cases.push_back(c);
    }

    bool is_union_match = !match_sname.empty() && unions.count(match_sname);

    if (is_union_match) {
        auto* ptr = builder.CreateAlloca(match_ty);
        builder.CreateStore(match_val, ptr);
        auto* tag_ptr = builder.CreateStructGEP(match_ty, ptr, 0, "tag.ptr");
        auto* tag = builder.CreateLoad(Type::getInt32Ty(ctx), tag_ptr, "tag");

        BasicBlock* currentBB = nullptr;
        for (size_t i = 0; i < cases.size(); i++) {
            auto* caseBB = BasicBlock::Create(ctx, "match.case", fn);
            auto* nextBB = (i == cases.size() - 1 && else_case)
                ? BasicBlock::Create(ctx, "match.else", fn)
                : (i == cases.size() - 1 ? endBB : BasicBlock::Create(ctx, "match.next", fn));

            if (!currentBB) {
                auto* checkBB = BasicBlock::Create(ctx, "match.check", fn);
                if (builder.GetInsertBlock() && !builder.GetInsertBlock()->getTerminator())
                    builder.CreateBr(checkBB);
                builder.SetInsertPoint(checkBB);
                currentBB = checkBB;
            }

            builder.SetInsertPoint(currentBB);
            int expected = unions[match_sname].variants[i].tag;
            auto* cmp = builder.CreateICmpEQ(tag, ConstantInt::get(Type::getInt32Ty(ctx), expected));
            builder.CreateCondBr(cmp, caseBB, nextBB);

            builder.SetInsertPoint(caseBB);
            auto& vi = unions[match_sname].variants[i];
            if (vi.payload_type && !vi.payload_type->isVoidTy()) {
                auto* payload_ptr = builder.CreateStructGEP(match_ty, ptr, 1, "payload.ptr");
                auto* casted = builder.CreateBitCast(payload_ptr, PointerType::getUnqual(vi.payload_type));
                auto* payload = builder.CreateLoad(vi.payload_type, casted, "payload");

                if (cases[i]->left->right && cases[i]->left->right->children &&
                    !cases[i]->left->right->children->empty()) {
                    auto* arg = (*cases[i]->left->right->children)[0];
                    if (arg->left && arg->left->node_type == ASTNodeType::Identifier && arg->left->token) {
                        auto* alloc = builder.CreateAlloca(vi.payload_type);
                        builder.CreateStore(payload, alloc);
                        named_values[arg->left->token->value] = alloc;
                        named_types[arg->left->token->value] = vi.payload_type;
                    }
                }
            }

            if (cases[i]->right) genNode(cases[i]->right);
            if (!has_return && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(endBB);
            currentBB = nextBB;
        }

        if (else_case) {
            builder.SetInsertPoint(currentBB);
            if (else_case->right) genNode(else_case->right);
            if (!has_return && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(endBB);
        }
    } else {
        auto* prevBB = builder.GetInsertBlock();
        auto* checkBB = BasicBlock::Create(ctx, "match.check", fn);
        if (prevBB && !prevBB->getTerminator()) builder.CreateBr(checkBB);
        builder.SetInsertPoint(checkBB);

        for (size_t i = 0; i < cases.size(); i++) {
            auto* caseBB = BasicBlock::Create(ctx, "match.case", fn);
            BasicBlock* nextBB = (i == cases.size() - 1)
                ? (else_case ? BasicBlock::Create(ctx, "match.else", fn) : endBB)
                : BasicBlock::Create(ctx, "match.next", fn);

            auto* pat_val = genExpr(cases[i]->left);
            if (!pat_val) pat_val = ConstantInt::get(Type::getInt32Ty(ctx), 0);

            auto* cmp = builder.CreateICmpEQ(match_val, pat_val);
            builder.CreateCondBr(cmp, caseBB, nextBB);

            builder.SetInsertPoint(caseBB);
            if (cases[i]->right) genNode(cases[i]->right);
            if (!has_return && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(endBB);

            if (nextBB != endBB) builder.SetInsertPoint(nextBB);
        }

        if (else_case) {
            auto* currBB = builder.GetInsertBlock();
            if (currBB && !currBB->getTerminator()) {
                if (else_case->right) genNode(else_case->right);
                if (!has_return && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(endBB);
            }
        }
    }

    builder.SetInsertPoint(endBB);
}

// ── Deferred flush ──────────────────────────────────────────────────────────

void IRGen::emitDeferred() {
    for (auto it = deferred.rbegin(); it != deferred.rend(); ++it) {
        genNode(*it);
    }
    deferred.clear();
}

// ── Expression dispatch ─────────────────────────────────────────────────────

Value* IRGen::genNode(ASTNode* node) {
    if (!node) return nullptr;
    switch (node->node_type) {
        case ASTNodeType::VarDeclaration:
        case ASTNodeType::ConstDeclaration:
            genVar(node, node->node_type == ASTNodeType::ConstDeclaration);
            return nullptr;
        case ASTNodeType::ReturnStatement:  genReturn(node); return nullptr;
        case ASTNodeType::Assignment:       genAssign(node); return nullptr;
        case ASTNodeType::BreakStatement:
            if (loop_end) builder.CreateBr(loop_end);
            return nullptr;
        case ASTNodeType::SkipStatement:
            if (loop_continue) builder.CreateBr(loop_continue);
            return nullptr;
        case ASTNodeType::DeferStatement:
            if (node->left) deferred.push_back(node->left);
            return nullptr;
        case ASTNodeType::IfStatement:
        case ASTNodeType::ElseIfStatement:
            genIf(node); return nullptr;
        case ASTNodeType::LoopStatement:    genLoop(node); return nullptr;
        case ASTNodeType::MatchStatement:   genMatch(node); return nullptr;
        case ASTNodeType::Block:
        case ASTNodeType::IfBody:
        case ASTNodeType::ElseBody:
        case ASTNodeType::LoopBody:
        case ASTNodeType::MatchBody:        genBlock(node); return nullptr;
        case ASTNodeType::TryBlock:
        case ASTNodeType::TryExpression:
            return genTryExpr(node);
        case ASTNodeType::CatchExpression:
            if (node->left) return genNode(node->left);
            return nullptr;
        default:
            return genExpr(node);
    }
}

Value* IRGen::genExpr(ASTNode* node) {
    if (!node) return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    switch (node->node_type) {
        case ASTNodeType::IntegerLiteral:
        case ASTNodeType::FloatLiteral:
        case ASTNodeType::BoolLiteral:
        case ASTNodeType::CharLiteral:
        case ASTNodeType::StringLiteral:
            return genLiteral(node);
        case ASTNodeType::Identifier:       return genIdentifier(node);
        case ASTNodeType::BinaryExpression: return genBinary(node);
        case ASTNodeType::UnaryExpression:  return genUnary(node);
        case ASTNodeType::FunctionCall:     return genCall(node);
        case ASTNodeType::MemberAccess:     return genMemberAccess(node);
        case ASTNodeType::ArrayLiteral:     return genArrayLiteral(node);
        case ASTNodeType::TupleLiteral:     return genTupleLiteral(node);
        case ASTNodeType::RangeExpression:  return genRangeLiteral(node);
        default:                            return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    }
}

// ── Literals ────────────────────────────────────────────────────────────────

Value* IRGen::genLiteral(ASTNode* node) {
    if (!node || !node->token) return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    auto& val = node->token->value;

    switch (node->node_type) {
        case ASTNodeType::IntegerLiteral:
            return ConstantInt::get(Type::getInt32Ty(ctx), std::stoll(val));
        case ASTNodeType::FloatLiteral:
            return ConstantFP::get(Type::getDoubleTy(ctx), val);
        case ASTNodeType::BoolLiteral:
            return val == "true" ? ConstantInt::getTrue(ctx) : ConstantInt::getFalse(ctx);
        case ASTNodeType::CharLiteral:
            return ConstantInt::get(Type::getInt8Ty(ctx), val.empty() ? 0 : (unsigned char)val[0]);
        case ASTNodeType::StringLiteral: {
            auto* gv = builder.CreateGlobalString(val, ".str");
            auto* zero = ConstantInt::get(Type::getInt32Ty(ctx), 0);
            return builder.CreateGEP(gv->getValueType(), gv, {zero, zero}, "str.ptr");
        }
        default:
            return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    }
}

// ── Identifier ──────────────────────────────────────────────────────────────

Value* IRGen::genIdentifier(ASTNode* node) {
    if (!node || !node->token) return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    auto& name = node->token->value;

    auto it = named_values.find(name);
    if (it != named_values.end()) {
        return builder.CreateLoad(named_types[name], it->second, name.c_str());
    }

    auto eit = enums.find(name);
    if (eit != enums.end()) {
        return nullptr;
    }

    if (name == "null") {
        return Constant::getNullValue(current_ret_type ? current_ret_type : Type::getInt32Ty(ctx));
    }

    auto fn = module.getFunction(name);
    if (fn) return fn;

    return ConstantInt::get(Type::getInt32Ty(ctx), 0);
}

// ── Binary operations ───────────────────────────────────────────────────────

Value* IRGen::genBinary(ASTNode* node) {
    if (!node) return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    auto* lhs = genExpr(node->left);
    auto* rhs = genExpr(node->right);
    if (!lhs || !rhs) return lhs ? lhs : rhs;

    auto tok = node->token;
    if (!tok) return lhs;

    Type* ty = lhs->getType();
    if (rhs->getType() != ty) {
        if (isIntTy(ty) && isIntTy(rhs->getType())) {
            auto w1 = cast<IntegerType>(ty)->getBitWidth();
            auto w2 = cast<IntegerType>(rhs->getType())->getBitWidth();
            if (w1 < w2) lhs = builder.CreateSExt(lhs, rhs->getType());
            else if (w1 > w2) rhs = builder.CreateSExt(rhs, ty);
            ty = lhs->getType();
        }
    }

    auto isFl = [](Type* t) { return t->isHalfTy() || t->isFloatTy() || t->isDoubleTy() || t->isFP128Ty(); };
    bool fl = isFl(ty);

    switch (tok->type) {
        case TokenType::Plus:
            return fl ? builder.CreateFAdd(lhs, rhs) : builder.CreateAdd(lhs, rhs);
        case TokenType::Minus:
            return fl ? builder.CreateFSub(lhs, rhs) : builder.CreateSub(lhs, rhs);
        case TokenType::Star:
            return fl ? builder.CreateFMul(lhs, rhs) : builder.CreateMul(lhs, rhs);
        case TokenType::Slash:
            return fl ? builder.CreateFDiv(lhs, rhs) : builder.CreateSDiv(lhs, rhs);
        case TokenType::Percent:
            return builder.CreateSRem(lhs, rhs);

        case TokenType::EqualsEquals:
            return fl ? builder.CreateFCmpOEQ(lhs, rhs) : builder.CreateICmpEQ(lhs, rhs);
        case TokenType::NotEquals:
            return fl ? builder.CreateFCmpUNE(lhs, rhs) : builder.CreateICmpNE(lhs, rhs);
        case TokenType::LessThan:
            return fl ? builder.CreateFCmpOLT(lhs, rhs) : builder.CreateICmpSLT(lhs, rhs);
        case TokenType::LessThanEquals:
            return fl ? builder.CreateFCmpOLE(lhs, rhs) : builder.CreateICmpSLE(lhs, rhs);
        case TokenType::GreaterThan:
            return fl ? builder.CreateFCmpOGT(lhs, rhs) : builder.CreateICmpSGT(lhs, rhs);
        case TokenType::GreaterThanEquals:
            return fl ? builder.CreateFCmpOGE(lhs, rhs) : builder.CreateICmpSGE(lhs, rhs);

        case TokenType::AndAnd:
            return builder.CreateAnd(lhs, rhs);
        case TokenType::OrOr:
            return builder.CreateOr(lhs, rhs);
        case TokenType::And:
            return builder.CreateAnd(lhs, rhs);
        case TokenType::Or:
            return builder.CreateOr(lhs, rhs);
        case TokenType::Caret:
            return builder.CreateXor(lhs, rhs);
        case TokenType::ShiftLeft:
            return builder.CreateShl(lhs, rhs);
        case TokenType::ShiftRight:
            return builder.CreateAShr(lhs, rhs);

        default:
            return lhs;
    }
}

// ── Unary operations ────────────────────────────────────────────────────────

Value* IRGen::genUnary(ASTNode* node) {
    if (!node || !node->token) return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    auto& op = node->token->value;
    auto* inner = genExpr(node->left);
    if (!inner) return ConstantInt::get(Type::getInt32Ty(ctx), 0);

    auto isFl = [](Type* t) { return t->isHalfTy() || t->isFloatTy() || t->isDoubleTy() || t->isFP128Ty(); };

    if (op == "-") {
        return isFl(inner->getType()) ? builder.CreateFNeg(inner) : builder.CreateNeg(inner);
    }
    if (op == "!") {
        return builder.CreateXor(inner, ConstantInt::get(inner->getType(), 1), "not");
    }
    if (op == "~") {
        return builder.CreateXor(inner, ConstantInt::getAllOnesValue(inner->getType()), "bitnot");
    }
    if (op == "&") {
        if (node->left && node->left->node_type == ASTNodeType::Identifier && node->left->token) {
            auto it = named_values.find(node->left->token->value);
            if (it != named_values.end()) return it->second;
        }
        return Constant::getNullValue(PointerType::getUnqual(ctx));
    }
    if (op == ".*") {
        Type* load_ty = Type::getInt32Ty(ctx);
        if (auto* ai = dyn_cast<AllocaInst>(inner))
            load_ty = ai->getAllocatedType();
        else if (auto* li = dyn_cast<LoadInst>(inner))
            load_ty = li->getType();
        return builder.CreateLoad(load_ty, inner, "deref");
    }
    return inner;
}

// ── Function call ───────────────────────────────────────────────────────────

Value* IRGen::genCall(ASTNode* node) {
    if (!node || !node->token) return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    auto& name = node->token->value;

    std::vector<Value*> args;
    if (node->children) {
        for (auto* arg : *node->children) {
            if (arg->left) {
                auto* ev = genExpr(arg->left);
                if (ev) args.push_back(ev);
            }
        }
    }

    auto* fn = module.getFunction(name);
    if (!fn) {
        errs() << "Error: call to undeclared function '" << name << "'\n";
        return Constant::getNullValue(Type::getInt32Ty(ctx));
    }

    std::vector<Value*> call_args;
    for (size_t i = 0; i < args.size() && i < fn->arg_size(); i++) {
        auto* expected = fn->getArg(i)->getType();
        if (args[i]->getType() != expected) {
            if (isIntTy(args[i]->getType()) && isIntTy(expected)) {
                auto w1 = cast<IntegerType>(args[i]->getType())->getBitWidth();
                auto w2 = cast<IntegerType>(expected)->getBitWidth();
                if (w1 < w2) call_args.push_back(builder.CreateSExt(args[i], expected));
                else if (w1 > w2) call_args.push_back(builder.CreateTrunc(args[i], expected));
                else call_args.push_back(args[i]);
            } else {
                call_args.push_back(args[i]);
            }
        } else {
            call_args.push_back(args[i]);
        }
    }

    Value* ret = fn->getReturnType()->isVoidTy()
        ? builder.CreateCall(fn, call_args)
        : builder.CreateCall(fn, call_args, "call");
    if (ret->getType()->isVoidTy()) return Constant::getNullValue(Type::getInt32Ty(ctx));
    return ret;
}

// ── Member access ───────────────────────────────────────────────────────────

Value* IRGen::genMemberAccess(ASTNode* node) {
    if (!node) return ConstantInt::get(Type::getInt32Ty(ctx), 0);

    if (node->left && node->left->node_type == ASTNodeType::Identifier && node->left->token &&
        node->right && node->right->node_type == ASTNodeType::Identifier && node->right->token) {
        auto& left = node->left->token->value;
        auto& right = node->right->token->value;

        auto eit = enums.find(left);
        if (eit != enums.end()) {
            auto vit = eit->second.values.find(right);
            if (vit != eit->second.values.end()) {
                return ConstantInt::get(eit->second.backing, vit->second);
            }
        }

        auto erit = errors.find(left);
        if (erit != errors.end()) {
            auto vit = erit->second.find(right);
            if (vit != erit->second.end()) {
                return ConstantInt::get(Type::getInt32Ty(ctx), vit->second);
            }
        }
    }

    if (node->left && node->left->node_type == ASTNodeType::Identifier && node->left->token &&
        node->right && node->right->node_type == ASTNodeType::FunctionCall) {
        auto& var = node->left->token->value;
        auto it = named_values.find(var);
        if (it != named_values.end()) {
            Type* st_ty = named_types[var];
            std::string sname;
            for (auto& [sn, _] : types.structs) {
                if (st_ty == types.structs[sn] ||
                    (st_ty->isPointerTy() && current_struct == sn)) {
                    sname = sn;
                    break;
                }
            }
            if (sname.empty()) {
                // fallback: try to match by struct_methods keys
                for (auto& [sn, _] : struct_methods) {
                    if (var.find(sn) == 0 || current_struct == sn) {
                        sname = sn;
                        break;
                    }
                }
            }

            auto mit = struct_methods.find(sname);
            if (mit != struct_methods.end()) {
                auto& mname = node->right->token->value;
                std::string mangled = sname + "." + mname;
                auto* mfn = module.getFunction(mangled);
                if (!mfn) {
                    std::vector<Type*> at;
                    at.push_back(st_ty);
                    if (node->right->children) {
                        for (auto* a : *node->right->children) {
                            if (a->left) at.push_back(genExpr(a->left)->getType());
                        }
                    }
                    auto* ft = FunctionType::get(Type::getVoidTy(ctx), at, false);
                    mfn = Function::Create(ft, Function::ExternalLinkage, mangled, module);
                }

                std::vector<Value*> args;
                args.push_back(it->second);
                if (node->right->children) {
                    for (auto* a : *node->right->children) {
                        if (a->left) args.push_back(genExpr(a->left));
                    }
                }
                auto* ret = builder.CreateCall(mfn, args);
                if (ret->getType()->isVoidTy()) return Constant::getNullValue(Type::getInt32Ty(ctx));
                return ret;
            }
        }
    }

    if (node->left && node->left->node_type == ASTNodeType::Identifier && node->left->token) {
        auto& var = node->left->token->value;
        auto it = named_values.find(var);
        if (it != named_values.end()) {
            Type* st_ty = named_types[var];
            bool is_ptr = st_ty->isPointerTy();

            std::string sname;
            for (auto& [sn, _] : types.structs) {
                if (st_ty == types.structs[sn]) {
                    sname = sn;
                    break;
                }
            }
            if (sname.empty() && is_ptr && !current_struct.empty()) {
                sname = current_struct;
            }
            auto sit = types.structs.find(sname);
            if (sit == types.structs.end()) goto union_path;
            auto* base_ty = sit->second;

            std::string fname;
            if (node->right && node->right->token) fname = node->right->token->value;

            unsigned fidx = 0;
            auto fnit = types.field_names.find(sname);
            if (fnit != types.field_names.end()) {
                auto& fnames = fnit->second;
                for (unsigned i = 0; i < fnames.size(); i++) {
                    if (fnames[i] == fname) { fidx = i; break; }
                }
            }

            Value* ptr = it->second;
            if (!is_ptr) {
                auto* loaded = builder.CreateLoad(base_ty, ptr);
                auto* alloc = builder.CreateAlloca(base_ty);
                builder.CreateStore(loaded, alloc);
                ptr = alloc;
            }

            auto* gep = builder.CreateStructGEP(base_ty, ptr, fidx, "field");
            Type* fty = base_ty->getElementType(fidx);

            return builder.CreateLoad(fty, gep, fname.c_str());
        }
    }

union_path:
    if (node->left && node->left->node_type == ASTNodeType::Identifier && node->left->token &&
        node->right && node->right->node_type == ASTNodeType::FunctionCall) {
        auto& uname = node->left->token->value;
        if (unions.count(uname)) return genUnionConstruct(node);
    }

    return ConstantInt::get(Type::getInt32Ty(ctx), 0);
}

// ── Array literal ───────────────────────────────────────────────────────────

Value* IRGen::genArrayLiteral(ASTNode* node) {
    if (!node || !node->children || node->children->empty())
        return ConstantArray::get(ArrayType::get(Type::getInt32Ty(ctx), 0), {});

    Type* elem = Type::getInt32Ty(ctx);
    if ((*node->children)[0]->left) {
        auto* first = genExpr((*node->children)[0]->left);
        if (first) elem = first->getType();
    }

    std::vector<Constant*> elems;
    bool all_constant = true;
    for (auto* child : *node->children) {
        if (child->left) {
            auto* v = genExpr(child->left);
            if (v) {
                if (auto* cv = dyn_cast<Constant>(v)) {
                    elems.push_back(cv);
                } else {
                    all_constant = false;
                    elems.push_back(Constant::getNullValue(elem));
                }
            } else {
                elems.push_back(Constant::getNullValue(elem));
            }
        }
    }

    if (all_constant)
        return ConstantArray::get(ArrayType::get(elem, elems.size()), elems);

    auto* arr_ty = ArrayType::get(elem, elems.size());
    auto* alloc = builder.CreateAlloca(arr_ty);
    for (size_t i = 0; i < elems.size(); i++) {
        auto* gep = builder.CreateGEP(arr_ty, alloc, {ConstantInt::get(Type::getInt32Ty(ctx), 0),
                                                       ConstantInt::get(Type::getInt32Ty(ctx), (unsigned)i)});
        builder.CreateStore(elems[i], gep);
    }
    return builder.CreateLoad(arr_ty, alloc);
}

// ── Tuple literal ───────────────────────────────────────────────────────────

Value* IRGen::genTupleLiteral(ASTNode* node) {
    if (!node || !node->children || node->children->empty()) {
        auto* ty = StructType::get(ctx, ArrayRef<Type*>({Type::getInt32Ty(ctx)}));
        SmallVector<Constant*, 1> empty = {ConstantInt::get(Type::getInt32Ty(ctx), 0)};
        return ConstantStruct::get(ty, empty);
    }

    std::vector<Type*> types;
    std::vector<Constant*> values;
    bool all_constant = true;
    for (auto* child : *node->children) {
        if (child->left) {
            auto* v = genExpr(child->left);
            if (v) {
                types.push_back(v->getType());
                if (auto* cv = dyn_cast<Constant>(v)) {
                    values.push_back(cv);
                } else {
                    all_constant = false;
                    values.push_back(Constant::getNullValue(v->getType()));
                }
            } else {
                types.push_back(Type::getInt32Ty(ctx));
                values.push_back(ConstantInt::get(Type::getInt32Ty(ctx), 0));
            }
        }
    }

    if (all_constant)
        return ConstantStruct::get(StructType::get(ctx, types), values);

    auto* sty = StructType::get(ctx, types);
    auto* alloc = builder.CreateAlloca(sty);
    for (size_t i = 0; i < types.size(); i++) {
        auto* gep = builder.CreateStructGEP(sty, alloc, i);
        builder.CreateStore(values[i], gep);
    }
    return builder.CreateLoad(sty, alloc);
}

// ── Range literal ───────────────────────────────────────────────────────────

Value* IRGen::genRangeLiteral(ASTNode* node) {
    auto* l = node->left ? genExpr(node->left) : ConstantInt::get(Type::getInt32Ty(ctx), 0);
    auto* r = node->right ? genExpr(node->right) : ConstantInt::get(Type::getInt32Ty(ctx), 0);
    auto* range_ty = StructType::get(ctx, ArrayRef<Type*>({l->getType(), r->getType()}));
    auto* lc = dyn_cast<Constant>(l);
    auto* rc = dyn_cast<Constant>(r);
    if (lc && rc)
        return ConstantStruct::get(range_ty, {lc, rc});
    auto* alloc = builder.CreateAlloca(range_ty);
    auto* gep0 = builder.CreateStructGEP(range_ty, alloc, 0);
    auto* gep1 = builder.CreateStructGEP(range_ty, alloc, 1);
    builder.CreateStore(l, gep0);
    builder.CreateStore(r, gep1);
    return builder.CreateLoad(range_ty, alloc);
}

// ── Try expression ──────────────────────────────────────────────────────────

Value* IRGen::genTryExpr(ASTNode* node) {
    if (!node || !node->left) return ConstantInt::get(Type::getInt32Ty(ctx), 0);

    auto* fn = builder.GetInsertBlock()->getParent();
    auto* call = genExpr(node->left);
    if (!call) return ConstantInt::get(Type::getInt32Ty(ctx), 0);

    auto* call_ty = call->getType();
    auto* st = dyn_cast<StructType>(call_ty);
    if (!st || st->getNumElements() < 2 || !st->getElementType(0)->isIntegerTy(1)) {
        return call;
    }

    auto* successBB = BasicBlock::Create(ctx, "try.success", fn);
    auto* endBB = BasicBlock::Create(ctx, "try.end", fn);

    auto* flag = builder.CreateExtractValue(call, 0, "err.flag");

    if (node->right && node->right->node_type == ASTNodeType::CatchExpression) {
        auto* catchBB = BasicBlock::Create(ctx, "try.catch", fn);
        builder.CreateCondBr(flag, catchBB, successBB);

        builder.SetInsertPoint(successBB);
        builder.CreateExtractValue(call, 1, "ok.val");
        builder.CreateBr(endBB);

        builder.SetInsertPoint(catchBB);
        if (node->right->left) genNode(node->right->left);
        builder.CreateBr(endBB);
    } else {
        auto* propBB = BasicBlock::Create(ctx, "try.propagate", fn);
        builder.CreateCondBr(flag, propBB, successBB);

        builder.SetInsertPoint(propBB);
        builder.CreateRet(call);
        has_return = true;

        builder.SetInsertPoint(successBB);
    }

    builder.SetInsertPoint(endBB);
    return builder.CreateExtractValue(call, 1, "try.val");
}

// ── Union constructor ───────────────────────────────────────────────────────

Value* IRGen::genUnionConstruct(ASTNode* node) {
    if (!node || !node->left || !node->right) return Constant::getNullValue(Type::getInt32Ty(ctx));

    auto& uname = node->left->token->value;
    auto* call = node->right;
    auto& vname = call->token->value;

    auto uit = unions.find(uname);
    if (uit == unions.end()) return Constant::getNullValue(Type::getInt32Ty(ctx));

    UnionVariant* vi = nullptr;
    for (auto& v : uit->second.variants) {
        if (v.name == vname) { vi = &v; break; }
    }
    if (!vi) return Constant::getNullValue(Type::getInt32Ty(ctx));

    auto* union_ty = uit->second.ll_type;
    auto* alloc = builder.CreateAlloca(union_ty);

    auto* tag_ptr = builder.CreateStructGEP(union_ty, alloc, 0, "tag.ptr");
    builder.CreateStore(ConstantInt::get(Type::getInt32Ty(ctx), vi->tag), tag_ptr);

    if (vi->payload_type && !vi->payload_type->isVoidTy() &&
        call->children && !call->children->empty()) {
        auto* arg = (*call->children)[0];
        if (arg->left) {
            auto* val = genExpr(arg->left);
            if (val) {
                auto* payload_ptr = builder.CreateStructGEP(union_ty, alloc, 1, "payload.ptr");
                auto* casted = builder.CreateBitCast(payload_ptr, PointerType::getUnqual(vi->payload_type));
                builder.CreateStore(val, casted);
            }
        }
    }

    return builder.CreateLoad(union_ty, alloc, "union.val");
}

} // namespace codegen
} // namespace razen
