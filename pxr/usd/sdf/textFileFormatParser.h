//
// Copyright 2024 Pixar
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
#include "pxr/usd/sdf/path.h"

// transitively including pxr/base/tf/pxrPEGTL/pegtl.h
#include "pxr/usd/sdf/pathParser.h"
#include "pxr/usd/sdf/parserHelpers.h"
#include "pxr/usd/sdf/parserValueContext.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/textParserContext.h"

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
struct HexDigit : PEGTL_NS::xdigit {};
struct OctDigit : PEGTL_NS::range<0, 7> {};
struct Utf8NoEolf : PEGTL_NS::minus<PEGTL_NS::utf8::any, PEGTL_NS::one<'\n', '\r'>> {};

// Escape character sets as defined by `TfEscapeStringReplaceChar` plus quotes
struct EscapeSingleCharacter :
    PEGTL_NS::seq<PEGTL_NS::one<'\\', 'a', 'b', 'f', 'n', 'r', 't', 'v', '\'', '"'>> {};
struct EscapeHex :
    PEGTL_NS::seq<PEGTL_NS::one<'x'>,
                  PEGTL_NS::rep_opt<2, HexDigit>> {};
struct EscapeOct :
    PEGTL_NS::seq<OctDigit,
                  PEGTL_NS::rep_opt<2, OctDigit>> {};
struct Escaped :
    PEGTL_NS::seq<PEGTL_NS::one<'\\'>,
                  PEGTL_NS::sor<EscapeSingleCharacter, EscapeHex, EscapeOct>> {};

// keyword
struct KeywordAdd : TAO_PEGTL_KEYWORD("add") {};
struct KeywordAppend : TAO_PEGTL_KEYWORD("append") {};
struct KeywordClass : TAO_PEGTL_KEYWORD("class") {};
struct KeywordConfig : TAO_PEGTL_KEYWORD("config") {};
struct KeywordConnect : TAO_PEGTL_KEYWORD("connect") {};
struct KeywordCustom : TAO_PEGTL_KEYWORD("custom") {};
struct KeywordCustomData : TAO_PEGTL_KEYWORD("customData") {};
struct KeywordDefault : TAO_PEGTL_KEYWORD("default") {};
struct KeywordDef : TAO_PEGTL_KEYWORD("def") {};
struct KeywordDelete : TAO_PEGTL_KEYWORD("delete") {};
struct KeywordDictionary : TAO_PEGTL_KEYWORD("dictionary") {};
struct KeywordDisplayUnit : TAO_PEGTL_KEYWORD("displayUnit") {};
struct KeywordDoc : TAO_PEGTL_KEYWORD("doc") {};
struct KeywordInherits : TAO_PEGTL_KEYWORD("inherits") {};
struct KeywordKind : TAO_PEGTL_KEYWORD("kind") {};
struct KeywordNameChildren : TAO_PEGTL_KEYWORD("nameChildren") {};
struct KeywordNone : TAO_PEGTL_KEYWORD("None") {};
struct KeywordOffset : TAO_PEGTL_KEYWORD("offset") {};
struct KeywordOver : TAO_PEGTL_KEYWORD("over") {};
struct KeywordPayload : TAO_PEGTL_KEYWORD("payload") {};
struct KeywordPermission : TAO_PEGTL_KEYWORD("permission") {};
struct KeywordPrefixSubstitutions : TAO_PEGTL_KEYWORD("prefixSubstitutions") {};
struct KeywordPrepend : TAO_PEGTL_KEYWORD("prepend") {};
struct KeywordProperties : TAO_PEGTL_KEYWORD("properties") {};
struct KeywordReferences : TAO_PEGTL_KEYWORD("references") {};
struct KeywordRelocates : TAO_PEGTL_KEYWORD("relocates") {};
struct KeywordRel : TAO_PEGTL_KEYWORD("rel") {};
struct KeywordReorder : TAO_PEGTL_KEYWORD("reorder") {};
struct KeywordRootPrims : TAO_PEGTL_KEYWORD("rootPrims") {};
struct KeywordScale : TAO_PEGTL_KEYWORD("scale") {};
struct KeywordSubLayers : TAO_PEGTL_KEYWORD("subLayers") {};
struct KeywordSuffixSubstitutions : TAO_PEGTL_KEYWORD("suffixSubstitutions") {};
struct KeywordSpecializes : TAO_PEGTL_KEYWORD("specializes") {};
struct KeywordSymmetryArguments : TAO_PEGTL_KEYWORD("symmetryArguments") {};
struct KeywordSymmetryFunction : TAO_PEGTL_KEYWORD("symmetryFunction") {};
struct KeywordTimeSamples : TAO_PEGTL_KEYWORD("timeSamples") {};
struct KeywordUniform : TAO_PEGTL_KEYWORD("uniform") {};
struct KeywordVariantSet : TAO_PEGTL_KEYWORD("variantSet") {};
struct KeywordVariantSets : TAO_PEGTL_KEYWORD("variantSets") {};
struct KeywordVariants : TAO_PEGTL_KEYWORD("variants") {};
struct KeywordVarying : TAO_PEGTL_KEYWORD("varying") {};

struct Keywords : PEGTL_NS::sor<
KeywordAdd,
KeywordAppend,
KeywordClass,
KeywordConfig,
KeywordConnect,
KeywordCustom,
KeywordCustomData,
KeywordDefault,
KeywordDef,
KeywordDelete,
KeywordDictionary,
KeywordDisplayUnit,
KeywordDoc,
KeywordInherits,
KeywordKind,
KeywordNameChildren,
KeywordNone,
KeywordOffset,
KeywordOver,
KeywordPayload,
KeywordPermission,
KeywordPrefixSubstitutions,
KeywordPrepend,
KeywordProperties,
KeywordReferences,
KeywordRelocates,
KeywordRel,
KeywordReorder,
KeywordRootPrims,
KeywordScale,
KeywordSubLayers,
KeywordSuffixSubstitutions,
KeywordSpecializes,
KeywordSymmetryArguments,
KeywordSymmetryFunction,
KeywordTimeSamples,
KeywordUniform,
KeywordVariantSet,
KeywordVariantSets,
KeywordVariants,
KeywordVarying 
> {};

struct MathKeywordInf : TAO_PEGTL_KEYWORD("inf") {};
struct MathKeywordNan : TAO_PEGTL_KEYWORD("nan") {};

struct MathKeywords : PEGTL_NS::sor<
    MathKeywordInf,
    MathKeywordNan> {};

// PythonStyleComment = # (NonCrlfUtf8Character)*
// CppStyleSingleLineComment = // (NonCrlfUtf8Character)*
// CppStyleMultiLineComment = /* (!(*/) (Utf8Character)*) */
// Comment = PythonStyleComment /
//           CppStyleSingleLineComment /
//           CppStyleMultiLineComment
struct PythonStyleComment : PEGTL_NS::disable<
    PEGTL_NS::one<'#'>, PEGTL_NS::star<PEGTL_NS::not_at<PEGTL_NS::eolf>,
    Utf8NoEolf>> {};
struct CppStyleSingleLineComment : PEGTL_NS::disable<
    PEGTL_NS::two<'/'>, PEGTL_NS::star<PEGTL_NS::not_at<PEGTL_NS::eolf>,
    Utf8NoEolf>> {};
