#include "ir.h"
#include "../ast/token_utils.h"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <sstream>
#include <iostream>

using namespace llvm;

namespace razen {
namespace codegen {

// ── Primitive type mapping ──────────────────────────────────────────────────

static Type* primitiveType(LLVMContext& Ctx, const std::string& name) {
    if (name == "void" || name == "noret")  return Type::getVoidTy(Ctx);
    if (name == "bool")                     return Type::getInt1Ty(Ctx);
    if (name == "char")                     return Type::getInt8Ty(Ctx);
    if (name == "i8" || name == "u8")       return Type::getInt8Ty(Ctx);
    if (name == "i16" || name == "u16")     return Type::getInt16Ty(Ctx);
    if (name == "i32" || name == "u32" || name == "int" || name == "uint")
                                            return Type::getInt32Ty(Ctx);
    if (name == "i64" || name == "u64" || name == "isize" || name == "usize")
                                            return Type::getInt64Ty(Ctx);
    if (name == "i128" || name == "u128")   return Type::getInt128Ty(Ctx);
    if (name == "f16")                      return Type::getHalfTy(Ctx);
    if (name == "f32" || name == "float")   return Type::getFloatTy(Ctx);
    if (name == "f64")                      return Type::getDoubleTy(Ctx);
    if (name == "f128")                     return Type::getFP128Ty(Ctx);
    if (name == "any")                      return PointerType::getUnqual(Ctx);
    if (name == "str" || name == "string")  return PointerType::getUnqual(Ctx);
    return Type::getInt32Ty(Ctx);
}

// ── Type resolution from AST node ───────────────────────────────────────────

Type* IRGen::razenType(ASTNode* node) {
    if (!node) return Type::getVoidTy(ctx);

    if (node->node_type == ASTNodeType::ArrayType) {
        Type* elem = node->left ? razenType(node->left) : Type::getInt8Ty(ctx);
        if (node->middle && node->middle->token) {
            unsigned sz = std::stoul(node->middle->token->value);
            return ArrayType::get(elem, sz);
        }
        return PointerType::getUnqual(ctx);
    }

    if (node->node_type != ASTNodeType::VarType) return Type::getVoidTy(ctx);
    auto tok = node->token;
    if (!tok) return Type::getVoidTy(ctx);

    auto& name = tok->value;
    auto tt = tok->type;

    // Pointer: *T
    if (tt == TokenType::Star) {
        Type* inner = node->left ? razenType(node->left) : Type::getInt8Ty(ctx);
        return PointerType::getUnqual(inner);
    }

    // Optional: ?T  →  { i1, T }
    if (tt == TokenType::QuestionMark) {
        Type* inner = node->left ? razenType(node->left) : Type::getVoidTy(ctx);
        return StructType::get(ctx, {Type::getInt1Ty(ctx), inner});
    }

    // Failable: !T  →  { i1, T }
    if (tt == TokenType::ExclamationMark) {
        Type* inner = node->left ? razenType(node->left) : Type::getVoidTy(ctx);
        return StructType::get(ctx, {Type::getInt1Ty(ctx), inner});
    }

    // Error union: Error!T  →  { i1, T }
    if (tt == TokenType::Identifier && node->left) {
        Type* inner = razenType(node->left);
        return StructType::get(ctx, {Type::getInt1Ty(ctx), inner});
    }

    // Self reference
    if (name == "Self" && !current_struct.empty()) {
        auto it = types.structs.find(current_struct);
        if (it != types.structs.end())
            return PointerType::getUnqual(it->second);
        return PointerType::getUnqual(ctx);
    }

    // Named struct/union
    if (tt == TokenType::Identifier) {
        auto sit = types.structs.find(name);
        if (sit != types.structs.end()) return sit->second;
        auto ait = types.aliases.find(name);
        if (ait != types.aliases.end()) return ait->second;
        return Type::getInt32Ty(ctx);
    }

    return primitiveType(ctx, name);
}

Type* IRGen::razenType(const TypeInfo* ti) {
    if (!ti) return Type::getVoidTy(ctx);
    switch (ti->category) {
        case TypeCategory::Void: case TypeCategory::Noret:
            return Type::getVoidTy(ctx);
        case TypeCategory::Bool:
            return Type::getInt1Ty(ctx);
        case TypeCategory::Integer: {
            auto& n = ti->name;
            if (n == "i8" || n == "u8") return Type::getInt8Ty(ctx);
            if (n == "i16" || n == "u16") return Type::getInt16Ty(ctx);
            if (n == "i32" || n == "u32" || n == "int" || n == "uint") return Type::getInt32Ty(ctx);
            if (n == "i64" || n == "u64" || n == "isize" || n == "usize") return Type::getInt64Ty(ctx);
            if (n == "i128" || n == "u128") return Type::getInt128Ty(ctx);
            return Type::getInt32Ty(ctx);
        }
        case TypeCategory::Float: {
            auto& n = ti->name;
            if (n == "f16") return Type::getHalfTy(ctx);
            if (n == "f32" || n == "float") return Type::getFloatTy(ctx);
            if (n == "f64") return Type::getDoubleTy(ctx);
            if (n == "f128") return Type::getFP128Ty(ctx);
            return Type::getDoubleTy(ctx);
        }
        case TypeCategory::Char:
            return Type::getInt8Ty(ctx);
        case TypeCategory::Str: case TypeCategory::String: case TypeCategory::Any:
            return PointerType::getUnqual(ctx);
        case TypeCategory::Pointer:
            return PointerType::getUnqual(ctx);
        case TypeCategory::Optional: case TypeCategory::Failable: case TypeCategory::ErrorUnion: {
            Type* inner = ti->elem_type ? razenType(ti->elem_type) : Type::getVoidTy(ctx);
            return StructType::get(ctx, {Type::getInt1Ty(ctx), inner});
        }
        case TypeCategory::Array: {
            Type* elem = ti->elem_type ? razenType(ti->elem_type) : Type::getInt8Ty(ctx);
            if (ti->array_size > 0)
                return ArrayType::get(elem, ti->array_size);
            return PointerType::getUnqual(ctx);
        }
        case TypeCategory::Struct: {
            auto it = types.structs.find(ti->name);
            if (it != types.structs.end()) return it->second;
            return Type::getInt32Ty(ctx);
        }
        case TypeCategory::Enum:
            return Type::getInt32Ty(ctx);
        case TypeCategory::Union: {
            auto it = types.structs.find(ti->name);
            if (it != types.structs.end()) return it->second;
            return Type::getInt32Ty(ctx);
        }
        case TypeCategory::ErrorSet:
            return Type::getInt32Ty(ctx);
        default:
            return Type::getInt32Ty(ctx);
    }
}

Type* IRGen::resolveType(ASTNode* node) {
    if (!node) return Type::getVoidTy(ctx);
    if (node->node_type == ASTNodeType::VarType &&
        node->token && node->token->value == "Self") {
        auto it = types.structs.find(current_struct);
        if (it != types.structs.end())
            return PointerType::getUnqual(it->second);
        return PointerType::getUnqual(ctx);
    }
    return razenType(node);
}

// ── Alloca helper (mem2reg-friendly) ────────────────────────────────────────

void IRGen::createEntryBlockAlloca(Function* fn, const std::string& name, Type* ty) {
    IRBuilder<> tmp(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    auto* alloca = tmp.CreateAlloca(ty, nullptr, name);
    named_values[name] = alloca;
    named_types[name] = ty;
}

Value* IRGen::loadVariable(const std::string& name) {
    auto it = named_values.find(name);
    if (it == named_values.end()) return nullptr;
    return builder.CreateLoad(named_types[name], it->second, name.c_str());
}

void IRGen::storeVariable(const std::string& name, Value* val) {
    auto it = named_values.find(name);
    if (it == named_values.end()) return;
    builder.CreateStore(val, it->second);
}

// ── GEP helpers ─────────────────────────────────────────────────────────────

Value* IRGen::createGEP(Value* ptr, Type* ty, unsigned idx) {
    return builder.CreateStructGEP(ty, ptr, idx);
}

Value* IRGen::createStructGEP(Value* ptr, Type* ty, unsigned, unsigned idx1) {
    return builder.CreateGEP(ty, ptr, {ConstantInt::get(Type::getInt32Ty(ctx), 0),
                                        ConstantInt::get(Type::getInt32Ty(ctx), idx1)});
}

// ── Pointee type tracking (opaque ptr workaround) ──────────────────────────

void IRGen::setPointeeType(Value* ptr, Type* ty) {
    if (ptr && ty) pointee_types[ptr] = ty;
}

Type* IRGen::getPointeeType(Value* ptr) {
    if (!ptr) return Type::getInt32Ty(ctx);
    // Check our map first
    auto it = pointee_types.find(ptr);
    if (it != pointee_types.end()) return it->second;
    // Try LLVM instruction introspection
    if (auto* ai = dyn_cast<AllocaInst>(ptr))
        return ai->getAllocatedType();
    if (auto* gep = dyn_cast<GetElementPtrInst>(ptr))
        return gep->getResultElementType();
    if (auto* gv = dyn_cast<GlobalVariable>(ptr))
        return gv->getValueType();
    if (auto* li = dyn_cast<LoadInst>(ptr)) {
        // If loading from a pointer, try to propagate pointee type
        auto* src = li->getPointerOperand();
        return getPointeeType(src);
    }
    // Fallback
    return Type::getInt32Ty(ctx);
}

// ── Integer widening (signed/unsigned aware) ───────────────────────────────

Value* IRGen::widenInt(Value* val, Type* target, bool is_unsigned) {
    if (!val || !target || val->getType() == target) return val;
    if (!isIntTy(val->getType()) || !isIntTy(target)) return val;
    auto w1 = cast<IntegerType>(val->getType())->getBitWidth();
    auto w2 = cast<IntegerType>(target)->getBitWidth();
    if (w1 < w2) {
        return is_unsigned ? builder.CreateZExt(val, target)
                           : builder.CreateSExt(val, target);
    }
    if (w1 > w2) return builder.CreateTrunc(val, target);
    return val;
}

// ── Module dumping ──────────────────────────────────────────────────────────

std::string IRGen::dumpModule(Module& m) {
    std::string str;
    raw_string_ostream os(str);
    m.print(os, nullptr);
    os.flush();
    return str;
}

} // namespace codegen
} // namespace razen
