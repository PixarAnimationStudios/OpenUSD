//
// Copyright 2023 Pixar
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
#ifndef PXR_USD_SDF_TEXT_FILE_FORMAT_PARSER_H
#define PXR_USD_SDF_TEXT_FILE_FORMAT_PARSER_H

#include "pxr/pxr.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/debugCodes.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/parserHelpers.h"
#include "pxr/usd/sdf/path.h"

// transitively including pxr/base/tf/pxrPEGTL/pegtl.h
#include "pxr/usd/sdf/pathParser.h"
#include "pxr/usd/sdf/parserHelpers.h"
#include "pxr/usd/sdf/parserValueContext.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/textParserContext.h"
#include "pxr/usd/sdf/textParserHelpers.h"

#include <vector>
#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////
// TextFileFormat grammar:
// We adopt the convention in the following PEGTL rules where they take
// care of "internal padding" (i.e. whitespace within the grammar rule 
// itself) but not "external padding" (i.e. they will not consume
// whitespace prior to the first token, nor whitespace following the
// last token in the rule).
//
// The exception to this rule is the class of "separators" which do
// try to consume leading and trailing whitespace where appropriate

namespace Sdf_TextFileFormatParser {

namespace PEGTL_NS = tao::TAO_PEGTL_NAMESPACE;

// special characters
// note - Dot comes from pathParser.h
struct SingleQuote : PEGTL_NS::one<'\''> {};
struct DoubleQuote : PEGTL_NS::one<'"'> {};
struct LeftParen : PEGTL_NS::one<'('> {};
struct RightParen : PEGTL_NS::one<')'> {};
struct LeftBrace : PEGTL_NS::one<'['> {};
struct RightBrace : PEGTL_NS::one<']'> {};
struct LeftCurlyBrace : PEGTL_NS::one<'{'> {};
struct RightCurlyBrace : PEGTL_NS::one<'}'> {};
struct LeftAngleBracket : PEGTL_NS::one<'<'> {};
struct RightAngleBracket : PEGTL_NS::one<'>'> {};
struct At : PEGTL_NS::one<'@'> {};
struct Equals : PEGTL_NS::one<'='> {};
struct Minus : PEGTL_NS::one<'-'> {};
struct Exponent : PEGTL_NS::one<'e', 'E'> {};
struct Space : PEGTL_NS::one<' ', '\t'> {};

// character classes
struct Digit : PEGTL_NS::digit {};

// keywords
struct StrAdd : TAO_PEGTL_STRING("add") {};
struct StrAppend : TAO_PEGTL_STRING("append") {};
struct StrClass : TAO_PEGTL_STRING("class") {};
struct StrConfig : TAO_PEGTL_STRING("config") {};
struct StrConnect : TAO_PEGTL_STRING("connect") {};
struct StrCustom : TAO_PEGTL_STRING("custom") {};
struct StrCustomData : TAO_PEGTL_STRING("customData") {};
struct StrDefault : TAO_PEGTL_STRING("default") {};
struct StrDef : TAO_PEGTL_STRING("def") {};
struct StrDelete : TAO_PEGTL_STRING("delete") {};
struct StrDictionary : TAO_PEGTL_STRING("dictionary") {};
struct StrDisplayUnit : TAO_PEGTL_STRING("displayUnit") {};
struct StrDoc : TAO_PEGTL_STRING("doc") {};
struct StrInherits : TAO_PEGTL_STRING("inherits") {};
struct StrKind : TAO_PEGTL_STRING("kind") {};
struct StrNameChildren : TAO_PEGTL_STRING("nameChildren") {};
struct StrNone : TAO_PEGTL_STRING("None") {};
struct StrOffset: TAO_PEGTL_STRING("offset") {};
struct StrOver : TAO_PEGTL_STRING("over") {};
struct StrPayload : TAO_PEGTL_STRING("payload") {};
struct StrPermission : TAO_PEGTL_STRING("permission") {};
struct StrPrefixSubstitutions : TAO_PEGTL_STRING("prefixSubstitutions") {};
struct StrPrepend : TAO_PEGTL_STRING("prepend") {};
struct StrProperties : TAO_PEGTL_STRING("properties") {};
struct StrReferences : TAO_PEGTL_STRING("references") {};
struct StrRelocates : TAO_PEGTL_STRING("relocates") {};
struct StrRel : TAO_PEGTL_STRING("rel") {};
struct StrReorder : TAO_PEGTL_STRING("reorder") {};
struct StrRootPrims : TAO_PEGTL_STRING("rootPrims") {};
struct StrScale : TAO_PEGTL_STRING("scale") {};
struct StrSubLayers : TAO_PEGTL_STRING("subLayers") {};
struct StrSuffixSubstitutions : TAO_PEGTL_STRING("suffixSubstitutions") {};
struct StrSpecializes : TAO_PEGTL_STRING("specializes") {};
struct StrSymmetryArguments : TAO_PEGTL_STRING("symmetryArguments") {};
struct StrSymmetryFunction : TAO_PEGTL_STRING("symmetryFunction") {};
struct StrTimeSamples : TAO_PEGTL_STRING("timeSamples") {};
struct StrUniform : TAO_PEGTL_STRING("uniform") {};
struct StrVariantSet : TAO_PEGTL_STRING("variantSet") {};
struct StrVariantSets : TAO_PEGTL_STRING("variantSets") {};
struct StrVariants : TAO_PEGTL_STRING("variants") {};
struct StrVarying : TAO_PEGTL_STRING("varying") {};

struct StrKeywords : PEGTL_NS::sor<StrAdd, StrAppend, StrClass, StrConfig,
  StrConnect, StrCustom, StrCustomData, StrDefault, StrDef, StrDelete,
  StrDictionary, StrDisplayUnit, StrDoc, StrInherits, StrKind, StrNameChildren,
  StrNone, StrOffset, StrOver, StrPayload, StrPermission,
  StrPrefixSubstitutions, StrPrepend, StrProperties, StrReferences,
  StrRelocates, StrRel, StrReorder, StrRootPrims, StrScale, StrSubLayers,
  StrSuffixSubstitutions, StrSpecializes, StrSymmetryArguments, 
  StrSymmetryFunction, StrTimeSamples, StrUniform, StrVariantSets,
  StrVariantSet, StrVariants, StrVarying> {};

struct StrInf : TAO_PEGTL_STRING("inf") {};
struct StrNan : TAO_PEGTL_STRING("nan") {};

struct StrMathKeywords : PEGTL_NS::sor<StrInf, StrNan> {};

template <typename R>
struct Keyword : PEGTL_NS::seq<
    R, PEGTL_NS::not_at<PEGTL_NS::identifier_other>> {};

struct KeywordAdd : Keyword<StrAdd> {};
struct KeywordAppend : Keyword<StrAppend> {};
struct KeywordClass : Keyword<StrClass> {};
struct KeywordConfig : Keyword<StrConfig> {};
struct KeywordConnect : Keyword<StrConnect> {};
struct KeywordCustom : Keyword<StrCustom> {};
struct KeywordCustomData : Keyword<StrCustomData> {};
struct KeywordDefault : Keyword<StrDefault> {};
struct KeywordDef : Keyword<StrDef> {};
struct KeywordDelete : Keyword<StrDelete> {};
struct KeywordDictionary : Keyword<StrDictionary> {};
struct KeywordDisplayUnit : Keyword<StrDisplayUnit> {};
struct KeywordDoc : Keyword<StrDoc> {};
struct KeywordInherits : Keyword<StrInherits> {};
struct KeywordKind : Keyword<StrKind> {};
struct KeywordNameChildren : Keyword<StrNameChildren> {};
struct KeywordNone : Keyword<StrNone> {};
struct KeywordOffset : Keyword<StrOffset> {};
struct KeywordOver : Keyword<StrOver> {};
struct KeywordPayload : Keyword<StrPayload> {};
struct KeywordPermission : Keyword<StrPermission> {};
struct KeywordPrefixSubstitutions : Keyword<StrPrefixSubstitutions> {};
struct KeywordPrepend : Keyword<StrPrepend> {};
struct KeywordProperties : Keyword<StrProperties> {};
struct KeywordReferences : Keyword<StrReferences> {};
struct KeywordRelocates : Keyword<StrRelocates> {};
struct KeywordRel : Keyword<StrRel> {};
struct KeywordReorder : Keyword<StrReorder> {};
struct KeywordRootPrims : Keyword<StrRootPrims> {};
struct KeywordScale : Keyword<StrScale> {};
struct KeywordSubLayers : Keyword<StrSubLayers> {};
struct KeywordSuffixSubstitutions : Keyword<StrSuffixSubstitutions> {};
struct KeywordSpecializes : Keyword<StrSpecializes> {};
struct KeywordSymmetryArguments : Keyword<StrSymmetryArguments> {};
struct KeywordSymmetryFunction : Keyword<StrSymmetryFunction> {};
struct KeywordTimeSamples : Keyword<StrTimeSamples> {};
struct KeywordUniform : Keyword<StrUniform> {};
struct KeywordVariantSet : Keyword<StrVariantSet> {};
struct KeywordVariantSets : Keyword<StrVariantSets> {};
struct KeywordVariants : Keyword<StrVariants> {};
struct KeywordVarying : Keyword<StrVarying> {};

struct Keywords : Keyword<StrKeywords> {};

struct MathKeywordInf : Keyword<StrInf> {};
struct MathKeywordNan : Keyword<StrNan> {};

struct MathKeywords : Keyword<StrMathKeywords> {};

struct Utf8CharacterNoEolf
{
    using rule_t = Utf8CharacterNoEolf;

