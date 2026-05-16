#include <iostream>
#include <fstream>
#include <vector>
#include <string>
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
#include <llvm/Support/TargetSelect.h>
#include <sys/stat.h>

using namespace razen;

struct CLIOpts {
    bool verbose = false;
    std::vector<std::string> files;
};

static void printHelp(const char* prog) {
    std::cout << "Usage: " << prog << " [flags] [files...]\n\n"
              << "Flags:\n"
              << "  -h, --help       Print this help\n"
              << "  --version        Print version\n"
              << "  -v, --verbose    Full phase-by-phase debug output\n"
              << "  --debug          Alias for --verbose\n"
              << "  -f, --files      Treat remaining args as files\n"
              << "\n"
              << "With no files, compiles 10 built-in samples.\n"
              << "Otherwise compiles each .rzn file, emits output/<name>.o/.s\n";
}

static void printVersion() {
    std::cout << "razenc " << codegen::Codegen::CODEGEN_VERSION
              << " (LLVM 20, x86-64)\n";
}

static CLIOpts parseArgs(int argc, char** argv) {
    CLIOpts opts;
    bool in_files = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            std::exit(0);
        } else if (arg == "--version") {
            printVersion();
            std::exit(0);
        } else if (arg == "--verbose" || arg == "-v" || arg == "--debug") {
            opts.verbose = true;
        } else if (arg == "-f" || arg == "--files") {
            in_files = true;
        } else if (!in_files && arg[0] == '-') {
            std::cerr << "Unknown flag: " << arg << "\n";
            std::exit(1);
        } else {
            opts.files.push_back(arg);
        }
    }
    return opts;
}

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        std::cerr << "Error: cannot open " << path << "\n";
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

static bool compileSource(const std::string& label, const std::string& source,
                          bool verbose) {
    auto phase = [&](const std::string& name) {
        if (verbose) std::cout << "\n" << CREAM << name << RESET << "  ";
    };
    auto ok = [&](const std::string& msg) {
        if (verbose) std::cout << "\t\t" << msg << "\t\tDone\n";
    };

    if (verbose)
        std::cout << "\n" << CYAN << "\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81 " << label
                  << " \xe2\x94\x81\xe2\x94\x81\xe2\x94\x81" << RESET << "\n"
                  << GREY << "Source:" << RESET << "\n" << source << "\n\n";

    phase("Phase 1");
    std::vector<Token> token_list;
    try {
        token_list = parseToTokens(source);
    } catch (const LexerError& err) {
        if (verbose) std::cout << RED << "Lexer error: " << err.what() << RESET << "\n";
        else std::cout << label << ": Lexer error: " << err.what() << "\n";
        return false;
    }
    if (verbose) printTokens(token_list);
    ok("Lexer");

    phase("Phase 2");
    std::vector<ASTNode*> ast_nodes;
    try {
        ast_nodes = buildAST(token_list, source);
    } catch (const AstError& err) {
        if (verbose) std::cout << RED << "AST error: " << err.what() << RESET << "\n";
        else std::cout << label << ": AST error: " << err.what() << "\n";
        return false;
    }
    if (verbose) printAST(ast_nodes);
    ok("AST");

    phase("Phase 3");
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
        if (verbose) std::cout << "\n" << RED << "Semantic errors." << RESET << "\n";
        else std::cout << label << ": Semantic analysis failed\n";
        freeASTNodes(ast_nodes);
        return false;
    }
    ok("Semantic Analysis");

    phase("Phase 4");
    codegen::Codegen cg(label);
    cg.generate(ast_nodes);
    std::string ir_output = cg.getIR();
    writeIR(label, ir_output, !ir_output.empty());
    ok("Codegen");
    if (verbose) {
        std::cout << "\n" << PEACH << "--- LLVM IR ---" << RESET << "\n"
                  << ir_output
                  << PEACH << "--- End LLVM IR ---" << RESET << "\n";
    }

    phase("Phase 5");
    cg.optimize();
    std::string opt_ir = cg.getIR();
    writeIR(label + "_opt", opt_ir, true);
    ok("Optimization");

    phase("Phase 6");
    mkdir("output", 0755);
    std::string obj = "output/" + label + ".o";
    std::string asm_ = "output/" + label + ".s";
    cg.emitObject(obj);
    cg.emitAssembly(asm_);
    if (verbose) std::cout << "\t\tEmit " << label << ".o/.s\t\tDone\n";

    if (verbose) {
        std::cout << "\n" << PEACH << "--- Optimized IR ---" << RESET << "\n"
                  << opt_ir
                  << PEACH << "--- End Optimized IR ---" << RESET << "\n";
    }

    freeASTNodes(ast_nodes);
    return true;
}

struct SampleInfo { const char* name; const char* code; };
static const SampleInfo SAMPLES[] = {
    {"HELLO_RET",     sample6::HELLO_RET},
    {"ADD_FUNC",      sample6::ADD_FUNC},
    {"FACTORIAL_REC", sample6::FACTORIAL_REC},
    {"FIBONACCI_REC", sample6::FIBONACCI_REC},
    {"IF_ELSE_NESTED",sample6::IF_ELSE_NESTED},
    {"ENUM_MATCH",    sample6::ENUM_MATCH},
    {"INFINITE_LOOP", sample6::INFINITE_LOOP},
    {"UNION_TAGGED",  sample6::UNION_TAGGED},
    {"NESTED_IF",     sample6::NESTED_IF},
    {"COMPREHENSIVE", sample6::COMPREHENSIVE},
};

int main(int argc, char** argv) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    CLIOpts opts = parseArgs(argc, argv);

    setLexerVerbose(opts.verbose);
    setParserVerbose(opts.verbose);

    int failed = 0;

    if (opts.files.empty()) {
        if (opts.verbose)
            std::cout << LIGHT_GREEN << "Razen Lang - Full Frontend (Phases 1-6)" << RESET << "\n"
                      << GREY << "Lexer | AST | Semantic Analysis | LLVM IR Codegen | Optimize | Emit" << RESET << "\n\n";

        for (auto& s : SAMPLES) {
            if (!compileSource(s.name, s.code, opts.verbose))
                failed++;
        }
    } else {
        for (auto& f : opts.files) {
            std::string source = readFile(f);
            if (source.empty()) { failed++; continue; }
            std::string label = f;
            size_t dot = label.rfind('.');
            if (dot != std::string::npos) label = label.substr(0, dot);
            size_t slash = label.rfind('/');
            if (slash != std::string::npos) label = label.substr(slash + 1);

            if (!compileSource(label, source, opts.verbose))
                failed++;
        }
    }

    return failed ? 1 : 0;
}
