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
    void optimize();
    bool emitObject(const std::string& path);
    bool emitAssembly(const std::string& path);
    std::string getIR() { return ir.dumpIR(); }

    static constexpr int CODEGEN_VERSION = 3;
};

} // namespace codegen
} // namespace razen