    template <typename ParseInput>
    static bool match(ParseInput& in)
    {
        if (!in.empty())
        {
            auto utf8_char = PEGTL_NS::internal::peek_utf8::peek(in);
            if (utf8_char.size != 0)
            {
                // any UTF8-character that isn't a CR / LF
                if ((static_cast<uint32_t>(utf8_char.data) == 0x000Au) ||
                    (static_cast<uint32_t>(utf8_char.data) == 0x000Du))
                {
                    return false;
                }

                in.bump(utf8_char.size);
                return true;
            }
        }

        return false;
    }
};

struct PythonStyleComment : PEGTL_NS::disable<
    PEGTL_NS::one<'#'>, PEGTL_NS::star<PEGTL_NS::not_at<PEGTL_NS::eolf>,
    Utf8CharacterNoEolf>> {};
struct CppStyleSingleLineComment : PEGTL_NS::disable<
    PEGTL_NS::two<'/'>, PEGTL_NS::star<PEGTL_NS::not_at<PEGTL_NS::eolf>,
    Utf8CharacterNoEolf>> {};
struct CppStyleMultiLineComment : PEGTL_NS::disable<
    PEGTL_NS::seq<PEGTL_NS::one<'/'>, PEGTL_NS::one<'*'>,
    PEGTL_NS::until<PEGTL_NS::seq<PEGTL_NS::one<'*'>,
    PEGTL_NS::one<'/'>>>>> {};
struct Comment : PEGTL_NS::sor<
    PythonStyleComment,
    CppStyleSingleLineComment,
    CppStyleMultiLineComment> {};

// whitespace rules
// TokenSpace represents whitespace between tokens,
// which can include space, tab, and c++ multiline style comments
// but MUST include a single space / tab character, that is:
// def/*comment*/foo is illegal but
// def /*comment*/foo or
// def/*comment*/ foo are both legal
struct TokenSpace : PEGTL_NS::sor<
    PEGTL_NS::seq<PEGTL_NS::plus<Space>,
        PEGTL_NS::opt<PEGTL_NS::list_tail<CppStyleMultiLineComment, Space>>>,
        PEGTL_NS::seq<PEGTL_NS::list<CppStyleMultiLineComment, Space>,
            PEGTL_NS::plus<Space>>> {};

struct EolWhitespace : PEGTL_NS::star<PEGTL_NS::sor<Space, Comment>> {};
struct Crlf : PEGTL_NS::sor<
    PEGTL_NS::seq<PEGTL_NS::one<'\r'>, PEGTL_NS::one<'\n'>>,
    PEGTL_NS::one<'\r', '\n'>> {};
struct NewLine : PEGTL_NS::seq<
    EolWhitespace, 
    Crlf> {};
struct NewLines : PEGTL_NS::plus<NewLine> {};

// array type
struct ArrayType : PEGTL_NS::if_must<
    LeftBrace,
    PEGTL_NS::opt<TokenSpace>,
    RightBrace> {};

// separators
struct ListSeparator : PEGTL_NS::seq<PEGTL_NS::one<','>,
    PEGTL_NS::opt<NewLines>> {};
struct ListEnd : PEGTL_NS::sor<
    ListSeparator,
    PEGTL_NS::opt<NewLines>> {};
struct StatementSeparator : PEGTL_NS::sor<
    PEGTL_NS::seq<PEGTL_NS::one<';'>, PEGTL_NS::opt<NewLines>>,
    NewLines> {};
struct StatementEnd : PEGTL_NS::sor<
    StatementSeparator,
    PEGTL_NS::opt<NewLines>> {};
struct NamespaceSeparator : PEGTL_NS::one<':'> {};
struct CXXNamespaceSeparator : PEGTL_NS::seq<NamespaceSeparator,
    NamespaceSeparator> {};
struct Assignment : PEGTL_NS::seq<PEGTL_NS::opt<TokenSpace>,
    Equals, PEGTL_NS::opt<TokenSpace>> {};

// numbers
struct ExponentPart : PEGTL_NS::opt_must<
    Exponent,
    PEGTL_NS::opt<PEGTL_NS::one<'+', '-'>>,
    PEGTL_NS::plus<Digit>> {};
struct NumberStandard : PEGTL_NS::seq<
    PEGTL_NS::opt<Minus>, PEGTL_NS::plus<Digit>,
    PEGTL_NS::opt_must<Sdf_PathParser::Dot, PEGTL_NS::plus<Digit>>,
    ExponentPart> {};
struct NumberLeadingDot : PEGTL_NS::seq<
    PEGTL_NS::opt<Minus>,
    PEGTL_NS::if_must<Sdf_PathParser::Dot,
    PEGTL_NS::plus<Digit>>,
    ExponentPart> {};
struct Number : PEGTL_NS::sor<
    MathKeywordInf, 
    PEGTL_NS::seq<Minus, MathKeywordInf>,
    MathKeywordNan,
    NumberStandard,
    NumberLeadingDot> {};

// string supporting methods
struct Utf8SingleQuoteCharacter
{
    using rule_t = Utf8SingleQuoteCharacter;
    using analyze_t = PEGTL_NS::analysis::generic<
        PEGTL_NS::analysis::rule_type::any >;

    template <typename ParseInput>
    static bool match(ParseInput& in)
    {
        if (!in.empty())
        {
            // peek at the next character in the input
            auto utf8_char = PEGTL_NS::internal::peek_utf8::peek(in);
            if (utf8_char.size != 0)
            {
                // a quote can be consumed if it's preceeded by a '\'
                if (static_cast<uint32_t>(utf8_char.data) == 0x005Cu)
                {
                    // consume and check the next character
                    // unfortunately there isn't a replace, so
                    // we've consumed this character even if it
                    // ultimately wasn't a valid match
                    // this is ok, because without the closed quote
                    // it can't be any other valid production
                    in.bump(utf8_char.size);
                    auto utf8_char2 = PEGTL_NS::internal::peek_utf8::peek(in);
                    if (utf8_char2.size != 0)
                    {
                        // if it's a CR or LF, it's an error,
                        // everything else is ok
                        if ((static_cast<uint32_t>(utf8_char2.data) != 0x000Au) && 
                            (static_cast<uint32_t>(utf8_char2.data) != 0x000Du))
                        {
                            in.bump(utf8_char2.size);
                            return true;
                        }
                    }

                    return false;
                }

                // if it's a CR, LF, or ', don't consume
                if ((static_cast<uint32_t>(utf8_char.data) != 0x000Au) && 
                    (static_cast<uint32_t>(utf8_char.data) != 0x000Du) &&
                    (static_cast<uint32_t>(utf8_char.data) != 0x0027u))
                {
                    in.bump(utf8_char.size);
                    return true;
                }
            }
        }

        return false;
    }
};

struct Utf8DoubleQuoteCharacter
{
    using rule_t = Utf8DoubleQuoteCharacter;
    using analyze_t = PEGTL_NS::analysis::generic<
        PEGTL_NS::analysis::rule_type::any >;

    template <typename ParseInput>
    static bool match(ParseInput& in)
    {
        if (!in.empty())
        {
            // peek at the next character in the input
            auto utf8_char = PEGTL_NS::internal::peek_utf8::peek(in);
            if (utf8_char.size != 0)
            {
                // a double quote can be consumed if it's preceeded by a '\'
                if (static_cast<uint32_t>(utf8_char.data) == 0x005Cu)
                {
                    // consume and check the next character
                    // unfortunately there isn't a replace, so
                    // we've consumed this character even if it
                    // ultimately wasn't a valid match
                    // this is ok, because without the closed quote
                    // it can't be any other valid production
                    in.bump(utf8_char.size);
                    auto utf8_char2 = PEGTL_NS::internal::peek_utf8::peek(in);
                    if (utf8_char2.size != 0)
                    {
                        // if it's a CR or LF, it's an error,
                        // everything else is ok
                        if ((static_cast<uint32_t>(utf8_char2.data) != 0x000Au) &&
                            (static_cast<uint32_t>(utf8_char2.data) != 0x000Du))
                        {
                            in.bump(utf8_char2.size);
                            return true;
                        }
                    }

                    return false;
                }

                // if it's a CR, LF, or ", don't consume
                if ((static_cast<uint32_t>(utf8_char.data) != 0x000Au) && 
                    (static_cast<uint32_t>(utf8_char.data) != 0x000Du) &&
                    (static_cast<uint32_t>(utf8_char.data) != 0x0022u))
                {
                    in.bump(utf8_char.size);
                    return true;
                }
            }
        }

        return false;
    }
};

struct Utf8SingleQuoteMultilineCharacter
{
    using rule_t = Utf8SingleQuoteMultilineCharacter;
    using analyze_t = PEGTL_NS::analysis::generic<
        PEGTL_NS::analysis::rule_type::any >;

    template <typename ParseInput>
    static bool match(ParseInput& in)
    {
        if (!in.empty())
        {
            // peek at the next character in the input
            auto utf8_char = PEGTL_NS::internal::peek_utf8::peek(in);
            if (utf8_char.size != 0)
            {
                // a quote can be consumed if it's preceeded by a '\'
                if (static_cast<uint32_t>(utf8_char.data) == 0x005Cu)
                {
                    // consume and check the next character
                    // unfortunately there isn't a replace, so
                    // we've consumed this character even if it
                    // ultimately wasn't a valid match
                    // this is ok, because without the closed quote
                    // it can't be any other valid production
                    in.bump(utf8_char.size);
                    auto utf8_char2 = PEGTL_NS::internal::peek_utf8::peek(in);
                    if (utf8_char2.size != 0)
                    {
                        in.bump(utf8_char2.size);
                        return true;
                    }

                    return false;
                }

                // if it's an unescaped ', don't consume
                if (static_cast<uint32_t>(utf8_char.data) != 0x0027u)
                {
                    in.bump(utf8_char.size);
                    return true;
                }
            }
        }

        return false;
    }
};

struct Utf8DoubleQuoteMultilineCharacter
{
    using rule_t = Utf8DoubleQuoteMultilineCharacter;
    using analyze_t = PEGTL_NS::analysis::generic<
        PEGTL_NS::analysis::rule_type::any >;

