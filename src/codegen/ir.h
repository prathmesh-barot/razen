#pragma once
#include <string>
#include <vector>
#include <sstream>
#include "../ast/node.h"
#include "../semantic/type_info.h"

namespace razen {
namespace codegen {

struct IRBuilder {
    std::ostringstream module;
    std::string indent_str;
    size_t tmp_counter = 0;
    size_t label_counter = 0;
    size_t str_counter = 0;

    void emitLine(const std::string& line);
    void emitRaw(const std::string& text);
    void emitPreamble(const std::string& source_name);
    void emitLibcDecls();
    std::string tmp(const std::string& prefix = "%tmp");
    std::string label(const std::string& prefix = ".L");
    std::string strName();
    std::string getIR();
};

std::string typeToLLVM(const TypeInfo* type);
std::string typeToLLVM(ASTNode* type_node);

} // namespace codegen
} // namespace razen
