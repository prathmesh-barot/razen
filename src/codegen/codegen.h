#pragma once
#include "ir.h"
#include "../ast/node.h"
#include <string>
#include <vector>

namespace razen {
namespace codegen {

// ── Top-level codegen driver ────────────────────────────────────────────────
struct Codegen {
    IRGen ir;

    explicit Codegen(const std::string& source = "main.rz") : ir(source) {}

    // Run full codegen on a flat AST node list
    void generate(const std::vector<ASTNode*>& nodes);

    // Return textual LLVM IR
    std::string getIR() { return ir.dumpIR(); }

private:
    // ── Forward to IRGen methods ──
    void genTop(ASTNode* node) { ir.genTopLevel(node); }
};

} // namespace codegen
} // namespace razen
