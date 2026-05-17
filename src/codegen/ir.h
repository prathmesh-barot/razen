#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/DerivedTypes.h>
#include "../ast/node.h"
#include "../semantic/type_info.h"

namespace razen {
namespace codegen {

// ── Type helpers ──────────────────────────────────────────────────────────────
inline bool isIntTy(llvm::Type* ty) { return ty && ty->isIntegerTy(); }
inline bool isFloatTy(llvm::Type* ty) {
    return ty && (ty->isHalfTy() || ty->isFloatTy() || ty->isDoubleTy() || ty->isFP128Ty());
}

// ── Named type registry ─────────────────────────────────────────────────────
struct TypeRegistry {
    std::unordered_map<std::string, llvm::StructType*> structs;
    std::unordered_map<std::string, llvm::Type*> aliases;
    std::unordered_map<std::string, std::vector<std::string>> field_names;
};

// ── LLVM IR generation context ──────────────────────────────────────────────
struct IRGen {
    llvm::LLVMContext ctx;
    llvm::Module module;
    llvm::IRBuilder<> builder;
    TypeRegistry types;

    // ── Variable scope: name → alloca instruction ──
    std::unordered_map<std::string, llvm::AllocaInst*> named_values;
    std::unordered_map<std::string, llvm::Type*> named_types;

    // ── Control-flow labels ──
    llvm::BasicBlock* loop_continue = nullptr;
    llvm::BasicBlock* loop_end = nullptr;
    std::string current_func_name;
    llvm::Type* current_ret_type = nullptr;
    llvm::Function* current_llvm_function = nullptr;
    bool has_return = false;

    // ── Deferred statements (LIFO flush at scope exit) ──
    std::vector<ASTNode*> deferred;

    // ── Type metadata collected during AST walk ──
    struct EnumInfo {
        llvm::Type* backing;
        std::unordered_map<std::string, int> values;
    };
    struct UnionVariant {
        std::string name;
        int tag;
        llvm::Type* payload_type;
    };
    struct UnionInfo {
        llvm::StructType* ll_type;
        int max_payload;
        std::vector<UnionVariant> variants;
    };

    std::unordered_map<std::string, EnumInfo> enums;
    std::unordered_map<std::string, UnionInfo> unions;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> errors;

    // struct method list (for mangling)
    std::unordered_map<std::string, std::vector<ASTNode*>> struct_methods;
    std::string current_struct;

    // ── Runtime global init tracking ──
    struct GlobalInit {
        llvm::GlobalVariable* gv;
        ASTNode* init_node;
    };
    std::vector<GlobalInit> pending_global_inits;
    void emitGlobalCtors();

    // ── String literal dedup ──
    std::unordered_map<std::string, llvm::GlobalVariable*> string_globals;

    explicit IRGen(const std::string& source = "main.rz")
        : module(source, ctx), builder(ctx) {}
    IRGen(const IRGen&) = delete;
    IRGen& operator=(const IRGen&) = delete;
    IRGen(IRGen&&) = delete;
    IRGen& operator=(IRGen&&) = delete;

    // ── Public API ──
    llvm::Type* razenType(ASTNode* node);
    llvm::Type* razenType(const TypeInfo* ti);
    llvm::Type* resolveType(ASTNode* node);
    llvm::Value* genNode(ASTNode* node);
    llvm::Value* genExpr(ASTNode* node);
    void genTopLevel(ASTNode* node);
    void genFunc(ASTNode* node);
    void genVar(ASTNode* node, bool is_const);
    void genBlock(ASTNode* node);
    void genReturn(ASTNode* node);
    void genIf(ASTNode* node);
    void genLoop(ASTNode* node);
    void genAssign(ASTNode* node);
    void genMatch(ASTNode* node);
    void genEnum(ASTNode* node);
    void genUnion(ASTNode* node);
    void genError(ASTNode* node);
    void genStruct(ASTNode* node);
    void genExt(ASTNode* node);

    llvm::Value* genLiteral(ASTNode* node);
    llvm::Value* genIdentifier(ASTNode* node);
    llvm::Value* genBinary(ASTNode* node);
    llvm::Value* genUnary(ASTNode* node);
    llvm::Value* genCall(ASTNode* node);
    llvm::Value* genMemberAccess(ASTNode* node);
    llvm::Value* genArrayLiteral(ASTNode* node);
    llvm::Value* genTupleLiteral(ASTNode* node);
    llvm::Value* genRangeLiteral(ASTNode* node);
    llvm::Value* genTryExpr(ASTNode* node);
    llvm::Value* genUnionConstruct(ASTNode* node);

    void emitDeferred();
    void createEntryBlockAlloca(llvm::Function* fn, const std::string& name, llvm::Type* ty);
    llvm::Value* loadVariable(const std::string& name);
    void storeVariable(const std::string& name, llvm::Value* val);
    llvm::Value* createGEP(llvm::Value* ptr, llvm::Type* ty, unsigned idx);
    llvm::Value* createStructGEP(llvm::Value* ptr, llvm::Type* ty, unsigned idx0, unsigned idx1);
    llvm::Value* evalConstantNode(ASTNode* node);

    std::string dumpIR() { return dumpModule(module); }
    static std::string dumpModule(llvm::Module& m);
};

} // namespace codegen
} // namespace razen