struct CppStyleMultiLineComment : PEGTL_NS::disable<
    PEGTL_NS::seq<PEGTL_NS::one<'/'>, PEGTL_NS::one<'*'>,
    PEGTL_NS::until<PEGTL_NS::seq<PEGTL_NS::one<'*'>,
    PEGTL_NS::one<'/'>>>>> {};
struct Comment : PEGTL_NS::sor<
    PythonStyleComment,
    CppStyleSingleLineComment,
    CppStyleMultiLineComment> {};

// whitespace rules
// TokenSeparator represents whitespace between tokens,
// which can include space, tab, and c++ multiline style comments
// but MUST include a single space / tab character, that is:
// def/*comment*/foo is illegal but
// def /*comment*/foo or
// def/*comment*/ foo are both legal
// TokenSeparator = (Space)+ (CppStyleMultiLineComment (Space)*)?)* /
//                  (CppStyleMultiLineComment (Space)*)?)* (Space)+
struct TokenSeparator : PEGTL_NS::sor<
    PEGTL_NS::seq<PEGTL_NS::plus<Space>,
        PEGTL_NS::opt<PEGTL_NS::list_tail<CppStyleMultiLineComment, Space>>>,
        PEGTL_NS::seq<PEGTL_NS::list<CppStyleMultiLineComment, Space>,
            PEGTL_NS::plus<Space>>> {};

// SpaceOrComment = Space /
//                  Comment
// EolWhitespace = (SpaceOrComment)*
// NewLine = EolWhitespace CrLf
// NewLines = (NewLine)+
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
    PEGTL_NS::opt<TokenSeparator>,
    RightBrace> {};

// separators
// ListSeparator = , (NewLines)?
// ListEnd = ListSeparator /
//           (NewLines)?
// StatementSeparator = ; (NewLines)? /
//                      NewLines
// StatementEnd = StatementSeparator /
//			      (NewLines)?
// Assignment = (TokenSeparator)? = (TokenSeparator)?
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
struct Assignment : PEGTL_NS::seq<PEGTL_NS::opt<TokenSeparator>,
    Equals, PEGTL_NS::opt<TokenSeparator>> {};

// generic lists
template <typename R>
struct ListOf : PEGTL_NS::seq<
    PEGTL_NS::list<
        PEGTL_NS::seq<
            PEGTL_NS::opt<TokenSeparator>,
            R,
            PEGTL_NS::opt<TokenSeparator>>,
        ListSeparator>,
    ListEnd> {};

// generic statements
template <typename R>
struct StatementSequenceOf : PEGTL_NS::seq<
    PEGTL_NS::list<
        PEGTL_NS::seq<
            PEGTL_NS::opt<TokenSeparator>,
            R,
            PEGTL_NS::opt<TokenSeparator>>,
        StatementSeparator>,
    StatementEnd> {};

// numbers
// Number = ((-)? ((Digit)+ / (Digit)+ . (Digit)+ / . (Digit)+) (ExponentPart)?) /
//          inf /
//		    -inf /
// 		    nan
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

// strings
// EscapedDoubleQuote = \"
// DoubleQuoteSingleLineStringChar = EscapedDoubleQuote / !" NonCrlfUtf8Character
// EscapedSingleQuote = \'
// SingleQuoteSingleLineStringChar = EscapedSingleQuote / !' NonCrlfUtf8Character
// DoubleQuoteMultiLineStringChar = EscapedDoubleQuote / !" Utf8Character
// SingleQuoteMultiLineStringChar = EscapedSingleQuote / !' Utf8Character
// String = "  DoubleQuoteSingleLineStringChar* " /
//	 """ DoubleQuoteMultiLineStringChar* """ /
//   ' SingleQuoteSingleLineStringChar* ' /
//	''' SingleQuoteMultiLineStringChar* '

struct MultilineContents : PEGTL_NS::sor<Escaped, PEGTL_NS::utf8::any> {};
struct ThreeSingleQuotes :
    PEGTL_NS::seq<SingleQuote, SingleQuote, SingleQuote> {};
struct ThreeDoubleQuotes :
    PEGTL_NS::seq<DoubleQuote, DoubleQuote, DoubleQuote> {};
struct MultilineSingleQuoteString : PEGTL_NS::if_must<
    ThreeSingleQuotes,
    PEGTL_NS::until<ThreeSingleQuotes, MultilineContents>> {};
struct MultilineDoubleQuoteString : PEGTL_NS::if_must<
    ThreeDoubleQuotes,
    PEGTL_NS::until<ThreeDoubleQuotes, MultilineContents>> {};
struct SinglelineContents : PEGTL_NS::sor<Escaped, Utf8NoEolf> {};
struct SinglelineSingleQuoteString : PEGTL_NS::if_must<
    SingleQuote,
    PEGTL_NS::until<SingleQuote, SinglelineContents>> {};
struct SinglelineDoubleQuoteString : PEGTL_NS::if_must<
    DoubleQuote,
    PEGTL_NS::until<DoubleQuote, SinglelineContents>> {};
struct SingleQuoteString : PEGTL_NS::sor<
    MultilineSingleQuoteString,
    SinglelineSingleQuoteString> {};
struct DoubleQuoteString : PEGTL_NS::sor<
    MultilineDoubleQuoteString,
    SinglelineDoubleQuoteString> {};
struct String : PEGTL_NS::sor<
    SingleQuoteString,
    DoubleQuoteString> {};

// // asset references
struct AtAtAt : PEGTL_NS::seq<At, At, At> {};
struct EscapeAtAtAt :
    PEGTL_NS::seq<PEGTL_NS::one<'\\'>, AtAtAt> {};

// AssetRefCharacter = !@ NonCrlfUtf8Character
// EscapedTripleAt = \@@@
// AssetRefTripleCharacter = EscapedTripleAt / !@ NonCrlfUtf8Character
// AssetRef = @ (AssetRefCharacter)* @ /
//            @@@ (AssetRefTripleCharacter)* (@ / @@)? @@@
struct AssetRef : PEGTL_NS::sor<
    PEGTL_NS::if_must<
        AtAtAt,
        // A triple quoted asset path is closed by 3-5 @, with the last three
        // always "closing" and the previous 0-2 being considered a part of the
        // asset path.
        PEGTL_NS::until<PEGTL_NS::seq<AtAtAt, PEGTL_NS::rep_opt<2, At>>,
                        PEGTL_NS::sor<EscapeAtAtAt, Utf8NoEolf>>>,
    PEGTL_NS::if_must<
        At,
        PEGTL_NS::until<At, Utf8NoEolf>>>{};

// path reference
// PathRef = <> /
//           < Path >
struct PathRef : PEGTL_NS::if_must<
    LeftAngleBracket,
    PEGTL_NS::sor<
        RightAngleBracket,
        PEGTL_NS::seq<
            Sdf_PathParser::Path,
            RightAngleBracket>>> {};

// LayerOffset = offset Assignment Number /
//	             scale Assignment Number
struct LayerOffset : PEGTL_NS::seq<
    PEGTL_NS::sor<
        KeywordOffset,
        KeywordScale>,
    Assignment,
    Number> {};

// grammar rule that matches UTF-8 identifiers
struct BaseIdentifier : Sdf_PathParser::Utf8Identifier {};
struct KeywordlessIdentifier : PEGTL_NS::minus<
    Sdf_PathParser::Utf8Identifier, Keywords> {};
