#include "ast.h"
#include "codegen.h"
#include <cstdio>

extern FILE* yyin;
extern int yyparse();

int main(int argc, char* argv[]) {
    TheModule = new llvm::Module("tinyc", TheContext);

    if (argc > 1) {
        FILE* f = fopen(argv[1], "r");
        if (!f) {
            fprintf(stderr, "Cannot open file: %s\n", argv[1]);
            return 1;
        }
        yyin = f;
    }

    yyparse();

    bool jit = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--jit") == 0) jit = true;
    }

    TheModule->print(llvm::outs(), nullptr);

    return 0;
}
