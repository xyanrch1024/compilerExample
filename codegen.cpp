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

// --- ExprAST ---

llvm::Value* NumberAST::codegen() {
    return llvm::ConstantInt::get(TheContext, llvm::APInt(32, Val, true));
}

llvm::Value* VariableAST::codegen() {
    if (!Sym || !Sym->Addr)
        return LogErrorV("unknown variable");
    return Builder.CreateLoad(llvm::Type::getInt32Ty(TheContext), Sym->Addr, Sym->Name.c_str());
}

llvm::Value* BinaryAST::codegen() {
    llvm::Value* L = LHS->codegen();
    llvm::Value* R = RHS->codegen();
    if (!L || !R) return nullptr;
    switch (Op) {
        case '+': return Builder.CreateAdd(L, R, "addtmp");
        case '-': return Builder.CreateSub(L, R, "subtmp");
        case '*': return Builder.CreateMul(L, R, "multmp");
        case '/': return Builder.CreateSDiv(L, R, "divtmp");
        default:  return LogErrorV("invalid binary operator");
    }
}

llvm::Value* CompAST::codegen() {
    llvm::Value* L = LHS->codegen();
    llvm::Value* R = RHS->codegen();
    if (!L || !R) return nullptr;
    llvm::CmpInst::Predicate Pred;
    switch (Op) {
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

// --- StmtAST ---

llvm::Value* VarDeclAST::codegen() {
    llvm::Function* F = Builder.GetInsertBlock()->getParent();
    Sym->Addr = CreateEntryBlockAlloca(F, Sym->Name);
    return Sym->Addr;
}

llvm::Value* AssignAST::codegen() {
    if (!Sym || !Sym->Addr)
        return LogErrorV("unknown variable");
    llvm::Value* V = Val->codegen();
    if (!V) return nullptr;
    Builder.CreateStore(V, Sym->Addr);
    return V;
}

llvm::Value* PrintAST::codegen() {
    llvm::Value* V = Val->codegen();
    if (!V) return nullptr;

    llvm::Function* PrintfF = getPrintfDecl();
    llvm::Value* FormatStr = Builder.CreateGlobalStringPtr("%d\n", "printfmt");
    return Builder.CreateCall(PrintfF, {FormatStr, V}, "printfcall");
}

llvm::Value* ReturnAST::codegen() {
    llvm::Value* V = Val->codegen();
    if (!V) return nullptr;
    Builder.CreateRet(V);
    return V;
}

llvm::Value* IfAST::codegen() {
    llvm::Value* CondV = Cond->codegen();
    if (!CondV) return nullptr;

    llvm::Function* F = Builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(TheContext, "then", F);
    llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(TheContext, "else");
    llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(TheContext, "ifcont");

    Builder.CreateCondBr(CondV, ThenBB, ElseBB);

    Builder.SetInsertPoint(ThenBB);
    if (Then) Then->codegen();
    if (!Builder.GetInsertBlock()->getTerminator())
        Builder.CreateBr(MergeBB);

    F->insert(F->end(), ElseBB);
    Builder.SetInsertPoint(ElseBB);
    if (Else) Else->codegen();
    if (!Builder.GetInsertBlock()->getTerminator())
        Builder.CreateBr(MergeBB);

    F->insert(F->end(), MergeBB);
    Builder.SetInsertPoint(MergeBB);

    return nullptr;
}

llvm::Value* WhileAST::codegen() {
    llvm::Function* F = Builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* CondBB = llvm::BasicBlock::Create(TheContext, "whilecond", F);
    llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(TheContext, "whilebody");
    llvm::BasicBlock* AfterBB = llvm::BasicBlock::Create(TheContext, "whilecont");

    Builder.CreateBr(CondBB);
    Builder.SetInsertPoint(CondBB);
    llvm::Value* CondV = Cond->codegen();
    if (!CondV) return nullptr;
    Builder.CreateCondBr(CondV, LoopBB, AfterBB);

    F->insert(F->end(), LoopBB);
    Builder.SetInsertPoint(LoopBB);
    if (Body) Body->codegen();
    if (!Builder.GetInsertBlock()->getTerminator())
        Builder.CreateBr(CondBB);

    F->insert(F->end(), AfterBB);
    Builder.SetInsertPoint(AfterBB);

    return nullptr;
}

llvm::Value* BlockAST::codegen() {
    for (auto* S : Stmts) {
        S->codegen();
    }
    return nullptr;
}

// --- FunctionAST ---

llvm::Function* FunctionAST::codegen() {
    llvm::FunctionType* FT = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(TheContext),
        false
    );
    llvm::Function* F = llvm::Function::Create(
        FT, llvm::Function::ExternalLinkage, Name, TheModule
    );

    if (Name != "main") {
        F->setName(Name);
    }

    llvm::BasicBlock* BB = llvm::BasicBlock::Create(TheContext, "entry", F);
    Builder.SetInsertPoint(BB);

    Body->codegen();

    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateRet(llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0, true)));
    }

    llvm::verifyFunction(*F, &llvm::errs());
    return F;
}