struct NamespacedIdentifier : PEGTL_NS::seq<
    BaseIdentifier,
    PEGTL_NS::plus<NamespaceSeparator, BaseIdentifier>> {};

// CXXNamespacedIdentifier = KeywordlessIdentifier (:: KeywordlessIdentifier)+
// Identifier = CXXNamespacedIdentifier /
//              KeywordlessIdentifier
struct Identifier : PEGTL_NS::seq<
    KeywordlessIdentifier,
    PEGTL_NS::star<
        PEGTL_NS::seq<
            CXXNamespaceSeparator,
            KeywordlessIdentifier>>> {};
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

struct TypedTupleValue;
struct TypedListValue;
struct EmptyListValue;
struct PathRefValue : PathRef {};

// TypedValue = AtomicValue /
//              TupleValue /
//	            [ (TokenSpace)? ] /
//              ListValue /
//              PathRefValue
struct TypedValue : PEGTL_NS::sor<
    AtomicValue,
    TypedTupleValue,
    EmptyListValue,
    TypedListValue,
    PathRefValue> {};

// tuple values
// TupleItem = AtomicValue /
//		       TupleValue
// TupleInterior = TupleInterior ListSeparator (TokenSpace)? TupleItem (TokenSpace)? /
//                 (TokenSpace)? TupleItem (TokenSpace)?
// TupleValue = ( (NewLines)? TupleInterior ListEnd (TokenSpace)? )
struct TupleValue;
struct TupleValueOpen : LeftParen {};
struct TupleValueClose : RightParen {};
struct TupleValueItem : PEGTL_NS::sor<
    NumberValue,
    IdentifierValue,
    StringValue,
    AssetRefValue,
    TupleValue> {};
struct TupleValue : PEGTL_NS::if_must<
    TupleValueOpen,
    PEGTL_NS::opt<NewLines>,
    ListOf<TupleValueItem>,
    PEGTL_NS::opt<TokenSeparator>,
    TupleValueClose> {};
struct TypedTupleValue : TupleValue {};

// list values
// ListItem = AtomicValue /
//            ListValue /
//            TupleValue
// ListInterior = ListInterior ListSeparator (TokenSpace)? ListItem (TokenSpace)? / 
//                (TokenSpace)? ListItem (TokenSpace)?
// ListValue = [ (NewLines)? ListInterior ListEnd (TokenSpace)? ]
struct ListValue;
struct ListValueOpen : LeftBrace {};
struct ListValueClose : RightBrace {};
struct ListValueItem : PEGTL_NS::sor<
    NumberValue,
    IdentifierValue,
    StringValue,
    AssetRefValue,
    ListValue,
    TupleValue> {};
struct ListValue : PEGTL_NS::if_must<
    ListValueOpen,
    PEGTL_NS::opt<NewLines>,
    ListOf<ListValueItem>,
    PEGTL_NS::opt<TokenSeparator>,
    ListValueClose> {};
struct TypedListValue : ListValue {};

// empty list value uses LeftBrace / RightBrace
// rather than ListValueOpen / ListValueClose
// because it doesn't want to execute the
// action semantics on reduction
struct EmptyListValue : PEGTL_NS::seq<
    LeftBrace,
    PEGTL_NS::opt<TokenSeparator>,
    RightBrace> {};

// dictionary values
// DictionaryKey = String /
//			       Identifier /
//                 Keyword
// KeyValuePair = DictionaryKey Assignment TypedValue
// KeyDictionaryValuePair = DictionaryKey Assignment DictionaryValue
// DictionaryItemTypedValue = Identifier TokenSpace KeyValuePair /
//				              Identifier (TokenSpace)? [ (TokenSpace)? ] TokenSpace KeyValuePair
// DictionaryItemDictionaryValue = dictionary TokenSpace KeyDictionaryValuePair
// DictionaryItem = DictionaryItemDictionaryValue /
//                  DictionaryItemTypedValue
// DictionaryInterior = DictionaryInterior StaementSeparator (TokenSpace)? DictionaryItem (TokenSpace)? / 
//                      (TokenSpace)? DictionaryItem (TokenSpace)?
// DictionaryValue = { (NewLines)? DictionaryInterior StatementEnd (TokenSpace)? }
struct DictionaryValue;
struct DictionaryValueOpen : LeftCurlyBrace {};
struct DictionaryValueClose : RightCurlyBrace {};
struct DictionaryKey : PEGTL_NS::sor<
    String,
    Identifier,
    Keywords> {};
struct DictionaryType : PEGTL_NS::seq<
    Identifier,
    PEGTL_NS::opt<
        PEGTL_NS::seq<
            PEGTL_NS::opt<TokenSeparator>,
            ArrayType>>> {};
struct DictionaryValueItem : PEGTL_NS::sor<
    PEGTL_NS::if_must<
        KeywordDictionary,
        TokenSeparator,
        DictionaryKey,
        Assignment,
        DictionaryValue>,
    PEGTL_NS::seq<
        DictionaryType,
        TokenSeparator,
        DictionaryKey,
        Assignment,
        TypedValue>> {};
struct DictionaryValue : PEGTL_NS::if_must<
    DictionaryValueOpen,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<StatementSequenceOf<DictionaryValueItem>>,
    PEGTL_NS::opt<TokenSeparator>,
    DictionaryValueClose> {};

// shared metadata
// MetadataOpen = LeftParen
// MetadataClose = RightParen
struct MetadataOpen : LeftParen {};
struct MetadataClose : RightParen {};

// MetadataKey = customData /
//               symmetryArguments /
//               Identifier
// MetadataValue = None /
//			       DictionaryValue /
//			       TypedValue
// KeyValueMetadata = Identifier Assignment MetadataValue
struct MetadataKey : PEGTL_NS::sor<
    KeywordCustomData,
    KeywordSymmetryArguments,
    Identifier> {};
struct KeyValueMetadata : PEGTL_NS::seq<
    MetadataKey,
    Assignment,
    PEGTL_NS::sor<
        KeywordNone,
        DictionaryValue,
        TypedValue>> {};
struct LayerKeyValueMetadata : PEGTL_NS::seq<
    Identifier,
    Assignment,
    PEGTL_NS::sor<
        KeywordNone,
        DictionaryValue,
        TypedValue>> {};

// DocMetadata = doc Assignment String
struct DocMetadata : PEGTL_NS::seq<
    KeywordDoc,
    Assignment,
    String> {};

struct ListOpKeyword : PEGTL_NS::sor<
    KeywordAdd,
    KeywordDelete,
    KeywordAppend,
    KeywordPrepend,
    KeywordReorder> {};

struct NoneOrTypedListValue : PEGTL_NS::sor<
    KeywordNone,
    TypedListValue> {};
struct ListOpKeyValueMetadata : PEGTL_NS::seq<
    Identifier,
    Assignment,
    PEGTL_NS::must<NoneOrTypedListValue>> {};

// ListOpMetadataValue = None /
//                       ListValue
// GeneralListOpMetadata = add TokenSeparator Identifier Assignment ListOpMetadataValue /
//                         delete TokenSeparator Identifier Assignment ListOpMetadatValue /
//                         append TokenSeparator Identifier Assignment ListOpMetadatValue /
//                         prepend TokenSeparator Identifier Assignment ListOpMetadatValue /
//                         reorder TokenSeparator Identifier Assignment ListOpMetadatValue
struct GeneralListOpMetadata : PEGTL_NS::seq<
    ListOpKeyword,
    TokenSeparator,
    Identifier,
    Assignment,
    PEGTL_NS::sor<
        KeywordNone,
        TypedListValue>> {};

