#include "ir.h"
#include "../ast/token_utils.h"

namespace razen {
namespace codegen {

void IRBuilder::emitLine(const std::string& line) {
    module << indent_str << line << "\n";
}

void IRBuilder::emitRaw(const std::string& text) {
    module << text;
}

void IRBuilder::emitPreamble(const std::string& source_name) {
    emitLine("; ModuleID = '" + source_name + "'");
    emitLine("source_filename = \"" + source_name + "\"");
    emitLine("target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"");
    emitLine("target triple = \"x86_64-pc-linux-gnu\"");
    emitLine("");
}

void IRBuilder::emitLibcDecls() {
    emitLine("declare i32 @printf(i8*, ...)");
    emitLine("declare i32 @puts(i8*)");
    emitLine("declare void @exit(i32)");
    emitLine("declare void @abort() noreturn");
    emitLine("");
}

std::string IRBuilder::tmp(const std::string& prefix) {
    return prefix + "." + std::to_string(tmp_counter++);
}

std::string IRBuilder::label(const std::string& prefix) {
    return prefix + "." + std::to_string(label_counter++);
}

std::string IRBuilder::strName() {
    return "@.str." + std::to_string(str_counter++);
}

std::string IRBuilder::getIR() {
    return module.str();
}

std::string typeToLLVM(const TypeInfo* type) {
    if (!type) return "void";
    switch (type->category) {
        case TypeCategory::Void:    return "void";
        case TypeCategory::Bool:    return "i1";
        case TypeCategory::Integer: {
            auto& name = type->name;
            if (name == "i1" || name == "u1") return "i1";
            if (name == "i2" || name == "u2") return "i2";
            if (name == "i4" || name == "u4") return "i4";
            if (name == "i8" || name == "u8") return "i8";
            if (name == "i16" || name == "u16") return "i16";
            if (name == "i32" || name == "u32" || name == "int" || name == "uint") return "i32";
            if (name == "i64" || name == "u64" || name == "isize" || name == "usize") return "i64";
            if (name == "i128" || name == "u128") return "i128";
            return "i32";
        }
        case TypeCategory::Float: {
            auto& name = type->name;
            if (name == "f16") return "half";
            if (name == "f32" || name == "float") return "float";
            if (name == "f64") return "double";
            if (name == "f128") return "fp128";
            return "double";
        }
        case TypeCategory::Char:    return "i8";
        case TypeCategory::Str:     return "i32";
        case TypeCategory::String:  return "i32";
        case TypeCategory::Any:     return "i8*";
        case TypeCategory::Pointer: return "ptr";
        case TypeCategory::Noret:   return "void";
        case TypeCategory::Optional: {
            auto inner = type->elem_type ? typeToLLVM(type->elem_type) : "void";
            return "{ i1, " + inner + " }";
        }
        case TypeCategory::Failable: {
            auto inner = type->elem_type ? typeToLLVM(type->elem_type) : "void";
            return "{ i1, " + inner + " }";
        }
        case TypeCategory::ErrorUnion: {
            auto ok = type->ok_type ? typeToLLVM(type->ok_type) : "void";
            return "{ i1, " + ok + " }";
        }
        case TypeCategory::Array: {
            auto elem = type->elem_type ? typeToLLVM(type->elem_type) : "i8";
            if (type->array_size > 0)
                return "[" + std::to_string(type->array_size) + " x " + elem + "]";
            return "ptr";
        }
        case TypeCategory::Struct:  return "%" + type->name;
        case TypeCategory::Enum:    return "i32";
        case TypeCategory::Union:   return "%" + type->name;
        case TypeCategory::ErrorSet: return "i32";
        default: return "void";
    }
}

std::string typeToLLVM(ASTNode* type_node) {
    if (!type_node) return "void";
    if (type_node->node_type == ASTNodeType::ArrayType) {
        auto elem = type_node->left ? typeToLLVM(type_node->left) : "i8";
        std::string size_str;
        if (type_node->middle && type_node->middle->token) {
            size_str = "[" + type_node->middle->token->value + " x ";
        } else {
            return "ptr";
        }
        return size_str + elem + "]";
    }
    if (type_node->node_type != ASTNodeType::VarType) return "void";
    auto tok = type_node->token;
    if (!tok) return "void";
    auto tt = tok->type;
    auto& name = tok->value;

    if (isIntegerType(tt)) {
        if (name == "i1" || name == "u1") return "i1";
        if (name == "i2" || name == "u2") return "i2";
        if (name == "i4" || name == "u4") return "i4";
        if (name == "i8" || name == "u8") return "i8";
        if (name == "i16" || name == "u16") return "i16";
        if (name == "i32" || name == "u32" || name == "int" || name == "uint") return "i32";
        if (name == "i64" || name == "u64" || name == "isize" || name == "usize") return "i64";
        if (name == "i128" || name == "u128") return "i128";
        return "i32";
    }
    if (isFloatType(tt)) {
        if (name == "f16") return "half";
        if (name == "f32" || name == "float") return "float";
        if (name == "f64") return "double";
        if (name == "f128") return "fp128";
        return "double";
    }

    switch (tt) {
        case TokenType::Bool:   return "i1";
        case TokenType::Char:   return "i8";
        case TokenType::Void:   return "void";
        case TokenType::Noret:  return "void";
        case TokenType::Str:    return "i32";
        case TokenType::String: return "i32";
        case TokenType::Any:    return "i8*";
        case TokenType::Star:   return "ptr";
        case TokenType::QuestionMark: {
            auto inner = type_node->left ? typeToLLVM(type_node->left) : "void";
            return "{ i1, " + inner + " }";
        }
        case TokenType::ExclamationMark: {
            auto inner = type_node->left ? typeToLLVM(type_node->left) : "void";
            return "{ i1, " + inner + " }";
        }
        default: {
            if (type_node->left) return typeToLLVM(type_node->left);
            return "void";
        }
    }
}

} // namespace codegen
} // namespace razen
