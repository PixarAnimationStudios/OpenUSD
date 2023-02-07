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
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/pathParser.h"
#include "pxr/usd/sdf/tokens.h"
#include "path.tab.h"

#include <string>

#ifndef fileno
#define fileno(fd) ArchFileNo(fd)
#endif
#ifndef isatty
#define isatty(fd) ArchFileIsaTTY(fd)
#endif

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

// As a pure parser, we must define the following
#define YY_DECL int pathUtf8Yylex(YYSTYPE *yylval_param, yyscan_t yyscanner)

%}

/* Configuration options for flex */
%option noyywrap
%option nounput
%option reentrant
%option bison-bridge

 /* character classes
  * defines UTF-8 encoded byte values for standard ASCII
  * and multi-byte UTF-8 character sets
  * valid multi-byte UTF-8 sequences are as follows:
  * For an n-byte encoded UTF-8 character, the last n-1 bytes range [\x80-\xbf]
  * 2-byte UTF-8 characters, first byte in range [\xc2-\xdf]
  * 3-byte UTF-8 characters, first byte in range [\xe0-\xef]
  * 4-byte UTF-8 characters, first byte in range [\xf0-\xf4]
  * ASCII characters span [\x41-\x5a] (upper case) [\x61-\x7a] (lower case) [\x30-39] (digits)
  */
ALPHA1      [\x41-\x5a]
ALPHA2      [\x61-\x7a]
DIGIT       [\x30-\x39]
UEND        [\x80-\xbf]
U2PRE       [\xc2-\xdf]
U3PRE       [\xe0-\xef]
U4PRE       [\xf0-\xf4]
UNDER       [_]
DASH        [\-]
BAR         [\|]
ALPHA       {ALPHA1}|{ALPHA2}
ALPHANUM    {ALPHA}|{DIGIT}
UTF8X       {U2PRE}{UEND}|{U3PRE}{UEND}{UEND}|{U4PRE}{UEND}{UEND}{UEND}
UTF8        {ALPHANUM}|{UTF8X}
UTF8U       {UTF8}|{UNDER}
UTF8UD      {UTF8U}|{DASH}
UTF8UDB     {UTF8UD}|{BAR}

%%

"mapper"     {
    yylval_param->token = SdfPathTokens->mapperIndicator;
    return TOK_MAPPER;
}
"expression" {
    yylval_param->token = SdfPathTokens->expressionIndicator;
    return TOK_EXPRESSION;
}

 /* In a Unicode enabled scheme, 'identifiers' are generally
  * categorized as something that begins with something in the
  * XID_Start category followed by zero or more things in the
  * XID_Continue category.  Since the number of characters in
  * these classes are large, we can't explicitly validate them
  * here easily, so the lex rule is pretty permissive with some
  * further validation done in code prior to calling what was
  * read an 'identifier'.  Note this rule will also match
  * standard ASCII strings because the UTF-8 encoded byte 
  * representation is the same for these characters.
  */
{UTF8U}{UTF8U}* {
    yylval_param->token = TfToken(yytext);

    // the matched string is semantically an identifier if
    // it satisfies XID_Start XID_Continue*
    // otherwise it can only be a prim name
    std::string matchedString = std::string(yytext, yyleng);
    if (TfIsValidIdentifier(matchedString))
	{
		return TOK_IDENTIFIER;
	}
	else if (TfIsValidPrimName(matchedString))
	{
		return TOK_PRIM_NAME;
	}
    else if (SdfPath::IsValidVariantIdentifier(matchedString))
    {
        return TOK_VARIANT_NAME;
    }
	else
	{
		return -1;
	}
}

 /* Namespaced identifiers are identifiers separated by a ':' character
  * same deal as above - the lex rule is permissive and further validation
  * is handled underneath.
  */
{UTF8U}{UTF8U}*(:{UTF8U}{UTF8U}*)+ {
    yylval_param->token = TfToken(yytext);

    // namespaced identifiers are property specifiers, not prim
    // so they must match identifier names to be accepted
    // note that it will be validated twice (once here and once when
    // adding the property part to the path) but without validation
    // here we won't get the nice ill-formed path warnings / errors
    std::string matchedString = std::string(yytext, yyleng);
    constexpr char delim = ':';
    std::string::const_iterator current = matchedString.begin();
    std::string::const_iterator end = matchedString.end();
    std::string::const_iterator subSequenceBegin = matchedString.begin();
    while (current != end)
    {
        if (*current == delim)
        {
            if (!_TfIsValidIdentifierSubsequence(matchedString, subSequenceBegin, current))
            {
                // this indicates that the given subsequence is not a valid identifier
                // making the whole namespaced identifier token invalid
                // return an error to the parser which will emit a warning / error
                return -1;
            }

            subSequenceBegin = current + 1;
        }

        current++;
    }

    // the loop above would not have checked the last subsequence, so do that now
    return _TfIsValidIdentifierSubsequence(matchedString, subSequenceBegin, end) ? TOK_NAMESPACED_IDENTIFIER : -1;
}

 /* A more lenient definition of an identifier for locks that can include
  * hyphens (e.g. GUIDs). These potentially have dashes in all but the first character.
  * Prims with non-identifier names can't actually be created.
  */
{UTF8U}{UTF8UD}* {
    yylval_param->token = TfToken(yytext);
    return TOK_PRIM_NAME;
}

 /* In addition to allowing prim names, we allow variant names to have
  * both hyphens ('-') and bars ('|') and to start with those, digits, or underscores.
  * Again, permissive with additional validation underneath.
  * Note that anything that didn't have a hyphen / bar in it explicitly will
  * have already matched one of the previous rules and be either an identifier or prim name.
  * 
  * Altogether, variant names also have a '.' in front (optionally) which is 
  * handled by the parser, not the lexer rules.
  */
{UTF8UDB}{UTF8UDB}* {
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
