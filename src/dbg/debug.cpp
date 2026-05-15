#include "debug.h"
#include <iostream>
#include <iomanip>
#include <cstring>

namespace razen {

static const auto& RST = RESET;
static const auto& GREY_ = GREY;
static const auto& ORANGE_ = ORANGE;
static const auto& CYAN_ = CYAN;
static const auto& CREAM_ = CREAM;
static const auto& GREEN_ = GREEN;
static const auto& LGREEN_ = LIGHT_GREEN;
static const auto& PEACH_ = PEACH;
static const auto& MAGENTA_ = MAGENTA;
static const auto& BLUE_ = BLUE;
static const auto& YELLOW_ = YELLOW;

const char* tokenTypeName(TokenType tt) {
    switch (tt) {
        case TokenType::Identifier: return "Identifier";
        case TokenType::IntegerValue: return "IntegerValue";
        case TokenType::DecimalValue: return "DecimalValue";
        case TokenType::CharValue: return "CharValue";
        case TokenType::StringValue: return "StringValue";
        case TokenType::I1: return "I1";
        case TokenType::I2: return "I2";
        case TokenType::I4: return "I4";
        case TokenType::I8: return "I8";
        case TokenType::I16: return "I16";
        case TokenType::I32: return "I32";
        case TokenType::I64: return "I64";
        case TokenType::I128: return "I128";
        case TokenType::Isize: return "Isize";
        case TokenType::Int: return "Int";
        case TokenType::U1: return "U1";
        case TokenType::U2: return "U2";
        case TokenType::U4: return "U4";
        case TokenType::U8: return "U8";
        case TokenType::U16: return "U16";
        case TokenType::U32: return "U32";
        case TokenType::U64: return "U64";
        case TokenType::U128: return "U128";
        case TokenType::Usize: return "Usize";
        case TokenType::Uint: return "Uint";
        case TokenType::F16: return "F16";
        case TokenType::F32: return "F32";
        case TokenType::F64: return "F64";
        case TokenType::F128: return "F128";
        case TokenType::Float: return "Float";
        case TokenType::Bool: return "Bool";
        case TokenType::Char: return "Char";
        case TokenType::Void: return "Void";
        case TokenType::Noret: return "Noret";
        case TokenType::Any: return "Any";
        case TokenType::Str: return "Str";
        case TokenType::String: return "String";
        case TokenType::Type: return "Type";
        case TokenType::Enum: return "Enum";
        case TokenType::Union: return "Union";
        case TokenType::Error: return "Error";
        case TokenType::Struct: return "Struct";
        case TokenType::Behave: return "Behave";
        case TokenType::Ext: return "Ext";
        case TokenType::Func: return "Func";
        case TokenType::Pub: return "Pub";
        case TokenType::Mod: return "Mod";
        case TokenType::Use: return "Use";
        case TokenType::Const: return "Const";
        case TokenType::Mut: return "Mut";
        case TokenType::If: return "If";
        case TokenType::Else: return "Else";
        case TokenType::Match: return "Match";
        case TokenType::Loop: return "Loop";
        case TokenType::Ret: return "Ret";
        case TokenType::Break: return "Break";
        case TokenType::Skip: return "Skip";
        case TokenType::Try: return "Try";
        case TokenType::Catch: return "Catch";
        case TokenType::Defer: return "Defer";
        case TokenType::Test: return "Test";
        case TokenType::True: return "True";
        case TokenType::False: return "False";
        case TokenType::Async: return "Async";
        case TokenType::Needs: return "Needs";
        case TokenType::Comment: return "Comment";
        case TokenType::Equals: return "Equals";
        case TokenType::ColonEquals: return "ColonEquals";
        case TokenType::PlusEquals: return "PlusEquals";
        case TokenType::MinusEquals: return "MinusEquals";
        case TokenType::StarEquals: return "StarEquals";
        case TokenType::SlashEquals: return "SlashEquals";
        case TokenType::PercentEquals: return "PercentEquals";
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::Slash: return "Slash";
        case TokenType::Percent: return "Percent";
        case TokenType::EqualsEquals: return "EqualsEquals";
        case TokenType::NotEquals: return "NotEquals";
        case TokenType::LessThan: return "LessThan";
        case TokenType::LessThanEquals: return "LessThanEquals";
        case TokenType::GreaterThan: return "GreaterThan";
        case TokenType::GreaterThanEquals: return "GreaterThanEquals";
        case TokenType::ExclamationMark: return "ExclamationMark";
        case TokenType::AndAnd: return "AndAnd";
        case TokenType::OrOr: return "OrOr";
        case TokenType::And: return "And";
        case TokenType::Or: return "Or";
        case TokenType::Caret: return "Caret";
        case TokenType::Tilde: return "Tilde";
        case TokenType::ShiftLeft: return "ShiftLeft";
        case TokenType::ShiftRight: return "ShiftRight";
        case TokenType::Dot: return "Dot";
        case TokenType::Comma: return "Comma";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Colon: return "Colon";
        case TokenType::QuestionMark: return "QuestionMark";
        case TokenType::LeftParen: return "LeftParen";
        case TokenType::RightParen: return "RightParen";
        case TokenType::LeftBrace: return "LeftBrace";
        case TokenType::RightBrace: return "RightBrace";
        case TokenType::LeftBracket: return "LeftBracket";
        case TokenType::RightBracket: return "RightBracket";
        case TokenType::Arrow: return "Arrow";
        case TokenType::BigArrow: return "BigArrow";
        case TokenType::TildeArrow: return "TildeArrow";
        case TokenType::DotDotDot: return "DotDotDot";
        case TokenType::DotDot: return "DotDot";
        case TokenType::DotDotEquals: return "DotDotEquals";
        case TokenType::At: return "At";
        case TokenType::EOF_: return "EOF";
    }
    return "Unknown";
}

const char* nodeTypeName(ASTNodeType nt) {
    switch (nt) {
        case ASTNodeType::Invalid: return "Invalid";
        case ASTNodeType::IntegerLiteral: return "IntegerLiteral";
        case ASTNodeType::FloatLiteral: return "FloatLiteral";
        case ASTNodeType::StringLiteral: return "StringLiteral";
        case ASTNodeType::CharLiteral: return "CharLiteral";
        case ASTNodeType::BoolLiteral: return "BoolLiteral";
        case ASTNodeType::TupleLiteral: return "TupleLiteral";
        case ASTNodeType::ArrayLiteral: return "ArrayLiteral";
        case ASTNodeType::ArrayType: return "ArrayType";
        case ASTNodeType::VarDeclaration: return "VarDeclaration";
        case ASTNodeType::ConstDeclaration: return "ConstDeclaration";
        case ASTNodeType::TypeAliasDeclaration: return "TypeAliasDeclaration";
        case ASTNodeType::EnumDeclaration: return "EnumDeclaration";
        case ASTNodeType::EnumField: return "EnumField";
        case ASTNodeType::StructDeclaration: return "StructDeclaration";
        case ASTNodeType::StructField: return "StructField";
        case ASTNodeType::UnionDeclaration: return "UnionDeclaration";
        case ASTNodeType::UnionField: return "UnionField";
        case ASTNodeType::ErrorDeclaration: return "ErrorDeclaration";
        case ASTNodeType::ErrorField: return "ErrorField";
        case ASTNodeType::ModuleDeclaration: return "ModuleDeclaration";
        case ASTNodeType::UseDeclaration: return "UseDeclaration";
        case ASTNodeType::BehaveDeclaration: return "BehaveDeclaration";
        case ASTNodeType::ExtDeclaration: return "ExtDeclaration";
        case ASTNodeType::Annotation: return "Annotation";
        case ASTNodeType::FunctionDeclaration: return "FunctionDeclaration";
        case ASTNodeType::Parameter: return "Parameter";
        case ASTNodeType::Parameters: return "Parameters";
        case ASTNodeType::VarType: return "VarType";
        case ASTNodeType::ReturnType: return "ReturnType";
        case ASTNodeType::GenericParams: return "GenericParams";
        case ASTNodeType::BinaryExpression: return "BinaryExpression";
        case ASTNodeType::UnaryExpression: return "UnaryExpression";
        case ASTNodeType::RangeExpression: return "RangeExpression";
        case ASTNodeType::MemberAccess: return "MemberAccess";
        case ASTNodeType::Identifier: return "Identifier";
        case ASTNodeType::BuiltinExpression: return "BuiltinExpression";
        case ASTNodeType::ReturnStatement: return "ReturnStatement";
        case ASTNodeType::BreakStatement: return "BreakStatement";
        case ASTNodeType::SkipStatement: return "SkipStatement";
        case ASTNodeType::TryBlock: return "TryBlock";
        case ASTNodeType::Assignment: return "Assignment";
        case ASTNodeType::Block: return "Block";
        case ASTNodeType::IfStatement: return "IfStatement";
        case ASTNodeType::ElseIfStatement: return "ElseIfStatement";
        case ASTNodeType::IfBody: return "IfBody";
        case ASTNodeType::ElseBody: return "ElseBody";
        case ASTNodeType::LoopStatement: return "LoopStatement";
        case ASTNodeType::LoopBody: return "LoopBody";
        case ASTNodeType::MatchStatement: return "MatchStatement";
        case ASTNodeType::MatchCase: return "MatchCase";
        case ASTNodeType::MatchBody: return "MatchBody";
        case ASTNodeType::CaptureBlock: return "CaptureBlock";
        case ASTNodeType::TryExpression: return "TryExpression";
        case ASTNodeType::CatchExpression: return "CatchExpression";
        case ASTNodeType::DeferStatement: return "DeferStatement";
        case ASTNodeType::FunctionCall: return "FunctionCall";
        case ASTNodeType::Argument: return "Argument";
        case ASTNodeType::Comment: return "Comment";
    }
    return "Unknown";
}

static const char* nodeColour(ASTNodeType nt) {
    switch (nt) {
        case ASTNodeType::FunctionDeclaration: return LGREEN_;
        case ASTNodeType::VarDeclaration:
        case ASTNodeType::ConstDeclaration:
        case ASTNodeType::TypeAliasDeclaration:
        case ASTNodeType::EnumDeclaration:
        case ASTNodeType::StructDeclaration:
        case ASTNodeType::UnionDeclaration:
        case ASTNodeType::ErrorDeclaration:
        case ASTNodeType::BehaveDeclaration:
        case ASTNodeType::ExtDeclaration:
        case ASTNodeType::ModuleDeclaration:
        case ASTNodeType::UseDeclaration: return GREEN_;
        case ASTNodeType::ReturnStatement: return CYAN_;
        case ASTNodeType::CaptureBlock:
        case ASTNodeType::BreakStatement:
        case ASTNodeType::SkipStatement: return CYAN_;
        case ASTNodeType::IfStatement:
        case ASTNodeType::LoopStatement:
        case ASTNodeType::MatchStatement:
        case ASTNodeType::DeferStatement: return MAGENTA_;
        case ASTNodeType::BinaryExpression:
        case ASTNodeType::UnaryExpression:
        case ASTNodeType::TryExpression:
        case ASTNodeType::CatchExpression: return YELLOW_;
        case ASTNodeType::IntegerLiteral:
        case ASTNodeType::FloatLiteral:
        case ASTNodeType::BoolLiteral:
        case ASTNodeType::CharLiteral:
        case ASTNodeType::StringLiteral:
        case ASTNodeType::TupleLiteral:
        case ASTNodeType::ArrayLiteral: return ORANGE_;
        case ASTNodeType::Identifier:
        case ASTNodeType::EnumField:
        case ASTNodeType::StructField:
        case ASTNodeType::UnionField:
        case ASTNodeType::ErrorField: return PEACH_;
        case ASTNodeType::FunctionCall: return BLUE_;
        case ASTNodeType::ElseIfStatement:
        case ASTNodeType::TryBlock: return MAGENTA_;
        case ASTNodeType::RangeExpression: return YELLOW_;
        case ASTNodeType::GenericParams: return CREAM_;
        case ASTNodeType::Parameter:
        case ASTNodeType::Parameters: return CREAM_;
        case ASTNodeType::Assignment: return YELLOW_;
        case ASTNodeType::Block:
        case ASTNodeType::IfBody:
        case ASTNodeType::ElseBody:
        case ASTNodeType::LoopBody:
        case ASTNodeType::MatchBody:
        case ASTNodeType::MatchCase:
        case ASTNodeType::VarType:
        case ASTNodeType::ReturnType:
        case ASTNodeType::ArrayType: return GREY_;
        default: return RST;
    }
}

void printTokens(const std::vector<Token>& token_list) {
    size_t n = token_list.size();
    std::cout << CYAN_ << "Tokens" << RST << " (" << n << ")\n";

    if (n == 0) {
        std::cout << "  " << GREY_ << "(none)" << RST << "\n";
        return;
    }

    for (size_t i = 0; i < token_list.size(); ++i) {
        const Token& tok = token_list[i];
        // Match Zig format exactly: [{d:>3}] — right-aligned index in 3-char field
        std::cout << "  " << GREY_ << "[" << std::setw(3) << i << "]" << RST
                  << "  " << GREY_ << "Type:" << RST << " " << CREAM_
                  << tokenTypeName(tok.type) << RST
                  << "  " << GREY_ << "Value:" << RST << " " << ORANGE_
                  << "'" << tok.value << "'" << RST
                  << "  " << GREY_ << "Line:" << RST << " " << tok.line << "\n";
    }
    std::cout << "\n";
}

void printNode(const ASTNode* n, size_t depth) {
    std::cout << nodeColour(n->node_type) << nodeTypeName(n->node_type) << RST;

    if (n->token) {
        if (!n->token->value.empty()) {
            std::cout << "  " << ORANGE_ << n->token->value << RST;
        }
    }

    if (n->is_pub) std::cout << "  " << GREY_ << "pub" << RST;
    if (n->is_const) std::cout << "  " << GREY_ << "const" << RST;
    if (n->is_mut) std::cout << "  " << GREY_ << "mut" << RST;
    if (n->is_global) std::cout << "  " << GREY_ << "global" << RST;

    std::cout << "\n";

    if (n->left) {
        for (size_t i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << GREY_ << "\u2514\u2500 left:" << RST << " ";
        printNode(n->left, depth + 1);
    }
    if (n->middle) {
        for (size_t i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << GREY_ << "\u2514\u2500 mid:" << RST << " ";
        printNode(n->middle, depth + 1);
    }
    if (n->right) {
        for (size_t i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << GREY_ << "\u2514\u2500 right:" << RST << " ";
        printNode(n->right, depth + 1);
    }

    if (n->children) {
        for (auto* child : *n->children) {
            for (size_t i = 0; i < depth; ++i) std::cout << "  ";
            std::cout << GREY_ << "\u2514\u2500 item:" << RST << " ";
            printNode(child, depth + 1);
        }
    }
}

void printAST(const std::vector<ASTNode*>& ast_nodes) {
    size_t n = ast_nodes.size();
    std::cout << CYAN_ << "AST" << RST << " (" << n << " top-level node"
              << (n == 1 ? "" : "s") << ")\n";

    if (n == 0) {
        std::cout << "  " << GREY_ << "(empty)" << RST << "\n";
    } else {
        for (auto* node : ast_nodes) {
            std::cout << "  ";
            printNode(node, 1);
        }
    }
    std::cout << "\n";
}

} // namespace razen
