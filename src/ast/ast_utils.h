#pragma once
#include "node.h"
#include <vector>
#include <cstdlib>

namespace razen {

inline ASTNode* createDefaultAstNode() {
    ASTNode* n = new ASTNode();
    n->node_type = ASTNodeType::Invalid;
    n->token = std::nullopt;
    n->left = nullptr;
    n->middle = nullptr;
    n->right = nullptr;
    n->children = nullptr;
    n->is_const = false;
    n->is_mut = false;
    n->is_global = false;
    n->is_pub = false;
    n->is_async = false;
    n->owns_children = true;
    return n;
}

inline ASTNode* createAstNode(ASTNodeType type) {
    ASTNode* n = createDefaultAstNode();
    n->node_type = type;
    return n;
}

inline std::vector<ASTNode*>* createChildList() {
    return new std::vector<ASTNode*>();
}

inline void appendChild(ASTNode* parent, ASTNode* child) {
    if (parent->children == nullptr) {
        parent->children = createChildList();
    }
    parent->children->push_back(child);
}

// free an AST tree recursively
inline void freeASTNode(ASTNode* n) {
    if (!n) return;

    // free children list
    if (n->children) {
        for (auto* child : *n->children) {
            freeASTNode(child);
        }
        delete n->children;
        n->children = nullptr;
    }

    // free left, middle, right — these are always separate from children
    freeASTNode(n->left);
    freeASTNode(n->middle);
    freeASTNode(n->right);

    delete n;
}

// free a list of top-level AST nodes
inline void freeASTNodes(std::vector<ASTNode*>& nodes) {
    for (auto* n : nodes) {
        freeASTNode(n);
    }
    nodes.clear();
}

} // namespace razen
