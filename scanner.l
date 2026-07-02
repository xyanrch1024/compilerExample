%{
#include "parser.tab.hpp"
#include "ast.h"
void yyerror(const char* s);
%}

%option noyywrap
%option noinput
%option nounput

%%
"int"           { return T_INT; }
"print"         { return T_PRINT; }
"if"            { return T_IF; }
"else"          { return T_ELSE; }
"while"         { return T_WHILE; }
"return"        { return T_RETURN; }
[a-zA-Z_][a-zA-Z0-9_]*  { yylval.strval = strdup(yytext); return T_ID; }
[0-9]+          { yylval.numval = atoi(yytext); return T_NUM; }
"="             { return '='; }
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"("             { return '('; }
")"             { return ')'; }
"{"             { return '{'; }
"}"             { return '}'; }
";"             { return ';'; }
"<"             { return T_LT; }
">"             { return T_GT; }
"<="            { return T_LE; }
">="            { return T_GE; }
"=="            { return T_EQ; }
"!="            { return T_NE; }
[ \t\n\r]+     { /* skip whitespace */ }
"//".*          { /* skip line comment */ }
.               { yyerror("unknown token"); }
%%
