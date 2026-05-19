#include "codegen.h"
#include "../ast/ast_utils.h"
#include "../ast/token_utils.h"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
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
    auto* int1 = Type::getInt1Ty(ctx);
    auto* int8 = Type::getInt8Ty(ctx);
    auto* int64 = Type::getInt64Ty(ctx);
    auto* floatTy = Type::getFloatTy(ctx);
    auto* doubleTy = Type::getDoubleTy(ctx);

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
    // Razen runtime library (fmt.cpp)
    {
        mod.getOrInsertFunction("__razen_print_str", FunctionType::get(voidTy, {i8ptr}, false));
        mod.getOrInsertFunction("__razen_print_str_newline", FunctionType::get(voidTy, {i8ptr}, false));
        mod.getOrInsertFunction("__razen_print_char", FunctionType::get(voidTy, {int8}, false));
        mod.getOrInsertFunction("__razen_print_bool", FunctionType::get(voidTy, {int1}, false));
        mod.getOrInsertFunction("__razen_print_int", FunctionType::get(voidTy, {int32}, false));
        mod.getOrInsertFunction("__razen_print_uint", FunctionType::get(voidTy, {int32}, false));
        mod.getOrInsertFunction("__razen_print_int64", FunctionType::get(voidTy, {int64}, false));
        mod.getOrInsertFunction("__razen_print_uint64", FunctionType::get(voidTy, {int64}, false));
        mod.getOrInsertFunction("__razen_print_float", FunctionType::get(voidTy, {floatTy}, false));
        mod.getOrInsertFunction("__razen_print_double", FunctionType::get(voidTy, {doubleTy}, false));
        mod.getOrInsertFunction("__razen_print_ptr", FunctionType::get(voidTy, {i8ptr}, false));
        mod.getOrInsertFunction("__razen_print_newline", FunctionType::get(voidTy, false));
    }

    // Pass 1: collect type declarations (structs, enums, unions, errors, behaviours)
    for (auto* node : nodes) {
        if (!node) continue;
        switch (node->node_type) {
            case ASTNodeType::StructDeclaration:  ir.genStruct(node); break;
            case ASTNodeType::EnumDeclaration:    ir.genEnum(node);   break;
            case ASTNodeType::UnionDeclaration:   ir.genUnion(node);  break;
            case ASTNodeType::ErrorDeclaration:   ir.genError(node);  break;
            case ASTNodeType::BehaveDeclaration:  ir.genBehave(node); break;
            default: break;
        }
    }

    // Pass 1.5: generate vtable types and instances for trait implementations
    for (auto* node : nodes) {
        if (!node) continue;
        if (node->node_type == ASTNodeType::StructDeclaration ||
            node->node_type == ASTNodeType::UnionDeclaration) {
            ir.genTraitVTable(node);
        }
    }

    // Pass 1.75: emit struct method bodies with mangled names
    // Must run before general codegen so calls from main() find them.
    for (auto& [sname, methods] : ir.struct_methods) {
        ir.current_struct = sname;
        for (auto* meth : methods) {
            if (meth) ir.genFunc(meth);
        }
    }
    ir.current_struct.clear();

    // Pass 2: emit function declarations (ext func) for forward references
    for (auto* node : nodes) {
        if (node && node->node_type == ASTNodeType::ExtDeclaration) {
            ir.genExt(node);
        }
    }

    // Pass 3: emit function definitions, globals, and type aliases
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
            case ASTNodeType::TypeAliasDeclaration:
                if (node->token && node->left)
                    ir.types.aliases[node->token->value] = ir.razenType(node->left);
                break;
            default: break;
        }
    }

    ir.emitGlobalCtors();

    verifyModule(mod, &errs());
}

// ── Global constructor emission ─────────────────────────────────────────────

