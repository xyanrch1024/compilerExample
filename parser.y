%{
#include <cstdio>
#include <cstdlib>
int yylex();
void yyerror(const char* s);
extern int yylineno;
%}

%code requires {
#include "ast.h"
}

%union {
    int numval;
    char* strval;
    ExprAST* expr;
    StmtAST* stmt;
    BlockAST* block;
    FunctionAST* func;
}

%token <numval> T_NUM
%token <strval> T_ID
%token T_INT T_PRINT T_RETURN
%token T_IF T_ELSE T_WHILE
%token T_LT T_GT T_LE T_GE T_EQ T_NE

%type <expr> expr term factor
%type <stmt> stmt var_decl assign_stmt print_stmt return_stmt if_stmt while_stmt
%type <block> block stmt_list
%type <func> function

%left T_EQ T_NE
%left T_LT T_GT T_LE T_GE
%left '+' '-'
%left '*' '/'

%%

program
    : function                      { /* done */ }
    ;

function
    : T_INT T_ID '(' ')' block      {
        $$ = new FunctionAST(std::string($2), $5);
        $$->codegen();
        free($2);
    }
    ;

block
    : '{' stmt_list '}'             { $$ = $2; }
    ;

stmt_list
    : stmt_list stmt                {
        if ($2) $1->add($2);
        $$ = $1;
    }
    |                               { $$ = new BlockAST(); }
    ;

stmt
    : var_decl ';'                  { $$ = $1; }
    | assign_stmt ';'               { $$ = $1; }
    | print_stmt ';'                { $$ = $1; }
    | return_stmt ';'               { $$ = $1; }
    | if_stmt                       { $$ = $1; }
    | while_stmt                    { $$ = $1; }
    | block                         { $$ = $1; }
    ;

var_decl
    : T_INT T_ID                    {
        $$ = new VarDeclAST(std::string($2));
        free($2);
    }
    ;

assign_stmt
    : T_ID '=' expr                 {
        $$ = new AssignAST(std::string($1), $3);
        free($1);
    }
    ;

print_stmt
    : T_PRINT '(' expr ')'          { $$ = new PrintAST($3); }
    ;

return_stmt
    : T_RETURN expr                 { $$ = new ReturnAST($2); }
    ;

if_stmt
    : T_IF '(' expr ')' stmt T_ELSE stmt    { $$ = new IfAST($3, $5, $7); }
    | T_IF '(' expr ')' stmt                { $$ = new IfAST($3, $5, nullptr); }
    ;

while_stmt
    : T_WHILE '(' expr ')' stmt     { $$ = new WhileAST($3, $5); }
    ;

expr
    : expr '+' term                 { $$ = new BinaryAST('+', $1, $3); }
    | expr '-' term                 { $$ = new BinaryAST('-', $1, $3); }
    | expr T_LT term                { $$ = new CompAST(T_LT, $1, $3); }
    | expr T_GT term                { $$ = new CompAST(T_GT, $1, $3); }
    | expr T_LE term                { $$ = new CompAST(T_LE, $1, $3); }
    | expr T_GE term                { $$ = new CompAST(T_GE, $1, $3); }
    | expr T_EQ term                { $$ = new CompAST(T_EQ, $1, $3); }
    | expr T_NE term                { $$ = new CompAST(T_NE, $1, $3); }
    | term                          { $$ = $1; }
    ;

term
    : term '*' factor               { $$ = new BinaryAST('*', $1, $3); }
    | term '/' factor               { $$ = new BinaryAST('/', $1, $3); }
    | factor                        { $$ = $1; }
    ;

factor
    : T_NUM                         { $$ = new NumberAST($1); }
    | T_ID                          { $$ = new VariableAST(std::string($1)); free($1); }
    | '(' expr ')'                  { $$ = $2; }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr, "error: %s at line %d\n", s, yylineno);
}
