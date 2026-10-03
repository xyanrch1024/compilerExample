#include "ast.h"
#include "optimize.h"
#include "sema.h"
#include <cstdio>
#include <cstring>
#include <llvm/Passes/OptimizationLevel.h>

extern FILE* yyin;
extern int yyparse();

int main(int argc, char* argv[]) {
    TheModule = new llvm::Module("tinyc", TheContext);

    const char* filename = nullptr;
    llvm::OptimizationLevel optLevel = llvm::OptimizationLevel::O0;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-O", 2) == 0) {
            if (strcmp(argv[i], "-O0") == 0)
                optLevel = llvm::OptimizationLevel::O0;
            else if (strcmp(argv[i], "-O1") == 0)
                optLevel = llvm::OptimizationLevel::O1;
            else if (strcmp(argv[i], "-O2") == 0)
                optLevel = llvm::OptimizationLevel::O2;
            else if (strcmp(argv[i], "-O3") == 0)
                optLevel = llvm::OptimizationLevel::O3;
            else {
                fprintf(stderr, "unknown optimization level: %s\n", argv[i]);
                return 1;
            }
            continue;
        }
        if (argv[i][0] == '-')
            continue;
        filename = argv[i];
    }

    FILE* file = nullptr;
    if (filename) {
        file = fopen(filename, "r");
        if (!file) {
            fprintf(stderr, "Cannot open file: %s\n", filename);
            return 1;
        }
        yyin = file;
        SourcePath = filename;
    }

    int parseStatus = yyparse();
    if (file)
        fclose(file);
    if (parseStatus != 0 || !ProgramAST)
        return 1;

    Sema sema(SourcePath);
    sema.analyze(ProgramAST);
    sema.report(stderr);
    if (sema.hasErrors())
        return 1;

    ProgramAST->codegen();
    optimizeModule(*TheModule, optLevel);
    TheModule->print(llvm::outs(), nullptr);
    return 0;
}
