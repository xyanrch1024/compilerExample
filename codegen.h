#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function* F, const std::string& Name);
llvm::Function* getPrintfDecl();

#endif
