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
class Codegen;
class ExprSemaVisitor;
class ExprCodegenVisitor;
class StmtSemaVisitor;
class StmtCodegenVisitor;

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
class ProgramAST;
extern ProgramAST* Program;
extern const char* SourcePath;

class ExprAST {
    friend class Sema;
    friend class Codegen;

protected:
    int Line;
    ExprType Ty = ExprType::Error;
    explicit ExprAST(int line) : Line(line) {}

public:
    virtual ~ExprAST() = default;
    virtual ExprType accept(ExprSemaVisitor& visitor, const std::set<Symbol*>& assigned) = 0;
    virtual llvm::Value* accept(ExprCodegenVisitor& visitor) = 0;
    int line() const { return Line; }
    ExprType type() const { return Ty; }
};

class NumberAST : public ExprAST {
    friend class Sema;
    friend class Codegen;
    int Val;

public:
    NumberAST(int v, int line) : ExprAST(line), Val(v) {}
    ExprType accept(ExprSemaVisitor& visitor, const std::set<Symbol*>& assigned) override;
    llvm::Value* accept(ExprCodegenVisitor& visitor) override;
};

class VariableAST : public ExprAST {
    friend class Sema;
    friend class Codegen;
    std::string Name;
    Symbol* Sym = nullptr;

public:
    VariableAST(const std::string& n, int line) : ExprAST(line), Name(n) {}
    ExprType accept(ExprSemaVisitor& visitor, const std::set<Symbol*>& assigned) override;
    llvm::Value* accept(ExprCodegenVisitor& visitor) override;
};

class BinaryAST : public ExprAST {
    friend class Sema;
    friend class Codegen;
    char Op;
    ExprAST *LHS, *RHS;

public:
    BinaryAST(char op, ExprAST* l, ExprAST* r, int line)
        : ExprAST(line), Op(op), LHS(l), RHS(r) {}
    ExprType accept(ExprSemaVisitor& visitor, const std::set<Symbol*>& assigned) override;
    llvm::Value* accept(ExprCodegenVisitor& visitor) override;
};

class CompAST : public ExprAST {
    friend class Sema;
    friend class Codegen;
    int Op;
    ExprAST *LHS, *RHS;

public:
    CompAST(int op, ExprAST* l, ExprAST* r, int line)
        : ExprAST(line), Op(op), LHS(l), RHS(r) {}
    ExprType accept(ExprSemaVisitor& visitor, const std::set<Symbol*>& assigned) override;
    llvm::Value* accept(ExprCodegenVisitor& visitor) override;
};

class CallAST : public ExprAST {
    friend class Sema;
    friend class Codegen;
    std::string Callee;
    FunctionAST* Func = nullptr;

public:
    CallAST(const std::string& callee, int line) : ExprAST(line), Callee(callee) {}
    ExprType accept(ExprSemaVisitor& visitor, const std::set<Symbol*>& assigned) override;
    llvm::Value* accept(ExprCodegenVisitor& visitor) override;
};

class StmtAST {
    friend class Sema;
    friend class Codegen;

protected:
    int Line;
    bool DoesReturn = false;
    explicit StmtAST(int line) : Line(line) {}

public:
    virtual ~StmtAST() = default;
    virtual Flow accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) = 0;
    virtual llvm::Value* accept(StmtCodegenVisitor& visitor) = 0;
    int line() const { return Line; }
    bool alwaysReturns() const { return DoesReturn; }
};

class VarDeclAST : public StmtAST {
    friend class Sema;
    friend class Codegen;
    std::string Name;
    Symbol* Sym = nullptr;

public:
    VarDeclAST(const std::string& n, int line) : StmtAST(line), Name(n) {}
    Flow accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) override;
    llvm::Value* accept(StmtCodegenVisitor& visitor) override;
};

class AssignAST : public StmtAST {
    friend class Sema;
    friend class Codegen;
    std::string Name;
    ExprAST* Val;
    Symbol* Sym = nullptr;

public:
    AssignAST(const std::string& n, ExprAST* v, int line)
        : StmtAST(line), Name(n), Val(v) {}
    Flow accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) override;
    llvm::Value* accept(StmtCodegenVisitor& visitor) override;
};

class PrintAST : public StmtAST {
    friend class Sema;
    friend class Codegen;
    ExprAST* Val;

public:
    PrintAST(ExprAST* v, int line) : StmtAST(line), Val(v) {}
    Flow accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) override;
    llvm::Value* accept(StmtCodegenVisitor& visitor) override;
};

class ReturnAST : public StmtAST {
    friend class Sema;
    friend class Codegen;
    ExprAST* Val;

public:
    ReturnAST(ExprAST* v, int line) : StmtAST(line), Val(v) {}
    Flow accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) override;
    llvm::Value* accept(StmtCodegenVisitor& visitor) override;
};

class CallStmtAST : public StmtAST {
    friend class Sema;
    friend class Codegen;
    CallAST* Call;

public:
    CallStmtAST(CallAST* call, int line) : StmtAST(line), Call(call) {}
    Flow accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) override;
    llvm::Value* accept(StmtCodegenVisitor& visitor) override;
};

class IfAST : public StmtAST {
    friend class Sema;
    friend class Codegen;
    ExprAST* Cond;
    StmtAST* Then;
    StmtAST* Else;

public:
    IfAST(ExprAST* c, StmtAST* t, StmtAST* e, int line)
        : StmtAST(line), Cond(c), Then(t), Else(e) {}
    Flow accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) override;
    llvm::Value* accept(StmtCodegenVisitor& visitor) override;
};

class WhileAST : public StmtAST {
    friend class Sema;
    friend class Codegen;
    ExprAST* Cond;
    StmtAST* Body;

public:
    WhileAST(ExprAST* c, StmtAST* b, int line)
        : StmtAST(line), Cond(c), Body(b) {}
    Flow accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) override;
    llvm::Value* accept(StmtCodegenVisitor& visitor) override;
};

class BlockAST : public StmtAST {
    friend class Sema;
    friend class Codegen;
    std::vector<StmtAST*> Stmts;

public:
    BlockAST() : StmtAST(0) {}
    void add(StmtAST* s) { Stmts.push_back(s); }
    void setLine(int line) { Line = line; }
    Flow accept(StmtSemaVisitor& visitor, std::set<Symbol*> assigned) override;
    llvm::Value* accept(StmtCodegenVisitor& visitor) override;
};

class FunctionAST {
    friend class Sema;
    friend class Codegen;
    std::string Name;
    BlockAST* Body;
    int Line;
    llvm::Function* IR = nullptr;

public:
    FunctionAST(const std::string& n, BlockAST* b, int line)
        : Name(n), Body(b), Line(line) {}
    const std::string& name() const { return Name; }
    llvm::Function* ir() const { return IR; }
    void accept(Sema& sema);
    llvm::Function* accept(Codegen& codegen);
    int line() const { return Line; }
};

class ProgramAST {
    friend class Sema;
    friend class Codegen;
    std::vector<FunctionAST*> Functions;
    int Line;

public:
    explicit ProgramAST(int line) : Line(line) {}
    void add(FunctionAST* function) { Functions.push_back(function); }
    void accept(Sema& sema);
    void accept(Codegen& codegen);
    void codegen();
    int line() const { return Line; }
};

#endif
