#include <iostream>
#include "lexer/lexer.h"
#include "lexer/errors.h"
#include "ast/errors.h"
#include "ast/ast_utils.h"
#include "parser/parser.h"
#include "dbg/debug.h"
#include "semantic/analyzer.h"
#include "semantic/symbol_table.h"
#include "samples/sample1.h"
#include "samples/sample2.h"
#include "samples/sample3.h"
#include "samples/sample4.h"
#include "samples/sample5.h"
#include "samples/comments.h"
#include "samples/stage2.h"
#include "samples/semantic_errors.h"
#include "codegen/codegen.h"

using namespace razen;

static void convertCode(const char* label, const char* source) {
    std::cout << "\n" << CYAN << "\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81 " << label
              << " \xe2\x94\x81\xe2\x94\x81\xe2\x94\x81" << RESET << "\n";
    std::cout << GREY << "Source:" << RESET << "\n" << source << "\n\n";

    // Phase 1 — lex
    std::cout << CREAM << "Phase 1" << RESET << "  ";
    std::vector<Token> token_list;
    try {
        token_list = parseToTokens(source);
    } catch (const LexerError& err) {
        std::cout << RED << "Lexer error: " << err.what() << RESET << "\n";
        return;
    }
    printTokens(token_list);

    // Phase 2 — AST
    std::cout << CREAM << "Phase 2" << RESET << "  ";
    std::vector<ASTNode*> ast_nodes;
    try {
        ast_nodes = buildAST(token_list, source);
    } catch (const AstError& err) {
        std::cout << RED << "AST error: " << err.what() << RESET << "\n";
        return;
    }
    printAST(ast_nodes);

    // Phase 3 — Semantic
    std::cout << CREAM << "Phase 3" << RESET << "  ";
    auto* gs = new Scope();
    SymbolTable st(gs, gs);

    std::unordered_set<std::string> whitelist = {
        "std", "self", "true", "false", "null",
        "print", "println", "eprint", "eprintln",
        "exit", "assert", "panic", "clock_ms", "clock_ns"
    };

    Analyzer analyzer(gs, gs, std::move(whitelist), std::move(st));
    analyzer.analyze(ast_nodes);

    if (analyzer.has_errors) {
        std::cout << "\n" << RED << "Compilation failed due to semantic errors." << RESET << "\n";
        freeASTNodes(ast_nodes);
        return;
    }
    std::cout << "\t\tSemantic Analysis\t\t\tDone\n";

    // Phase 4 — LLVM IR Codegen
    std::cout << CREAM << "Phase 4" << RESET << "  ";
    codegen::Codegen cg(label);
    cg.generate(ast_nodes);
    std::string ir_output = cg.getIR();
    std::cout << "\t\tLLVM IR Codegen\t\t\t\tDone\n";
    std::cout << "\n" << PEACH << "--- LLVM IR ---" << RESET << "\n";
    std::cout << ir_output;
    if (ir_output.size() > 500) {
        std::cout << "... [output truncated, " << ir_output.size() << " bytes total]\n";
    }
    std::cout << PEACH << "--- End LLVM IR ---" << RESET << "\n";

    freeASTNodes(ast_nodes);
}

