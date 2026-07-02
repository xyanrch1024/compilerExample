#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdio>
#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>

llvm::Value* LogErrorV(const char* str);

extern llvm::LLVMContext TheContext;
extern llvm::IRBuilder<> Builder;
extern llvm::Module* TheModule;
extern std::map<std::string, llvm::AllocaInst*> NamedValues;

class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual llvm::Value* codegen() = 0;
};

class NumberAST : public ExprAST {
    int Val;
public:
    NumberAST(int v) : Val(v) {}
    llvm::Value* codegen() override;
};

class VariableAST : public ExprAST {
    std::string Name;
public:
    VariableAST(const std::string& n) : Name(n) {}
    llvm::Value* codegen() override;
};

class BinaryAST : public ExprAST {
    char Op;
    ExprAST *LHS, *RHS;
public:
    BinaryAST(char op, ExprAST* l, ExprAST* r) : Op(op), LHS(l), RHS(r) {}
    llvm::Value* codegen() override;
};

class CompAST : public ExprAST {
    int Op;  // T_LT, T_GT, T_LE, T_GE, T_EQ, T_NE
    ExprAST *LHS, *RHS;
public:
    CompAST(int op, ExprAST* l, ExprAST* r) : Op(op), LHS(l), RHS(r) {}
    llvm::Value* codegen() override;
};

class StmtAST {
public:
    virtual ~StmtAST() = default;
    virtual llvm::Value* codegen() = 0;
};

class VarDeclAST : public StmtAST {
    std::string Name;
public:
    VarDeclAST(const std::string& n) : Name(n) {}
    llvm::Value* codegen() override;
};

class AssignAST : public StmtAST {
    std::string Name;
    ExprAST* Val;
public:
    AssignAST(const std::string& n, ExprAST* v) : Name(n), Val(v) {}
    llvm::Value* codegen() override;
};

class PrintAST : public StmtAST {
    ExprAST* Val;
public:
    PrintAST(ExprAST* v) : Val(v) {}
    llvm::Value* codegen() override;
};

class ReturnAST : public StmtAST {
    ExprAST* Val;
public:
    ReturnAST(ExprAST* v) : Val(v) {}
    llvm::Value* codegen() override;
};

class IfAST : public StmtAST {
    ExprAST* Cond;
    StmtAST* Then;
    StmtAST* Else;
public:
    IfAST(ExprAST* c, StmtAST* t, StmtAST* e) : Cond(c), Then(t), Else(e) {}
    llvm::Value* codegen() override;
};

class WhileAST : public StmtAST {
    ExprAST* Cond;
    StmtAST* Body;
public:
    WhileAST(ExprAST* c, StmtAST* b) : Cond(c), Body(b) {}
    llvm::Value* codegen() override;
};

class BlockAST : public StmtAST {
    std::vector<StmtAST*> Stmts;
public:
    BlockAST() = default;
    void add(StmtAST* s) { Stmts.push_back(s); }
    llvm::Value* codegen() override;
};

class FunctionAST {
    std::string Name;
    BlockAST* Body;
public:
    FunctionAST(const std::string& n, BlockAST* b) : Name(n), Body(b) {}
    llvm::Function* codegen();
};

#endif
