#include "sema.h"
#include "parser.tab.hpp"
#include <algorithm>
#include <iterator>

static const char* typeName(ExprType type) {
    switch (type) {
    case ExprType::Int: return "int";
    case ExprType::Bool: return "bool";
    case ExprType::Error: return "error";
    }
    return "error";
}

static const char* cmpOpName(int op) {
    switch (op) {
    case T_LT: return "<";
    case T_GT: return ">";
    case T_LE: return "<=";
    case T_GE: return ">=";
    case T_EQ: return "==";
    case T_NE: return "!=";
    default: return "?";
    }
}

static bool isError(ExprType type) {
    return type == ExprType::Error;
}

Sema::Sema(std::string filename) : Filename(std::move(filename)) {}

bool Sema::analyze(FunctionAST* program) {
    if (!program)
        return false;
    program->analyze(*this);
    return ErrorCount == 0;
}

void Sema::report(FILE* out) const {
    for (const Diag& diag : Diags) {
        fprintf(out, "%s:%d: %s: %s\n",
                Filename.c_str(), diag.Line, diag.Kind.c_str(), diag.Msg.c_str());
    }
}

void Sema::diagnose(int line, const char* kind, const std::string& msg) {
    Diags.push_back(Diag{line, kind, msg});
}

void Sema::error(int line, const std::string& msg) {
    diagnose(line, "error", msg);
    ErrorCount++;
}

void Sema::note(int line, const std::string& msg) {
    diagnose(line, "note", msg);
}

void Sema::warning(int line, const std::string& msg) {
    diagnose(line, "warning", msg);
}

void Sema::enterScope() {
    Scopes.emplace_back();
}

void Sema::leaveScope() {
    Scopes.pop_back();
}

Symbol* Sema::declare(const std::string& name, int line) {
    auto& scope = Scopes.back();
    auto found = scope.find(name);
    if (found != scope.end()) {
        error(line, "redefinition of '" + name + "'");
        note(found->second->DeclLine, "previous definition is here");
        return found->second;
    }

    auto symbol = std::make_unique<Symbol>();
    symbol->Name = name;
    symbol->Type = ExprType::Int;
    symbol->DeclLine = line;
    Symbol* raw = symbol.get();
    Symbols.push_back(std::move(symbol));
    scope[name] = raw;
    return raw;
}

Symbol* Sema::lookup(const std::string& name) {
    for (auto scope = Scopes.rbegin(); scope != Scopes.rend(); ++scope) {
        auto found = scope->find(name);
        if (found != scope->end())
            return found->second;
    }
    return nullptr;
}

ExprType NumberAST::analyze(Sema&, const std::set<Symbol*>&) {
    Ty = ExprType::Int;
    return Ty;
}

ExprType VariableAST::analyze(Sema& S, const std::set<Symbol*>& Assigned) {
    Sym = S.lookup(Name);
    if (!Sym) {
        S.error(Line, "use of undeclared identifier '" + Name + "'");
        Ty = ExprType::Error;
        return Ty;
    }
    if (!Assigned.count(Sym))
        S.error(Line, "'" + Name + "' is used uninitialized");
    Ty = ExprType::Int;
    return Ty;
}

ExprType BinaryAST::analyze(Sema& S, const std::set<Symbol*>& Assigned) {
    ExprType left = LHS->analyze(S, Assigned);
    ExprType right = RHS->analyze(S, Assigned);
    if (isError(left) || isError(right)) {
        Ty = ExprType::Error;
        return Ty;
    }
    if (left != ExprType::Int || right != ExprType::Int) {
        S.error(Line, std::string("operands of '") + Op + "' must be int");
        Ty = ExprType::Error;
        return Ty;
    }
    Ty = ExprType::Int;
    return Ty;
}