int main() {
    std::cout << LIGHT_GREEN << "Razen Lang - C++ Full Frontend Test" << RESET << "\n";
    std::cout << GREY << "Lexer + AST Builder + Semantic Analyzer (Phase 1, 2 & 3)" << RESET << "\n\n";

    convertCode("RETURN_ZERO", sample1::RETURN_ZERO);
    convertCode("RETURN_ZERO", sample2::RETURN_ZERO);
    convertCode("ARITH_EXPR", sample2::ARITH_EXPR);
    convertCode("IF_ELSE", sample2::IF_ELSE);
    convertCode("FULL_PROGRAM", sample2::FULL_PROGRAM);
    convertCode("PHASE_2_EXHAUSTIVE", sample2::PHASE_2_EXHAUSTIVE);
    convertCode("DEFER_ORDER", sample3::DEFER_ORDER);
    convertCode("DEFER_BEFORE_RETURN", sample3::DEFER_BEFORE_RETURN);
    convertCode("TRY_CATCH_BASIC", sample3::TRY_CATCH_BASIC);
    convertCode("TAGGED_UNION", sample3::TAGGED_UNION);
    convertCode("TAGGED_UNION_STRUCT_VARIANT", sample3::TAGGED_UNION_STRUCT_VARIANT);
    convertCode("SELF_IN_BEHAVE", sample3::SELF_IN_BEHAVE);
    convertCode("USE_PATH", sample3::USE_PATH);
    convertCode("DEBUG_ASSERT", sample3::DEBUG_ASSERT);
    convertCode("OS_SAMPLE", sample3::OS_SAMPLE);
    convertCode("STRUCT_MATCH_DEFER", sample3::STRUCT_MATCH_DEFER);
    convertCode("MATCH_PAYLOAD", sample4::MATCH_PAYLOAD);
    convertCode("UNION_CONSTRUCTOR", sample4::UNION_CONSTRUCTOR);
    convertCode("ASSIGNMENT_IN_MATCH", sample4::ASSIGNMENT_IN_MATCH);
    convertCode("COMBINED_C6_C7_C8", sample4::COMBINED_C6_C7_C8);
    convertCode("OPTIONAL_TYPE", sample5::OPTIONAL_TYPE);
    convertCode("ERROR_UNION_RETURN", sample5::ERROR_UNION_RETURN);
    convertCode("ENUM_BACKING_TYPE", sample5::ENUM_BACKING_TYPE);
    convertCode("ENUM_BIT_FLAGS", sample5::ENUM_BIT_FLAGS);
    convertCode("PUB_VISIBILITY", sample5::PUB_VISIBILITY);
    convertCode("CONST_FUNC", sample5::CONST_FUNC);
    convertCode("FORMAT_STRING", sample5::FORMAT_STRING);
    convertCode("REFERENCES", sample5::REFERENCES);
    convertCode("BEHAVE_NEEDS", sample5::BEHAVE_NEEDS);
    convertCode("ALLOCATOR_BUILTINS", sample5::ALLOCATOR_BUILTINS);
    convertCode("COMBINED_FEATURES", sample5::COMBINED_FEATURES);
    convertCode("S01_IMMUTABLE_ASSIGN", semantic_errors::S01_IMMUTABLE_ASSIGN);
    convertCode("S02_REDECLARED_VAR", semantic_errors::S02_REDECLARED_VAR);
    convertCode("S03_UNDECLARED_IDENT", semantic_errors::S03_UNDECLARED_IDENT);
    convertCode("S04_ARG_COUNT_MISMATCH", semantic_errors::S04_ARG_COUNT_MISMATCH);
    convertCode("S05_RETURN_TYPE_MISMATCH", semantic_errors::S05_RETURN_TYPE_MISMATCH);
    convertCode("S06_BREAK_OUTSIDE_LOOP", semantic_errors::S06_BREAK_OUTSIDE_LOOP);
    convertCode("S07_IF_COND_NOT_BOOL", semantic_errors::S07_IF_COND_NOT_BOOL);
    convertCode("S08_STRUCT_FIELD_NOT_FOUND", semantic_errors::S08_STRUCT_FIELD_NOT_FOUND);
    convertCode("S09_UNDECLARED_FUNC", semantic_errors::S09_UNDECLARED_FUNC);
    convertCode("S10_GLOBAL_DUPLICATE", semantic_errors::S10_GLOBAL_DUPLICATE);

    // Comments and escape sequences
    convertCode("SINGLE_LINE_BASIC", sample_comments::SINGLE_LINE_BASIC);
    convertCode("SINGLE_LINE_AFTER_CODE", sample_comments::SINGLE_LINE_AFTER_CODE);
    convertCode("SINGLE_LINE_MULTIPLE", sample_comments::SINGLE_LINE_MULTIPLE);
    convertCode("BLOCK_BASIC", sample_comments::BLOCK_BASIC);
    convertCode("BLOCK_MULTI_LINE", sample_comments::BLOCK_MULTI_LINE);
    convertCode("BLOCK_BEFORE_CODE", sample_comments::BLOCK_BEFORE_CODE);
    convertCode("STRING_ESCAPES", sample_comments::STRING_ESCAPES);
    convertCode("CHAR_ESCAPES", sample_comments::CHAR_ESCAPES);
    convertCode("MIXED_FEATURES", sample_comments::MIXED_FEATURES);
    convertCode("EMPTY_BLOCK", sample_comments::EMPTY_BLOCK);

    // Stage 2 features
    convertCode("BREAK_SKIP", sample_stage2::BREAK_SKIP);
    convertCode("ELSE_IF", sample_stage2::ELSE_IF);
    convertCode("RANGE_EXPR", sample_stage2::RANGE_EXPR);
    convertCode("TRY_BLOCK", sample_stage2::TRY_BLOCK);
    convertCode("GENERIC_FUNC", sample_stage2::GENERIC_FUNC);
    convertCode("GENERIC_STRUCT", sample_stage2::GENERIC_STRUCT);
    convertCode("PUB_DECLARATIONS", sample_stage2::PUB_DECLARATIONS);
    convertCode("BEHAVE_RENAME", sample_stage2::BEHAVE_RENAME);
    convertCode("BITWISE_NOT", sample_stage2::BITWISE_NOT);
    convertCode("TYPE_ALIAS_COMPLEX", sample_stage2::TYPE_ALIAS_COMPLEX);
    convertCode("EXT_STRUCT", sample_stage2::EXT_STRUCT);
    convertCode("VOID_NO_RET", sample_stage2::VOID_NO_RET);
}
