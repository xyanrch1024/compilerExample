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

bool Sema::analyze(ProgramAST* program) {
    if (!program)
        return false;
    program->accept(*this);
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

FunctionAST* Sema::declareFunction(FunctionAST* function) {
    if (function->name() == "printf")
        error(function->line(), "function name 'printf' is reserved");

    auto found = Functions.find(function->name());
    if (found != Functions.end()) {
        error(function->line(), "redefinition of function '" + function->name() + "'");
        note(found->second->line(), "previous definition is here");
        return found->second;
    }
    Functions[function->name()] = function;
    return function;
}

FunctionAST* Sema::lookupFunction(const std::string& name) {
    auto found = Functions.find(name);
    if (found == Functions.end())
        return nullptr;
    return found->second;
}

ExprType Sema::visit(CallAST& node, const std::set<Symbol*>&) {
    node.Func = lookupFunction(node.Callee);
    if (!node.Func) {
        error(node.Line, "call to undeclared function '" + node.Callee + "'");
        node.Ty = ExprType::Error;
        return node.Ty;
    }
    node.Ty = ExprType::Int;
    return node.Ty;
}

ExprType Sema::visit(NumberAST& node, const std::set<Symbol*>&) {
    node.Ty = ExprType::Int;
    return node.Ty;
}

ExprType Sema::visit(VariableAST& node, const std::set<Symbol*>& assigned) {
    node.Sym = lookup(node.Name);
    if (!node.Sym) {
        error(node.Line, "use of undeclared identifier '" + node.Name + "'");
        node.Ty = ExprType::Error;
        return node.Ty;
    }
    if (!assigned.count(node.Sym))
        error(node.Line, "'" + node.Name + "' is used uninitialized");
    node.Ty = ExprType::Int;
    return node.Ty;
}

ExprType Sema::visit(BinaryAST& node, const std::set<Symbol*>& assigned) {
    ExprType left = node.LHS->accept(*this, assigned);
    ExprType right = node.RHS->accept(*this, assigned);
    if (isError(left) || isError(right)) {
        node.Ty = ExprType::Error;
        return node.Ty;
    }
    if (left != ExprType::Int || right != ExprType::Int) {
        error(node.Line, std::string("operands of '") + node.Op + "' must be int");
        node.Ty = ExprType::Error;
        return node.Ty;
    }
    node.Ty = ExprType::Int;
    return node.Ty;
}

ExprType Sema::visit(CompAST& node, const std::set<Symbol*>& assigned) {
    ExprType left = node.LHS->accept(*this, assigned);
    ExprType right = node.RHS->accept(*this, assigned);
    if (isError(left) || isError(right)) {
        node.Ty = ExprType::Error;
        return node.Ty;
    }
    if (left != ExprType::Int || right != ExprType::Int) {
        error(node.Line, std::string("operands of '") + cmpOpName(node.Op) + "' must be int");
        node.Ty = ExprType::Error;
        return node.Ty;
    }
    node.Ty = ExprType::Bool;
    return node.Ty;
}

Flow Sema::visit(VarDeclAST& node, std::set<Symbol*> assigned) {
    node.Sym = declare(node.Name, node.Line);
    return Flow{std::move(assigned), false};
}

Flow Sema::visit(AssignAST& node, std::set<Symbol*> assigned) {
    node.Sym = lookup(node.Name);
    if (!node.Sym)
        error(node.Line, "assignment to undeclared identifier '" + node.Name + "'");

    ExprType value = node.Val->accept(*this, assigned);
    if (!isError(value) && value != ExprType::Int)
        error(node.Line, std::string("cannot assign ") + typeName(value) + " to int");

    if (node.Sym && value == ExprType::Int)
        assigned.insert(node.Sym);
    return Flow{std::move(assigned), false};
}

Flow Sema::visit(PrintAST& node, std::set<Symbol*> assigned) {
    ExprType value = node.Val->accept(*this, assigned);
    if (!isError(value) && value != ExprType::Int)
        error(node.Line, std::string("print expects int, got ") + typeName(value));
    return Flow{std::move(assigned), false};
}

Flow Sema::visit(CallStmtAST& node, std::set<Symbol*> assigned) {
    node.Call->accept(*this, assigned);
    return Flow{std::move(assigned), false};
}

Flow Sema::visit(ReturnAST& node, std::set<Symbol*> assigned) {
    ExprType value = node.Val->accept(*this, assigned);
    if (!isError(value) && value != ExprType::Int)
        error(node.Line, std::string("return value must be int, got ") + typeName(value));
    node.DoesReturn = true;
    return Flow{std::move(assigned), true};
}

static void checkCondition(Sema& sema, ExprAST* cond, const char* kind) {
    ExprType type = cond->type();
    if (isError(type) || type == ExprType::Bool)
        return;
    sema.error(cond->line(), std::string("condition of ") + kind + " must be bool, got " + typeName(type));
}

Flow Sema::visit(IfAST& node, std::set<Symbol*> assigned) {
    node.Cond->accept(*this, assigned);
    checkCondition(*this, node.Cond, "if");

    Flow thenFlow = node.Then ? node.Then->accept(*this, assigned) : Flow{assigned, false};
    Flow elseFlow = node.Else ? node.Else->accept(*this, assigned) : Flow{assigned, false};

    std::set<Symbol*> outgoing;
    if (node.Else) {
        std::set_intersection(
            thenFlow.Assigned.begin(), thenFlow.Assigned.end(),
            elseFlow.Assigned.begin(), elseFlow.Assigned.end(),
            std::inserter(outgoing, outgoing.end()));
    } else {
        outgoing = assigned;
    }

    node.DoesReturn = node.Else && thenFlow.AlwaysReturns && elseFlow.AlwaysReturns;
    return Flow{std::move(outgoing), node.DoesReturn};
}

Flow Sema::visit(WhileAST& node, std::set<Symbol*> assigned) {
    node.Cond->accept(*this, assigned);
    checkCondition(*this, node.Cond, "while");
    if (node.Body)
        node.Body->accept(*this, assigned);
    return Flow{std::move(assigned), false};
}

Flow Sema::visit(BlockAST& node, std::set<Symbol*> assigned) {
    enterScope();
    bool reachable = true;
    bool alwaysReturns = false;
    for (StmtAST* stmt : node.Stmts) {
        if (!reachable)
            error(stmt->line(), "unreachable code");
        Flow next = stmt->accept(*this, assigned);
        if (reachable && next.AlwaysReturns) {
            alwaysReturns = true;
            reachable = false;
        }
        assigned = std::move(next.Assigned);
    }
    leaveScope();
    node.DoesReturn = alwaysReturns;
    return Flow{std::move(assigned), alwaysReturns};
}

void Sema::visit(FunctionAST& function) {
    Flow body = function.Body->accept(*this, {});
    if (!body.AlwaysReturns)
        warning(function.Line, "control reaches end of function '" + function.Name + "'");
}

void Sema::visit(ProgramAST& program) {
    for (FunctionAST* function : program.Functions)
        declareFunction(function);

    bool hasMain = false;
    for (FunctionAST* function : program.Functions) {
        if (function->name() == "main")
            hasMain = true;
        function->accept(*this);
    }
    if (!hasMain)
        error(program.Line, "program must define 'main'");
}
