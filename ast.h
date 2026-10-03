#ifndef AST_H
#define AST_H

#include <cstdio>
#include <set>
#include <string>
#include <vector>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>

class Sema;

enum class ExprType {
    Int,
    Bool,
    Error
};

struct Symbol {
    std::string Name;
    ExprType Type = ExprType::Int;
    int DeclLine = 0;
    llvm::AllocaInst* Addr = nullptr;
};

struct Flow {
    std::set<Symbol*> Assigned;
    bool AlwaysReturns = false;
};

llvm::Value* LogErrorV(const char* str);

extern llvm::LLVMContext TheContext;
extern llvm::IRBuilder<> Builder;
extern llvm::Module* TheModule;

class FunctionAST;
extern FunctionAST* ProgramAST;
extern const char* SourcePath;

class ExprAST {
protected:
    int Line;
    ExprType Ty = ExprType::Error;
    explicit ExprAST(int line) : Line(line) {}

public:
    virtual ~ExprAST() = default;
    virtual llvm::Value* codegen() = 0;
    virtual ExprType analyze(Sema& S, const std::set<Symbol*>& Assigned) = 0;
    int line() const { return Line; }
    ExprType type() const { return Ty; }
};

class NumberAST : public ExprAST {
    int Val;

public:
    NumberAST(int v, int line) : ExprAST(line), Val(v) {}
    llvm::Value* codegen() override;
    ExprType analyze(Sema& S, const std::set<Symbol*>& Assigned) override;
};

class VariableAST : public ExprAST {
    std::string Name;
    Symbol* Sym = nullptr;

public:
    VariableAST(const std::string& n, int line) : ExprAST(line), Name(n) {}
    llvm::Value* codegen() override;
    ExprType analyze(Sema& S, const std::set<Symbol*>& Assigned) override;
};

class BinaryAST : public ExprAST {
    char Op;
    ExprAST *LHS, *RHS;

public:
    BinaryAST(char op, ExprAST* l, ExprAST* r, int line)
        : ExprAST(line), Op(op), LHS(l), RHS(r) {}
    llvm::Value* codegen() override;
    ExprType analyze(Sema& S, const std::set<Symbol*>& Assigned) override;
};

class CompAST : public ExprAST {
    int Op;
    ExprAST *LHS, *RHS;

public:
    CompAST(int op, ExprAST* l, ExprAST* r, int line)
        : ExprAST(line), Op(op), LHS(l), RHS(r) {}
    llvm::Value* codegen() override;
    ExprType analyze(Sema& S, const std::set<Symbol*>& Assigned) override;
};

class StmtAST {
protected:
    int Line;
    bool DoesReturn = false;
    explicit StmtAST(int line) : Line(line) {}

public:
    virtual ~StmtAST() = default;
    virtual llvm::Value* codegen() = 0;
    virtual Flow analyze(Sema& S, std::set<Symbol*> Assigned) = 0;
    int line() const { return Line; }
    bool alwaysReturns() const { return DoesReturn; }
};

class VarDeclAST : public StmtAST {
    std::string Name;
    Symbol* Sym = nullptr;

public:
    VarDeclAST(const std::string& n, int line) : StmtAST(line), Name(n) {}
    llvm::Value* codegen() override;
    Flow analyze(Sema& S, std::set<Symbol*> Assigned) override;
};

class AssignAST : public StmtAST {
    std::string Name;
    ExprAST* Val;
    Symbol* Sym = nullptr;

public:
    AssignAST(const std::string& n, ExprAST* v, int line)
        : StmtAST(line), Name(n), Val(v) {}
    llvm::Value* codegen() override;
    Flow analyze(Sema& S, std::set<Symbol*> Assigned) override;
};

class PrintAST : public StmtAST {
    ExprAST* Val;

public:
    PrintAST(ExprAST* v, int line) : StmtAST(line), Val(v) {}
    llvm::Value* codegen() override;
    Flow analyze(Sema& S, std::set<Symbol*> Assigned) override;
};

class ReturnAST : public StmtAST {
    ExprAST* Val;

public:
    ReturnAST(ExprAST* v, int line) : StmtAST(line), Val(v) {}
    llvm::Value* codegen() override;
    Flow analyze(Sema& S, std::set<Symbol*> Assigned) override;
};

class IfAST : public StmtAST {
    ExprAST* Cond;
    StmtAST* Then;
    StmtAST* Else;

public:
    IfAST(ExprAST* c, StmtAST* t, StmtAST* e, int line)
        : StmtAST(line), Cond(c), Then(t), Else(e) {}
    llvm::Value* codegen() override;
    Flow analyze(Sema& S, std::set<Symbol*> Assigned) override;
};

class WhileAST : public StmtAST {
    ExprAST* Cond;
    StmtAST* Body;

public:
    WhileAST(ExprAST* c, StmtAST* b, int line)
        : StmtAST(line), Cond(c), Body(b) {}
    llvm::Value* codegen() override;
    Flow analyze(Sema& S, std::set<Symbol*> Assigned) override;
};

class BlockAST : public StmtAST {
    std::vector<StmtAST*> Stmts;

public:
    BlockAST() : StmtAST(0) {}
    void add(StmtAST* s) { Stmts.push_back(s); }
    void setLine(int line) { Line = line; }
    llvm::Value* codegen() override;
    Flow analyze(Sema& S, std::set<Symbol*> Assigned) override;
};

class FunctionAST {
    std::string Name;
    BlockAST* Body;
    int Line;

public:
    FunctionAST(const std::string& n, BlockAST* b, int line)
        : Name(n), Body(b), Line(line) {}
    llvm::Function* codegen();
    void analyze(Sema& S);
    int line() const { return Line; }
};

#endif