    template <typename ParseInput>
    static bool match(ParseInput& in)
    {
        if (!in.empty())
        {
            // peek at the next character in the input
            auto utf8_char = PEGTL_NS::internal::peek_utf8::peek(in);
            if (utf8_char.size != 0)
            {
                // a double quote can be consumed if it's preceeded by a '\'
                if (static_cast<uint32_t>(utf8_char.data) == 0x005Cu)
                {
                    // consume and check the next character
                    // unfortunately there isn't a replace, so
                    // we've consumed this character even if it
                    // ultimately wasn't a valid match
                    // this is ok, because without the closed quote
                    // it can't be any other valid production
                    in.bump(utf8_char.size);
                    auto utf8_char2 = PEGTL_NS::internal::peek_utf8::peek(in);
                    if (utf8_char2.size != 0)
                    {
                        in.bump(utf8_char2.size);
                        return true;
                    }

                    return false;
                }

                // unescaped quotes are allowed in the string,
                // but only if they are a single double quote or a set of two
                // double quotes (3 would close the string)
                if (static_cast<uint32_t>(utf8_char.data) == 0x0022u)
                {
                    auto nextChar = in.peek_uint8(1);
                    auto nextNextChar = in.peek_uint8(2);
                    if (static_cast<uint32_t>(nextChar) == 0x0022u &&
                        static_cast<uint32_t>(nextNextChar) == 0x0022u)
                    {
                        // this would mark the end of the multi-line string
                        return false;
                    }
                    else
                    {
                        // find to consume the quote
                        in.bump(utf8_char.size);
                        return true;
                    }    
                }
                else
                {
                    // valid
                    in.bump(utf8_char.size);
                    return true;
                }
            }
        }

        return false;
    }
};

// strings
struct EmptyMultilineSingleQuoteString : PEGTL_NS::seq<
    SingleQuote, SingleQuote, SingleQuote,
    SingleQuote, SingleQuote, SingleQuote> {};
struct EmptyMultilineDoubleQuoteString : PEGTL_NS::seq<
    DoubleQuote, DoubleQuote, DoubleQuote,
    DoubleQuote, DoubleQuote, DoubleQuote> {};
struct MultilineSingleQuoteString : PEGTL_NS::if_must<
    PEGTL_NS::seq<SingleQuote, SingleQuote, SingleQuote>,
    PEGTL_NS::plus<Utf8SingleQuoteMultilineCharacter>,
    PEGTL_NS::seq<SingleQuote, SingleQuote, SingleQuote>> {};
struct MultilineDoubleQuoteString : PEGTL_NS::if_must<
    PEGTL_NS::seq<DoubleQuote, DoubleQuote, DoubleQuote>,
    PEGTL_NS::plus<Utf8DoubleQuoteMultilineCharacter>,
    PEGTL_NS::seq<DoubleQuote, DoubleQuote, DoubleQuote>> {};
struct EmptySingleQuoteString : PEGTL_NS::seq<SingleQuote, SingleQuote> {};
struct EmptyDoubleQuoteString : PEGTL_NS::seq<DoubleQuote, DoubleQuote> {};
struct SinglelineSingleQuoteString : PEGTL_NS::if_must<
    SingleQuote, PEGTL_NS::plus<Utf8SingleQuoteCharacter>, SingleQuote> {};
struct SinglelineDoubleQuoteString : PEGTL_NS::if_must<
    DoubleQuote, PEGTL_NS::plus<Utf8DoubleQuoteCharacter>, DoubleQuote> {};
struct SingleQuoteString : PEGTL_NS::sor<
    EmptyMultilineSingleQuoteString,
    MultilineSingleQuoteString,
    EmptySingleQuoteString,
    SinglelineSingleQuoteString> {};
struct DoubleQuoteString : PEGTL_NS::sor<
    EmptyMultilineDoubleQuoteString,
    MultilineDoubleQuoteString,
    EmptyDoubleQuoteString,
    SinglelineDoubleQuoteString> {};
struct String : PEGTL_NS::sor<
    SingleQuoteString,
    DoubleQuoteString> {};

// asset references
struct Utf8AssetPathCharacter
{
    using rule_t = Utf8AssetPathCharacter;
    using analyze_t = PEGTL_NS::analysis::generic<
        PEGTL_NS::analysis::rule_type::any >;

    template <typename ParseInput>
    static bool match(ParseInput& in)
    {
        while(!in.empty())
        {
            auto utf8_char = PEGTL_NS::internal::peek_utf8::peek(in);
            if (utf8_char.size != 0)
            {
                if ((static_cast<uint32_t>(utf8_char.data) == 0x000Au) ||
                    (static_cast<uint32_t>(utf8_char.data) == 0x000Du))
                {
                    // end of sequence
                    return false;
                }
                else if ((static_cast<uint32_t>(utf8_char.data) == 0x0040u))
                {
                    // this is the '@' signaling the end of the sequence
                    // we consumed what we can, don't consume this character
                    return true;
                }
                else
                {
                    // consume and keep going
                    in.bump(utf8_char.size);
                }
            }
            else
            {
                return false;
            }

        }

        return false;
    }
};

struct Utf8AssetPathEscapedCharacter
{
    using rule_t = Utf8AssetPathEscapedCharacter;
    using analyze_t = PEGTL_NS::analysis::generic<
        PEGTL_NS::analysis::rule_type::any >;

