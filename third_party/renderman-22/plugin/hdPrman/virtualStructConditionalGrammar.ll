%{
//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