void IRGen::emitGlobalCtors() {
    if (pending_global_inits.empty()) return;

    auto* voidTy = Type::getVoidTy(ctx);
    auto* fn = Function::Create(
        FunctionType::get(voidTy, false),
        Function::InternalLinkage, "__raz_global_init", module);
    auto* bb = BasicBlock::Create(ctx, "entry", fn);
    builder.SetInsertPoint(bb);

    for (auto& [gv, init_node] : pending_global_inits) {
        auto* val = genExpr(init_node);
        if (val) builder.CreateStore(val, gv);
    }

    builder.CreateRetVoid();
    appendToGlobalCtors(module, fn, 65535);
    pending_global_inits.clear();
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
            if (node->token && node->left)
                types.aliases[node->token->value] = razenType(node->left);
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

    // Extract trait implementations from ~> (Identifier nodes before brace)
    struct_traits[name].clear();
    if (node->children) {
        for (auto* child : *node->children) {
            if (child->node_type == ASTNodeType::Identifier) {
                if (child->token) struct_traits[name].insert(child->token->value);
            }
        }
    }

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

// ── Behaviour declaration ──────────────────────────────────────────────────

void IRGen::genBehave(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;
    if (vtables.count(name)) return;

    VTableInfo vi;
    vi.trait_name = name;

    if (node->children) {
        // Collect method signatures from behaviour
        for (auto* child : *node->children) {
            if (child->node_type == ASTNodeType::FunctionDeclaration && child->token) {
                VTableMethodInfo mi;
                mi.name = child->token->value;
                mi.idx = vi.methods.size();

                // Build function type from parameter types
                Type* ret = Type::getVoidTy(ctx);
                if (child->left && child->left->node_type == ASTNodeType::ReturnType && child->left->left)
                    ret = resolveType(child->left->left);

                std::vector<Type*> param_types;
                if (child->middle && child->middle->children) {
                    for (auto* p : *child->middle->children) {
                        if (p->node_type == ASTNodeType::Parameter) {
                            param_types.push_back(p->left ? resolveType(p->left) : Type::getInt32Ty(ctx));
                        }
                    }
                }
                mi.fn_type = FunctionType::get(ret, param_types, false);
                vi.methods.push_back(mi);
            }
        }
    }

    // Create vtable struct type for this behaviour
    if (!vi.methods.empty()) {
        std::vector<Type*> fn_ptr_types;
        for (auto& m : vi.methods) {
            fn_ptr_types.push_back(PointerType::getUnqual(m.fn_type));
        }
        vi.vtable_type = StructType::create(ctx, fn_ptr_types, name + ".vtable");
    }

    vtables[name] = vi;
}

// ── Trait vtable instance generation ──────────────────────────────────────

void IRGen::genTraitVTable(ASTNode* node) {
    if (!node || !node->token) return;
    auto& struct_name = node->token->value;

    struct_vtables[struct_name].clear();

    // Find which traits this struct claims to implement
    auto tit = struct_traits.find(struct_name);
    if (tit == struct_traits.end()) return;

    for (auto& trait_name : tit->second) {
        auto vit = vtables.find(trait_name);
        if (vit == vtables.end()) continue;

        auto& vi = vit->second;
        if (vi.methods.empty()) continue;

        // For each method in the trait, find the matching method in the struct
        std::vector<Constant*> fn_ptrs;
        for (auto& mi : vi.methods) {
            // Look for a method with matching name in struct_methods
            auto smit = struct_methods.find(struct_name);
            Constant* fn_ptr = Constant::getNullValue(PointerType::getUnqual(ctx));

            if (smit != struct_methods.end()) {
                for (auto* method : smit->second) {
                    if (method->token && method->token->value == mi.name) {
                        std::string mangled = struct_name + "." + mi.name;
                        auto* fn = module.getFunction(mangled);
                        if (fn) {
                            fn_ptr = ConstantExpr::getBitCast(fn, PointerType::getUnqual(mi.fn_type));
                        }
                        break;
                    }
                }
            }

            fn_ptrs.push_back(fn_ptr);
        }

        // Create global vtable instance
        auto* gv = new GlobalVariable(
            module, vi.vtable_type, true, GlobalValue::InternalLinkage,
            ConstantStruct::get(vi.vtable_type, fn_ptrs),
            struct_name + "." + trait_name + ".vtable");
        struct_vtables[struct_name][trait_name] = gv;
    }
}

// ── Trait method call dispatch ────────────────────────────────────────────

Value* IRGen::genTraitMethodCall(ASTNode* node, const std::string& struct_name,
                                 const std::string& trait_name, const std::string& method_name,
                                 Value* self_ptr, ASTNode* call_node) {
    // Look up the vtable global
    auto svit = struct_vtables.find(struct_name);
    if (svit == struct_vtables.end()) return nullptr;
    auto tvit = svit->second.find(trait_name);
    if (tvit == svit->second.end()) return nullptr;

    auto vit = vtables.find(trait_name);
    if (vit == vtables.end()) return nullptr;

    auto* gv = tvit->second;

    // Find method index
    unsigned idx = 0;
    bool found = false;
    for (auto& m : vit->second.methods) {
        if (m.name == method_name) { found = true; break; }
        idx++;
    }
    if (!found) return nullptr;

    // Load function pointer from vtable
    auto* zero = ConstantInt::get(Type::getInt32Ty(ctx), 0);
    auto* idx_val = ConstantInt::get(Type::getInt32Ty(ctx), idx);
    auto* gep = builder.CreateGEP(gv->getValueType(), gv, {zero, idx_val}, "vtable.gep");
    auto* fn_ptr = builder.CreateLoad(PointerType::getUnqual(ctx), gep, "vtable.fn");

    // Collect args
    std::vector<Value*> args;
    // self pointer is already provided
    if (self_ptr) args.push_back(self_ptr);

    // Add method call args
    if (call_node && call_node->children) {
        for (auto* a : *call_node->children) {
            if (a->left) {
                auto* arg_val = genExpr(a->left);
                if (arg_val) args.push_back(arg_val);
            }
        }
    }

    // Call through vtable
    auto* ret = builder.CreateCall(vit->second.methods[idx].fn_type, fn_ptr, args, "trait.call");
    if (ret->getType()->isVoidTy())
        return Constant::getNullValue(Type::getInt32Ty(ctx));
    return ret;
}

void IRGen::genExt(ASTNode* node) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    Type* ret = Type::getInt32Ty(ctx);
    if (node->left && node->left->node_type == ASTNodeType::ReturnType && node->left->left)
        ret = resolveType(node->left->left);

    std::vector<Type*> params;
    bool is_vararg = false;
    if (node->middle && node->middle->children) {
        for (auto* p : *node->middle->children) {
            if (p->node_type == ASTNodeType::Parameter) {
                if (p->token && p->token->type == TokenType::DotDotDot) {
                    is_vararg = true;
                    continue;
                }
                params.push_back(p->left ? resolveType(p->left) : Type::getInt32Ty(ctx));
            }
        }
    }

    auto* ft = FunctionType::get(ret, params, is_vararg);
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
    std::string func_name = current_struct.empty() ? name : (current_struct + "." + name);
    auto* fn = Function::Create(ft, Function::ExternalLinkage, func_name, module);

    named_values.clear();
    named_types.clear();
    unsigned_vars.clear();
    char_vars.clear();
    deferred.clear();
    has_return = false;
    current_func_name = name;
    current_llvm_function = fn;

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
                // Track pointee type: self is pointer to the struct type
                auto sit = types.structs.find(current_struct);
                if (sit != types.structs.end())
                    setPointeeType(named_values[pname], sit->second);
            } else {
                createEntryBlockAlloca(fn, pname, arg.getType());
                builder.CreateStore(&arg, named_values[pname]);
                // Track pointee type for *T parameters
                if (arg.getType()->isPointerTy() && node->middle && node->middle->children &&
                    idx < node->middle->children->size()) {
                    auto* pnode = (*node->middle->children)[idx];
                    if (pnode->node_type == ASTNodeType::Parameter && pnode->left &&
                        pnode->left->token && pnode->left->token->type == TokenType::Star &&
                        pnode->left->left) {
                        Type* inner = razenType(pnode->left->left);
                        if (inner) setPointeeType(named_values[pname], inner);
                    }
                }
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
    current_llvm_function = nullptr;
}

// ── Constant helpers for global init ─────────────────────────────────────────

static bool isConstantNode(ASTNode* node) {
    if (!node) return false;
    switch (node->node_type) {
        case ASTNodeType::IntegerLiteral:
        case ASTNodeType::FloatLiteral:
        case ASTNodeType::BoolLiteral:
        case ASTNodeType::CharLiteral:
        case ASTNodeType::StringLiteral:
            return true;
        default:
            return false;
    }
}

Value* IRGen::evalConstantNode(ASTNode* node) {
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
            auto it = string_globals.find(val);
            GlobalVariable* gv;
            if (it != string_globals.end()) {
                gv = it->second;
            } else {
                auto* arr = ConstantDataArray::getString(ctx, val, true);
                gv = new GlobalVariable(module, arr->getType(), true,
                    GlobalValue::PrivateLinkage, arr, ".str");
                string_globals[val] = gv;
            }
            auto* zero = ConstantInt::get(Type::getInt32Ty(ctx), 0);
            return ConstantExpr::getGetElementPtr(gv->getValueType(), gv, ArrayRef<Constant*>({zero, zero}));
        }
        default:
            return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    }
}

// ── Variable declaration ────────────────────────────────────────────────────

