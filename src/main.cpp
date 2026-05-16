#include <iostream>
#include "lexer/lexer.h"
#include "lexer/errors.h"
#include "ast/errors.h"
#include "ast/ast_utils.h"
#include "parser/parser.h"
#include "dbg/debug.h"
#include "semantic/analyzer.h"
#include "semantic/symbol_table.h"
#include "samples/sample6.h"
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
        writeIR(label, "", false);
        freeASTNodes(ast_nodes);
        return;
    }
    std::cout << "\t\tSemantic Analysis\t\t\tDone\n";

    // Phase 4 — LLVM IR Codegen
    std::cout << CREAM << "Phase 4" << RESET << "  ";
    codegen::Codegen cg(label);
    cg.generate(ast_nodes);
    std::string ir_output = cg.getIR();
    writeIR(label, ir_output, !ir_output.empty());
    std::cout << "\t\tLLVM IR Codegen\t\t\t\tDone\n";
    std::cout << "\n" << PEACH << "--- LLVM IR ---" << RESET << "\n";
    std::cout << ir_output;
    std::cout << PEACH << "--- End LLVM IR ---" << RESET << "\n";

    freeASTNodes(ast_nodes);
}

int main() {
    std::cout << LIGHT_GREEN << "Razen Lang - Full Frontend (Phases 1-4)" << RESET << "\n";
    std::cout << GREY << "Lexer | AST | Semantic Analysis | LLVM IR Codegen" << RESET << "\n\n";

    // ── 10 new verification samples ────────────────────────────────────────
    convertCode("HELLO_RET", sample6::HELLO_RET);
    convertCode("ADD_FUNC", sample6::ADD_FUNC);
    convertCode("FACTORIAL_REC", sample6::FACTORIAL_REC);
    convertCode("FIBONACCI_REC", sample6::FIBONACCI_REC);
    convertCode("IF_ELSE_NESTED", sample6::IF_ELSE_NESTED);
    convertCode("ENUM_MATCH", sample6::ENUM_MATCH);
    convertCode("INFINITE_LOOP", sample6::INFINITE_LOOP);
    convertCode("UNION_TAGGED", sample6::UNION_TAGGED);
    convertCode("NESTED_IF", sample6::NESTED_IF);
    convertCode("COMPREHENSIVE", sample6::COMPREHENSIVE);
}
