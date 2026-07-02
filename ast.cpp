#include "ast.h"

llvm::LLVMContext TheContext;
llvm::IRBuilder<> Builder(TheContext);
llvm::Module* TheModule = nullptr;
std::map<std::string, llvm::AllocaInst*> NamedValues;

llvm::Value* LogErrorV(const char* str) {
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}