void IRGen::genVar(ASTNode* node, bool is_const) {
    if (!node || !node->token) return;
    auto& name = node->token->value;

    Type* ty = node->left ? razenType(node->left) : nullptr;

    // Track unsigned / char variables
    if (node->left && node->left->token) {
        auto& type_name = node->left->token->value;
        if (isUnsignedRazenType(type_name)) {
            unsigned_vars.insert(name);
        }
        if (type_name == "char") {
            char_vars.insert(name);
        }
    }

    if (!current_llvm_function) {
        if (!ty) ty = Type::getInt32Ty(ctx);
        if (node->right && isConstantNode(node->right)) {
            auto* val = evalConstantNode(node->right);
            if (ty && val->getType() != ty && isIntTy(val->getType()) && isIntTy(ty)) {
                auto* ci = dyn_cast<ConstantInt>(val);
                if (ci) {
                    bool us = node->left && node->left->token && isUnsignedRazenType(node->left->token->value);
                    val = ConstantInt::get(ty, us ? ci->getValue().zext(ty->getIntegerBitWidth())
                                                  : ci->getValue().sext(ty->getIntegerBitWidth()));
                }
            }
            if (!ty) ty = val->getType();
            new GlobalVariable(module, ty, is_const, GlobalValue::InternalLinkage,
                              dyn_cast<Constant>(val), name);
        } else if (node->right) {
            auto* gv = new GlobalVariable(module, ty, is_const, GlobalValue::InternalLinkage,
                                          Constant::getNullValue(ty), name);
            pending_global_inits.push_back({gv, node->right});
        } else {
            if (!ty) ty = Type::getInt32Ty(ctx);
            new GlobalVariable(module, ty, is_const, GlobalValue::InternalLinkage,
                               Constant::getNullValue(ty), name);
        }
        return;
    }

    if (node->right) {
        auto* init = genExpr(node->right);
        if (init) {
            if (!ty) ty = init->getType();
            if (!ty) ty = Type::getInt32Ty(ctx);
            createEntryBlockAlloca(current_llvm_function, name, ty);
            builder.CreateStore(init, named_values[name]);
            // Propagate pointee type for pointer-typed initializers
            if (init->getType()->isPointerTy()) {
                Type* pointee = getPointeeType(init);
                if (pointee && pointee != init->getType())
                    setPointeeType(named_values[name], pointee);
            }
            return;
        }
    }

    if (!ty) ty = Type::getInt32Ty(ctx);
    createEntryBlockAlloca(current_llvm_function, name, ty);

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
            // Check if return type is an error union / optional  { i1, T }
            if (current_ret_type->isStructTy()) {
                auto* st = cast<StructType>(current_ret_type);
                if (st->getNumElements() == 2 && st->getElementType(0)->isIntegerTy(1)) {
                    Type* inner_ty = st->getElementType(1);

                    // Check if return expression is an error code (MemberAccess like MathError.DivisionByZero)
                    bool is_error_code = false;
                    if (node->left->node_type == ASTNodeType::MemberAccess &&
                        node->left->left && node->left->right &&
                        node->left->left->token && node->left->right->token) {
                        auto erit = errors.find(node->left->left->token->value);
                        if (erit != errors.end()) {
                            is_error_code = erit->second.count(node->left->right->token->value) > 0;
                        }
                    }

                    if (is_error_code || val->getType() != inner_ty) {
                        // Error return: { true, zeroinit }
                        Constant* err_elems[] = {
                            ConstantInt::getTrue(ctx),
                            Constant::getNullValue(inner_ty)
                        };
                        auto* ret = ConstantStruct::get(st, err_elems);
                        builder.CreateRet(ret);
                    } else {
                        // Success return: { false, val }
                        Constant* ok_elems[] = {
                            ConstantInt::getFalse(ctx),
                            dyn_cast<Constant>(val) ? dyn_cast<Constant>(val) : Constant::getNullValue(inner_ty)
                        };
                        auto* ret = ConstantStruct::get(st, ok_elems);
                        builder.CreateRet(ret);
                    }
                    return;
                }
            }

            if (val->getType() != current_ret_type) {
                if (isIntTy(val->getType()) && isIntTy(current_ret_type)) {
                    // Check if return value is unsigned
                    bool is_unsigned = false;
                    if (node->left->node_type == ASTNodeType::Identifier && node->left->token) {
                        is_unsigned = unsigned_vars.count(node->left->token->value);
                    }
                    val = widenInt(val, current_ret_type, is_unsigned);
                } if (current_ret_type->isVoidTy()) {
                    builder.CreateRetVoid();
                    return;
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

    // ── For-each loop: loop <expr> |<var>| { body } ────────────────────────
    if (node->middle && node->left) {
        auto* iterable = genExpr(node->left);
        if (!iterable) return;
        Type* iter_ty = iterable->getType();

        std::string var_name;
        if (node->middle->node_type == ASTNodeType::Identifier && node->middle->token)
            var_name = node->middle->token->value;

        if (iter_ty->isArrayTy()) {
            auto* arr_ty = cast<ArrayType>(iter_ty);
            Type* elem_ty = arr_ty->getElementType();
            unsigned len = arr_ty->getNumElements();

            auto* arr_alloca = builder.CreateAlloca(iter_ty);
            builder.CreateStore(iterable, arr_alloca);

            auto* idx_alloca = builder.CreateAlloca(Type::getInt32Ty(ctx), nullptr, "foreach.idx");
            builder.CreateStore(ConstantInt::get(Type::getInt32Ty(ctx), 0), idx_alloca);

            if (!var_name.empty()) {
                auto* elem_alloca = builder.CreateAlloca(elem_ty, nullptr, var_name);
                named_values[var_name] = elem_alloca;
                named_types[var_name] = elem_ty;
            }

            auto* condBB = BasicBlock::Create(ctx, "foreach.cond", fn);
            auto* bodyBB = BasicBlock::Create(ctx, "foreach.body", fn);
            auto* incBB  = BasicBlock::Create(ctx, "foreach.inc", fn);
            auto* endBB  = BasicBlock::Create(ctx, "foreach.end", fn);

            auto* prev_cont = loop_continue;
            auto* prev_end  = loop_end;
            loop_continue = incBB;
            loop_end = endBB;

            builder.CreateBr(condBB);
            builder.SetInsertPoint(condBB);

            auto* idx = builder.CreateLoad(Type::getInt32Ty(ctx), idx_alloca, "idx");
            auto* cmp = builder.CreateICmpULT(idx, ConstantInt::get(Type::getInt32Ty(ctx), len), "idx.cmp");
            builder.CreateCondBr(cmp, bodyBB, endBB);

            builder.SetInsertPoint(bodyBB);
            if (!var_name.empty()) {
                auto* gep = builder.CreateGEP(iter_ty, arr_alloca, {
                    ConstantInt::get(Type::getInt32Ty(ctx), 0),
                    builder.CreateLoad(Type::getInt32Ty(ctx), idx_alloca, "idx2")
                });
                auto* elem_val = builder.CreateLoad(elem_ty, gep, "elem");
                builder.CreateStore(elem_val, named_values[var_name]);
            }

            bool body_ret = false;
            if (node->right) {
                bool prev = has_return; has_return = false;
                genNode(node->right);
                body_ret = has_return;
                has_return = prev;
            }
            if (!body_ret && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(incBB);

            builder.SetInsertPoint(incBB);
            auto* next_idx = builder.CreateAdd(
                builder.CreateLoad(Type::getInt32Ty(ctx), idx_alloca, "idx3"),
                ConstantInt::get(Type::getInt32Ty(ctx), 1), "next");
            builder.CreateStore(next_idx, idx_alloca);
            builder.CreateBr(condBB);

            builder.SetInsertPoint(endBB);
            loop_continue = prev_cont;
            loop_end = prev_end;
            if (!var_name.empty()) {
                named_values.erase(var_name);
                named_types.erase(var_name);
            }
            return;
        }

        // Non-array iterable not yet supported — fall through to while-loop
    }

    // ── While loop: loop <cond> { body } ───────────────────────────────────
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
    bool body_ret = false;
    if (node->right) {
        bool prev = has_return; has_return = false;
        genNode(node->right);
        body_ret = has_return;
        has_return = prev;
    }
    if (!body_ret && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(loop_continue);

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

    // Pointer deref assignment: a.* = rhs
    if (lhs->node_type == ASTNodeType::UnaryExpression &&
        lhs->token && lhs->token->value == ".*") {
        auto* ptr = genExpr(lhs->left);
        if (ptr) {
            Type* pointee_ty = getPointeeType(ptr);
            doStore(ptr, pointee_ty);
        }
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
                if (sname.empty() && st_ty->isStructTy()) {
                    auto* sty = cast<StructType>(st_ty);
                    if (sty->hasName() && types.structs.count(sty->getName().str()))
                        sname = sty->getName().str();
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
                    ptr = builder.CreateLoad(st_ty, ptr);
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

        // Default block for unmatched tags (exhaustive match should never reach it)
        auto* defaultBB = BasicBlock::Create(ctx, "match.default", fn);

        BasicBlock* currentBB = nullptr;
        for (size_t i = 0; i < cases.size(); i++) {
            auto* caseBB = BasicBlock::Create(ctx, "match.case", fn);
            auto* nextBB = (i == cases.size() - 1 && else_case)
                ? BasicBlock::Create(ctx, "match.else", fn)
                : (i == cases.size() - 1 ? defaultBB : BasicBlock::Create(ctx, "match.next", fn));

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

                // Struct variant: Shape.Circle { radius }  → bindings in cases[i]->middle
                if (cases[i]->middle && cases[i]->middle->children &&
                    vi.payload_type->isStructTy()) {
                    auto* pst = cast<StructType>(vi.payload_type);
                    unsigned fidx = 0;
                    for (auto* field : *cases[i]->middle->children) {
                        if (field->node_type == ASTNodeType::Identifier && field->token) {
                            auto& fname = field->token->value;
                            if (fidx < pst->getNumElements()) {
                                Type* fty = pst->getElementType(fidx);
                                auto* fgep = builder.CreateStructGEP(pst, casted, fidx);
                                auto* fval = builder.CreateLoad(fty, fgep, fname);
                                auto* falloc = builder.CreateAlloca(fty, nullptr, fname);
                                builder.CreateStore(fval, falloc);
                                named_values[fname] = falloc;
                                named_types[fname] = fty;
                                fidx++;
                            }
                        }
                    }
                // Simple payload binding: Shape.Circle(radius)  → in cases[i]->left->right
                } else if (cases[i]->left->right && cases[i]->left->right->children &&
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

            bool case_ret = false;
            if (cases[i]->right) {
                bool prev = has_return; has_return = false;
                genNode(cases[i]->right);
                case_ret = has_return;
                has_return = prev;
            }
            if (!case_ret && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(endBB);
            currentBB = nextBB;
        }

        // Terminate default block (unreachable)
        builder.SetInsertPoint(defaultBB);
        builder.CreateUnreachable();

        if (else_case) {
            builder.SetInsertPoint(currentBB);
            bool else_ret = false;
            if (else_case->right) {
                bool prev = has_return; has_return = false;
                genNode(else_case->right);
                else_ret = has_return;
                has_return = prev;
            }
            if (!else_ret && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(endBB);
        }
    } else {
        auto* prevBB = builder.GetInsertBlock();
        auto* checkBB = BasicBlock::Create(ctx, "match.check", fn);
        if (prevBB && !prevBB->getTerminator()) builder.CreateBr(checkBB);
        builder.SetInsertPoint(checkBB);
        // Default block for unmatched values (exhaustive match should never reach it)
        auto* defaultBB = BasicBlock::Create(ctx, "match.default", fn);
        // Track else block in case there's an else_case
        BasicBlock* elseBB = nullptr;

        for (size_t i = 0; i < cases.size(); i++) {
            auto* caseBB = BasicBlock::Create(ctx, "match.case", fn);
            BasicBlock* nextBB;
            if (i == cases.size() - 1) {
                if (else_case) {
                    elseBB = BasicBlock::Create(ctx, "match.else", fn);
                    nextBB = elseBB;
                } else {
                    nextBB = defaultBB;
                }
            } else {
                nextBB = BasicBlock::Create(ctx, "match.next", fn);
            }

            auto* pat_val = genExpr(cases[i]->left);
            if (!pat_val) pat_val = ConstantInt::get(Type::getInt32Ty(ctx), 0);

            // Ensure both operands have the same type for ICmp
            if (match_val->getType() != pat_val->getType()) {
                if (isIntTy(match_val->getType()) && isIntTy(pat_val->getType())) {
                    // Widen to the larger type
                    auto* wider = match_val->getType()->getIntegerBitWidth() >= pat_val->getType()->getIntegerBitWidth()
                        ? match_val->getType() : pat_val->getType();
                    if (pat_val->getType() != wider)
                        pat_val = builder.CreateIntCast(pat_val, wider, false);
                    else
                        match_val = builder.CreateIntCast(match_val, wider, false);
                }
            }
            auto* cmp = builder.CreateICmpEQ(match_val, pat_val);
            builder.CreateCondBr(cmp, caseBB, nextBB);

            builder.SetInsertPoint(caseBB);
            bool case_ret = false;
            if (cases[i]->right) {
                bool prev = has_return; has_return = false;
                genNode(cases[i]->right);
                case_ret = has_return;
                has_return = prev;
            }
            if (!case_ret && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(endBB);

            if (nextBB != defaultBB) builder.SetInsertPoint(nextBB);
        }

        // Handle else case
        if (elseBB) {
            builder.SetInsertPoint(elseBB);
            bool else_ret = false;
            if (else_case->right) {
                bool prev = has_return; has_return = false;
                genNode(else_case->right);
                else_ret = has_return;
                has_return = prev;
            }
            if (!else_ret && !builder.GetInsertBlock()->getTerminator()) builder.CreateBr(endBB);
        }

        // Terminate default block (unreachable)
        builder.SetInsertPoint(defaultBB);
        builder.CreateUnreachable();
    }

    builder.SetInsertPoint(endBB);
    if (!endBB->hasNPredecessorsOrMore(1))
        builder.CreateUnreachable();
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
    // Struct literal { ... } — must be checked before general BinaryExpression
    if (node->node_type == ASTNodeType::BinaryExpression &&
        node->token && node->token->value == "{}") {
        return genStructLiteral(node);
    }
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
        case ASTNodeType::BuiltinExpression: return genBuiltinExpr(node);
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
            auto it = string_globals.find(val);
            GlobalVariable* gv;
            if (it != string_globals.end()) {
                gv = it->second;
            } else {
                // Create a properly named global string with dedup
                auto* str_constant = ConstantDataArray::getString(ctx, val, true);
                auto* global = new GlobalVariable(
                    module, str_constant->getType(), true,
                    GlobalValue::PrivateLinkage, str_constant, ".str." + std::to_string(string_globals.size()));
                string_globals[val] = global;
                gv = global;
            }
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

    // Look up global variables
    auto* gv = module.getNamedGlobal(name);
    if (gv) {
        return builder.CreateLoad(gv->getValueType(), gv, name.c_str());
    }

    return ConstantInt::get(Type::getInt32Ty(ctx), 0);
}

// ── Struct literal ───────────────────────────────────────────────────────────

Value* IRGen::genStructLiteral(ASTNode* node) {
    if (!node || !node->left) return ConstantInt::get(Type::getInt32Ty(ctx), 0);

    std::string struct_name;
    std::string variant_name;
    ASTNode* left = node->left;

    if (left->node_type == ASTNodeType::Identifier && left->token) {
        struct_name = left->token->value;
    } else if (left->node_type == ASTNodeType::MemberAccess && left->left && left->right) {
        if (left->left->token) struct_name = left->left->token->value;
        if (left->right->token) variant_name = left->right->token->value;
    }

    if (struct_name.empty()) return ConstantInt::get(Type::getInt32Ty(ctx), 0);

    // Union variant struct literal: Shape.Circle { radius = 5.0 }
    auto uit = unions.find(struct_name);
    if (uit != unions.end()) {
        UnionVariant* vi = nullptr;
        for (auto& v : uit->second.variants) {
            if (v.name == variant_name) { vi = &v; break; }
        }
        if (!vi) return ConstantInt::get(Type::getInt32Ty(ctx), 0);

        auto* union_ty = uit->second.ll_type;
        auto* alloc = builder.CreateAlloca(union_ty);
        auto* tag_ptr = builder.CreateStructGEP(union_ty, alloc, 0, "tag.ptr");
        builder.CreateStore(ConstantInt::get(Type::getInt32Ty(ctx), vi->tag), tag_ptr);

        if (vi->payload_type && !vi->payload_type->isVoidTy() &&
            node->right && node->right->node_type == ASTNodeType::Block && node->right->children) {

            auto* payload_ptr = builder.CreateStructGEP(union_ty, alloc, 1, "payload.ptr");
            auto* casted = builder.CreateBitCast(payload_ptr, PointerType::getUnqual(vi->payload_type));

            if (vi->payload_type->isStructTy() && vi->payload_type->getNumContainedTypes() > 0) {
                auto* payload_st = cast<StructType>(vi->payload_type);
                unsigned fidx = 0;
                for (auto* child : *node->right->children) {
                    if (child->node_type == ASTNodeType::Assignment && child->right) {
                        auto* val = genExpr(child->right);
                        if (val && fidx < payload_st->getNumElements()) {
                            auto* gep = builder.CreateStructGEP(payload_st, casted, fidx);
                            Type* expected = payload_st->getElementType(fidx);
                            if (val->getType() != expected && isIntTy(val->getType()) && isIntTy(expected))
                                val = widenInt(val, expected, false);
                            builder.CreateStore(val, gep);
                            fidx++;
                        }
                    }
                }
            } else if (!vi->payload_type->isStructTy()) {
                for (auto* child : *node->right->children) {
                    if (child->node_type == ASTNodeType::Assignment && child->right) {
                        auto* val = genExpr(child->right);
                        if (val) {
                            if (val->getType() != vi->payload_type && isIntTy(val->getType()) && isIntTy(vi->payload_type))
                                val = widenInt(val, vi->payload_type, false);
                            builder.CreateStore(val, casted);
                        }
                        break;
                    }
                }
            }
        }
        return builder.CreateLoad(union_ty, alloc, "union.val");
    }

    // Regular struct literal
    auto sit = types.structs.find(struct_name);
    if (sit == types.structs.end()) return ConstantInt::get(Type::getInt32Ty(ctx), 0);

    auto* struct_ty = sit->second;
    auto* alloc = builder.CreateAlloca(struct_ty);

    auto fnit = types.field_names.find(struct_name);
    std::vector<std::string> field_names;
    if (fnit != types.field_names.end()) field_names = fnit->second;

    if (node->right && node->right->node_type == ASTNodeType::Block && node->right->children) {
        for (auto* child : *node->right->children) {
            if (child->node_type == ASTNodeType::Assignment && child->left && child->left->token) {
                auto& fname = child->left->token->value;
                unsigned fidx = 0;
                for (unsigned i = 0; i < field_names.size(); i++) {
                    if (field_names[i] == fname) { fidx = i; break; }
                }
                if (child->right) {
                    auto* val = genExpr(child->right);
                    if (val) {
                        auto* gep = builder.CreateStructGEP(struct_ty, alloc, fidx);
                        Type* expected = struct_ty->getElementType(fidx);
                        if (val->getType() != expected && isIntTy(val->getType()) && isIntTy(expected))
                            val = widenInt(val, expected, false);
                        builder.CreateStore(val, gep);
                    }
                }
            }
        }
    }

    return builder.CreateLoad(struct_ty, alloc);
}

// ── Binary operations ───────────────────────────────────────────────────────

Value* IRGen::genBinary(ASTNode* node) {
    if (!node) return ConstantInt::get(Type::getInt32Ty(ctx), 0);

    auto tok = node->token;
    if (!tok) return ConstantInt::get(Type::getInt32Ty(ctx), 0);

    // ── Short-circuit evaluation for && / || ─────────────────────────────────
    if (tok->type == TokenType::AndAnd || tok->type == TokenType::OrOr) {
        auto* fn = builder.GetInsertBlock()->getParent();

        // Evaluate LHS and ensure it's i1
        auto* lhs = genExpr(node->left);
        if (!lhs) lhs = ConstantInt::getFalse(ctx);
        if (lhs->getType() != Type::getInt1Ty(ctx))
            lhs = builder.CreateICmpNE(lhs, ConstantInt::get(lhs->getType(), 0), "sc.lhs");

        auto* lhsBlock = builder.GetInsertBlock();
        auto* rhsBB = BasicBlock::Create(ctx, "sc.rhs", fn);
        auto* endBB = BasicBlock::Create(ctx, "sc.end", fn);

        if (tok->type == TokenType::AndAnd) {
            // a && b: if a is false → false, else evaluate b
            builder.CreateCondBr(lhs, rhsBB, endBB);
        } else {
            // a || b: if a is true → true, else evaluate b
            builder.CreateCondBr(lhs, endBB, rhsBB);
        }

        // Evaluate RHS
        builder.SetInsertPoint(rhsBB);
        auto* rhs_val = genExpr(node->right);
        if (!rhs_val) rhs_val = ConstantInt::getFalse(ctx);
        if (rhs_val->getType() != Type::getInt1Ty(ctx))
            rhs_val = builder.CreateICmpNE(rhs_val, ConstantInt::get(rhs_val->getType(), 0), "sc.rhs");
        auto* rhsBlock = builder.GetInsertBlock();
        builder.CreateBr(endBB);

        // Build PHI
        builder.SetInsertPoint(endBB);
        auto* phi = builder.CreatePHI(Type::getInt1Ty(ctx), 2, "sc.result");
        if (tok->type == TokenType::AndAnd) {
            phi->addIncoming(ConstantInt::getFalse(ctx), lhsBlock);
        } else {
            phi->addIncoming(ConstantInt::getTrue(ctx), lhsBlock);
        }
        phi->addIncoming(rhs_val, rhsBlock);
        return phi;
    }

    auto* lhs = genExpr(node->left);
    auto* rhs = genExpr(node->right);
    if (!lhs || !rhs) return lhs ? lhs : rhs;

    // Determine if operands are unsigned (check from identifier sources)
    bool lhs_unsigned = false;
    bool rhs_unsigned = false;
    if (node->left && node->left->node_type == ASTNodeType::Identifier && node->left->token) {
        lhs_unsigned = unsigned_vars.count(node->left->token->value);
    }
    if (node->right && node->right->node_type == ASTNodeType::Identifier && node->right->token) {
        rhs_unsigned = unsigned_vars.count(node->right->token->value);
    }

    Type* ty = lhs->getType();
    if (rhs->getType() != ty) {
        if (isIntTy(ty) && isIntTy(rhs->getType())) {
            auto w1 = cast<IntegerType>(ty)->getBitWidth();
            auto w2 = cast<IntegerType>(rhs->getType())->getBitWidth();
            if (w1 < w2) {
                lhs = widenInt(lhs, rhs->getType(), lhs_unsigned);
            } else if (w1 > w2) {
                rhs = widenInt(rhs, ty, rhs_unsigned);
            }
            ty = lhs->getType();
        } else if (ty->isDoubleTy() || ty->isFloatTy()) {
            if (isIntTy(rhs->getType()))
                rhs = widenIntToFloat(rhs, ty);
        } else if (rhs->getType()->isDoubleTy() || rhs->getType()->isFloatTy()) {
            if (isIntTy(ty))
                lhs = widenIntToFloat(lhs, rhs->getType());
            ty = lhs->getType();
        }
    }
    // Final type sync: both must match for any op
    if (lhs->getType() != rhs->getType()) {
        if (lhs->getType()->isDoubleTy() && rhs->getType()->isFloatTy())
            rhs = builder.CreateFPExt(rhs, lhs->getType());
        else if (lhs->getType()->isFloatTy() && rhs->getType()->isDoubleTy())
            lhs = builder.CreateFPExt(lhs, rhs->getType());
        else if (isIntTy(lhs->getType()) && isIntTy(rhs->getType()))
            ; // already handled above
        else
            return lhs; // types incompatible, bail out
    }
    ty = lhs->getType();

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
            return fl ? builder.CreateFCmpOLT(lhs, rhs) : (lhs_unsigned || rhs_unsigned
                ? builder.CreateICmpULT(lhs, rhs) : builder.CreateICmpSLT(lhs, rhs));
        case TokenType::LessThanEquals:
            return fl ? builder.CreateFCmpOLE(lhs, rhs) : (lhs_unsigned || rhs_unsigned
                ? builder.CreateICmpULE(lhs, rhs) : builder.CreateICmpSLE(lhs, rhs));
        case TokenType::GreaterThan:
            return fl ? builder.CreateFCmpOGT(lhs, rhs) : (lhs_unsigned || rhs_unsigned
                ? builder.CreateICmpUGT(lhs, rhs) : builder.CreateICmpSGT(lhs, rhs));
        case TokenType::GreaterThanEquals:
            return fl ? builder.CreateFCmpOGE(lhs, rhs) : (lhs_unsigned || rhs_unsigned
                ? builder.CreateICmpUGE(lhs, rhs) : builder.CreateICmpSGE(lhs, rhs));

        case TokenType::And:
            return builder.CreateAnd(lhs, rhs);
        case TokenType::Or:
            return builder.CreateOr(lhs, rhs);
        case TokenType::Caret:
            return builder.CreateXor(lhs, rhs);
        case TokenType::ShiftLeft:
            return builder.CreateShl(lhs, rhs);
        case TokenType::ShiftRight:
            return builder.CreateLShr(lhs, rhs);  // Use LShr (safer default for both signed/unsigned)

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
            if (it != named_values.end()) {
                setPointeeType(it->second, named_types[node->left->token->value]);
                return it->second;
            }
        }
        return Constant::getNullValue(PointerType::getUnqual(ctx));
    }
    if (op == ".*") {
        Type* load_ty = getPointeeType(inner);
        return builder.CreateLoad(load_ty, inner, "deref");
    }
    return inner;
}

// ── Function call ───────────────────────────────────────────────────────────

Value* IRGen::genCall(ASTNode* node) {
    if (!node || !node->token) return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    auto& name = node->token->value;

    // ── Builtin: SizeOf ──────────────────────────────────────────────────
    if (name == "SizeOf" || name == "@SizeOf") {
        auto getSizeType = [&](const std::string& tn) -> Type* {
            if (tn == "void" || tn == "noret")  return Type::getVoidTy(ctx);
            if (tn == "bool")                   return Type::getInt1Ty(ctx);
            if (tn == "char")                   return Type::getInt8Ty(ctx);
            if (tn == "i8" || tn == "u8")       return Type::getInt8Ty(ctx);
            if (tn == "i16" || tn == "u16")     return Type::getInt16Ty(ctx);
            if (tn == "i32" || tn == "u32" || tn == "int" || tn == "uint")
                                                return Type::getInt32Ty(ctx);
            if (tn == "i64" || tn == "u64" || tn == "isize" || tn == "usize")
                                                return Type::getInt64Ty(ctx);
            if (tn == "f32" || tn == "float")   return Type::getFloatTy(ctx);
            if (tn == "f64" || tn == "double")  return Type::getDoubleTy(ctx);
            auto sit = types.structs.find(tn);
            if (sit != types.structs.end())     return sit->second;
            auto ait = types.aliases.find(tn);
            if (ait != types.aliases.end())     return ait->second;
            return nullptr;
        };
        if (node->children && !node->children->empty()) {
            auto* first_arg = (*node->children)[0];
            if (first_arg->left &&
                first_arg->left->node_type == ASTNodeType::Identifier &&
                first_arg->left->token) {
                Type* ty = getSizeType(first_arg->left->token->value);
                if (ty) {
                    uint64_t sz = module.getDataLayout().getTypeAllocSize(ty);
                    return ConstantInt::get(Type::getInt32Ty(ctx), (uint32_t)sz);
                }
            }
        }
        return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    }

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
    size_t named_params = fn->arg_size();
    for (size_t i = 0; i < args.size(); i++) {
        if (i < named_params) {
            auto* expected = fn->getArg(i)->getType();
            if (args[i]->getType() != expected) {
                if (isIntTy(args[i]->getType()) && isIntTy(expected)) {
                    // Determine unsignedness from the argument expression
                    bool is_unsigned = false;
                    if (node->children && i < node->children->size()) {
                        auto* arg_node = (*node->children)[i];
                        if (arg_node->left && arg_node->left->node_type == ASTNodeType::Identifier && arg_node->left->token) {
                            is_unsigned = unsigned_vars.count(arg_node->left->token->value);
                        }
                    }
                    call_args.push_back(widenInt(args[i], expected, is_unsigned));
                } else {
                    // Types don't match — insert a safe cast to avoid invalid IR
                    auto* arg_ty = args[i]->getType();
                    if (arg_ty->isPointerTy() && expected->isIntegerTy())
                        call_args.push_back(builder.CreatePtrToInt(args[i], expected));
                    else if (arg_ty->isIntegerTy() && expected->isPointerTy())
                        call_args.push_back(builder.CreateIntToPtr(args[i], expected));
                    else if (arg_ty->isPointerTy() && expected->isPointerTy())
                        call_args.push_back(builder.CreateBitCast(args[i], expected));
                    else
                        call_args.push_back(builder.CreateBitCast(args[i], expected));
                }
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

// ── Format call (fmt.print / fmt.println) ──────────────────────────────────

Value* IRGen::genFormatCall(ASTNode* node, bool add_newline) {
    auto* call_node = node->right;
    if (!call_node || !call_node->children || call_node->children->empty())
        return ConstantInt::get(Type::getInt32Ty(ctx), 0);

    auto* fmt_child = (*call_node->children)[0];
    if (!fmt_child) return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    auto* fmt_node = fmt_child->left;
    if (!fmt_node) return ConstantInt::get(Type::getInt32Ty(ctx), 0);

    auto zero = ConstantInt::get(Type::getInt32Ty(ctx), 0);
    auto* i8ptr = PointerType::getUnqual(ctx);

    auto makeGlobalStr = [&](const std::string& s) -> Value* {
        auto* arr = ConstantDataArray::getString(ctx, s, true);
        auto* gv = new GlobalVariable(module, arr->getType(), true,
            GlobalValue::PrivateLinkage, arr, ".fmt");
        return builder.CreateGEP(gv->getValueType(), gv, {zero, zero}, "fmt.ptr");
    };

    // Helper to get runtime function
    auto getFn = [&](const char* name) -> Function* {
        return module.getFunction(name);
    };

    // Simple case: println/print(str) with no format args
    if (call_node->children->size() < 2 || !(*call_node->children)[1]->left) {
        if (add_newline) {
            auto* fn = getFn("__razen_print_str_newline");
            if (fn) {
                auto* str_val = genExpr(fmt_node);
                if (str_val) builder.CreateCall(fn, {str_val});
            }
        } else {
            auto* fn = getFn("__razen_print_str");
            if (fn) {
                auto* str_val = genExpr(fmt_node);
                if (str_val) builder.CreateCall(fn, {str_val});
            }
        }
        return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    }

    // With format args: parse format string and tuple of args
    std::string fmt_str = fmt_node->token ? fmt_node->token->value : "";

    auto* tuple_node = (*call_node->children)[1]->left;
    std::vector<Value*> args;
    std::vector<ASTNode*> arg_nodes;  // parallel: original AST node for each arg
    if (tuple_node && tuple_node->node_type == ASTNodeType::TupleLiteral && tuple_node->children) {
        for (auto* elem : *tuple_node->children) {
            if (elem) {
                auto* v = genExpr(elem);
                if (v) {
                    args.push_back(v);
                    arg_nodes.push_back(elem);
                }
            }
        }
    }

    auto* print_str_fn = getFn("__razen_print_str");
    auto* print_int_fn = getFn("__razen_print_int");
    auto* print_int64_fn = getFn("__razen_print_int64");
    auto* print_double_fn = getFn("__razen_print_double");
    auto* print_float_fn = getFn("__razen_print_float");
    auto* print_bool_fn = getFn("__razen_print_bool");
    auto* print_char_fn = getFn("__razen_print_char");
    auto* print_ptr_fn = getFn("__razen_print_ptr");
    auto* print_newline_fn = getFn("__razen_print_newline");

    size_t arg_idx = 0;
    size_t pos = 0;
    while (pos < fmt_str.size()) {
        size_t next = fmt_str.find("{}", pos);
        if (next == std::string::npos) {
            auto seg = fmt_str.substr(pos);
            if (!seg.empty() && print_str_fn) {
                auto* seg_ptr = makeGlobalStr(seg);
                builder.CreateCall(print_str_fn, {seg_ptr});
            }
            break;
        }

        if (next > pos && print_str_fn) {
            auto seg = fmt_str.substr(pos, next - pos);
            auto* seg_ptr = makeGlobalStr(seg);
            builder.CreateCall(print_str_fn, {seg_ptr});
        }

        if (arg_idx < args.size()) {
            auto* arg = args[arg_idx];
            auto* ty = arg->getType();
            if (ty->isPointerTy() && print_str_fn) {
                builder.CreateCall(print_str_fn, {arg});
            } else if (ty->isIntegerTy(1) && print_bool_fn) {
                builder.CreateCall(print_bool_fn, {arg});
            } else if (ty->isIntegerTy(8)) {
                // Check if this i8 came from a char variable vs u8
                bool is_char = false;
                if (arg_idx < arg_nodes.size() && arg_nodes[arg_idx] &&
                    arg_nodes[arg_idx]->node_type == ASTNodeType::Identifier &&
                    arg_nodes[arg_idx]->token) {
                    is_char = char_vars.count(arg_nodes[arg_idx]->token->value);
                }
                if (is_char && print_char_fn) {
                    builder.CreateCall(print_char_fn, {arg});
                } else if (print_int_fn) {
                    arg = builder.CreateIntCast(arg, Type::getInt32Ty(ctx), false);
                    builder.CreateCall(print_int_fn, {arg});
                }
            } else if (ty->isIntegerTy()) {
                auto bw = ty->getIntegerBitWidth();
                if (bw <= 32 && print_int_fn) {
                    if (bw < 32) arg = builder.CreateIntCast(arg, Type::getInt32Ty(ctx), false);
                    builder.CreateCall(print_int_fn, {arg});
                } else if (print_int64_fn) {
                    if (bw < 64) arg = builder.CreateIntCast(arg, Type::getInt64Ty(ctx), false);
                    builder.CreateCall(print_int64_fn, {arg});
                }
            } else if (ty->isDoubleTy() && print_double_fn) {
                builder.CreateCall(print_double_fn, {arg});
            } else if (ty->isFloatTy() && print_float_fn) {
                builder.CreateCall(print_float_fn, {arg});
            } else if (print_ptr_fn) {
                if (ty->isPointerTy()) {
                    builder.CreateCall(print_ptr_fn, {arg});
                } else {
                    auto* casted = builder.CreateBitCast(arg, i8ptr);
                    builder.CreateCall(print_ptr_fn, {casted});
                }
            }
            arg_idx++;
        } else if (print_str_fn) {
            auto* brace_ptr = makeGlobalStr("{}");
            builder.CreateCall(print_str_fn, {brace_ptr});
        }

        pos = next + 2;
    }

    if (add_newline && print_newline_fn)
        builder.CreateCall(print_newline_fn);

    return ConstantInt::get(Type::getInt32Ty(ctx), 0);
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
                // fallback: match by struct type name
                if (st_ty->isStructTy()) {
                    auto* sty = cast<StructType>(st_ty);
                    if (sty->hasName() && types.structs.count(sty->getName().str()))
                        sname = sty->getName().str();
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
            if (sname.empty() && st_ty->isStructTy()) {
                auto* sty = cast<StructType>(st_ty);
                if (sty->hasName() && types.structs.count(sty->getName().str()))
                    sname = sty->getName().str();
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
            } else {
                ptr = builder.CreateLoad(st_ty, ptr);
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

    // Module function call: fmt.print(...) / fmt.println(...)
    if (node->left && node->left->node_type == ASTNodeType::Identifier && node->left->token &&
        node->right && node->right->node_type == ASTNodeType::FunctionCall) {
        auto& mod_name = node->left->token->value;
        if (mod_name == "fmt") {
            auto& fn_name = node->right->token->value;
            if (fn_name == "print" || fn_name == "println")
                return genFormatCall(node, fn_name == "println");
        }
        return genCall(node->right);
    }

    return ConstantInt::get(Type::getInt32Ty(ctx), 0);
}

// ── Array literal ───────────────────────────────────────────────────────────

Value* IRGen::genArrayLiteral(ASTNode* node) {
    if (!node || !node->children || node->children->empty())
        return ConstantArray::get(ArrayType::get(Type::getInt32Ty(ctx), 0), {});

    Type* elem = Type::getInt32Ty(ctx);
    auto* first_child = (*node->children)[0];
    if (first_child) {
        auto* first = genExpr(first_child);
        if (first) elem = first->getType();
    }

    std::vector<Constant*> elems;
    bool all_constant = true;
    for (auto* child : *node->children) {
        if (child) {
            auto* v = genExpr(child);
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
        if (child) {
            auto* v = genExpr(child);
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
    BasicBlock* catchBB = nullptr;

    auto* flag = builder.CreateExtractValue(call, 0, "err.flag");

    if (node->right && node->right->node_type == ASTNodeType::CatchExpression) {
        catchBB = BasicBlock::Create(ctx, "try.catch", fn);
        builder.CreateCondBr(flag, catchBB, successBB);

        builder.SetInsertPoint(successBB);
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
        builder.CreateBr(endBB);
    }

    builder.SetInsertPoint(endBB);
    auto* phi = builder.CreatePHI(call->getType()->getStructElementType(1), 2, "try.val");
    phi->addIncoming(builder.CreateExtractValue(call, 1, "ok.val"), successBB);
    if (catchBB)
        phi->addIncoming(Constant::getNullValue(phi->getType()), catchBB);
    return phi;
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

// ── Builtin expression (SizeOf, AlignOf, TypeOf, as) ──────────────────────

Value* IRGen::genBuiltinExpr(ASTNode* node) {
    if (!node || !node->token) return ConstantInt::get(Type::getInt32Ty(ctx), 0);
    auto& name = node->token->value;

    if (name == "SizeOf") {
        Type* ty = node->left ? razenType(node->left) : nullptr;
        if (ty) {
            uint64_t sz = module.getDataLayout().getTypeAllocSize(ty);
            return ConstantInt::get(Type::getInt64Ty(ctx), sz);
        }
        return ConstantInt::get(Type::getInt64Ty(ctx), 0);
    }

    if (name == "AlignOf") {
        Type* ty = node->left ? razenType(node->left) : nullptr;
        if (ty) {
            uint64_t align = module.getDataLayout().getABITypeAlign(ty).value();
            return ConstantInt::get(Type::getInt64Ty(ctx), align);
        }
        return ConstantInt::get(Type::getInt64Ty(ctx), 0);
    }

    if (name == "TypeOf") {
        // @TypeOf(expr) — return type name of expression as a string constant
        auto* zero = ConstantInt::get(Type::getInt32Ty(ctx), 0);
        std::string type_name = node->data.empty() ? "unknown" : node->data;
        auto it = string_globals.find(type_name);
        GlobalVariable* gv;
        if (it != string_globals.end()) {
            gv = it->second;
        } else {
            auto* arr = ConstantDataArray::getString(ctx, type_name, true);
            gv = new GlobalVariable(module, arr->getType(), true,
                GlobalValue::PrivateLinkage, arr,
                ".typeof.str." + std::to_string(string_globals.size()));
            string_globals[type_name] = gv;
        }
        return builder.CreateGEP(gv->getValueType(), gv, {zero, zero}, "typeof.str.ptr");
    }

    if (name == "as") {
        if (!node->right) return ConstantInt::get(Type::getInt32Ty(ctx), 0);
        auto* val = genExpr(node->right);
        if (!val) return ConstantInt::get(Type::getInt32Ty(ctx), 0);

        Type* target = node->left ? razenType(node->left) : nullptr;
        if (!target) return val;
        if (val->getType() == target) return val;

        Type* src = val->getType();

        // Int -> Int (sign-extend or truncate)
        if (isIntTy(src) && isIntTy(target)) {
            auto srcBits = cast<IntegerType>(src)->getBitWidth();
            auto dstBits = cast<IntegerType>(target)->getBitWidth();
            if (srcBits < dstBits)
                return builder.CreateSExt(val, target, "as.sext");
            else if (srcBits > dstBits)
                return builder.CreateTrunc(val, target, "as.trunc");
            return val;
        }

        // Int -> Float
        if (isIntTy(src) && isFloatTy(target))
            return builder.CreateSIToFP(val, target, "as.sitofp");

        // Float -> Int
        if (isFloatTy(src) && isIntTy(target))
            return builder.CreateFPToSI(val, target, "as.fptosi");

        // Float -> Float (extend or truncate)
        if (isFloatTy(src) && isFloatTy(target)) {
            auto srcW = src->getPrimitiveSizeInBits();
            auto dstW = target->getPrimitiveSizeInBits();
            if (srcW < dstW)
                return builder.CreateFPExt(val, target, "as.fpext");
            else if (srcW > dstW)
                return builder.CreateFPTrunc(val, target, "as.fptrunc");
            return val;
        }

        // Pointer <-> Int
        if (src->isPointerTy() && isIntTy(target))
            return builder.CreatePtrToInt(val, target, "as.ptrtoint");
        if (isIntTy(src) && target->isPointerTy())
            return builder.CreateIntToPtr(val, target, "as.inttoptr");

        // Pointer <-> Pointer
        if (src->isPointerTy() && target->isPointerTy())
            return builder.CreateBitCast(val, target, "as.bitcast");

        // If no conversion matched, pass through
        return val;
    }

    if (name == "Dyn") {
        // @Dyn — dynamic dispatch marker; just a placeholder for now
        return ConstantInt::get(Type::getInt1Ty(ctx), 1);
    }

    return ConstantInt::get(Type::getInt32Ty(ctx), 0);
}

// ── Optimization pipeline (mem2reg + instcombine) ──────────────────────────

void Codegen::optimize() {
    auto& mod = ir.module;

    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // Verify before optimization
    for (auto& F : mod) {
        if (!F.isDeclaration()) {
            std::string err;
            raw_string_ostream err_os(err);
            if (verifyFunction(F, &err_os)) {
                llvm::errs() << "Pre-opt verify error in " << F.getName() << ": " << err << "\n";
            }
        }
    }

    llvm::FunctionPassManager FPM;
    FPM.addPass(llvm::PromotePass());
    FPM.addPass(llvm::InstCombinePass());

    llvm::ModulePassManager MPM;
    MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));
    MPM.run(mod, MAM);
}

// ── Object file emission ────────────────────────────────────────────────────

static bool emitFile(const std::string& path, llvm::Module& mod,
                     llvm::CodeGenFileType fileType) {
    std::string TripleStr = mod.getTargetTriple();
    if (TripleStr.empty()) TripleStr = "x86_64-pc-linux-gnu";
    mod.setTargetTriple(TripleStr);

    std::string Error;
    const llvm::Target* TheTarget = llvm::TargetRegistry::lookupTarget(TripleStr, Error);
    if (!TheTarget) { llvm::errs() << Error << "\n"; return false; }

    auto TM = std::unique_ptr<llvm::TargetMachine>(
        TheTarget->createTargetMachine(TripleStr, "generic", "",
                                       llvm::TargetOptions{},
                                       std::optional(llvm::Reloc::PIC_)));
    if (!TM) return false;
    mod.setDataLayout(TM->createDataLayout());

    std::error_code EC;
    llvm::raw_fd_ostream Out(path, EC, llvm::sys::fs::OF_None);
    if (EC) { llvm::errs() << EC.message() << "\n"; return false; }

    llvm::legacy::PassManager PM;
    if (TM->addPassesToEmitFile(PM, Out, nullptr, fileType)) {
        llvm::errs() << "Cannot emit file\n"; return false;
    }
    PM.run(mod);
    Out.flush();
    return true;
}

bool Codegen::emitObject(const std::string& path) {
    return emitFile(path, ir.module, llvm::CodeGenFileType::ObjectFile);
}

bool Codegen::emitAssembly(const std::string& path) {
    return emitFile(path, ir.module, llvm::CodeGenFileType::AssemblyFile);
}

} // namespace codegen
} // namespace razen
