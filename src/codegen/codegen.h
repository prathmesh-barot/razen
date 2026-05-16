#pragma once
#include "ir.h"
#include <string>
#include <vector>

namespace razen {
namespace codegen {

// ── Top-level codegen driver ────────────────────────────────────────────────
struct Codegen {
    IRGen ir;

    explicit Codegen(const std::string& source = "main.rz") : ir(source) {}
    Codegen(const Codegen&) = delete;
    Codegen& operator=(const Codegen&) = delete;
    Codegen(Codegen&&) = delete;
    Codegen& operator=(Codegen&&) = delete;

    void generate(const std::vector<ASTNode*>& nodes);
    std::string getIR() { return ir.dumpIR(); }
};

} // namespace codegen
} // namespace razen