// SharedMetadata = String /
//                  KeyValueMetadata /
//                  DocMetadata
// note for layers it's slightly different because the key
// in key / value pair metadata can only be Identifier
struct SharedMetadata : PEGTL_NS::sor<
    String,
    KeyValueMetadata,
    DocMetadata> {};
struct SharedWithListOpMetadata : PEGTL_NS::sor<
    SharedMetadata,
    GeneralListOpMetadata> {};
struct LayerSharedWithListOpMetadata : PEGTL_NS::sor<
    String,
    LayerKeyValueMetadata,
    DocMetadata,
    GeneralListOpMetadata> {};

// PermissionMetadata = permission Assignment Identifier
struct PermissionMetadata : PEGTL_NS::seq<
    KeywordPermission,
    Assignment,
    Identifier> {};

// SymmetryFunctionMetadata = symmetryFunction Assignment (Identifier)?
struct SymmetryFunctionMetadata : PEGTL_NS::seq<
    KeywordSymmetryFunction,
    Assignment,
    PEGTL_NS::opt<Identifier>> {};

// NameList = String /
//            [ (NewLines)? (TokenSeparator)? String (TokenSeparator)? (ListSeparator (TokenSeparator)? String (TokenSeparator)?)* ListEnd (TokenSeparator)? ]
struct NameList : PEGTL_NS::sor<
    String,
    PEGTL_NS::if_must<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        ListOf<String>,
        PEGTL_NS::opt<TokenSeparator>,
        RightBrace>> {};

// StringDictionaryItem = String (TokenSeparator)? : (TokenSeparator)? String
struct StringDictionaryItem : PEGTL_NS::seq<
    String,
    PEGTL_NS::opt<TokenSeparator>,
    NamespaceSeparator,
    PEGTL_NS::opt<TokenSeparator>,
    String> {};

// StringDictionary = { (NewLines)? ( (TokenSeparator)? StringDictionaryItem (TokenSeparator)? (ListSeparator (TokenSeparator)? StringDictionaryItem (TokenSeparator)?)* ListEnd)? (TokenSeparator)? }
struct StringDictionaryOpen : LeftCurlyBrace {};
struct StringDictionaryClose : RightCurlyBrace {};
struct StringDictionary : PEGTL_NS::must<
    StringDictionaryOpen,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<TokenSeparator>,
    PEGTL_NS::opt<ListOf<StringDictionaryItem>>,
    PEGTL_NS::opt<TokenSeparator>,
    StringDictionaryClose> {};

// time samples
// TimeSample = Number (TokenSeparator)? : (TokenSeparator)? None /
//              Number (TokenSeparator)? : (TokenSeparator)? TypedValue
struct TimeSample : PEGTL_NS::seq<
    Number,
    PEGTL_NS::opt<TokenSeparator>,
    NamespaceSeparator,
    PEGTL_NS::opt<TokenSeparator>,
    PEGTL_NS::sor<
        KeywordNone,
        TypedValue>> {};

// TimeSampleMap = { (NewLines)? ((TokenSeparator)? TimeSample (TokenSeparator)? (ListSeparator (TokenSeparator)? TimeSample (TokenSeparator)?)* ListEnd)? (TokenSeparator)? }
struct TimeSampleMap : PEGTL_NS::seq<
    LeftCurlyBrace,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<ListOf<TimeSample>>,
    PEGTL_NS::opt<TokenSeparator>,
    RightCurlyBrace> {};

// prim attribute metadata
// DisplayUnitMetadata = displayUnit Assignment Identifier
struct DisplayUnitMetadata : PEGTL_NS::if_must<
    KeywordDisplayUnit,
    Assignment,
    Identifier> {};

// AttributeMetadataItem = SharedWithListOpMetadata /
//                         DisplayUnitMetadata /
//                         PermissionMetadata / 
//                         SymmetryFunctionMetadata
struct AttributeMetadataItem : PEGTL_NS::sor<
    SharedWithListOpMetadata,
    DisplayUnitMetadata,
    PermissionMetadata,
    SymmetryFunctionMetadata> {};

// AttributeMetadata = ( (NewLines)? ((TokenSeparator)? AttributeMetadataItem (TokenSeparator)? (StatementSeparator (TokenSeparator)? AttributeMetadataItem (TokenSeparator)?)* StatementEnd)? (TokenSeparator)? )
struct AttributeMetadata : PEGTL_NS::if_must<
    MetadataOpen,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<StatementSequenceOf<AttributeMetadataItem>>,
    PEGTL_NS::opt<TokenSeparator>,
    MetadataClose> {};

// prim attribute definition
// AttributeVariability = config / uniform
struct AttributeVariability : PEGTL_NS::sor<
    KeywordConfig,
    KeywordUniform> {};

// AttributeType = (AttributeVariability TokenSeparator)? Identifier (TokenSeparator)? ([ (TokenSeparator)? ])?
struct AttributeType : PEGTL_NS::seq<
    PEGTL_NS::opt<
        PEGTL_NS::seq<
            AttributeVariability,
            TokenSeparator>>,
    Identifier,
    PEGTL_NS::opt<
        PEGTL_NS::seq<
            PEGTL_NS::opt<TokenSeparator>,
            ArrayType>>> {};

// AttributeDeclaration = AttributeType TokenSeparator NamespacedName
struct AttributeDeclaration : PEGTL_NS::seq<
    PEGTL_NS::opt<
        PEGTL_NS::seq<
            KeywordCustom,
            TokenSeparator>>,
    AttributeType,
    TokenSeparator,
    NamespacedName> {};

// AttributeValue = None / TypedValue
// AttributeAssignment = Assignment AttributeValue
struct AttributeAssignment : PEGTL_NS::seq<
    Assignment,
    PEGTL_NS::sor<
        KeywordNone,
        TypedValue>> {};
struct AttributeAssignmentOptional : PEGTL_NS::opt<
    AttributeAssignment> {};

// ConnectValue = KeywordNone /
//                PathRef /
//                [ (NewLines)? ((TokenSeparator)? (PathRef) (TokenSeparator)? (ListSeparator (TokenSeparator)? (PathRef) (TokenSeparator)?)* ListEnd)? (TokenSeparator)?] 
struct ConnectValue : PEGTL_NS::sor<
    KeywordNone,
    PathRef,
    PEGTL_NS::if_must<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<ListOf<PathRef>>,
        PEGTL_NS::opt<TokenSeparator>,
        RightBrace>> {};

// AttributeSpec = ConnectListOp (parsed as part of PropertySpec)
//                 AttributeDeclaration (AttributeAssignment)? (TokenSeparator)? (AttributeMetadata)? /
//                 AttributeDeclaration (TokenSeparator)? Dot (TokenSeparator)? TimeSamples Assignment TimeSampleMap /
//                 AttributeDeclaration (TokenSeparator)? Dot (TokenSeparator)? Connect Assigment ConnectValue
struct AttributeSpec : PEGTL_NS::seq<
    AttributeDeclaration,
    PEGTL_NS::if_must_else<
        PEGTL_NS::seq<
            PEGTL_NS::opt<TokenSeparator>,
            Sdf_PathParser::Dot,
            PEGTL_NS::opt<TokenSeparator>>,
        PEGTL_NS::sor<
            PEGTL_NS::if_must<
                KeywordTimeSamples,
                PEGTL_NS::seq<
                    Assignment,
                    TimeSampleMap>>,
            PEGTL_NS::if_must<
                KeywordConnect,
                PEGTL_NS::seq<
                    Assignment,
                    ConnectValue>>>,
        PEGTL_NS::seq<
            AttributeAssignmentOptional,
            PEGTL_NS::opt<TokenSeparator>,
            PEGTL_NS::opt<AttributeMetadata>>>> {};

