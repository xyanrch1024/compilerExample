#include "ast.h"

llvm::LLVMContext TheContext;
llvm::IRBuilder<> Builder(TheContext);
llvm::Module* TheModule = nullptr;
FunctionAST* ProgramAST = nullptr;
const char* SourcePath = "<stdin>";

llvm::Value* LogErrorV(const char* str) {
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}
