#ifndef VISITOR_H
#define VISITOR_H

#include "ast.h"

class ExprSemaVisitor {
public:
    virtual ~ExprSemaVisitor() = default;
    virtual ExprType visit(NumberAST& node, const std::set<Symbol*>& assigned) = 0;
    virtual ExprType visit(VariableAST& node, const std::set<Symbol*>& assigned) = 0;
    virtual ExprType visit(BinaryAST& node, const std::set<Symbol*>& assigned) = 0;
    virtual ExprType visit(CompAST& node, const std::set<Symbol*>& assigned) = 0;
    virtual ExprType visit(CallAST& node, const std::set<Symbol*>& assigned) = 0;
};

class StmtSemaVisitor {
public:
    virtual ~StmtSemaVisitor() = default;
    virtual Flow visit(VarDeclAST& node, std::set<Symbol*> assigned) = 0;
    virtual Flow visit(AssignAST& node, std::set<Symbol*> assigned) = 0;
    virtual Flow visit(PrintAST& node, std::set<Symbol*> assigned) = 0;
    virtual Flow visit(ReturnAST& node, std::set<Symbol*> assigned) = 0;
    virtual Flow visit(CallStmtAST& node, std::set<Symbol*> assigned) = 0;
    virtual Flow visit(IfAST& node, std::set<Symbol*> assigned) = 0;
    virtual Flow visit(WhileAST& node, std::set<Symbol*> assigned) = 0;
    virtual Flow visit(BlockAST& node, std::set<Symbol*> assigned) = 0;
};

class ExprCodegenVisitor {
public:
    virtual ~ExprCodegenVisitor() = default;
    virtual llvm::Value* visit(NumberAST& node) = 0;
    virtual llvm::Value* visit(VariableAST& node) = 0;
    virtual llvm::Value* visit(BinaryAST& node) = 0;
    virtual llvm::Value* visit(CompAST& node) = 0;
    virtual llvm::Value* visit(CallAST& node) = 0;
};

class StmtCodegenVisitor {
public:
    virtual ~StmtCodegenVisitor() = default;
    virtual llvm::Value* visit(VarDeclAST& node) = 0;
    virtual llvm::Value* visit(AssignAST& node) = 0;
    virtual llvm::Value* visit(PrintAST& node) = 0;
    virtual llvm::Value* visit(ReturnAST& node) = 0;
    virtual llvm::Value* visit(CallStmtAST& node) = 0;
    virtual llvm::Value* visit(IfAST& node) = 0;
    virtual llvm::Value* visit(WhileAST& node) = 0;
    virtual llvm::Value* visit(BlockAST& node) = 0;
};

#endif
