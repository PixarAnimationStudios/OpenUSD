%{
//
// Copyright 2016 Pixar
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
#include "pxr/usd/sdf/pathParser.h"
#include "pxr/usd/sdf/tokens.h"
#include "path.tab.h"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

// As a pure parser, we must define the following
#define YY_DECL int pathYylex(YYSTYPE *yylval_param, yyscan_t yyscanner)

%}

/* Configuration options for flex */
%option noyywrap
%option nounput
%option reentrant
%option bison-bridge

%%

"mapper"     {
    yylval_param->token = SdfPathTokens->mapperIndicator;
    return TOK_MAPPER;
}
"expression" {
    yylval_param->token = SdfPathTokens->expressionIndicator;
    return TOK_EXPRESSION;
}

 /* This should always match identifiers in the menva parser.  We
  * currently allow valid C/Python identifiers.
  */
[[:alpha:]_][[:alnum:]_]* {
    yylval_param->token = TfToken(yytext);
    return TOK_IDENTIFIER;
}

 /* Match any number of colon delimited C/Python identifiers.
  */
[[:alpha:]_][[:alnum:]_]*(:[[:alpha:]_][[:alnum:]_]*)+ {
    yylval_param->token = TfToken(yytext);
    return TOK_NAMESPACED_IDENTIFIER;
}

 /* This is more lenient than an identifier for locks, which use GUIDs
  * in paths and the GUIDs use hyphens.  Prims with non-identifier names
  * can't actually be created.
  */
[[:alpha:]_][[:alnum:]_\-]* {
    yylval_param->token = TfToken(yytext);
    return TOK_PRIM_NAME;
}

 /* In addition to allowing prim names, we allow variant names to have
  * hyphens and bars, and to start with a digit, underbar, bar or hyphen.
  * We have to be careful here not to match a prim name so in the case
  * where we start with [:alnum:] we must have at least one bar somewhere
  * in the name.
  *
  * Altogether, variant names match .?[[:alnum:]_|\-]*;  the optional
  * leading dot is handled by the parser.
  */
([[:digit:]_|\-][[:alnum:]_|\-]*|[[:alnum:]_\-]*\|[[:alnum:]_|\-]*) {
    yylval_param->token = TfToken(yytext);
    return TOK_VARIANT_NAME;
}

".." {
    return TOK_DOTDOT;
}

[ \t]+ { 
    return TOK_WHITESPACE; 
}

. { 
    return yytext[0];
}

%%
