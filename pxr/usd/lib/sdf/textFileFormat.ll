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

#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/textParserContext.h"
#include "pxr/usd/sdf/parserHelpers.h"

// Token table from yacc file
#include "textFileFormat.tab.h"

using std::map;
using std::vector;

#define YYSTYPE Sdf_ParserHelpers::Value

// As a pure parser, we must define the following
#define YY_DECL int textFileFormatYylex(YYSTYPE *yylval_param, yyscan_t yyscanner)

// The context object will be used to store global state for the parser.
#define YY_EXTRA_TYPE Sdf_TextParserContext *

%}

/* Configuration options for flex */
%option noyywrap
%option nounput
%option reentrant
%option bison-bridge

/* States */
%x SLASHTERIX_COMMENT

%%

    /* skip over whitespace and comments */
    /* handle the first line # comment specially, since it contains the
       magic token */
[[:blank:]]+ {}
"#"[^\r\n]* {
        if (yyextra->menvaLineNo == 1) {
            (*yylval_param) = std::string(yytext, yyleng);
            return TOK_MAGIC;
        }
    }
"//"[^\r\n]* {}
"/*" BEGIN SLASHTERIX_COMMENT ;
<SLASHTERIX_COMMENT>.|\n|\r ;
<SLASHTERIX_COMMENT>"*/" BEGIN INITIAL ;

    /* newline is returned as TOK_NL
     * Note that newlines embedded in quoted strings and tuples are counted
     * as part of the token and do NOT emit a separate TOK_NL.
     */
((\r\n)|\r|\n) {
        yyextra->menvaLineNo++;
        return TOK_NL;
    }

    /* literal keywords.  we return the yytext so that the yacc grammar
       can make use of it. */
"add"                 { (*yylval_param) = std::string(yytext, yyleng); return TOK_ADD; }
"attributes"          { (*yylval_param) = std::string(yytext, yyleng); return TOK_ATTRIBUTES; }
"class"               { (*yylval_param) = std::string(yytext, yyleng); return TOK_CLASS; }
"config"              { (*yylval_param) = std::string(yytext, yyleng); return TOK_CONFIG; }
"connect"             { (*yylval_param) = std::string(yytext, yyleng); return TOK_CONNECT; }
"custom"              { (*yylval_param) = std::string(yytext, yyleng); return TOK_CUSTOM; }
"customData"          { (*yylval_param) = std::string(yytext, yyleng); return TOK_CUSTOMDATA; }
"default"             { (*yylval_param) = std::string(yytext, yyleng); return TOK_DEFAULT; }
"def"                 { (*yylval_param) = std::string(yytext, yyleng); return TOK_DEF; }
"delete"              { (*yylval_param) = std::string(yytext, yyleng); return TOK_DELETE; }
"dictionary"          { (*yylval_param) = std::string(yytext, yyleng); return TOK_DICTIONARY; }
"displayUnit"         { (*yylval_param) = std::string(yytext, yyleng); return TOK_DISPLAYUNIT; }
"doc"                 { (*yylval_param) = std::string(yytext, yyleng); return TOK_DOC; }
"inherits"            { (*yylval_param) = std::string(yytext, yyleng); return TOK_INHERITS; }
"kind"                { (*yylval_param) = std::string(yytext, yyleng); return TOK_KIND; }
"mapper"              { (*yylval_param) = std::string(yytext, yyleng); return TOK_MAPPER; }
"nameChildren"        { (*yylval_param) = std::string(yytext, yyleng); return TOK_NAMECHILDREN; }
"None"                { (*yylval_param) = std::string(yytext, yyleng); return TOK_NONE; }
"offset"              { (*yylval_param) = std::string(yytext, yyleng); return TOK_OFFSET; }
"over"                { (*yylval_param) = std::string(yytext, yyleng); return TOK_OVER; }
"payload"             { (*yylval_param) = std::string(yytext, yyleng); return TOK_PAYLOAD; }
"permission"          { (*yylval_param) = std::string(yytext, yyleng); return TOK_PERMISSION; }
"prefixSubstitutions" { (*yylval_param) = std::string(yytext, yyleng); return TOK_PREFIX_SUBSTITUTIONS; }
"properties"          { (*yylval_param) = std::string(yytext, yyleng); return TOK_PROPERTIES; }
"references"          { (*yylval_param) = std::string(yytext, yyleng); return TOK_REFERENCES; }
"relocates"           { (*yylval_param) = std::string(yytext, yyleng); return TOK_RELOCATES; }
"rel"                 { (*yylval_param) = std::string(yytext, yyleng); return TOK_REL; }
"reorder"             { (*yylval_param) = std::string(yytext, yyleng); return TOK_REORDER; }
"rootPrims"           { (*yylval_param) = std::string(yytext, yyleng); return TOK_ROOTPRIMS; }
"scale"               { (*yylval_param) = std::string(yytext, yyleng); return TOK_SCALE; }
"subLayers"           { (*yylval_param) = std::string(yytext, yyleng); return TOK_SUBLAYERS; }
"specializes"         { (*yylval_param) = std::string(yytext, yyleng); return TOK_SPECIALIZES; }
"symmetryArguments"   { (*yylval_param) = std::string(yytext, yyleng); return TOK_SYMMETRYARGUMENTS; }
"symmetryFunction"    { (*yylval_param) = std::string(yytext, yyleng); return TOK_SYMMETRYFUNCTION; }
"timeSamples"         { (*yylval_param) = std::string(yytext, yyleng); return TOK_TIME_SAMPLES; }
"uniform"             { (*yylval_param) = std::string(yytext, yyleng); return TOK_UNIFORM; }
"variantSet"          { (*yylval_param) = std::string(yytext, yyleng); return TOK_VARIANTSET; }
"variantSets"         { (*yylval_param) = std::string(yytext, yyleng); return TOK_VARIANTSETS; }
"variants"            { (*yylval_param) = std::string(yytext, yyleng); return TOK_VARIANTS; }
"varying"             { (*yylval_param) = std::string(yytext, yyleng); return TOK_VARYING; }

    /* unquoted C/Python identifier */