// prim relationship metadata
// RelationshipMetadataItem = String /
// 				             KeyValueMetadata /
// 				             ListOpMetadata /
// 				             DocMetadata /
// 				             PermissionMetadata /
// 				             SymmetryFunctionMetadata
struct RelationshipMetadataItem : PEGTL_NS::sor<
    SharedWithListOpMetadata,
    PermissionMetadata,
    SymmetryFunctionMetadata> {};

// RelationshipMetadataList = RelationshipMetadataList StatementSeparator (TokenSeparator)? RelationshipMetadataItem (TokenSeparator)? /
//                         (TokenSeparator)? RelationshipMetadataItem (TokenSeparator)?
// RelationshipMetadata = ( (NewLines)? (RelationshipMetadataList StatementEnd)? (TokenSeparator)? 
struct RelationshipMetadata : PEGTL_NS::if_must<
    MetadataOpen,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<StatementSequenceOf<RelationshipMetadataItem>>,
    PEGTL_NS::opt<TokenSeparator>,
    MetadataClose> {};

// prim relationship definition
// RelationshipAssignment = Assignment None /
//                          Assignment PathRef /
//                          Assignment [ ((NewLines)? ((TokenSeparator)? PathRef (TokenSeparator)? (ListSeparator (TokenSeparator)? PathRef (TokenSeparator)?)* ListEnd)? (TokenSeparator)?)? ]
struct RelationshipAssignmentOpen : LeftBrace {};
struct RelationshipAssignmentClose : RightBrace {};
struct RelationshipAssignment : PEGTL_NS::seq<
    Assignment,
    PEGTL_NS::sor<
        KeywordNone,
        PathRef,
        PEGTL_NS::if_must<
            RelationshipAssignmentOpen,
            PEGTL_NS::opt<
                PEGTL_NS::seq<
                    PEGTL_NS::opt<NewLines>,
                    PEGTL_NS::opt<ListOf<PathRef>>,
                    PEGTL_NS::opt<TokenSeparator>>>,
            RelationshipAssignmentClose>>> {};

// RelationshipType = rel /
//                    custom TokenSeparator rel /
//                    custom TokenSeparator varying TokenSeparator rel /
//                    varying TokenSeparator rel
struct RelationshipType : PEGTL_NS::sor<
    KeywordRel,
    PEGTL_NS::seq<
        KeywordVarying,
        TokenSeparator,
        KeywordRel>,
    PEGTL_NS::seq<
        KeywordCustom,
        TokenSeparator,
        PEGTL_NS::sor<
            KeywordRel,
            PEGTL_NS::seq<
                KeywordVarying,
                TokenSeparator,
                KeywordRel>>>> {};

// RelationshipSpec = RelationshipListOp (parsed as part of PropertySpec)
//                    RelationshipType TokenSeparator NamespacedName (RelationshipAssignment)? (TokenSeparator)? (RelationshipMetadata)? /
//                    RelationshipType TokenSeparator NamespacedName (TokenSeparator)? [ (TokenSeparator)? PathRef (TokenSeparator)? ] /
//                    RelationshipType TokenSeparator NamespacedName (TokenSeparator)? . (TokenSeparator)? timeSamples Assignment TimeSampleMap /
//                    RelationshipType TokenSeparator NamespacedName (TokenSeparator)? . (TokenSeparator)? default Assignment PathRef
struct RelationshipTargetOpen : LeftBrace {};
struct RelationshipTargetClose : RightBrace {};
struct RelationshipAssignmentOptional : PEGTL_NS::opt<
    RelationshipAssignment> {};
struct RelationshipSpec : PEGTL_NS::seq<
    RelationshipType,
    TokenSeparator,
    NamespacedName,
    PEGTL_NS::if_must_else<
            PEGTL_NS::seq<
                PEGTL_NS::opt<TokenSeparator>,
                Sdf_PathParser::Dot,
                PEGTL_NS::opt<TokenSeparator>>,
            PEGTL_NS::sor<
                PEGTL_NS::if_must<
                    KeywordTimeSamples,
                    PEGTL_NS::seq<
                        Assignment,
                        TimeSampleMap>>,
                PEGTL_NS::if_must<
                    KeywordDefault,
                    PEGTL_NS::seq<
                        Assignment,
                        PathRef>>>,
            PEGTL_NS::sor<
                PEGTL_NS::seq<
                    RelationshipAssignmentOptional,
                    PEGTL_NS::opt<TokenSeparator>,
                    PEGTL_NS::opt<RelationshipMetadata>>,
                PEGTL_NS::seq<
                    PEGTL_NS::opt<TokenSeparator>,
                    PEGTL_NS::if_must<
                        RelationshipTargetOpen,
                        PEGTL_NS::opt<TokenSeparator>,
                        PathRef,
                        PEGTL_NS::opt<TokenSeparator>,
                        RelationshipTargetClose>>>>> {};

// prim metadata
// InheritsOrSpecializesList = None /
//                             PathRef /
//                             [ (NewLines)? ( (TokenSeparator)? PathRef (TokenSeparator)? (ListSeparator (TokenSeparator)? PathRef (TokenSeparator)?)* ListEnd)? (TokenSeparator)? ]
struct InheritsOrSpecializesList : PEGTL_NS::sor<
    KeywordNone,
    PathRef,
    PEGTL_NS::seq<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<ListOf<PathRef>>,
        PEGTL_NS::opt<TokenSeparator>,
        RightBrace>> {};

// SpecializesMetadata = specializes Assignment InheritsOrSpecializesList /
//                       add specializes Assignment InheritsOrSpecializesList /
//                       delete specializes Assignment InheritsOrSpecializesList /
//                       append specializes Assignment InheritsOrSpecializesList /
//                       prepend specializes Assignment InheritsOrSpecializesList /
//                       reorder specializes Assignment InheritsOrSpecializesList
struct SpecializesMetadata : PEGTL_NS::seq<
    KeywordSpecializes,
    Assignment,
    InheritsOrSpecializesList> {};

// InheritsMetadata = inherits Assignment InheritsOrSpecializesList /
//                    add inherits Assignment InheritsOrSpecializesList /
//                    delete inherits Assignment InheritsOrSpecializesList /
//                    append inherits Assignment InheritsOrSpecializesList /
//                    prepend inherits Assignment InheritsOrSpecializesList /
//                    reorder inherits Assignment InheritsOrSpecializesList
struct InheritsMetadata : PEGTL_NS::seq<
    KeywordInherits,
    Assignment,
    InheritsOrSpecializesList> {};

// ReferenceParameter = customData Assignment DictionaryValue /
//                      LayerOffset
struct ReferenceParameter : PEGTL_NS::sor<
    PEGTL_NS::seq<
        KeywordCustomData,
        Assignment,
        DictionaryValue>,
    LayerOffset> {};

