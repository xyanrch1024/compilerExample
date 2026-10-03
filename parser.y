%{
#include <cstdio>
#include <cstdlib>
int yylex();
void yyerror(const char* s);
%}

%code requires {
#include "ast.h"
}

%locations

%union {
    int numval;
    char* strval;
    ExprAST* expr;
    StmtAST* stmt;
    BlockAST* block;
}

%token <numval> T_NUM
%token <strval> T_ID
%token T_INT T_PRINT T_RETURN
%token T_IF T_ELSE T_WHILE
%token T_LT T_GT T_LE T_GE T_EQ T_NE

%type <expr> expr add_expr term factor
%type <stmt> stmt var_decl assign_stmt print_stmt return_stmt if_stmt while_stmt
%type <block> block stmt_list

%expect 1

%%

program
    : function
    ;

function
    : T_INT T_ID '(' ')' block      {
        ProgramAST = new FunctionAST(std::string($2), $5, @1.first_line);
        free($2);
    }
    ;

block
    : '{' stmt_list '}'             {
        $$ = $2;
        $$->setLine(@1.first_line);
    }
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
        $$ = new VarDeclAST(std::string($2), @1.first_line);
        free($2);
    }
    ;

assign_stmt
    : T_ID '=' expr                 {
        $$ = new AssignAST(std::string($1), $3, @1.first_line);
        free($1);
    }
    ;

print_stmt
    : T_PRINT '(' expr ')'          { $$ = new PrintAST($3, @1.first_line); }
    ;

return_stmt
    : T_RETURN expr                 { $$ = new ReturnAST($2, @1.first_line); }
    ;

if_stmt
    : T_IF '(' expr ')' stmt T_ELSE stmt    { $$ = new IfAST($3, $5, $7, @1.first_line); }
    | T_IF '(' expr ')' stmt                { $$ = new IfAST($3, $5, nullptr, @1.first_line); }
    ;

while_stmt
    : T_WHILE '(' expr ')' stmt     { $$ = new WhileAST($3, $5, @1.first_line); }
    ;

/* Comparisons sit below + and -, so `a < b + c` is `a < (b + c)`. */
expr
    : expr T_LT add_expr            { $$ = new CompAST(T_LT, $1, $3, @2.first_line); }
    | expr T_GT add_expr            { $$ = new CompAST(T_GT, $1, $3, @2.first_line); }
    | expr T_LE add_expr            { $$ = new CompAST(T_LE, $1, $3, @2.first_line); }
    | expr T_GE add_expr            { $$ = new CompAST(T_GE, $1, $3, @2.first_line); }
    | expr T_EQ add_expr            { $$ = new CompAST(T_EQ, $1, $3, @2.first_line); }
    | expr T_NE add_expr            { $$ = new CompAST(T_NE, $1, $3, @2.first_line); }
    | add_expr                      { $$ = $1; }
    ;

add_expr
    : add_expr '+' term             { $$ = new BinaryAST('+', $1, $3, @2.first_line); }
    | add_expr '-' term             { $$ = new BinaryAST('-', $1, $3, @2.first_line); }
    | term                          { $$ = $1; }
    ;

term
    : term '*' factor               { $$ = new BinaryAST('*', $1, $3, @2.first_line); }
    | term '/' factor               { $$ = new BinaryAST('/', $1, $3, @2.first_line); }
    | factor                        { $$ = $1; }
    ;

factor
    : T_NUM                         { $$ = new NumberAST($1, @1.first_line); }
    | T_ID                          {
        $$ = new VariableAST(std::string($1), @1.first_line);
        free($1);
    }
    | '(' expr ')'                  { $$ = $2; }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr, "%s:%d: error: %s\n", SourcePath, yylloc.first_line, s);
}
