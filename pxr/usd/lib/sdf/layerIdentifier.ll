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

#include <string>
#include "layerIdentifier.tab.h"

#define YYSTYPE std::string

// Lexical scanner type.
typedef void *yyscan_t;

// As a pure parser, we must define the following
#define YY_DECL int layerIdentifierYylex(YYSTYPE *yylval_param, yyscan_t yyscanner)

%}

/* Configuration options for flex */
%option noyywrap
%option nounput
%option reentrant
%option bison-bridge

%%

 /* Match characters other than the argument separators at the beginning
    of an identifier as the path to the layer.
  */
^[^?]* {
    (*yylval_param) = yytext;
    return TOK_LAYER_PATH;
}

 /* Return argument separators and = as themselves. */
[?&=] {
    return yytext[0];
}

 /* unquoted C/Python identifier */
[[:alpha:]_][[:alnum:]_]* {
    (*yylval_param) = std::string(yytext, yyleng);
    return TOK_IDENTIFIER;
}

 /* Argument values can contain any printable character except the above
    delimiters.
  */
[^?&=]* {
    (*yylval_param) = std::string(yytext, yyleng);
    return TOK_VALUE;
}

%%