// ReferenceParameterList = (TokenSeparator)? ReferenceParameter (TokenSeparator)? (StatementSeparator (TokenSeparator)? ReferenceParameter (TokenSeparator)?)*
// ReferenceParameters = ( (NewLines)? (ReferenceParameterList StatementEnd)? (TokenSeparator)? )
// ReferenceListItem = AssetRef (TokenSeparator)? (PathRef)? (TokenSeparator)? (ReferenceParameters)? /
//			           PathRef (TokenSeparator)? (ReferenceParameters)?
struct ReferenceParametersOpen : LeftParen {};
struct ReferenceParametersClose : RightParen {};
struct ReferenceListItem : PEGTL_NS::sor<
    PEGTL_NS::seq<
        AssetRef,
        PEGTL_NS::opt<TokenSeparator>,
        PEGTL_NS::opt<PathRef>,
        PEGTL_NS::opt<TokenSeparator>,
        PEGTL_NS::opt<
            PEGTL_NS::seq<
                ReferenceParametersOpen,
                PEGTL_NS::opt<NewLines>,
                PEGTL_NS::opt<StatementSequenceOf<ReferenceParameter>>,
                PEGTL_NS::opt<TokenSeparator>,
                ReferenceParametersClose>>>,
    PEGTL_NS::seq<
        PathRef,
        PEGTL_NS::opt<TokenSeparator>,
        PEGTL_NS::opt<
            PEGTL_NS::seq<
                ReferenceParametersOpen,
                PEGTL_NS::opt<NewLines>,
                PEGTL_NS::opt<StatementSequenceOf<ReferenceParameter>>,
                PEGTL_NS::opt<TokenSeparator>,
                ReferenceParametersClose>>>> {};

// 	ReferenceListItems = (TokenSeparator)? ReferenceListItem (TokenSeparator)? (ListSeparator (TokenSeparator)? ReferenceListItem (TokenSeparator)?)*
//	ReferenceList = None /
//			        ReferenceListItem /
//			        [ (NewLines)? (ReferenceListItems ListEnd)? (TokenSeparator)? ]
struct ReferenceList : PEGTL_NS::sor<
    KeywordNone,
    ReferenceListItem,
    PEGTL_NS::seq<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<ListOf<ReferenceListItem>>,
        PEGTL_NS::opt<TokenSeparator>,
        RightBrace>> {};

// ReferencesMetadata = references Assignment ReferenceList /
//                      add references Assignment ReferenceList /
//                      delete references Assignment ReferenceList /
//                      append references Assignment ReferenceList /
//                      prepend references Assignment ReferenceList /
//                      reorder references Assignment ReferenceList
struct ReferencesMetadata : PEGTL_NS::seq<
    KeywordReferences,
    Assignment,
    ReferenceList> {};

// PayloadParameters = ( (NewLines)? ((TokenSeparator)? LayerOffset (TokenSeparator)? (StatementSeparator (TokenSeparator)? LayerOffset (TokenSeparator)?)* StatementEnd)? (TokenSeparator)? )
// PayloadListItem = AssetRef (TokenSeparator)? (PathRef)? (TokenSeparator)? (PayloadParameters)? /
//                   PathRef (TokenSeparator)? (PayloadParamaters)?
struct PayloadListItem : PEGTL_NS::sor<
    PEGTL_NS::seq<
        AssetRef,
        PEGTL_NS::opt<TokenSeparator>,
        PEGTL_NS::opt<PathRef>,
        PEGTL_NS::opt<TokenSeparator>,
        PEGTL_NS::opt<
            PEGTL_NS::seq<
                LeftParen,
                PEGTL_NS::opt<NewLines>,
                PEGTL_NS::opt<StatementSequenceOf<LayerOffset>>,
                PEGTL_NS::opt<TokenSeparator>,
                RightParen>>>,
    PEGTL_NS::seq<
        PathRef,
        PEGTL_NS::opt<TokenSeparator>,
        PEGTL_NS::opt<
            PEGTL_NS::seq<
                LeftParen,
                PEGTL_NS::opt<NewLines>,
                PEGTL_NS::opt<StatementSequenceOf<LayerOffset>>,
                PEGTL_NS::opt<TokenSeparator>,
                RightParen>>>> {};

// PayloadListItems = (TokenSeparator)? PayloadListItem (TokenSeparator)? (ListSeparator (TokenSeparator)? PayloadListItem (TokenSeparator)?)*
// PayloadList = None /
//               PayloadListItem /
//               [ (NewLines)? (PayloadListItems ListEnd)? (TokenSeparator)? ]
struct PayloadList : PEGTL_NS::sor<
    KeywordNone,
    PayloadListItem,
    PEGTL_NS::seq<
        LeftBrace,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<ListOf<PayloadListItem>>,
        PEGTL_NS::opt<TokenSeparator>,
        RightBrace>> {};

// PayloadMetadata = payload Assignment PayloadList /
//                   add payload Assignment PayloadList /
//                   delete payload Assignment PayloadList /
//                   append payload Assignment PayloadList /
//                   prepend payload Assignment PayloadList /
//                   reorder payload Assignment PayloadList
struct PayloadMetadata : PEGTL_NS::seq<
    KeywordPayload,
    Assignment,
    PayloadList> {};

// RelocatesItem = PathRef (TokenSeparator)? : (TokenSeparator)? PathRef
// RelocatesMetadata = relocates Assignment { (NewLines)? ((TokenSeparator)? RelocatesItem (TokenSeparator)? (ListSeparator (TokenSeparator)? RelocatesItem (TokenSeparator)?)* ListEnd)? (TokenSeparator)? }
struct RelocatesMapOpen : LeftCurlyBrace {};
struct RelocatesMapClose : RightCurlyBrace {};
struct RelocatesMetadata : PEGTL_NS::seq<
    KeywordRelocates,
    Assignment,
    PEGTL_NS::must<
        RelocatesMapOpen,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<ListOf<
            PEGTL_NS::seq<
                PathRef,
                PEGTL_NS::opt<TokenSeparator>,
                NamespaceSeparator,
                PEGTL_NS::opt<TokenSeparator>,
                PathRef>>>,
        PEGTL_NS::opt<TokenSeparator>,
        RelocatesMapClose>> {};

// VariantsMetadata = variants Assignment DictionaryValue
struct VariantsMetadata : PEGTL_NS::seq<
    KeywordVariants,
    Assignment,
    DictionaryValue> {};

// VariantSetsMetadata = variantSets Assignment NameList /
//                       add variantSets Assignment NameList /
//                       delete variantSets Assignment NameList /
//                       append variantSets Assignment NameList /
//                       prepend variantSets Assignment NameList /
//                       reorder variantSets Assignment NameList
struct VariantSetsMetadata : PEGTL_NS::seq<
    KeywordVariantSets,
    Assignment,
    NameList> {};

// KindMetadata = kind Assignment String
struct KindMetadata : PEGTL_NS::seq<
    KeywordKind,
    Assignment,
    String> {};

// PrefixOrSuffixSubstitutionsMetadata = prefixSubstitutions Assignment StringDictionary /
//                                       suffixSubstitutions Assignment StringDictionary
struct PrefixOrSuffixSubstitutionsMetadata : PEGTL_NS::seq<
    PEGTL_NS::sor<
        KeywordPrefixSubstitutions,
        KeywordSuffixSubstitutions>,
    Assignment,
    StringDictionary> {};

