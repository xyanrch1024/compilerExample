#ifndef SEMA_H
#define SEMA_H

#include "ast.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

class Sema {
public:
    explicit Sema(std::string filename);

    bool analyze(ProgramAST* program);
    void report(FILE* out) const;
    bool hasErrors() const { return ErrorCount != 0; }

    void error(int line, const std::string& msg);
    void note(int line, const std::string& msg);
    void warning(int line, const std::string& msg);

    void enterScope();
    void leaveScope();
    Symbol* declare(const std::string& name, int line);
    Symbol* lookup(const std::string& name);
    FunctionAST* declareFunction(FunctionAST* function);
    FunctionAST* lookupFunction(const std::string& name);

private:
    struct Diag {
        int Line;
        std::string Kind;
        std::string Msg;
    };

    void diagnose(int line, const char* kind, const std::string& msg);

    std::string Filename;
    std::vector<std::map<std::string, Symbol*>> Scopes;
    std::map<std::string, FunctionAST*> Functions;
    std::vector<std::unique_ptr<Symbol>> Symbols;
    std::vector<Diag> Diags;
    int ErrorCount = 0;
};

#endif