[[:alpha:]_][[:alnum:]_]* {
        (*yylval_param) = std::string(yytext, yyleng);
        return TOK_IDENTIFIER;
    }

    /* unquoted C++ namespaced identifier -- see bug 10775 */
[[:alpha:]_][[:alnum:]_]*(::[[:alpha:]_][[:alnum:]_]*)+ {
        (*yylval_param) = std::string(yytext, yyleng);
        return TOK_CXX_NAMESPACED_IDENTIFIER;
    }

    /* unquoted namespaced identifier.  matches any number of colon
     * delimited C/Python identifiers */
[[:alpha:]_][[:alnum:]_]*(:[[:alpha:]_][[:alnum:]_]*)+ {
        (*yylval_param) = std::string(yytext, yyleng);
        return TOK_NAMESPACED_IDENTIFIER;
    }

    /* scene paths */
\<[^\<\>\r\n]*\> {
        (*yylval_param) = Sdf_EvalQuotedString(yytext, yyleng, 1);
        return TOK_PATHREF;
    }

    /* asset references */
@([[:alnum:]$_/\. \-:]+([@#][[:alnum:]_/\.\-:]+)?)?@ {
        (*yylval_param) = Sdf_EvalQuotedString(yytext, yyleng, 1);
        return TOK_ASSETREF;
    }

    /* Singly quoted, single line strings with escapes.
       Note: we handle empty singly quoted strings below, to disambiguate
       them from the beginning of triply-quoted strings.
       Ex: "Foo \"foo\"" */
'([^'\r\n]|(\\.))+'   |  /* ' //<- unfreak out coloring code */
\"([^"\r\n]|(\\.))+\" {  /* " //<- unfreak out coloring code */
        (*yylval_param) = Sdf_EvalQuotedString(yytext, yyleng, 1);
        return TOK_STRING;
    }

    /* Empty singly quoted strings that aren't the beginning of
       a triply-quoted string. */
''/[^'] {  /* ' // <- keep syntax coloring from freaking out */
        (*yylval_param) = std::string();
        return TOK_STRING;
    }
\"\"/[^"] {
        (*yylval_param) = std::string();
        return TOK_STRING;
    }

    /* Triply quoted, multi-line strings with escapes.
       Ex: """A\n\"B\"\nC""" */
'''([^']|(\\.)|('{1,2}[^']))*'''        |  /* ' //<- unfreak out coloring code */
\"\"\"([^"]|(\\.)|(\"{1,2}[^"]))*\"\"\" {  /* " //<- unfreak out coloring code */

        unsigned int numlines = 0;
        (*yylval_param) = Sdf_EvalQuotedString(yytext, yyleng, 3, &numlines);
        yyextra->menvaLineNo += numlines;
        return TOK_STRING;
    }

    /* Super special case for negative 0.  We have to store this as a double to
     * preserve the sign.  There is no negative zero integral value, and we
     * don't know at this point what the final stored type will be. */
-0 {
        (*yylval_param) = double(-0.0);
        return TOK_NUMBER;
   }

    /* Positive integers: store as unsigned long. */
[[:digit:]]+ {
        bool outOfRange = false;
        (*yylval_param) = TfStringToULong(yytext, &outOfRange);
        if (outOfRange)
            return TOK_SYNTAX_ERROR;
        return TOK_NUMBER;
    }

    /* Negative integers: store as long. */
-[[:digit:]]+ {
        bool outOfRange = false;
        (*yylval_param) = TfStringToLong(yytext, &outOfRange);
        if (outOfRange)
            return TOK_SYNTAX_ERROR;
        return TOK_NUMBER;
    }

    /* Numbers with decimal places or exponents: store as double. */
-?[[:digit:]]+(\.[[:digit:]]*)?([eE][+\-]?[[:digit:]]+)?   |
-?\.[[:digit:]]+([eE][+\-]?[[:digit:]]+)? {
        (*yylval_param) = TfStringToDouble(yytext);
        return TOK_NUMBER;
    }

    /* regexps for negative infinity.  we don't handle inf and nan here
     * because they look like identifiers.  we handle them in parser where
     * we have the additional context we need to distinguish them from
     * identifiers. */
-inf {
        (*yylval_param) = -std::numeric_limits<double>::infinity();
        return TOK_NUMBER;
    }

    /* various single-character punctuation.  return the character
     * itself as the token.
     */
[=,:;\$\.\[\]\(\){}&@-] {
        return yytext[0];
    }

    /* the default rule is to ECHO any unmatched character.  by returning a
     * token that the parser does not know how to handle these become syntax
     * errors instead.
     */
<*>.|\\n {
        return TOK_SYNTAX_ERROR;
    }

%%