ExprType CompAST::analyze(Sema& S, const std::set<Symbol*>& Assigned) {
    ExprType left = LHS->analyze(S, Assigned);
    ExprType right = RHS->analyze(S, Assigned);
    if (isError(left) || isError(right)) {
        Ty = ExprType::Error;
        return Ty;
    }
    if (left != ExprType::Int || right != ExprType::Int) {
        S.error(Line, std::string("operands of '") + cmpOpName(Op) + "' must be int");
        Ty = ExprType::Error;
        return Ty;
    }
    Ty = ExprType::Bool;
    return Ty;
}

Flow VarDeclAST::analyze(Sema& S, std::set<Symbol*> Assigned) {
    Sym = S.declare(Name, Line);
    return Flow{std::move(Assigned), false};
}

Flow AssignAST::analyze(Sema& S, std::set<Symbol*> Assigned) {
    Sym = S.lookup(Name);
    if (!Sym)
        S.error(Line, "assignment to undeclared identifier '" + Name + "'");

    ExprType value = Val->analyze(S, Assigned);
    if (!isError(value) && value != ExprType::Int)
        S.error(Line, std::string("cannot assign ") + typeName(value) + " to int");

    if (Sym && value == ExprType::Int)
        Assigned.insert(Sym);
    return Flow{std::move(Assigned), false};
}

Flow PrintAST::analyze(Sema& S, std::set<Symbol*> Assigned) {
    ExprType value = Val->analyze(S, Assigned);
    if (!isError(value) && value != ExprType::Int)
        S.error(Line, std::string("print expects int, got ") + typeName(value));
    return Flow{std::move(Assigned), false};
}

Flow ReturnAST::analyze(Sema& S, std::set<Symbol*> Assigned) {
    ExprType value = Val->analyze(S, Assigned);
    if (!isError(value) && value != ExprType::Int)
        S.error(Line, std::string("return value must be int, got ") + typeName(value));
    DoesReturn = true;
    return Flow{std::move(Assigned), true};
}

static void checkCondition(Sema& S, ExprAST* cond, const char* kind) {
    ExprType type = cond->type();
    if (isError(type) || type == ExprType::Bool)
        return;
    S.error(cond->line(), std::string("condition of ") + kind + " must be bool, got " + typeName(type));
}

Flow IfAST::analyze(Sema& S, std::set<Symbol*> Assigned) {
    Cond->analyze(S, Assigned);
    checkCondition(S, Cond, "if");

    Flow thenFlow = Then ? Then->analyze(S, Assigned) : Flow{Assigned, false};
    Flow elseFlow = Else ? Else->analyze(S, Assigned) : Flow{Assigned, false};

    std::set<Symbol*> outgoing;
    if (Else) {
        std::set_intersection(
            thenFlow.Assigned.begin(), thenFlow.Assigned.end(),
            elseFlow.Assigned.begin(), elseFlow.Assigned.end(),
            std::inserter(outgoing, outgoing.end()));
    } else {
        outgoing = Assigned;
    }

    DoesReturn = Else && thenFlow.AlwaysReturns && elseFlow.AlwaysReturns;
    return Flow{std::move(outgoing), DoesReturn};
}

Flow WhileAST::analyze(Sema& S, std::set<Symbol*> Assigned) {
    Cond->analyze(S, Assigned);
    checkCondition(S, Cond, "while");
    if (Body)
        Body->analyze(S, Assigned);
    return Flow{std::move(Assigned), false};
}

Flow BlockAST::analyze(Sema& S, std::set<Symbol*> Assigned) {
    S.enterScope();
    bool reachable = true;
    bool alwaysReturns = false;
    for (StmtAST* stmt : Stmts) {
        if (!reachable)
            S.error(stmt->line(), "unreachable code");
        Flow next = stmt->analyze(S, Assigned);
        if (reachable && next.AlwaysReturns) {
            alwaysReturns = true;
            reachable = false;
        }
        Assigned = std::move(next.Assigned);
    }
    S.leaveScope();
    DoesReturn = alwaysReturns;
    return Flow{std::move(Assigned), alwaysReturns};
}

void FunctionAST::analyze(Sema& S) {
    Flow body = Body->analyze(S, {});
    if (!body.AlwaysReturns)
        S.warning(Line, "control reaches end of function '" + Name + "'");
}
