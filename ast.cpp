#include "ast.h"
#include "codegen.h"
#include "sema.h"
#include "visitor.h"

llvm::LLVMContext TheContext;
llvm::IRBuilder<> Builder(TheContext);
llvm::Module* TheModule = nullptr;
ProgramAST* Program = nullptr;
const char* SourcePath = "<stdin>";

llvm::Value* LogErrorV(const char* str) {
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}

ExprType NumberAST::accept(ExprSemaVisitor& visitor, const std::set<Symbol*>& assigned) {
    return visitor.visit(*this, assigned);
}
llvm::Value* NumberAST::accept(ExprCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

ExprType VariableAST::accept(ExprSemaVisitor& visitor, const std::set<Symbol*>& assigned) {
    return visitor.visit(*this, assigned);
}
llvm::Value* VariableAST::accept(ExprCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

ExprType BinaryAST::accept(ExprSemaVisitor& visitor, const std::set<Symbol*>& assigned) {
    return visitor.visit(*this, assigned);
}
llvm::Value* BinaryAST::accept(ExprCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

ExprType CompAST::accept(ExprSemaVisitor& visitor, const std::set<Symbol*>& assigned) {
    return visitor.visit(*this, assigned);
}
llvm::Value* CompAST::accept(ExprCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

ExprType CallAST::accept(ExprSemaVisitor& visitor, const std::set<Symbol*>& assigned) {
    return visitor.visit(*this, assigned);
}
llvm::Value* CallAST::accept(ExprCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

Flow VarDeclAST::accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) {
    return visitor.visit(*this, std::move(assigned));
}
llvm::Value* VarDeclAST::accept(StmtCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

Flow AssignAST::accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) {
    return visitor.visit(*this, std::move(assigned));
}
llvm::Value* AssignAST::accept(StmtCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

Flow PrintAST::accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) {
    return visitor.visit(*this, std::move(assigned));
}
llvm::Value* PrintAST::accept(StmtCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

Flow ReturnAST::accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) {
    return visitor.visit(*this, std::move(assigned));
}
llvm::Value* ReturnAST::accept(StmtCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

Flow CallStmtAST::accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) {
    return visitor.visit(*this, std::move(assigned));
}
llvm::Value* CallStmtAST::accept(StmtCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

Flow IfAST::accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) {
    return visitor.visit(*this, std::move(assigned));
}
llvm::Value* IfAST::accept(StmtCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

Flow WhileAST::accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) {
    return visitor.visit(*this, std::move(assigned));
}
llvm::Value* WhileAST::accept(StmtCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

Flow BlockAST::accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) {
    return visitor.visit(*this, std::move(assigned));
}
llvm::Value* BlockAST::accept(StmtCodegenVisitor& visitor) {
    return visitor.visit(*this);
}

void FunctionAST::accept(Sema& sema) {
    sema.visit(*this);
}

llvm::Function* FunctionAST::accept(Codegen& codegen) {
    return codegen.visit(*this);
}

void ProgramAST::accept(Sema& sema) {
    sema.visit(*this);
}

void ProgramAST::accept(Codegen& codegen) {
    codegen.visit(*this);
}
