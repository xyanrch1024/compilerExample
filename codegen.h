#ifndef CODEGEN_H
#define CODEGEN_H

#include "visitor.h"

llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function* F, const std::string& Name);
llvm::Function* getPrintfDecl();

class Codegen : public ExprCodegenVisitor, public StmtCodegenVisitor {
public:
    llvm::Value* visit(NumberAST& node) override;
    llvm::Value* visit(VariableAST& node) override;
    llvm::Value* visit(BinaryAST& node) override;
    llvm::Value* visit(CompAST& node) override;
    llvm::Value* visit(CallAST& node) override;

    llvm::Value* visit(VarDeclAST& node) override;
    llvm::Value* visit(AssignAST& node) override;
    llvm::Value* visit(PrintAST& node) override;
    llvm::Value* visit(ReturnAST& node) override;
    llvm::Value* visit(CallStmtAST& node) override;
    llvm::Value* visit(IfAST& node) override;
    llvm::Value* visit(WhileAST& node) override;
    llvm::Value* visit(BlockAST& node) override;

    llvm::Function* visit(FunctionAST& function);
    void visit(ProgramAST& program);

private:
    void declare(FunctionAST& function);
};

#endif