// PrimMetadataItem = String /
// 			          KeyValueMetadata /
// 			          ListOpMetadata /
// 			          DocMetadata /
// 			          KindMetadata /
// 			          PayloadMetadata /
// 			          InheritsMetadata /
// 			          SpecializesMetadata /
// 			          ReferencesMetadata /
// 			          RelocatesMetadata /
// 			          VariantsMetadata /
// 			          VariantSetsMetadata /
//                    PrefixOrSuffixSubstitutionsMetadata /
//                    PermissionMetadata /
//                    SymmetryFunctionMetadata
struct PrimMetadataItem : PEGTL_NS::sor<
    PEGTL_NS::if_must<
        ListOpKeyword,
        TokenSeparator,
        PEGTL_NS::if_must_else<
            KeywordReferences,
            PEGTL_NS::seq<
                Assignment,
                ReferenceList>,
            PEGTL_NS::if_must_else<
                KeywordPayload,
                PEGTL_NS::seq<
                    Assignment,
                    PayloadList>,
                PEGTL_NS::if_must_else<
                    PEGTL_NS::sor<
                        KeywordInherits,
                        KeywordSpecializes>,
                    PEGTL_NS::seq<
                        Assignment,
                        InheritsOrSpecializesList>,
                    PEGTL_NS::if_must_else<
                        KeywordVariantSets,
                        PEGTL_NS::seq<
                            Assignment,
                            NameList>,
                        ListOpKeyValueMetadata>>>>>,
    SharedMetadata,
    KindMetadata,
    ReferencesMetadata,
    PayloadMetadata,
    VariantsMetadata,
    VariantSetsMetadata,
    InheritsMetadata,
    SpecializesMetadata,
    RelocatesMetadata,
    PrefixOrSuffixSubstitutionsMetadata,
    PermissionMetadata,
    SymmetryFunctionMetadata> {};

// PrimMetadata = (NewLines)? (TokenSeparator)? (( (NewLines)? ((TokenSeparator)? PrimMetadataItem (TokenSeparator)? (StatementSeparator (TokenSeparator)? PrimMetadatItem (TokenSeparator)?)* StatementEnd)? (TokenSeparator)? ))?
struct PrimMetadata : PEGTL_NS::seq<
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<TokenSeparator>,
    PEGTL_NS::opt_must<
        MetadataOpen,
        PEGTL_NS::seq<
            PEGTL_NS::opt<NewLines>,
            PEGTL_NS::opt<StatementSequenceOf<PrimMetadataItem>>,
            PEGTL_NS::opt<TokenSeparator>>,
        MetadataClose>> {};

// prim definition
// PropertySpec = AttributeSpec /
//                RelationshipSpec
// ConnectListOp = add TokenSeparator AttributeDeclaration (TokenSeparator)? . (TokenSeparator)? connect Assignment ConnectValue /
//                 delete TokenSeparator AttributeDeclaration (TokenSeparator)? . (TokenSeparator)? connect Assignment ConnectValue /
//                 append TokenSeparator AttributeDeclaration (TokenSeparator)? . (TokenSeparator)? connect Assignment ConnectValue /
//                 prepend TokenSeparator AttributeDeclaration (TokenSeparator)? . (TokenSeparator)? connect Assignment ConnectValue /
//                 reorder TokenSeparator AttributeDeclaration (TokenSeparator)? . (TokenSeparator)? connect Assignment ConnectValue
// RelationshipListOp = add TokenSeparator RelationshipType TokenSeparator NamespacedName (RelationshipAssignment)? /
//                      delete TokenSeparator RelationshipType TokenSeparator NamespaceName (RelationshipAssignment)? /
//                      append TokenSeparator RelationshipType TokenSeparator NamespaceName (RelationshipAssignment)? /
//                      prepend TokenSeparator RelationshipType TokenSeparator NamespaceName (RelationshipAssignment)? /
//                      reorder TokenSeparator RelationshipType TokenSeparator NamespaceName (RelationshipAssignment)?
// Note this is not a direct translation - in order to greedily optimize
// we take note that attribute specs can contain connect list ops
// so we separate out list ops for the two specs
// and parse them separately as an additional production
// additionally, the keyword 'reorder' can start either list op 
// statements or the child / property order ones, so we have to greedily take that
// into account here as well (which aren't truly PropertySpecs but captured here for optimization)
struct PropertySpec : PEGTL_NS::sor<
    AttributeSpec,
    RelationshipSpec,
    PEGTL_NS::seq<
        ListOpKeyword,
        TokenSeparator,
        PEGTL_NS::if_must_else<
            RelationshipType,
            PEGTL_NS::seq<
                TokenSeparator,
                NamespacedName,
                RelationshipAssignmentOptional>,
            PEGTL_NS::seq<
                AttributeDeclaration,
                PEGTL_NS::opt<TokenSeparator>,
                Sdf_PathParser::Dot,
                PEGTL_NS::opt<TokenSeparator>,
                KeywordConnect,
                Assignment,
                ConnectValue>>>> {};

// ChildOrderStatement = reorder TokenSeparator nameChildren Assignment NameList
// PropertyOrderStatement = reorder TokenSeparator properties Assignment NameList
struct ChildOrPropertyOrderStatement : PEGTL_NS::seq<
    KeywordReorder,
    TokenSeparator,
    PEGTL_NS::sor<
        KeywordNameChildren,
        KeywordProperties>,
    Assignment,
    NameList> {};

// 	VariantStatement = String (TokenSeparator)? (PrimMetadata)? (NewLines)? (TokenSeparator)? { (NewLines)? (TokenSeparator)? (PrimContentsList)? (TokenSeparator)? } (NewLines)?
struct PrimContents;
struct VariantStatementOpen : LeftCurlyBrace {};
struct VariantStatementClose : RightCurlyBrace {};
struct VariantStatement : PEGTL_NS::seq<
    String,
    PEGTL_NS::opt<TokenSeparator>,
    PEGTL_NS::opt<PrimMetadata>,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<TokenSeparator>,
    PEGTL_NS::must<
        VariantStatementOpen,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<TokenSeparator>,
        PEGTL_NS::opt<PrimContents>,
        PEGTL_NS::opt<TokenSeparator>,
        VariantStatementClose,
        PEGTL_NS::opt<NewLines>>> {};

// 	VariantSetStatement = variantSet TokenSeparator String Assignment (NewLines)? (TokenSeparator)? { (NewLines)? ((TokenSeparator)? VariantStatement)+ (TokenSeparator)? }
struct VariantStatementListOpen : LeftCurlyBrace {};
struct VariantStatementListClose : RightCurlyBrace {};
struct VariantSetStatement : PEGTL_NS::seq<
    KeywordVariantSet,
    TokenSeparator,
    String,
    Assignment,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<TokenSeparator>,
    PEGTL_NS::must<
        VariantStatementListOpen,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::plus<
            PEGTL_NS::seq<
                PEGTL_NS::opt<TokenSeparator>,
                VariantStatement>>,
        PEGTL_NS::opt<TokenSeparator>,
        VariantStatementListClose>> {};

