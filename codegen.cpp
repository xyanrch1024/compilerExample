#include "ast.h"
#include "codegen.h"
#include "parser.tab.hpp"

llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function* F, const std::string& Name) {
    llvm::IRBuilder<> TmpB(&F->getEntryBlock(), F->getEntryBlock().begin());
    return TmpB.CreateAlloca(llvm::Type::getInt32Ty(TheContext), nullptr, Name);
}

llvm::Function* getPrintfDecl() {
    llvm::Function* F = TheModule->getFunction("printf");
    if (!F) {
        llvm::FunctionType* FT = llvm::FunctionType::get(
            llvm::IntegerType::getInt32Ty(TheContext),
            llvm::PointerType::get(llvm::Type::getInt8Ty(TheContext), 0),
            true
        );
        F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "printf", TheModule);
    }
    return F;
}

llvm::Value* Codegen::visit(CallAST& node) {
    return Builder.CreateCall(node.Func->ir(), {}, "calltmp");
}

llvm::Value* Codegen::visit(NumberAST& node) {
    return llvm::ConstantInt::get(TheContext, llvm::APInt(32, node.Val, true));
}

llvm::Value* Codegen::visit(VariableAST& node) {
    if (!node.Sym || !node.Sym->Addr)
        return LogErrorV("unknown variable");
    return Builder.CreateLoad(llvm::Type::getInt32Ty(TheContext), node.Sym->Addr, node.Sym->Name.c_str());
}

llvm::Value* Codegen::visit(BinaryAST& node) {
    llvm::Value* L = node.LHS->accept(*this);
    llvm::Value* R = node.RHS->accept(*this);
    if (!L || !R) return nullptr;
    switch (node.Op) {
        case '+': return Builder.CreateAdd(L, R, "addtmp");
        case '-': return Builder.CreateSub(L, R, "subtmp");
        case '*': return Builder.CreateMul(L, R, "multmp");
        case '/': return Builder.CreateSDiv(L, R, "divtmp");
        default:  return LogErrorV("invalid binary operator");
    }
}

llvm::Value* Codegen::visit(CompAST& node) {
    llvm::Value* L = node.LHS->accept(*this);
    llvm::Value* R = node.RHS->accept(*this);
    if (!L || !R) return nullptr;
    llvm::CmpInst::Predicate Pred;
    switch (node.Op) {
        case T_LT: Pred = llvm::CmpInst::ICMP_SLT; break;
        case T_GT: Pred = llvm::CmpInst::ICMP_SGT; break;
        case T_LE: Pred = llvm::CmpInst::ICMP_SLE; break;
        case T_GE: Pred = llvm::CmpInst::ICMP_SGE; break;
        case T_EQ: Pred = llvm::CmpInst::ICMP_EQ;  break;
        case T_NE: Pred = llvm::CmpInst::ICMP_NE;  break;
        default: return LogErrorV("invalid comparison operator");
    }
    return Builder.CreateICmp(Pred, L, R, "cmptmp");
}

llvm::Value* Codegen::visit(VarDeclAST& node) {
    llvm::Function* F = Builder.GetInsertBlock()->getParent();
    node.Sym->Addr = CreateEntryBlockAlloca(F, node.Sym->Name);
    return node.Sym->Addr;
}

llvm::Value* Codegen::visit(AssignAST& node) {
    if (!node.Sym || !node.Sym->Addr)
        return LogErrorV("unknown variable");
    llvm::Value* V = node.Val->accept(*this);
    if (!V) return nullptr;
    Builder.CreateStore(V, node.Sym->Addr);
    return V;
}

llvm::Value* Codegen::visit(PrintAST& node) {
    llvm::Value* V = node.Val->accept(*this);
    if (!V) return nullptr;

    llvm::Function* PrintfF = getPrintfDecl();
    llvm::Value* FormatStr = Builder.CreateGlobalStringPtr("%d\n", "printfmt");
    return Builder.CreateCall(PrintfF, {FormatStr, V}, "printfcall");
}

llvm::Value* Codegen::visit(CallStmtAST& node) {
    return node.Call->accept(*this);
}

llvm::Value* Codegen::visit(ReturnAST& node) {
    llvm::Value* V = node.Val->accept(*this);
    if (!V) return nullptr;
    Builder.CreateRet(V);
    return V;
}

llvm::Value* Codegen::visit(IfAST& node) {
    llvm::Value* CondV = node.Cond->accept(*this);
    if (!CondV) return nullptr;

    llvm::Function* F = Builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(TheContext, "then", F);
    llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(TheContext, "else");
    llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(TheContext, "ifcont");

    Builder.CreateCondBr(CondV, ThenBB, ElseBB);

    Builder.SetInsertPoint(ThenBB);
    if (node.Then) node.Then->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator())
        Builder.CreateBr(MergeBB);

    F->insert(F->end(), ElseBB);
    Builder.SetInsertPoint(ElseBB);
    if (node.Else) node.Else->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator())
        Builder.CreateBr(MergeBB);

    F->insert(F->end(), MergeBB);
    Builder.SetInsertPoint(MergeBB);

    return nullptr;
}

llvm::Value* Codegen::visit(WhileAST& node) {
    llvm::Function* F = Builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* CondBB = llvm::BasicBlock::Create(TheContext, "whilecond", F);
    llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(TheContext, "whilebody");
    llvm::BasicBlock* AfterBB = llvm::BasicBlock::Create(TheContext, "whilecont");

    Builder.CreateBr(CondBB);
    Builder.SetInsertPoint(CondBB);
    llvm::Value* CondV = node.Cond->accept(*this);
    if (!CondV) return nullptr;
    Builder.CreateCondBr(CondV, LoopBB, AfterBB);

    F->insert(F->end(), LoopBB);
    Builder.SetInsertPoint(LoopBB);
    if (node.Body) node.Body->accept(*this);
    if (!Builder.GetInsertBlock()->getTerminator())
        Builder.CreateBr(CondBB);

    F->insert(F->end(), AfterBB);
    Builder.SetInsertPoint(AfterBB);

    return nullptr;
}

llvm::Value* Codegen::visit(BlockAST& node) {
    for (StmtAST* stmt : node.Stmts)
        stmt->accept(*this);
    return nullptr;
}

void Codegen::declare(FunctionAST& function) {
    llvm::FunctionType* FT = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(TheContext),
        false
    );
    function.IR = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, function.Name, TheModule);
}

llvm::Function* Codegen::visit(FunctionAST& function) {
    llvm::BasicBlock* BB = llvm::BasicBlock::Create(TheContext, "entry", function.IR);
    Builder.SetInsertPoint(BB);

    function.Body->accept(*this);

    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateRet(llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0, true)));
    }

    llvm::verifyFunction(*function.IR, &llvm::errs());
    return function.IR;
}

void Codegen::visit(ProgramAST& program) {
    for (FunctionAST* function : program.Functions)
        declare(*function);
    for (FunctionAST* function : program.Functions)
        function->accept(*this);
}

void ProgramAST::codegen() {
    Codegen codegen;
    accept(codegen);
}
