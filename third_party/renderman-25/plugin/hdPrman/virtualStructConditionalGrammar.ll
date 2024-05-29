%{
//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/arch/fileSystem.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "virtualStructConditionalGrammar.tab.h"

PXR_NAMESPACE_USING_DIRECTIVE

struct _VSCGParserData;

#ifndef fileno
#define fileno(fd) ArchFileNo(fd)
#endif
#ifndef isatty
#define isatty(fd) ArchFileIsaTTY(fd)
#endif

%}

%option noyy_top_state
%option reentrant
%option bison-bridge bison-locations
%option nounput
%option prefix="virtualStructConditionalGrammarYy"

D           [0-9]
L           [a-zA-Z_]
H           [a-fA-F0-9]
E           [Ee][+-]?{D}+

ID           ({L}({L}|{D})*)

%%

"is"    {return OP_IS;}
"and"   {return OP_AND;}
"or"    {return OP_OR;}

"if"        {return KW_IF;}
"else"      {return KW_ELSE;}
"connected" {return KW_CONNECTED;}

"is not" {return OP_ISNOT;}

"copy" {return KW_COPY;}
"set" {return KW_SET;}
"ignore" {return KW_IGNORE;}
"connect" {return KW_CONNECT;}




-?{D}+|-?{D}+{E}|-?{D}*"."{D}+({E})?|-?{D}+"."{D}*({E})?  {
    yylval->string = strdup(yyget_text(yyscanner));
    return NUMBER;
}

'(\\.|[^\\'])*'   {
    char * str = yyget_text(yyscanner);
    yylval->string = strdup(str+1);
    yylval->string[strlen(str)-2] = '\0';
    return STRING;
}

\"(\\.|[^\\"])*\" {
    char * str = yyget_text(yyscanner);
    yylval->string = strdup(str+1);
    yylval->string[strlen(str)-2] = '\0';
    return STRING;
}

{ID}\[{D}+\] {
    yylval->string = strdup(yyget_text(yyscanner));
    return PARAM;
}

{ID} {
    yylval->string = strdup(yyget_text(yyscanner));
    return PARAM;
}

"(" {return LPAR;}
")" {return RPAR;}

"==" {return OP_EQ;}
"!=" {return OP_NOTEQ;}
">" {return OP_GT;}
"<" {return OP_LT;}
">=" {return OP_GTEQ;}
"<=" {return OP_LTEQ;}

[ \t\v\n\f]     {}

. {return UNRECOGNIZED_TOKEN;}

%%

int virtualStructConditionalGrammarYywrap(yyscan_t yyscanner)
{
    return 1;
}