// PrimItem = ChildOrderStatement /
//		      PropertyOrderStatement /
//		      PropertySpec /
//            PrimSpec /
//		      VariantSetStatement
struct PrimSpec;
struct PrimItem : PEGTL_NS::sor<
    PEGTL_NS::seq<
        ChildOrPropertyOrderStatement,
        PEGTL_NS::opt<TokenSeparator>,
        StatementSeparator>,
    PEGTL_NS::seq<
        PropertySpec,
        PEGTL_NS::opt<TokenSeparator>,
        StatementSeparator>,
    PEGTL_NS::seq<
        PrimSpec,
        PEGTL_NS::opt<TokenSeparator>,
        NewLines>,
    PEGTL_NS::seq<
        VariantSetStatement,
        PEGTL_NS::opt<TokenSeparator>,
        NewLines>> {};

// PrimContents = (NewLines)? ((TokenSeparator)? PrimItem)*
struct PrimContents : PEGTL_NS::seq<
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::star<
        PEGTL_NS::opt<TokenSeparator>,
        PrimItem>> {};

// PrimTypeName = Identifier ((TokenSeparator)? . (TokenSeparator)? Identifier)* 
struct PrimTypeName : PEGTL_NS::list<
    Identifier,
    PEGTL_NS::seq<
        PEGTL_NS::opt<TokenSeparator>,
        Sdf_PathParser::Dot,
        PEGTL_NS::opt<TokenSeparator>>> {};

// PrimSpec = def TokenSeparator (PrimTypeName TokenSeparator)? String (TokenSeparator)? (PrimMetadata)? (NewLines)? (TokenSeparator)? { PrimContents (TokenSeparator)? } /
//            over TokenSeparator (PrimTypeName TokenSeparator)? String (TokenSeparator)? (PrimMetadata)? (NewLines)? (TokenSeparator)? { PrimContents (TokenSeparator)? } /
//            class TokenSeparator (PrimTypeName TokenSeparator)? String (TokenSeparator)? (PrimMetadata)? (NewLines)? (TokenSeparator)? { PrimContents (TokenSeparator)? }
struct PrimMetadataOptional : PEGTL_NS::opt<
    PrimMetadata> {};
struct PrimSpec : PEGTL_NS::seq<
    PEGTL_NS::sor<
        KeywordDef,
        KeywordOver,
        KeywordClass>,
    TokenSeparator,
    PEGTL_NS::opt<
        PEGTL_NS::seq<
            PrimTypeName,
            TokenSeparator>>,
    String,
    PEGTL_NS::opt<TokenSeparator>,
    PrimMetadataOptional,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<TokenSeparator>,
    LeftCurlyBrace,
    PEGTL_NS::must<
        PrimContents,
        PEGTL_NS::opt<TokenSeparator>,
        RightCurlyBrace>> {};

// LayerOffsetList = ( (TokenSeparator)? LayerOffset (TokenSeparator)? (StatementSeparator (TokenSeparator)? LayerOffset (TokenSeparator)?)* StatementEnd )
// SublayerItem = AssetRef (TokenSeparator)? (LayerOffsetList)?
struct SublayerItem : PEGTL_NS::seq<
    AssetRef,
    PEGTL_NS::opt<TokenSeparator>,
    PEGTL_NS::opt<
        PEGTL_NS::seq<
            LeftParen,
            PEGTL_NS::opt<StatementSequenceOf<LayerOffset>>,
            PEGTL_NS::must<RightParen>>>> {};

// SublayerMetadata = subLayers Assignment [ (NewLines)? ((TokenSeparator)? SublayerItem (TokenSeparator)? (ListSeparator (TokenSeparator)? SublayerItem (TokenSeparator)?)* ListEnd)? (TokenSeparator)? ]
struct SublayerListOpen : LeftBrace {};
struct SublayerListClose : RightBrace {};
struct SublayerMetadata : PEGTL_NS::seq<
    KeywordSubLayers,
    Assignment,
    SublayerListOpen,
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<ListOf<SublayerItem>>,
    PEGTL_NS::opt<TokenSeparator>,
    PEGTL_NS::must<SublayerListClose>> {};

// LayerMetadataItem = SharedMetadata /
//                     SublayerMetadata
struct LayerMetadataItem : PEGTL_NS::sor<
    LayerSharedWithListOpMetadata,
    RelocatesMetadata,
    SublayerMetadata> {};

// LayerMetadata = (NewLines)? (TokenSeparator)? ( ( (NewLines)? (TokenSeparator)? LayerMetadataItem (TokenSeparator)? (StatementSeparator (TokenSeparator)? LayerMetadataItem (TokenSeparator)?)* StatementEnd)? ) )? (NewLines)?
struct LayerMetadata : PEGTL_NS::seq<
    PEGTL_NS::opt<NewLines>,
    PEGTL_NS::opt<TokenSeparator>,
    PEGTL_NS::opt_must<
        MetadataOpen,
        PEGTL_NS::opt<NewLines>,
        PEGTL_NS::opt<StatementSequenceOf<LayerMetadataItem>>,
        PEGTL_NS::opt<TokenSeparator>,
        MetadataClose>,
    PEGTL_NS::opt<NewLines>> {};

// LayerHeader = # (!(CrLf) AnyChar)*
struct LayerHeader : PEGTL_NS::sor<
    PEGTL_NS::seq<
        PEGTL_NS::one<'#'>,
        PEGTL_NS::until<NewLine>>, 
    PEGTL_NS::seq<
        PEGTL_NS::one<'#'>,
        PEGTL_NS::until<PEGTL_NS::eof>>> {};

// PrimList = PrimList NewLines (TokenSeparator)? PrimStatement (TokenSeparator)? /
//		      (TokenSeparator)? PrimStatement (TokenSeparator)?
// LayerSpec = LayerHeader (LayerMetadata)? (NewLines)? (TokenSeparator PrimList NewLines)? (EolWhitspace)?
struct LayerSpec : PEGTL_NS::seq<
    LayerHeader, 
    PEGTL_NS::opt<LayerMetadata>,
    PEGTL_NS::opt<
        PEGTL_NS::seq<
            PEGTL_NS::opt<NewLines>,
            PEGTL_NS::opt<
                PEGTL_NS::list<
                    PEGTL_NS::seq<
                        PEGTL_NS::opt<TokenSeparator>,
                        PEGTL_NS::sor<
                            PEGTL_NS::seq<
                                KeywordReorder,
                                TokenSeparator,
                                KeywordRootPrims,
                                Assignment,
                                NameList>,
                            PrimSpec>,
                        PEGTL_NS::opt<TokenSeparator>>,
                    NewLines>>,
            PEGTL_NS::opt<NewLines>>>,
    PEGTL_NS::opt<EolWhitespace>> {};

// LayerMetadataOnly = LayerHeader (LayerMetadata)?
// production used to interrogate layer metadata without reading the entire layer
struct LayerMetadataOnly : PEGTL_NS::seq<
    LayerHeader, 
    PEGTL_NS::opt<LayerMetadata>> {};

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

// TextParserDefaultErrorControl doesn't take the error_message as a template
// parameter directly, so it's wrapped here
struct TextParserControlValues
{
    template<typename Rule>
    static constexpr auto message = errorMessage<Rule>;
};

template <typename Rule>
using TextParserControl = 
    TextParserDefaultErrorControl<TextParserControlValues>::control<Rule>;

} // end namespace Sdf_TextFileFormatParser

PXR_NAMESPACE_CLOSE_SCOPE

#endif