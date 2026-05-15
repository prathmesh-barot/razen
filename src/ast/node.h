#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "../lexer/lexer.h"

namespace razen {

enum class ASTNodeType {
    Invalid,

    // literals
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    CharLiteral,
    BoolLiteral,
    ArrayLiteral,
    TupleLiteral,
    ArrayType,

    // declarations
    VarDeclaration,
    ConstDeclaration,
    TypeAliasDeclaration,
    EnumDeclaration,
    EnumField,
    StructDeclaration,
    StructField,
    UnionDeclaration,
    UnionField,
    ErrorDeclaration,
    ErrorField,
    ModuleDeclaration,
    UseDeclaration,
    BehaveDeclaration,
    ExtDeclaration,
    Annotation,

    // functions
    FunctionDeclaration,
    Parameter,
    Parameters,

    // types
    VarType,
    ReturnType,
    GenericParams,

    // expressions
    BinaryExpression,
    UnaryExpression,
    MemberAccess,
    Identifier,
    BuiltinExpression,
    RangeExpression,

    // statements
    ReturnStatement,
    Assignment,
    BreakStatement,
    SkipStatement,
    TryBlock,

    // block
    Block,

    // control flow
    IfStatement,
    ElseIfStatement,
    IfBody,
    ElseBody,
    LoopStatement,
    LoopBody,
    MatchStatement,
    MatchCase,
    MatchBody,
    CaptureBlock,
    TryExpression,
    CatchExpression,
    DeferStatement,

    // calls
    FunctionCall,
    Argument,

    // comment
    Comment,
};

struct ASTNode {
    ASTNodeType node_type = ASTNodeType::Invalid;

    // the token that best represents this node
    std::optional<Token> token;

    // child nodes
    ASTNode* left = nullptr;
    ASTNode* middle = nullptr;
    ASTNode* right = nullptr;
    std::vector<ASTNode*>* children = nullptr;

    // flags
    bool is_const = false;
    bool is_mut = false;
    bool is_global = false;
    bool is_pub = false;
    bool is_async = false;

    // ownership
    bool owns_children = true;
};

} // namespace razen