    template <typename ParseInput>
    static bool match(ParseInput& in)
    {
        while(!in.empty())
        {
            auto utf8_char = PEGTL_NS::internal::peek_utf8::peek(in);
            if (utf8_char.size != 0)
            {
                if ((static_cast<uint32_t>(utf8_char.data) == 0x000Au) ||
                    (static_cast<uint32_t>(utf8_char.data) == 0x000Du))
                {
                    // end of sequence
                    return false;
                }
                else if(static_cast<uint32_t>(utf8_char.data) == 0x0040u)
                {
                    // if we are not currently processing an escape, this
                    // could either be a consumeable character or the end
                    // of the stream - we need to look ahead to the next
                    // input (without consuming this one yet) to see if
                    // we need to potentially process the end sequence
                    auto nextChar = in.peek_uint8(1);
                    if (static_cast<uint32_t>(nextChar) == 0x0040u)
                    {
                        // the next one was a '@' as well, look for the last
                        nextChar = in.peek_uint8(2);
                        if (static_cast<uint32_t>(nextChar) == 0x0040u)
                        {
                            // that's it, we are done, signal success
                            // and don't consume any of the '@' chars
                            return true;
                        }
                        else
                        {
                            // we got 2 '@', but not a third
                            // so we consider that part of our
                            // asset string and eat the sequence
                            in.bump(2);
                        }
                    }
                    else
                    {
                        // the next byte wasn't another '@', so
                        // eat the first '@' and move on
                        in.bump(utf8_char.size);
                    }
                }
                else if (static_cast<uint32_t>(utf8_char.data) == 0x005Cu)
                {
                    // this is an escape sequence
                    // if we aren't escaping a '@@@' sequence, just consume
                    // and move on
                    auto nextChar = in.peek_uint8(1);
                    if (static_cast<uint32_t>(nextChar) != 0x0040u)
                    {
                        // consume the '\'
                        in.bump(utf8_char.size);
                    }
                    else
                    {
                        // it's a '@', keep going to see if we can
                        // eat the whole sequence
                        nextChar = in.peek_uint8(2);
                        if (static_cast<uint32_t>(nextChar) == 0x0040u)
                        {
                            nextChar = in.peek_uint8(3);
                            if (static_cast<uint32_t>(nextChar) == 0x0040u)
                            {
                                // that's the end of the escaped '@@@'
                                // eat the whole sequence (4 bytes)
                                in.bump(4);
                            }
                            else
                            {
                                // we had a \@@ sequence, but not a fully
                                // escaped one - nevertheless we can eat
                                // the whole thing
                                in.bump(3);
                            }
                        }
                        else
                        {
                            // the next char was not a '@', meaning
                            // we had a `\@' sequence, which is fine
                            // we can eat both of those for efficiency
                            in.bump(2);
                        }
                    }
                }
                else
                {
                    // consume and keep going
                    in.bump(utf8_char.size);
                }
            }
            else
            {
                return false;
            }

        }

        return false;
    }
};

struct AssetRef : PEGTL_NS::sor<
    PEGTL_NS::if_must<
        PEGTL_NS::seq<At, At, At>, 
        Utf8AssetPathEscapedCharacter, 
        PEGTL_NS::seq<At, At, At>>,
    PEGTL_NS::if_must<
        At,
        Utf8AssetPathCharacter,
        At>> {};

// path reference
struct PathRef : PEGTL_NS::if_must<
    LeftAngleBracket,
    PEGTL_NS::sor<
        RightAngleBracket,
        PEGTL_NS::seq<
            Sdf_PathParser::Path,
            RightAngleBracket>>> {};

// grammar rule that currently matches ASCII identifiers
// but that can be more easily changed in the future for UTF-8
struct BaseIdentifier : PEGTL_NS::identifier {};
struct KeywordlessIdentifier : PEGTL_NS::seq<
    PEGTL_NS::not_at<Keywords>,
    BaseIdentifier> {};
struct CXXNamespacedIdentifier : PEGTL_NS::seq<
    KeywordlessIdentifier,
    PEGTL_NS::plus<CXXNamespaceSeparator, KeywordlessIdentifier>> {};
struct NamespacedIdentifier : PEGTL_NS::seq<
    BaseIdentifier,
    PEGTL_NS::plus<NamespaceSeparator, BaseIdentifier>> {};
struct Identifier : PEGTL_NS::sor<
    CXXNamespacedIdentifier,
    KeywordlessIdentifier> {};
struct NamespacedName : PEGTL_NS::sor<
    NamespacedIdentifier, 
    BaseIdentifier,
    Keywords> {};

// atomic values
struct NumberValue : Number {};
struct IdentifierValue : Identifier {};
struct StringValue : String {};
struct AssetRefValue : AssetRef {};
struct AtomicValue : PEGTL_NS::sor<
    NumberValue,
    IdentifierValue,
    StringValue,
    AssetRefValue> {};

struct TupleValue;
struct ListValue;
struct EmptyListValue;
struct PathRefValue : PathRef {};
struct TypedValue : PEGTL_NS::sor<
    AtomicValue,
    TupleValue,
    EmptyListValue,
    ListValue,
    PathRefValue> {};

// tuple values
struct TupleValueOpen : LeftParen {};
struct TupleValueClose : RightParen {};
struct TupleValueItem : PEGTL_NS::sor<
    AtomicValue,
    TupleValue> {};
struct TupleValueItems : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>, 
        TupleValueItem,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct TupleValueInterior : PEGTL_NS::seq<
    PEGTL_NS::opt<NewLines>,
    TupleValueItems,
    PEGTL_NS::opt<TokenSpace>,
    ListEnd> {};
struct TupleValue : PEGTL_NS::if_must<
    TupleValueOpen,
    TupleValueInterior,
    PEGTL_NS::opt<TokenSpace>,
    TupleValueClose> {};

// list values
struct ListValueOpen : LeftBrace {};
struct ListValueClose : RightBrace {};
struct ListValueItem : PEGTL_NS::sor<
    AtomicValue,
    ListValue,
    TupleValue> {};
struct ListValueItems : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        ListValueItem,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct ListValueInterior : PEGTL_NS::seq<
    PEGTL_NS::opt<NewLines>,
    ListValueItems,
    PEGTL_NS::opt<TokenSpace>,
    ListEnd> {};
struct ListValue : PEGTL_NS::if_must<
    ListValueOpen,
    ListValueInterior,
    PEGTL_NS::opt<TokenSpace>,
    ListValueClose> {};

// empty list value uses LeftBrace / RightBrace
// rather than ListValueOpen / ListValueClose
// because it doesn't want to execute the
// action semantics on reduction
struct EmptyListValue : PEGTL_NS::seq<
    LeftBrace,
    PEGTL_NS::opt<TokenSpace>,
    RightBrace> {};

// dictionary values
struct DictionaryValue;
struct DictionaryValueOpen : LeftCurlyBrace {};
struct DictionaryValueClose : RightCurlyBrace {};
struct DictionaryKey : PEGTL_NS::sor<
    String,
    Identifier,
    Keywords> {};
struct DictionaryValueScalarType : Identifier {};
struct DictionaryValueShapedType : PEGTL_NS::seq<
    Identifier,
    PEGTL_NS::opt<TokenSpace>,
    ArrayType> {};
struct DictionaryValueType : PEGTL_NS::sor<
    DictionaryValueShapedType,
    DictionaryValueScalarType> {};
struct DictionaryElementTypedValueAssignment : PEGTL_NS::must<
    DictionaryKey,
    Assignment,
    TypedValue> {};
struct DictionaryElementDictionaryValueAssignment : PEGTL_NS::must<
    DictionaryKey,
    Assignment,
    DictionaryValue> {};
struct DictionaryElementTypedValue : PEGTL_NS::seq<
    DictionaryValueType,
    TokenSpace,
    DictionaryElementTypedValueAssignment> {};
struct DictionaryElementDictionaryValue : PEGTL_NS::if_must<
    KeywordDictionary,
    TokenSpace,
    DictionaryElementDictionaryValueAssignment> {};
struct DictionaryValueElement : PEGTL_NS::sor<
    DictionaryElementDictionaryValue,
    DictionaryElementTypedValue> {}; 
struct DictionaryValueItems : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        DictionaryValueElement,
        PEGTL_NS::opt<TokenSpace>>,
    StatementSeparator> {};
struct DictionaryValueInterior : PEGTL_NS::seq<
    DictionaryValueItems,
    StatementEnd> {};
struct DictionaryValue : PEGTL_NS::if_must<
    DictionaryValueOpen,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<DictionaryValueInterior>,
    PEGTL_NS::opt<TokenSpace>,
    DictionaryValueClose> {};

// metadata
struct MetadataValue : PEGTL_NS::sor<
    KeywordNone,
    DictionaryValue,
    TypedValue> {};

// time samples
struct ExtendedNumber : PEGTL_NS::sor<
    Number,
    Identifier> {};
struct TimeSampleExtendedNumber : ExtendedNumber {};
struct TimeSampleExtendedNumberSequence : PEGTL_NS::seq<
    TimeSampleExtendedNumber,
    PEGTL_NS::opt<TokenSpace>, 
    NamespaceSeparator> {};
struct TimeSampleExtendedNumberNone : KeywordNone {};
struct TimeSampleExtendedNumberValue : PEGTL_NS::seq<TypedValue> {};
struct TimeSample: PEGTL_NS::seq<
    TimeSampleExtendedNumberSequence,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::sor<
        TimeSampleExtendedNumberNone,
        TimeSampleExtendedNumberValue>> {};
struct TimeSamplesListInterior : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        TimeSample,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct TimeSamplesList : PEGTL_NS::seq<TimeSamplesListInterior, ListEnd> {};
struct TimeSamplesBegin : LeftCurlyBrace {};
struct TimeSamplesEnd : RightCurlyBrace {};
struct TimeSamplesValue : PEGTL_NS::if_must<
    TimeSamplesBegin,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<TimeSamplesList>,
    PEGTL_NS::opt<TokenSpace>,
    TimeSamplesEnd> {};

// list ops
struct MetadataListOpList : PEGTL_NS::sor<
    KeywordNone,
    ListValue> {};

// generic metadata shared between attributes and relationships
struct MetadataKey : PEGTL_NS::sor<
    KeywordCustomData,
    KeywordSymmetryArguments,
    Identifier> {};
struct MetadataKeyMetadata : PEGTL_NS::seq<
    MetadataKey,
    Assignment,
    MetadataValue> {};
struct DocString : String {};
struct DocMetadata : PEGTL_NS::if_must<
    KeywordDoc,
    Assignment,
    DocString> {};
struct PermissionIdentifier : Identifier {};
struct PermissionMetadata : PEGTL_NS::if_must<
    KeywordPermission,
    Assignment,
    PermissionIdentifier> {};
struct SymmetryFunctionIdentifier : Identifier {};
struct SymmetryFunctionEmpty : PEGTL_NS::seq<
    KeywordSymmetryFunction,
    Assignment> {};
struct SymmetryFunctionMetadata : PEGTL_NS::sor<
    PEGTL_NS::seq<
        KeywordSymmetryFunction, 
        Assignment,
        SymmetryFunctionIdentifier>,
    SymmetryFunctionEmpty> {};
struct NameListItem : String {};
struct NameListInterior : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        NameListItem,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct NameListBegin : LeftBrace {};
struct NameListEnd : RightBrace {};
struct NameList : PEGTL_NS::sor<
    NameListItem,
    PEGTL_NS::if_must<
        NameListBegin,
        PEGTL_NS::opt<NewLines>,
        NameListInterior,
        ListEnd,
        PEGTL_NS::opt<TokenSpace>,
        NameListEnd>> {};

// prim attributes
struct PrimAttributeMetadataListOpAddIdentifier : Identifier {};
struct PrimAttributeMetadataListOpDeleteIdentifier : Identifier {};
struct PrimAttributeMetadataListOpAppendIdentifier : Identifier {};
struct PrimAttributeMetadataListOpPrependIdentifier : Identifier {};
struct PrimAttributeMetadataListOpReorderIdentifier : Identifier {};
struct PrimAttributeMetadataListOpList : MetadataListOpList {};
struct PrimAttributeMetadataListOpAdd : PEGTL_NS::seq<
    KeywordAdd,
    TokenSpace,
    PrimAttributeMetadataListOpAddIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimAttributeMetadataListOpList>> {};
struct PrimAttributeMetadataListOpDelete : PEGTL_NS::seq<
    KeywordDelete,
    TokenSpace,
    PrimAttributeMetadataListOpDeleteIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimAttributeMetadataListOpList>> {};
struct PrimAttributeMetadataListOpAppend : PEGTL_NS::seq<
    KeywordAppend,
    TokenSpace,
    PrimAttributeMetadataListOpAppendIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimAttributeMetadataListOpList>> {};
struct PrimAttributeMetadataListOpPrepend : PEGTL_NS::seq<
    KeywordPrepend,
    TokenSpace,
    PrimAttributeMetadataListOpPrependIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimAttributeMetadataListOpList>> {};
struct PrimAttributeMetadataListOpReorder : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    PrimAttributeMetadataListOpReorderIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimAttributeMetadataListOpList>> {};
struct PrimAttributeListOpMetadata : PEGTL_NS::sor<
    PrimAttributeMetadataListOpAdd,
    PrimAttributeMetadataListOpDelete,
    PrimAttributeMetadataListOpAppend,
    PrimAttributeMetadataListOpPrepend,
    PrimAttributeMetadataListOpReorder> {};
struct PrimAttributeMetadataKey : Identifier {};
struct PrimAttributeMetadataValue : MetadataValue {};
struct PrimAttributeMetadataKeyMetadata : PEGTL_NS::seq<
    PrimAttributeMetadataKey,
    PEGTL_NS::must<
        Assignment,
        PrimAttributeMetadataValue>> {};
struct PrimAttributeMetadataString : String {};
struct PrimAttributeMetadataDisplayUnitIdentifier : Identifier {};
struct PrimAttributeDisplayUnitMetadata : PEGTL_NS::if_must<
    KeywordDisplayUnit,
    Assignment,
    PrimAttributeMetadataDisplayUnitIdentifier> {};
struct PrimAttributeMetadataItem : PEGTL_NS::sor<
    PrimAttributeMetadataString,
    PrimAttributeMetadataKeyMetadata,
    PrimAttributeListOpMetadata,
    DocMetadata,
    PermissionMetadata,
    SymmetryFunctionMetadata,
    PrimAttributeDisplayUnitMetadata> {};
struct PrimAttributeMetadataListInterior : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        PrimAttributeMetadataItem,
        PEGTL_NS::opt<TokenSpace>>,
    StatementSeparator> {};
struct PrimAttributeMetadataList : PEGTL_NS::if_must<
    LeftParen,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::sor<
        RightParen,
        PEGTL_NS::seq<
            PrimAttributeMetadataListInterior,
            StatementEnd,
            PEGTL_NS::opt<TokenSpace>,
            RightParen>>> {};
struct PrimAttributeVariability : PEGTL_NS::sor<
    KeywordUniform,
    KeywordConfig> {};
struct PrimAttributeStandardType : Identifier {};
struct PrimAttributeArrayType : PEGTL_NS::seq<
    Identifier,
    PEGTL_NS::opt<TokenSpace>,
    ArrayType> {};
struct PrimAttributeType : PEGTL_NS::sor<
    PrimAttributeArrayType,
    PrimAttributeStandardType> {};
struct PrimAttributeQualifiedTypeName : PrimAttributeType {};
struct PrimAttributeQualifiedType : PEGTL_NS::seq<
    PrimAttributeVariability,
    TokenSpace,
    PrimAttributeQualifiedTypeName> {};
struct PrimAttributeFullType : PEGTL_NS::sor<
    PrimAttributeQualifiedType,
    PrimAttributeType> {};
struct PrimAttributeValue : PEGTL_NS::sor<
    KeywordNone,
    TypedValue> {};
struct PrimAttributeAssignment : PEGTL_NS::if_must<
    Assignment,
    PrimAttributeValue> {};
struct PrimAttributeDefaultNamespacedName : NamespacedName {};
struct PrimAttributeAssignmentOptional : PEGTL_NS::opt<PrimAttributeAssignment> {};
struct PrimAttributeDefaultTypeName : PEGTL_NS::seq<
    PrimAttributeFullType,
    TokenSpace,
    PrimAttributeDefaultNamespacedName> {};
struct PrimAttributeDefault : PEGTL_NS::seq<
    PrimAttributeDefaultTypeName,
    PrimAttributeAssignmentOptional,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::opt<PrimAttributeMetadataList>> {};
struct PrimAttributeFallbackNamespacedName : NamespacedName {};
struct PrimAttributeFallbackTypeName : PEGTL_NS::seq<
    KeywordCustom,
    TokenSpace,
    PrimAttributeFullType,
    TokenSpace,
    PrimAttributeFallbackNamespacedName> {};
struct PrimAttributeFallback : PEGTL_NS::seq<
    PrimAttributeFallbackTypeName,
    PrimAttributeAssignmentOptional,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::opt<PrimAttributeMetadataList>> {};
struct PrimAttributeConnectName : PEGTL_NS::seq<
    NamespacedName,
    PEGTL_NS::opt<TokenSpace>,
    Sdf_PathParser::Dot,
    PEGTL_NS::opt<TokenSpace>,
    KeywordConnect> {};
struct PrimAttributeConnectItem : PathRef {};
struct PrimAttributeConnectList : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        PrimAttributeConnectItem,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct PrimAttributeConnectRhs : PEGTL_NS::sor<
    KeywordNone,
    PrimAttributeConnectItem,
    PEGTL_NS::if_must<
    LeftBrace,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::opt<NewLines>, 
    PEGTL_NS::sor<
        RightBrace, 
        PEGTL_NS::seq<
            PrimAttributeConnectList,
            ListEnd,
            PEGTL_NS::opt<TokenSpace>,
            RightBrace>>>> {};
struct PrimAttributeConnectValue : PrimAttributeConnectRhs {};
struct PrimAttributeAddConnectValue : PrimAttributeConnectRhs {};
struct PrimAttributeDeleteConnectValue : PrimAttributeConnectRhs {};
struct PrimAttributeAppendConnectValue : PrimAttributeConnectRhs {};
struct PrimAttributePrependConnectValue : PrimAttributeConnectRhs {};
struct PrimAttributeReorderConnectValue : PrimAttributeConnectRhs {};
struct PrimAttributeAddConnectAssignment : PEGTL_NS::seq<
    KeywordAdd,
    TokenSpace,
    PrimAttributeFullType,
    TokenSpace,
    PrimAttributeConnectName,
    Assignment> {};
struct PrimAttributeDeleteConnectAssignment : PEGTL_NS::seq<
    KeywordDelete,
    TokenSpace,
    PrimAttributeFullType,
    TokenSpace,
    PrimAttributeConnectName,
    Assignment> {};
struct PrimAttributeAppendConnectAssignment : PEGTL_NS::seq<
    KeywordAppend,
    TokenSpace,
    PrimAttributeFullType,
    TokenSpace,
    PrimAttributeConnectName,
    Assignment> {};
struct PrimAttributePrependConnectAssignment : PEGTL_NS::seq<
    KeywordPrepend,
    TokenSpace,
    PrimAttributeFullType,
    TokenSpace,
    PrimAttributeConnectName,
    Assignment> {};
struct PrimAttributeReorderConnectAssignment : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    PrimAttributeFullType,
    TokenSpace,
    PrimAttributeConnectName,
    Assignment> {};
struct PrimAttributeAddConnectStatement : PEGTL_NS::seq<
    PrimAttributeAddConnectAssignment,
    PEGTL_NS::must<PrimAttributeAddConnectValue>> {};
struct PrimAttributeDeleteConnectStatement : PEGTL_NS::seq<
    PrimAttributeDeleteConnectAssignment,
    PEGTL_NS::must<PrimAttributeDeleteConnectValue>> {};
struct PrimAttributeAppendConnectStatement : PEGTL_NS::seq<
    PrimAttributeAppendConnectAssignment,
    PEGTL_NS::must<PrimAttributeAppendConnectValue>> {};
struct PrimAttributePrependConnectStatement : PEGTL_NS::seq<
    PrimAttributePrependConnectAssignment,
    PEGTL_NS::must<PrimAttributePrependConnectValue>> {};
struct PrimAttributeReorderConnectStatement : PEGTL_NS::seq<
    PrimAttributeReorderConnectAssignment,
    PEGTL_NS::must<PrimAttributeReorderConnectValue>> {};
struct PrimAttributeListOpConnectStatement : PEGTL_NS::sor<
    PrimAttributeAddConnectStatement,
    PrimAttributeDeleteConnectStatement,
    PrimAttributeAppendConnectStatement,
    PrimAttributePrependConnectStatement,
    PrimAttributeReorderConnectStatement> {};
struct PrimAttributeConnectAssignment : PEGTL_NS::seq<
    PrimAttributeFullType,
    TokenSpace,
    PrimAttributeConnectName,
    Assignment> {};
struct PrimAttributeConnectStatement : PEGTL_NS::seq<
    PrimAttributeConnectAssignment,
    PEGTL_NS::must<PrimAttributeConnectValue>> {};
struct PrimAttributeTimeSamplesValue : TimeSamplesValue {};
struct PrimAttributeTimeSamplesName : PEGTL_NS::seq<
    NamespacedName,
    PEGTL_NS::opt<TokenSpace>,
    Sdf_PathParser::Dot,
    PEGTL_NS::opt<TokenSpace>,
    KeywordTimeSamples> {};
struct PrimAttributeTimeSamples : PEGTL_NS::seq<
    PrimAttributeFullType,
    TokenSpace,
    PrimAttributeTimeSamplesName,
    PEGTL_NS::must<
        Assignment,
        PrimAttributeTimeSamplesValue>> {};
struct PrimAttributeConnect : PEGTL_NS::sor<
    PrimAttributeConnectStatement,
    PrimAttributeListOpConnectStatement> {};
struct PrimAttribute : PEGTL_NS::sor<
    PrimAttributeFallback,
    PrimAttributeConnect,
    PrimAttributeTimeSamples,
    PrimAttributeDefault> {};

// prim relationships
struct PrimRelationshipMetadataListOpAddIdentifier : Identifier {};
struct PrimRelationshipMetadataListOpDeleteIdentifier : Identifier {};
struct PrimRelationshipMetadataListOpAppendIdentifier : Identifier {};
struct PrimRelationshipMetadataListOpPrependIdentifier : Identifier {};
struct PrimRelationshipMetadataListOpReorderIdentifier : Identifier {};
struct PrimRelationshipMetadataListOpList : MetadataListOpList {};
struct PrimRelationshipMetadataListOpAdd : PEGTL_NS::seq<
    KeywordAdd,
    TokenSpace,
    PrimRelationshipMetadataListOpAddIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimRelationshipMetadataListOpList>> {};
struct PrimRelationshipMetadataListOpDelete : PEGTL_NS::seq<
    KeywordDelete,
    TokenSpace,
    PrimRelationshipMetadataListOpDeleteIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimRelationshipMetadataListOpList>> {};
struct PrimRelationshipMetadataListOpAppend : PEGTL_NS::seq<
    KeywordAppend,
    TokenSpace,
    PrimRelationshipMetadataListOpAppendIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimRelationshipMetadataListOpList>> {};
struct PrimRelationshipMetadataListOpPrepend : PEGTL_NS::seq<
    KeywordPrepend,
    TokenSpace,
    PrimRelationshipMetadataListOpPrependIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimRelationshipMetadataListOpList>> {};
struct PrimRelationshipMetadataListOpReorder : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    PrimRelationshipMetadataListOpReorderIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimRelationshipMetadataListOpList>> {};
struct PrimRelationshipListOpMetadata : PEGTL_NS::sor<
    PrimRelationshipMetadataListOpAdd,
    PrimRelationshipMetadataListOpDelete,
    PrimRelationshipMetadataListOpAppend,
    PrimRelationshipMetadataListOpPrepend,
    PrimRelationshipMetadataListOpReorder> {};
struct PrimRelationshipMetadataKey : Identifier {};
struct PrimRelationshipMetadataValue : MetadataValue {};
struct PrimRelationshipMetadataKeyMetadata : PEGTL_NS::seq<
    PrimRelationshipMetadataKey,
    PEGTL_NS::must<
        Assignment,
        PrimRelationshipMetadataValue>> {};
struct PrimRelationshipMetadataString : String {};
struct PrimRelationshipMetadataItem : PEGTL_NS::sor<
    PrimRelationshipMetadataString,
    PrimRelationshipMetadataKeyMetadata,
    PrimRelationshipListOpMetadata,
    DocMetadata,
    PermissionMetadata,
    SymmetryFunctionMetadata> {};
struct PrimRelationshipMetadataListInterior : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        PrimRelationshipMetadataItem,
        PEGTL_NS::opt<TokenSpace>>,
    StatementSeparator> {};
struct PrimRelationshipMetadataList : PEGTL_NS::if_must<
    LeftParen,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::sor<
        RightParen, 
        PEGTL_NS::seq<
            PrimRelationshipMetadataListInterior,
            StatementEnd,
            PEGTL_NS::opt<TokenSpace>,
            RightParen>>> {};
struct PrimRelationshipName : NamespacedName {};
struct PrimRelationshipTimesamplesName : PEGTL_NS::seq<
    NamespacedName,
    PEGTL_NS::opt<TokenSpace>,
    Sdf_PathParser::Dot,
    PEGTL_NS::opt<TokenSpace>,
    KeywordTimeSamples> {};
struct PrimRelationshipDefaultName : PEGTL_NS::seq<
    NamespacedName,
    PEGTL_NS::opt<TokenSpace>,
    Sdf_PathParser::Dot,
    PEGTL_NS::opt<TokenSpace>,
    KeywordDefault> {};
struct PrimRelationshipTypeUniform : KeywordRel {};
struct PrimRelationshipTypeCustomUniform : PEGTL_NS::seq<
    KeywordCustom,
    TokenSpace,
    KeywordRel> {};
struct PrimRelationshipTypeCustomVarying : PEGTL_NS::seq<
    KeywordCustom,
    TokenSpace,
    KeywordVarying,
    TokenSpace,
    KeywordRel> {};
struct PrimRelationshipTypeVarying : PEGTL_NS::seq<
    KeywordVarying,
    TokenSpace,
    KeywordRel> {};
struct PrimRelationshipType: PEGTL_NS::sor<
    PrimRelationshipTypeUniform,
    PrimRelationshipTypeCustomUniform,
    PrimRelationshipTypeCustomVarying,
    PrimRelationshipTypeVarying> {};
struct PrimRelationshipTimeSamplesValue : TimeSamplesValue {};
struct PrimRelationshipTimeSamples : PEGTL_NS::seq<
    PrimRelationshipType,
    TokenSpace,
    PrimRelationshipTimesamplesName,
    PEGTL_NS::must<
        Assignment,
        PrimRelationshipTimeSamplesValue>> {};
struct PrimRelationshipDefault : PEGTL_NS::seq<
    PrimRelationshipType,
    TokenSpace,
    PrimRelationshipDefaultName,
    PEGTL_NS::must<
        Assignment,
        PathRef>> {};
struct PrimRelationshipTarget : PathRef {};
struct PrimRelationshipDefaultRef : PathRef {};
struct PrimRelationshipTargetList : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        PrimRelationshipTarget,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct PrimRelationshipTargetNone : PEGTL_NS::sor<
    KeywordNone, 
    PEGTL_NS::seq<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<TokenSpace>,
        RightBrace>> {}; 
struct PrimRelationshipAssignment : PEGTL_NS::if_must<
    Assignment,
    PEGTL_NS::sor<
        PrimRelationshipTarget,
        PrimRelationshipTargetNone,
        PEGTL_NS::seq<
            LeftBrace,
            PEGTL_NS::opt<NewLines>,
            PEGTL_NS::opt<PEGTL_NS::seq<
                PrimRelationshipTargetList, 
                ListEnd>>, 
            PEGTL_NS::opt<TokenSpace>,
            RightBrace>>> {};
struct PrimRelationshipStandardTypeName : PEGTL_NS::seq<
    PrimRelationshipType,
    TokenSpace,
    PrimRelationshipName> {};
struct PrimRelationshipListOpContent : PEGTL_NS::seq<
    PrimRelationshipType,
    TokenSpace,
    PrimRelationshipName,
    PEGTL_NS::opt<PrimRelationshipAssignment>> {};
struct PrimRelationshipAddListOp : PEGTL_NS::seq<
    KeywordAdd,
    TokenSpace,
    PrimRelationshipListOpContent> {};
struct PrimRelationshipDeleteListOp : PEGTL_NS::seq<
    KeywordDelete,
    TokenSpace,
    PrimRelationshipListOpContent> {};
struct PrimRelationshipPrependListOp : PEGTL_NS::seq<
    KeywordPrepend,
    TokenSpace,
    PrimRelationshipListOpContent> {};
struct PrimRelationshipAppendListOp : PEGTL_NS::seq<
    KeywordAppend,
    TokenSpace,
    PrimRelationshipListOpContent> {};
struct PrimRelationshipReorderListOp : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    PrimRelationshipListOpContent> {};
struct PrimRelationshipListOp : PEGTL_NS::sor<
    PrimRelationshipAddListOp,
    PrimRelationshipDeleteListOp,
    PrimRelationshipPrependListOp,
    PrimRelationshipAppendListOp,
    PrimRelationshipReorderListOp> {};
struct PrimRelationshipStandard : PEGTL_NS::seq<
    PrimRelationshipStandardTypeName,
    PEGTL_NS::opt<PrimRelationshipAssignment>,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::opt<PrimRelationshipMetadataList>> {};
struct PrimRelationshipList : PEGTL_NS::seq<
    PrimRelationshipType,
    TokenSpace,
    PrimRelationshipName,
    PEGTL_NS::opt<TokenSpace>,
    LeftBrace,
    PEGTL_NS::opt<TokenSpace>,
    PrimRelationshipTarget,
    PEGTL_NS::opt<TokenSpace>,
    RightBrace> {};
struct PrimRelationshipTypeStatements : PEGTL_NS::sor<
    PrimRelationshipStandard,
    PrimRelationshipList> {};
struct PrimRelationship : PEGTL_NS::sor<
    PrimRelationshipListOp,
    PrimRelationshipTimeSamples,
    PrimRelationshipDefault,
    PrimRelationshipTypeStatements> {};

// layer reference and offset
struct LayerRef : AssetRef {};
struct LayerRefOffsetValue : Number {};
struct LayerRefScaleValue : Number {};
struct LayerOffsetStatement : PEGTL_NS::sor<
    PEGTL_NS::if_must<KeywordOffset, Assignment, LayerRefOffsetValue>,
    PEGTL_NS::if_must<KeywordScale, Assignment, LayerRefScaleValue>> {};

// string dictionary
struct StringDictionaryElementKey : String {};
struct StringDictionaryElementValue : String {};
struct StringDictionaryElement : PEGTL_NS::seq<
    StringDictionaryElementKey,
    PEGTL_NS::opt<TokenSpace>,
    NamespaceSeparator,
    PEGTL_NS::opt<TokenSpace>,
    StringDictionaryElementValue> {};
struct StringDictionaryList : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        StringDictionaryElement,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct StringDictionary : PEGTL_NS::seq<
    DictionaryValueOpen,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<StringDictionaryList>,
    PEGTL_NS::opt<TokenSpace>,
    DictionaryValueClose> {};

// prim metadata
struct KindValue : String {};
struct KindMetadata : PEGTL_NS::seq<
    KeywordKind,
    PEGTL_NS::must<
        Assignment,
        KindValue>> {};
struct PayloadParameter : LayerOffsetStatement {};
struct PayloadParametersInterior : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        PayloadParameter,
        PEGTL_NS::opt<TokenSpace>>,
    StatementSeparator> {};
struct PayloadParameters : PEGTL_NS::seq<LeftParen,
    PEGTL_NS::sor<
        PEGTL_NS::seq<
            PEGTL_NS::opt<NewLines>,
            PEGTL_NS::opt<TokenSpace>,
            PayloadParametersInterior,
            PEGTL_NS::opt<TokenSpace>,
            StatementEnd>,
        PEGTL_NS::opt<NewLines>>,
    PEGTL_NS::opt<TokenSpace>, RightParen> {};
struct PayloadPrimPath : PathRef {};
struct OptionalPayloadPrimPath : PEGTL_NS::opt<PayloadPrimPath> {};
struct PayloadPathRef : PathRef {};
struct PayloadPathRefItem : PEGTL_NS::seq<
    PayloadPathRef,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::opt<PayloadParameters>> {};
struct PayloadLayerRefItem : PEGTL_NS::seq<
    LayerRef,
    PEGTL_NS::opt<TokenSpace>,
    OptionalPayloadPrimPath,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::opt<PayloadParameters>> {};
struct PayloadListItem : PEGTL_NS::sor<
    PayloadLayerRefItem,
    PayloadPathRefItem> {};
struct PayloadListInterior : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        PayloadListItem,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct PayloadList : PEGTL_NS::sor<
    KeywordNone,
    PayloadListItem,
    PEGTL_NS::seq<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<TokenSpace>,
        RightBrace>,
    PEGTL_NS::if_must<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        PayloadListInterior,
        ListEnd,
        PEGTL_NS::opt<TokenSpace>,
        RightBrace>> {}; 
struct PayloadMetadataKeyword : KeywordPayload {};
struct PayloadListOpAdd : PEGTL_NS::seq<
    KeywordAdd,
    TokenSpace,
    PayloadMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        PayloadList>> {};
struct PayloadListOpDelete : PEGTL_NS::seq<
    KeywordDelete,
    TokenSpace,
    PayloadMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        PayloadList>> {};
struct PayloadListOpAppend : PEGTL_NS::seq<
    KeywordAppend,
    TokenSpace,
    PayloadMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        PayloadList>> {};
struct PayloadListOpPrepend : PEGTL_NS::seq<
    KeywordPrepend,
    TokenSpace,
    PayloadMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        PayloadList>> {};
struct PayloadListOpReorder : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    PayloadMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        PayloadList>> {};
struct PayloadListOp : PEGTL_NS::seq<
    PayloadMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        PayloadList>> {};
struct PayloadMetadata : PEGTL_NS::sor<
    PayloadListOpAdd,
    PayloadListOpDelete,
    PayloadListOpAppend,
    PayloadListOpPrepend,
    PayloadListOpReorder,
    PayloadListOp> {};
struct InheritListItem : PathRef {};
struct InheritListInterior: PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        InheritListItem,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct InheritList : PEGTL_NS::sor<
    KeywordNone, 
    InheritListItem,
    PEGTL_NS::seq<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<TokenSpace>,
        RightBrace>,
    PEGTL_NS::if_must<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        InheritListInterior,
        ListEnd,
        PEGTL_NS::opt<TokenSpace>,
        RightBrace>> {}; 
struct InheritsMetadataKeyword : KeywordInherits {};
struct InheritsListOpAdd : PEGTL_NS::seq<
    KeywordAdd,
    TokenSpace,
    InheritsMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        InheritList>> {};
struct InheritsListOpDelete : PEGTL_NS::seq<
    KeywordDelete,
    TokenSpace,
    InheritsMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        InheritList>> {};
struct InheritsListOpAppend : PEGTL_NS::seq<
    KeywordAppend,
    TokenSpace,
    InheritsMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        InheritList>> {};
struct InheritsListOpPrepend : PEGTL_NS::seq<
    KeywordPrepend,
    TokenSpace,
    InheritsMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        InheritList>> {};
struct InheritsListOpReorder : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    InheritsMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        InheritList>> {};
struct InheritsListOp : PEGTL_NS::seq<
    InheritsMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        InheritList>> {};
struct InheritsMetadata : PEGTL_NS::sor<
    InheritsListOpAdd,
    InheritsListOpDelete,
    InheritsListOpAppend,
    InheritsListOpPrepend,
    InheritsListOpReorder,
    InheritsListOp> {};
struct SpecializesListItem : PathRef {};
struct SpecializesListInterior: PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        SpecializesListItem,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct SpecializesList : PEGTL_NS::sor<
    KeywordNone,
    SpecializesListItem,
    PEGTL_NS::seq<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<TokenSpace>,
        RightBrace>,
    PEGTL_NS::if_must<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        SpecializesListInterior,
        ListEnd,
        PEGTL_NS::opt<TokenSpace>,
        RightBrace>> {};
struct SpecializesMetadataKeyword : KeywordSpecializes {};
struct SpecializesListOpAdd : PEGTL_NS::seq<
    KeywordAdd,
    TokenSpace,
    SpecializesMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        SpecializesList>> {};
struct SpecializesListOpDelete : PEGTL_NS::seq<
    KeywordDelete,
    TokenSpace,
    SpecializesMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        SpecializesList>> {};
struct SpecializesListOpAppend : PEGTL_NS::seq<
    KeywordAppend,
    TokenSpace,
    SpecializesMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        SpecializesList>> {};
struct SpecializesListOpPrepend : PEGTL_NS::seq<
    KeywordPrepend,
    TokenSpace,
    SpecializesMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        SpecializesList>> {};
struct SpecializesListOpReorder : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    SpecializesMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        SpecializesList>> {};
struct SpecializesListOp : PEGTL_NS::seq<
    SpecializesMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        SpecializesList>> {};
struct SpecializesMetadata : PEGTL_NS::sor<
    SpecializesListOpAdd,
    SpecializesListOpDelete,
    SpecializesListOpAppend,
    SpecializesListOpPrepend,
    SpecializesListOpReorder,
    SpecializesListOp> {};
struct ReferenceParameter : PEGTL_NS::sor<
    PEGTL_NS::seq<KeywordCustomData, Assignment, DictionaryValue>,
    LayerOffsetStatement> {};
struct ReferenceParametersInterior : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        ReferenceParameter,
        PEGTL_NS::opt<TokenSpace>>,
    StatementSeparator> {};
struct ReferenceParameters : PEGTL_NS::seq<LeftParen,
    PEGTL_NS::sor<
        PEGTL_NS::seq<
            PEGTL_NS::opt<NewLines>,
            PEGTL_NS::opt<TokenSpace>,
            ReferenceParametersInterior,
            PEGTL_NS::opt<TokenSpace>, StatementEnd>,
        PEGTL_NS::opt<NewLines>>,
    PEGTL_NS::opt<TokenSpace>, RightParen> {};
struct ReferencePrimPath : PathRef {};
struct OptionalReferencePrimPath : PEGTL_NS::opt<ReferencePrimPath> {};
struct ReferencePathRef : PathRef {};
struct ReferencePathRefItem : PEGTL_NS::seq<
    ReferencePathRef,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::opt<ReferenceParameters>> {};
struct ReferenceLayerRefItem : PEGTL_NS::seq<
    LayerRef,
    PEGTL_NS::opt<TokenSpace>,
    OptionalReferencePrimPath,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::opt<ReferenceParameters>> {};
struct ReferenceListItem : PEGTL_NS::sor<
    ReferenceLayerRefItem,
    ReferencePathRefItem> {};
struct ReferenceListInterior : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        ReferenceListItem,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct ReferenceList : PEGTL_NS::sor<
    KeywordNone,
    ReferenceListItem,
    PEGTL_NS::seq<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<TokenSpace>,
        RightBrace>,
    PEGTL_NS::if_must<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        ReferenceListInterior,
        ListEnd,
        PEGTL_NS::opt<TokenSpace>,
        RightBrace>> {};
struct ReferencesMetadataKeyword : KeywordReferences {};
struct ReferencesListOpAdd : PEGTL_NS::seq<
    KeywordAdd,
    TokenSpace,
    ReferencesMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        ReferenceList>> {};
struct ReferencesListOpDelete : PEGTL_NS::seq<
    KeywordDelete,
    TokenSpace,
    ReferencesMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        ReferenceList>> {};
struct ReferencesListOpAppend : PEGTL_NS::seq<
    KeywordAppend,
    TokenSpace,
    ReferencesMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        ReferenceList>> {};
struct ReferencesListOpPrepend : PEGTL_NS::seq<
    KeywordPrepend,
    TokenSpace,
    ReferencesMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        ReferenceList>> {};
struct ReferencesListOpReorder : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    ReferencesMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        ReferenceList>> {};
struct ReferencesListOp : PEGTL_NS::seq<
    ReferencesMetadataKeyword,
    PEGTL_NS::must<
        Assignment,
        ReferenceList>> {};
struct ReferencesMetadata : PEGTL_NS::sor<
    ReferencesListOpAdd,
    ReferencesListOpDelete,
    ReferencesListOpAppend,
    ReferencesListOpPrepend,
    ReferencesListOpReorder,
    ReferencesListOp> {};
struct RelocatesStatement : PEGTL_NS::seq<
    PathRef,
    PEGTL_NS::must<
        PEGTL_NS::opt<TokenSpace>,
        NamespaceSeparator,
        PEGTL_NS::opt<TokenSpace>,
        PathRef>> {};
struct RelocatesStatementList : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        RelocatesStatement,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct RelocatesMap : PEGTL_NS::seq<
    LeftCurlyBrace,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::sor<
        PEGTL_NS::seq<
            PEGTL_NS::opt<RelocatesStatementList>,
            PEGTL_NS::opt<TokenSpace>,
            ListEnd>,
        PEGTL_NS::opt<NewLines>>,
    PEGTL_NS::opt<TokenSpace>,
    RightCurlyBrace> {};
struct RelocatesMetadata : PEGTL_NS::seq<
    KeywordRelocates,
    PEGTL_NS::must<
        Assignment,
        RelocatesMap>> {};
struct VariantsMetadata : PEGTL_NS::seq<
    KeywordVariants,
    PEGTL_NS::must<
        Assignment,
        DictionaryValue>> {};
struct VariantSetsListOpAdd : PEGTL_NS::seq<
    KeywordAdd,
    TokenSpace,
    KeywordVariantSets,
    PEGTL_NS::must<
        Assignment,
        NameList>> {};
struct VariantSetsListOpDelete : PEGTL_NS::seq<
    KeywordDelete,
    TokenSpace,
    KeywordVariantSets,
    PEGTL_NS::must<
        Assignment,
        NameList>> {};
struct VariantSetsListOpAppend : PEGTL_NS::seq<
    KeywordAppend,
    TokenSpace,
    KeywordVariantSets,
    PEGTL_NS::must<
        Assignment,
        NameList>> {};
struct VariantSetsListOpPrepend : PEGTL_NS::seq<
    KeywordPrepend,
    TokenSpace,
    KeywordVariantSets,
    PEGTL_NS::must<
        Assignment,
        NameList>> {};
struct VariantSetsListOpReorder : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    KeywordVariantSets,
    PEGTL_NS::must<
        Assignment,
        NameList>> {};
struct VariantSetsListOp : PEGTL_NS::seq<
    KeywordVariantSets,
    PEGTL_NS::must<
        Assignment,
        NameList>> {};
struct VariantSetsMetadata : PEGTL_NS::sor<
    VariantSetsListOpAdd,
    VariantSetsListOpDelete,
    VariantSetsListOpAppend,
    VariantSetsListOpPrepend,
    VariantSetsListOpReorder,
    VariantSetsListOp> {};
struct PrefixSubstitutionsMetadata : PEGTL_NS::seq<
    KeywordPrefixSubstitutions,
    PEGTL_NS::must<
        Assignment,
        StringDictionary>> {};
struct SuffixSubstitutionsMetadata : PEGTL_NS::seq<
    KeywordSuffixSubstitutions,
    PEGTL_NS::must<
        Assignment,
        StringDictionary>> {};
struct PrimMetadataString : String {};
struct PrimMetadataKey : Identifier {};
struct PrimMetadataValue : MetadataValue {};
struct PrimMetadataListOpAddIdentifier : Identifier {};
struct PrimMetadataListOpDeleteIdentifier : Identifier {};
struct PrimMetadataListOpAppendIdentifier : Identifier {};
struct PrimMetadataListOpPrependIdentifier : Identifier {};
struct PrimMetadataListOpReorderIdentifier : Identifier {};
struct PrimMetadataListOpList : MetadataListOpList {};
struct PrimMetadataListOpAdd : PEGTL_NS::seq<
    KeywordAdd,
    TokenSpace,
    PrimMetadataListOpAddIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimMetadataListOpList>> {};
struct PrimMetadataListOpDelete : PEGTL_NS::seq<
    KeywordDelete,
    TokenSpace,
    PrimMetadataListOpDeleteIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimMetadataListOpList>> {};
struct PrimMetadataListOpAppend : PEGTL_NS::seq<
    KeywordAppend,
    TokenSpace,
    PrimMetadataListOpAppendIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimMetadataListOpList>> {};
struct PrimMetadataListOpPrepend : PEGTL_NS::seq<
    KeywordPrepend,
    TokenSpace,
    PrimMetadataListOpPrependIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimMetadataListOpList>> {};
struct PrimMetadataListOpReorder : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    PrimMetadataListOpReorderIdentifier,
    PEGTL_NS::must<
        Assignment,
        PrimMetadataListOpList>> {};
struct PrimMetadataListOpMetadata : PEGTL_NS::sor<
    PrimMetadataListOpAdd,
    PrimMetadataListOpDelete,
    PrimMetadataListOpAppend,
    PrimMetadataListOpPrepend,
    PrimMetadataListOpReorder> {};
struct PrimMetadataKeyMetadata : PEGTL_NS::seq<
    PrimMetadataKey,
    PEGTL_NS::must<
        Assignment,
        PrimMetadataValue>> {};
struct PrimMetadataItem : PEGTL_NS::sor<
    PrimMetadataString,
    PrimMetadataKeyMetadata,
    PrimMetadataListOpMetadata,
    DocMetadata,
    KindMetadata,
    PermissionMetadata,
    PayloadMetadata,
    InheritsMetadata,
    SpecializesMetadata,
    ReferencesMetadata,
    RelocatesMetadata,
    VariantsMetadata,
    VariantSetsMetadata,
    SymmetryFunctionMetadata,
    PrefixSubstitutionsMetadata,
    SuffixSubstitutionsMetadata> {};
struct PrimMetadataList : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        PrimMetadataItem,
        PEGTL_NS::opt<TokenSpace>>,
    StatementSeparator> {};
struct PrimMetadataInterior : PEGTL_NS::sor<
    PEGTL_NS::seq<
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<TokenSpace>,
        PrimMetadataList,
        StatementEnd>,
    PEGTL_NS::opt<NewLines>> {}; 
struct PrimMetadata : PEGTL_NS::sor<
    PEGTL_NS::seq<
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<TokenSpace>,
        LeftParen,
        PrimMetadataInterior,
        PEGTL_NS::opt<TokenSpace>,
        RightParen,
        PEGTL_NS::opt<NewLines>>,
    PEGTL_NS::opt<NewLines>> {};

// prims
// (VariantName and VariantSetName conflict with rule structures
// in the included path parser so we use PrimVariantName and 
// PrimVariantSetName instead)
struct PrimStatement;
struct PrimContentsList;
struct PrimIdentifier : String {};
struct PrimProperty : PEGTL_NS::sor<
    PrimAttribute,
    PrimRelationship> {};
struct PrimVariantName : String {};
struct VariantStatement : PEGTL_NS::seq<
    PrimVariantName,
    PEGTL_NS::must<
        PEGTL_NS::opt<TokenSpace>,
        PrimMetadata,
        PEGTL_NS::opt<TokenSpace>,
        LeftCurlyBrace,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<TokenSpace>,
        PEGTL_NS::opt<PrimContentsList>,
        PEGTL_NS::opt<TokenSpace>,
        RightCurlyBrace,
        PEGTL_NS::opt<NewLines>>> {};
struct VariantList : PEGTL_NS::plus<PEGTL_NS::seq<
    PEGTL_NS::opt<TokenSpace>,
    VariantStatement,
    PEGTL_NS::opt<TokenSpace>>> {};
struct PrimVariantSetName : String {};
struct VariantSetStatement : PEGTL_NS::seq<
    KeywordVariantSet,
    PEGTL_NS::must<
        TokenSpace,
        PrimVariantSetName,
        Assignment,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<TokenSpace>,
        LeftCurlyBrace,
        PEGTL_NS::opt<NewLines>,
        VariantList,
        PEGTL_NS::opt<TokenSpace>,
        RightCurlyBrace>> {};
struct PrimChildOrderStatement : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    KeywordNameChildren,
    PEGTL_NS::must<
        Assignment,
        NameList>> {};
struct PrimPropertyOrderStatement : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    KeywordProperties,
    PEGTL_NS::must<
        Assignment,
        NameList>> {};
struct PrimContentsListItem : PEGTL_NS::sor<
    PEGTL_NS::seq<
        PrimChildOrderStatement,
        PEGTL_NS::opt<TokenSpace>,
        StatementSeparator>,
    PEGTL_NS::seq<
        PrimPropertyOrderStatement,
        PEGTL_NS::opt<TokenSpace>,
        StatementSeparator>,
    PEGTL_NS::seq<
        PrimStatement,
        PEGTL_NS::opt<TokenSpace>,
        NewLines>,
    PEGTL_NS::seq<
        VariantSetStatement,
        PEGTL_NS::opt<TokenSpace>,
        NewLines>,
    PEGTL_NS::seq<
        PrimProperty,
        PEGTL_NS::opt<TokenSpace>,
        StatementSeparator>> {};
struct PrimContentsList : PEGTL_NS::plus<
    PEGTL_NS::opt<TokenSpace>,
    PrimContentsListItem> {};
struct PrimContentsListOp : PEGTL_NS::sor<
    PEGTL_NS::seq<PEGTL_NS::opt<NewLines>, PrimContentsList>,
    PEGTL_NS::opt<NewLines>> {};
struct PrimStatementInterior : PEGTL_NS::seq<
    PrimIdentifier,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::opt<PrimMetadata>,
    PEGTL_NS::opt<TokenSpace>,
    LeftCurlyBrace,
    PEGTL_NS::must<
        PEGTL_NS::opt<TokenSpace>,
        PrimContentsListOp,
        PEGTL_NS::opt<TokenSpace>,
        RightCurlyBrace>> {};
struct PrimTypeName : PEGTL_NS::list<Identifier, PEGTL_NS::seq<
    PEGTL_NS::opt<TokenSpace>,
    Sdf_PathParser::Dot,
    PEGTL_NS::opt<TokenSpace>>> {};
struct PrimDefSpecifier : KeywordDef {};
struct PrimClassSpecifier : KeywordClass {};
struct PrimOverSpecifier : KeywordOver {};
struct PrimReorderNameList : NameList {};
struct PrimDefinition : PEGTL_NS::seq<
    PrimDefSpecifier,
    PEGTL_NS::must<
        TokenSpace,
        PEGTL_NS::sor<
            PrimStatementInterior, 
            PEGTL_NS::seq<
                PrimTypeName,
                TokenSpace,
                PrimStatementInterior>>>> {};
struct PrimClass : PEGTL_NS::seq<
    PrimClassSpecifier,
    PEGTL_NS::must<
        TokenSpace,
        PEGTL_NS::sor<
            PrimStatementInterior, 
            PEGTL_NS::seq<
                PrimTypeName,
                TokenSpace,
                PrimStatementInterior>>>> {};
struct PrimOver : PEGTL_NS::seq<
    PrimOverSpecifier,
    PEGTL_NS::must<
        TokenSpace,
        PEGTL_NS::sor<
            PrimStatementInterior,
            PEGTL_NS::seq<
                PrimTypeName,
                TokenSpace,
                PrimStatementInterior>>>> {};
struct PrimReorder : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    KeywordRootPrims,
    PEGTL_NS::must<
        Assignment,
        PrimReorderNameList>> {};
struct PrimStatement : PEGTL_NS::sor<
    PrimDefinition, 
    PrimClass,
    PrimOver,
    PrimReorder> {};

// layer metadata
struct LayerMetadataListOpAddIdentifier : Identifier {};
struct LayerMetadataListOpDeleteIdentifier : Identifier {};
struct LayerMetadataListOpAppendIdentifier : Identifier {};
struct LayerMetadataListOpPrependIdentifier : Identifier {};
struct LayerMetadataListOpReorderIdentifier : Identifier {};
struct LayerMetadataListOpList : MetadataListOpList {};
struct LayerMetadataKey : Identifier {};
struct LayerMetadataValue : MetadataValue {};
struct LayerMetadataListOpAdd : PEGTL_NS::seq<
    KeywordAdd,
    TokenSpace,
    LayerMetadataListOpAddIdentifier,
    PEGTL_NS::must<
        Assignment,
        LayerMetadataListOpList>> {};
struct LayerMetadataListOpDelete : PEGTL_NS::seq<
    KeywordDelete,
    TokenSpace,
    LayerMetadataListOpDeleteIdentifier,
    PEGTL_NS::must<
        Assignment,
        LayerMetadataListOpList>> {};
struct LayerMetadataListOpAppend : PEGTL_NS::seq<
    KeywordAppend,
    TokenSpace,
    LayerMetadataListOpAppendIdentifier,
    PEGTL_NS::must<
        Assignment,
        LayerMetadataListOpList>> {};
struct LayerMetadataListOpPrepend : PEGTL_NS::seq<
    KeywordPrepend,
    TokenSpace,
    LayerMetadataListOpPrependIdentifier,
    PEGTL_NS::must<
        Assignment,
        LayerMetadataListOpList>> {};
struct LayerMetadataListOpReorder : PEGTL_NS::seq<
    KeywordReorder,
    TokenSpace,
    LayerMetadataListOpReorderIdentifier,
    PEGTL_NS::must<
        Assignment,
        LayerMetadataListOpList>> {};
struct LayerMetadataListOpMetadata : PEGTL_NS::sor<
    LayerMetadataListOpAdd,
    LayerMetadataListOpDelete,
    LayerMetadataListOpAppend,
    LayerMetadataListOpPrepend,
    LayerMetadataListOpReorder> {};
struct LayerMetadataKeyMetadata : PEGTL_NS::seq<
    LayerMetadataKey,
    PEGTL_NS::must<
        Assignment,
        LayerMetadataValue>> {};
struct LayerOffsetList : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        LayerOffsetStatement,
        PEGTL_NS::opt<TokenSpace>>,
    StatementSeparator> {};
struct LayerOffset : PEGTL_NS::if_must<
    LeftParen,
    LayerOffsetList,
    PEGTL_NS::opt<TokenSpace>,
    StatementEnd,
    RightParen> {};
struct SublayerStatement : PEGTL_NS::seq<
    LayerRef,
    PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::opt<LayerOffset>> {};
struct SublayerListInterior : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        SublayerStatement,
        PEGTL_NS::opt<TokenSpace>>,
    ListSeparator> {};
struct SublayerList : PEGTL_NS::seq<LeftBrace, PEGTL_NS::opt<TokenSpace>,
    PEGTL_NS::sor<
        PEGTL_NS::seq<PEGTL_NS::opt<NewLines>, SublayerListInterior, ListEnd>,
        PEGTL_NS::opt<NewLines>>,
    PEGTL_NS::opt<TokenSpace>, RightBrace> {}; 
struct SublayersMetadata : PEGTL_NS::seq<
    KeywordSubLayers,
    PEGTL_NS::must<
        Assignment,
        SublayerList>> {};
struct LayerMetadataString : String {};
struct LayerMetadataItem : PEGTL_NS::sor<
    LayerMetadataString,
    LayerMetadataKeyMetadata,
    LayerMetadataListOpMetadata,
    DocMetadata,
    SublayersMetadata> {};
struct LayerMetadataListInterior : PEGTL_NS::list<PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSpace>,
        LayerMetadataItem,
        PEGTL_NS::opt<TokenSpace>>,
    StatementSeparator> {};
struct LayerMetadataList : PEGTL_NS::sor<
    PEGTL_NS::seq<
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<TokenSpace>,
        LayerMetadataListInterior,
        PEGTL_NS::opt<TokenSpace>,
        StatementEnd>,
    PEGTL_NS::opt<NewLines>> {};
struct LayerMetadata : PEGTL_NS::sor<
    PEGTL_NS::seq<
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<TokenSpace>,
        PEGTL_NS::if_must<
            LeftParen,
            LayerMetadataList,
            PEGTL_NS::opt<TokenSpace>,
            RightParen,
            PEGTL_NS::opt<NewLines>>>,
    PEGTL_NS::opt<NewLines>> {};

// layers
struct PrimList : PEGTL_NS::list<PEGTL_NS::seq<
    PEGTL_NS::opt<TokenSpace>,
    PrimStatement,
    PEGTL_NS::opt<TokenSpace>>,
    NewLines> {};
struct LayerContent : PEGTL_NS::sor<
    PEGTL_NS::seq<
        PEGTL_NS::opt<LayerMetadata>,
        PEGTL_NS::opt<TokenSpace>,
        PrimList,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<EolWhitespace>>,
    PEGTL_NS::opt<LayerMetadata>> {};
struct LayerHeader : PEGTL_NS::sor<
    PEGTL_NS::seq<
        PEGTL_NS::one<'#'>,
        PEGTL_NS::until<NewLine>>, 
    PEGTL_NS::seq<
        PEGTL_NS::one<'#'>,
        PEGTL_NS::until<PEGTL_NS::eof>>> {};
struct Layer : PEGTL_NS::sor<
    PEGTL_NS::seq<LayerHeader, LayerContent>,
    LayerHeader> {};
struct LayerMetadataOnly : PEGTL_NS::sor<
    PEGTL_NS::seq<LayerHeader, PEGTL_NS::opt<LayerMetadata>>,
    LayerHeader> {};

////////////////////////////////////////////////////////////////////////
// TextFileFormat actions

template <class Rule>
inline std::string _GetUnnamespacedType()
{
    std::string rule = PEGTL_NS::internal::demangle<Rule>();
    if (TfStringEndsWith(rule, ">"))
    {
        // filters out PEGTL specific rules like seq, star, etc.
        return "";
    }

    // otherwise we have the full struct type here, we only want
    // the unnamespaced parts
    size_t namespaceIndex = rule.rfind("::");
    if (namespaceIndex != std::string::npos)
    {
        // we want the substring after that
        return rule.substr(namespaceIndex + 2) + "\n";
    }

    // unable to match ::
    return "";
}

template <class Rule>
struct TextParserAction : PEGTL_NS::nothing<Rule> {};

////////////////////////////////////////////////////////////////////////
// TextFileFormat custom control

template <typename Controller, template<typename...> class Base = PEGTL_NS::normal>
struct TextParserDefaultErrorControl
{
    template <typename Rule>
    struct control : Base<Rule>
    {
        template <typename Input, typename... States>
        static void success(const Input& in, States&&... st) noexcept
        {
            if constexpr(Controller::template emit<Rule>)
            {
                TF_DEBUG(SDF_TEXT_FILE_FORMAT_RULES).Msg(_GetUnnamespacedType<Rule>());
            }

            Base<Rule>::success(in, st...);
        }

