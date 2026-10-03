#include "ast.h"
#include "sema.h"
#include <cstdio>

extern FILE* yyin;
extern int yyparse();

int main(int argc, char* argv[]) {
    TheModule = new llvm::Module("tinyc", TheContext);

    const char* filename = nullptr;
    for (int i = 1; i < argc; i++) {
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
    TheModule->print(llvm::outs(), nullptr);
    return 0;
}