        template <typename Input, typename... States>
        [[noreturn]]
        static void raise(const Input& in, [[maybe_unused]] States&&... st)
        {
            if constexpr(Controller::template message<Rule> != nullptr)
            {
                // use custom error message for rule
                constexpr const char* errorMessage = 
                    Controller::template message<Rule>;
                throw PEGTL_NS::parse_error(errorMessage, in);
            }
            else
            {
                // emit default parse error for the rule
                Base<Rule>::raise(in, st...);
            }
        }
    };
};

// default error message is nullptr, which redirects parse error
// message to default control class raise method
template <typename> inline constexpr const char* errorMessage = nullptr;

// default emit rule is true, this enables debugging for which
// rules successfully matched - some rules may turn this off
// to avoid over emission of e.g. whitespace matching
template <typename> inline constexpr bool emitRule = true;

// TextParserDefaultControl doesn't take the error_message as a template
// parameter directly, so it's wrapped here
struct TextParserControlValues
{
    template<typename Rule>
    static constexpr auto message = errorMessage<Rule>;

    template<typename Rule>
    static constexpr auto emit = emitRule<Rule>;
};

template <typename Rule>
using TextParserControl = 
    TextParserDefaultErrorControl<TextParserControlValues>::control<Rule>;

} // end namespace Sdf_TextFileFormatParser

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_TEXT_FILE_FORMAT_PARSER_H