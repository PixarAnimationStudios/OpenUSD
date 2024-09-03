//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file Sdf/textFileFormatParser.h

#ifndef PXR_USD_SDF_TEXT_FILE_FORMAT_PARSER_H
#define PXR_USD_SDF_TEXT_FILE_FORMAT_PARSER_H

#include "pxr/pxr.h"
#include "pxr/base/pegtl/pegtl.hpp"
#include "pxr/base/pegtl/pegtl/contrib/trace.hpp"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/debugCodes.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/path.h"
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

namespace PEGTL_NS = PXR_PEGTL_NAMESPACE;

// special characters
// note - Dot comes from pathParser.h
struct SingleQuote : PEGTL_NS::one<'\''> {};
struct DoubleQuote : PEGTL_NS::one<'"'> {};
struct LeftParen : PEGTL_NS::one<'('> {};
struct RightParen : PEGTL_NS::one<')'> {};
struct LeftBracket : PEGTL_NS::one<'['> {};
struct RightBracket : PEGTL_NS::one<']'> {};
struct LeftBrace : PEGTL_NS::one<'{'> {};
struct RightBrace : PEGTL_NS::one<'}'> {};
struct LeftAngleBracket : PEGTL_NS::one<'<'> {};
struct RightAngleBracket : PEGTL_NS::one<'>'> {};
struct At : PEGTL_NS::one<'@'> {};
struct Colon : PEGTL_NS::one<':'> {};
struct Equals : PEGTL_NS::one<'='> {};
struct Sign : PEGTL_NS::one<'+', '-'> {};
struct Minus : PEGTL_NS::one<'-'> {};
struct Exponent : PEGTL_NS::one<'e', 'E'> {};
struct Space : PEGTL_NS::one<' ', '\t'> {};
struct Ampersand : PEGTL_NS::one<'&'> {};

// character classes
struct Digit : PEGTL_NS::digit {};
struct HexDigit : PEGTL_NS::xdigit {};
struct OctDigit : PEGTL_NS::range<0, 7> {};
struct Eol : PEGTL_NS::one<'\r', '\n'> {};
struct Eolf : PEGTL_NS::eolf {};
struct Utf8 : PEGTL_NS::utf8::any {};
struct Utf8NoEolf : PEGTL_NS::minus<Utf8, Eol> {};

// Escape character sets as defined by `TfEscapeStringReplaceChar` plus quotes
struct EscapeSingleCharacter :
    PEGTL_NS::seq<
        PEGTL_NS::one<'\\', 'a', 'b', 'f', 'n', 'r', 't', 'v', '\'', '"'>> {};
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
struct KeywordAdd : PXR_PEGTL_KEYWORD("add") {};
struct KeywordAppend : PXR_PEGTL_KEYWORD("append") {};
struct KeywordBezier: PXR_PEGTL_KEYWORD("bezier") {};
struct KeywordClass : PXR_PEGTL_KEYWORD("class") {};
struct KeywordConfig : PXR_PEGTL_KEYWORD("config") {};
struct KeywordConnect : PXR_PEGTL_KEYWORD("connect") {};
struct KeywordCurve : PXR_PEGTL_KEYWORD("curve") {};
struct KeywordCustom : PXR_PEGTL_KEYWORD("custom") {};
struct KeywordCustomData : PXR_PEGTL_KEYWORD("customData") {};
struct KeywordDefault : PXR_PEGTL_KEYWORD("default") {};
struct KeywordDef : PXR_PEGTL_KEYWORD("def") {};
struct KeywordDelete : PXR_PEGTL_KEYWORD("delete") {};
struct KeywordDictionary : PXR_PEGTL_KEYWORD("dictionary") {};
struct KeywordDisplayUnit : PXR_PEGTL_KEYWORD("displayUnit") {};
struct KeywordDoc : PXR_PEGTL_KEYWORD("doc") {};
struct KeywordHeld : PXR_PEGTL_KEYWORD("held") {};
struct KeywordHermite : PXR_PEGTL_KEYWORD("hermite") {};
struct KeywordInherits : PXR_PEGTL_KEYWORD("inherits") {};
struct KeywordKind : PXR_PEGTL_KEYWORD("kind") {};
struct KeywordLinear : PXR_PEGTL_KEYWORD("linear") {};
struct KeywordLoop : PXR_PEGTL_KEYWORD("loop") {};
struct KeywordNameChildren : PXR_PEGTL_KEYWORD("nameChildren") {};
struct KeywordNone : PXR_PEGTL_KEYWORD("None") {};
struct KeywordNone_LC : PXR_PEGTL_KEYWORD("none") {};
struct KeywordOffset : PXR_PEGTL_KEYWORD("offset") {};
struct KeywordOscillate : PXR_PEGTL_KEYWORD("oscillate") {};
struct KeywordOver : PXR_PEGTL_KEYWORD("over") {};
struct KeywordPayload : PXR_PEGTL_KEYWORD("payload") {};
struct KeywordPermission : PXR_PEGTL_KEYWORD("permission") {};
struct KeywordPost : PXR_PEGTL_KEYWORD("post") {};
struct KeywordPrefixSubstitutions : PXR_PEGTL_KEYWORD("prefixSubstitutions") {};
struct KeywordPre : PXR_PEGTL_KEYWORD("pre") {};
struct KeywordPrepend : PXR_PEGTL_KEYWORD("prepend") {};
struct KeywordProperties : PXR_PEGTL_KEYWORD("properties") {};
struct KeywordReferences : PXR_PEGTL_KEYWORD("references") {};
struct KeywordRelocates : PXR_PEGTL_KEYWORD("relocates") {};
struct KeywordRel : PXR_PEGTL_KEYWORD("rel") {};
struct KeywordReorder : PXR_PEGTL_KEYWORD("reorder") {};
struct KeywordRootPrims : PXR_PEGTL_KEYWORD("rootPrims") {};
struct KeywordRepeat : PXR_PEGTL_KEYWORD("repeat") {};
struct KeywordReset : PXR_PEGTL_KEYWORD("reset") {};
struct KeywordScale : PXR_PEGTL_KEYWORD("scale") {};
struct KeywordSloped : PXR_PEGTL_KEYWORD("sloped") {};
struct KeywordSubLayers : PXR_PEGTL_KEYWORD("subLayers") {};
struct KeywordSuffixSubstitutions : PXR_PEGTL_KEYWORD("suffixSubstitutions") {};
struct KeywordSpecializes : PXR_PEGTL_KEYWORD("specializes") {};
struct KeywordSpline : PXR_PEGTL_KEYWORD("spline") {};
struct KeywordSymmetryArguments : PXR_PEGTL_KEYWORD("symmetryArguments") {};
struct KeywordSymmetryFunction : PXR_PEGTL_KEYWORD("symmetryFunction") {};
struct KeywordTimeSamples : PXR_PEGTL_KEYWORD("timeSamples") {};
struct KeywordUniform : PXR_PEGTL_KEYWORD("uniform") {};
struct KeywordVariantSet : PXR_PEGTL_KEYWORD("variantSet") {};
struct KeywordVariantSets : PXR_PEGTL_KEYWORD("variantSets") {};
struct KeywordVariants : PXR_PEGTL_KEYWORD("variants") {};
struct KeywordVarying : PXR_PEGTL_KEYWORD("varying") {};

struct Keywords : PEGTL_NS::sor<
KeywordAdd,
KeywordAppend,
KeywordBezier,
KeywordClass,
KeywordConfig,
KeywordConnect,
KeywordCurve,
KeywordCustom,
KeywordCustomData,
KeywordDefault,
KeywordDef,
KeywordDelete,
KeywordDictionary,
KeywordDisplayUnit,
KeywordDoc,
KeywordHeld,
KeywordHermite,
KeywordInherits,
KeywordKind,
KeywordLinear,
KeywordLoop,
KeywordNameChildren,
KeywordNone,
KeywordNone_LC,
KeywordOffset,
KeywordOscillate,
KeywordOver,
KeywordPayload,
KeywordPermission,
KeywordPost,
KeywordPre,
KeywordPrefixSubstitutions,
KeywordPrepend,
KeywordProperties,
KeywordReferences,
KeywordRelocates,
KeywordRel,
KeywordReorder,
KeywordRootPrims,
KeywordRepeat,
KeywordReset,
KeywordScale,
KeywordSloped,
KeywordSubLayers,
KeywordSuffixSubstitutions,
KeywordSpecializes,
KeywordSpline,
KeywordSymmetryArguments,
KeywordSymmetryFunction,
KeywordTimeSamples,
KeywordUniform,
KeywordVariantSet,
KeywordVariantSets,
KeywordVariants,
KeywordVarying 
> {};

struct MathKeywordInf : PXR_PEGTL_KEYWORD("inf") {};
struct MathKeywordNan : PXR_PEGTL_KEYWORD("nan") {};

// PythonStyleComment = # (NonCrlfUtf8Character)*
// CppStyleSingleLineComment = // (NonCrlfUtf8Character)*
// CppStyleMultiLineComment = /* (!(*/) (Utf8Character)*) */
// Comment = PythonStyleComment /
//           CppStyleSingleLineComment /
//           CppStyleMultiLineComment
struct CppStyleMultilineOpen :
    PEGTL_NS::seq<PEGTL_NS::one<'/'>, PEGTL_NS::one<'*'>> {};
struct CppStyleMultilineClose :
    PEGTL_NS::seq<PEGTL_NS::one<'*'>, PEGTL_NS::one<'/'>> {};
struct SingleLineContents : PEGTL_NS::star<PEGTL_NS::not_at<Eolf>, Utf8> {};
struct PythonStyleComment :PEGTL_NS::disable<
    PEGTL_NS::one<'#'>, SingleLineContents> {};
struct CppStyleSingleLineComment : PEGTL_NS::disable<
    PEGTL_NS::two<'/'>, SingleLineContents> {};
struct CppStyleMultiLineComment : PEGTL_NS::disable<
    CppStyleMultilineOpen,
    PEGTL_NS::until<CppStyleMultilineClose, Utf8>>{};
// Use to avoid '/' backtracking
struct CppStyleComment :
    PEGTL_NS::seq<
        PEGTL_NS::one<'/'>,
        PEGTL_NS::sor<
            PEGTL_NS::disable<PEGTL_NS::one<'*'>,
                              PEGTL_NS::until<CppStyleMultilineClose, Utf8>>,
            PEGTL_NS::disable<PEGTL_NS::one<'/'>, SingleLineContents>
        >
    > {};
struct Comment : PEGTL_NS::sor<PythonStyleComment, CppStyleComment> {};
    
// whitespace rules
// TokenSeparator represents whitespace between tokens,
// which can include space, tab, and c++ multiline style comments
// but MUST include a single space / tab character, that is:
// def/*comment*/foo is illegal but
// def /*comment*/foo or
// def/*comment*/ foo are both legal
// TokenSeparator = (Space)+ (CppStyleMultiLineComment (Space)*)?)* /
//                  (CppStyleMultiLineComment (Space)*)?)* (Space)+
struct InlinePadding :
    PEGTL_NS::sor<Space, CppStyleMultiLineComment>{};
struct SinglelinePadding :
    PEGTL_NS::sor<Space, Comment>{};
struct MultilinePadding :
    PEGTL_NS::sor<Space, Eol, Comment>{};
struct TokenSeparator :
    PEGTL_NS::pad<Space, CppStyleMultiLineComment, InlinePadding> {};

// array type
struct ArrayType : PEGTL_NS::if_must<
    LeftBracket,
    PEGTL_NS::star<InlinePadding>,
    RightBracket> {};

// separators
// ListSeparator = , (NewLines)?
// ListEnd = ListSeparator /
//           (NewLines)?
// StatementSeparator = ; (NewLines)? /
//                      NewLines
// StatementEnd = StatementSeparator /
//			      (NewLines)?
// Assignment = (TokenSeparator)? = (TokenSeparator)?
struct ListSeparator : PEGTL_NS::one<','> {};
struct StatementSeparator :
    PEGTL_NS::sor<Eol, PEGTL_NS::one<';'>> {};
struct NamespaceSeparator : Colon {};
struct CXXNamespaceSeparator : PEGTL_NS::two<':'> {};
struct Assignment : PEGTL_NS::pad<Equals, InlinePadding> {};

// generic lists
template <typename R>
struct ListOf :
    PEGTL_NS::list_tail<
        R, PEGTL_NS::pad<ListSeparator, InlinePadding, MultilinePadding>> {};

// generic statements
template <typename R>
struct StatementSequenceOf :
    PEGTL_NS::list_tail<
        R,
        PEGTL_NS::pad<StatementSeparator,
                      SinglelinePadding, MultilinePadding>> {};

// numbers
// Number = ((-)? ((Digit)+ / (Digit)+ . (Digit)* / . (Digit)+) 
//          (ExponentPart)?) /
//          inf /
//		    -inf /
// 		    nan
struct ExponentPart : PEGTL_NS::opt_must<
    Exponent,
    PEGTL_NS::opt<Sign>,
    PEGTL_NS::plus<Digit>> {};
struct NumberStandard : PEGTL_NS::seq<
    PEGTL_NS::plus<Digit>,
    PEGTL_NS::opt_must<Sdf_PathParser::Dot, PEGTL_NS::star<Digit>>,
    ExponentPart> {};
struct NumberLeadingDot : PEGTL_NS::seq<
    PEGTL_NS::if_must<Sdf_PathParser::Dot, PEGTL_NS::plus<Digit>>,
    ExponentPart> {};
struct Number : PEGTL_NS::sor<
    PEGTL_NS::seq<
        PEGTL_NS::opt<Minus>,
        PEGTL_NS::sor<NumberStandard,
                      NumberLeadingDot,
                      MathKeywordInf>>,
    MathKeywordNan> {};

// strings
// EscapedDoubleQuote = \"
// DoubleQuoteSingleLineStringChar = EscapedDoubleQuote / !" 
// NonCrlfUtf8Character
// EscapedSingleQuote = \'
// SingleQuoteSingleLineStringChar = EscapedSingleQuote / !' 
// NonCrlfUtf8Character
// DoubleQuoteMultiLineStringChar = EscapedDoubleQuote / !" Utf8Character
// SingleQuoteMultiLineStringChar = EscapedSingleQuote / !' Utf8Character
// String = "  DoubleQuoteSingleLineStringChar* " /
//	 """ DoubleQuoteMultiLineStringChar* """ /
//   ' SingleQuoteSingleLineStringChar* ' /
//	''' SingleQuoteMultiLineStringChar* '
struct MultilineContents : PEGTL_NS::sor<Escaped, Utf8> {};
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
struct NamespacedIdentifier :
    PEGTL_NS::list<BaseIdentifier, NamespaceSeparator>{};

// CXXNamespacedIdentifier = KeywordlessIdentifier (:: KeywordlessIdentifier)+
// Identifier = CXXNamespacedIdentifier /
//              KeywordlessIdentifier
struct Identifier :
    PEGTL_NS::list<KeywordlessIdentifier, CXXNamespaceSeparator> {};
struct NamespacedName : PEGTL_NS::sor<NamespacedIdentifier, Keywords> {};

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
// TupleInterior = TupleInterior ListSeparator (TokenSpace)? TupleItem 
//                 (TokenSpace)? /
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
    PEGTL_NS::pad<
        ListOf<TupleValueItem>,
        MultilinePadding>,
    TupleValueClose> {};
struct TypedTupleValue : TupleValue {};

// list values
// ListItem = AtomicValue /
//            ListValue /
//            TupleValue
// ListInterior = ListInterior ListSeparator (TokenSpace)? ListItem 
//                (TokenSpace)? / 
//                (TokenSpace)? ListItem (TokenSpace)?
// ListValue = [ (NewLines)? ListInterior ListEnd (TokenSpace)? ]
struct ListValue;
struct ListValueOpen : LeftBracket {};
struct ListValueClose : RightBracket {};
struct ListValueItem : PEGTL_NS::sor<
    NumberValue,
    IdentifierValue,
    StringValue,
    AssetRefValue,
    ListValue,
    TupleValue> {};
struct ListValue : PEGTL_NS::if_must<
    ListValueOpen,
    PEGTL_NS::pad<
        ListOf<ListValueItem>,
        MultilinePadding>,
    ListValueClose> {};
struct TypedListValue : ListValue {};

// empty list value uses LeftBracket / RightBracket
// rather than ListValueOpen / ListValueClose
// because it doesn't want to execute the
// action semantics on reduction
struct EmptyListValue : PEGTL_NS::seq<
    LeftBracket,
    PEGTL_NS::star<InlinePadding>,
    RightBracket> {};

// dictionary values
// DictionaryKey = String /
//			       Identifier /
//                 Keyword
// KeyValuePair = DictionaryKey Assignment TypedValue
// KeyDictionaryValuePair = DictionaryKey Assignment DictionaryValue
// DictionaryItemTypedValue = Identifier TokenSpace KeyValuePair /
//				              Identifier (TokenSpace)? [ (TokenSpace)? ] 
//				              TokenSpace KeyValuePair
// DictionaryItemDictionaryValue = dictionary TokenSpace KeyDictionaryValuePair
// DictionaryItem = DictionaryItemDictionaryValue /
//                  DictionaryItemTypedValue
// DictionaryInterior = DictionaryInterior StaementSeparator (TokenSpace)? 
//                      DictionaryItem (TokenSpace)? / 
//                      (TokenSpace)? DictionaryItem (TokenSpace)?
// DictionaryValue = { (NewLines)? DictionaryInterior StatementEnd 
//                     (TokenSpace)? }
struct DictionaryValue;
struct DictionaryValueOpen : LeftBrace {};
struct DictionaryValueClose : RightBrace {};
struct DictionaryKey : PEGTL_NS::sor<
    String,
    BaseIdentifier> {};
struct DictionaryType : PEGTL_NS::seq<
    Identifier,
    PEGTL_NS::opt<PEGTL_NS::star<InlinePadding>, ArrayType>> {};
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
    PEGTL_NS::pad_opt<StatementSequenceOf<DictionaryValueItem>,
                      MultilinePadding>,
    DictionaryValueClose> {};

// shared metadata
// MetadataOpen = LeftParen
// MetadataClose = RightParen
struct MetadataOpen : LeftParen {};
struct MetadataClose : RightParen {};

template <typename ItemType>
struct MetadataBlock :PEGTL_NS::if_must<
    MetadataOpen,
    PEGTL_NS::pad_opt<StatementSequenceOf<ItemType>,
                      MultilinePadding>,
    MetadataClose> {};


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
// GeneralListOpMetadata = add TokenSeparator Identifier Assignment 
//                         ListOpMetadataValue /
//                         delete TokenSeparator Identifier Assignment 
//                         ListOpMetadatValue /
//                         append TokenSeparator Identifier Assignment 
//                         ListOpMetadatValue /
//                         prepend TokenSeparator Identifier Assignment 
//                         ListOpMetadatValue /
//                         reorder TokenSeparator Identifier Assignment 
//                         ListOpMetadatValue
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
struct PermissionMetadata : PEGTL_NS::if_must<
    KeywordPermission,
    Assignment,
    Identifier> {};

// SymmetryFunctionMetadata = symmetryFunction Assignment (Identifier)?
struct SymmetryFunctionMetadata : PEGTL_NS::seq<
    KeywordSymmetryFunction,
    Assignment,
    PEGTL_NS::opt<Identifier>> {};

// NameList = String /
//            [ (NewLines)? (TokenSeparator)? String (TokenSeparator)? 
//            (ListSeparator (TokenSeparator)? String (TokenSeparator)?)* 
//            ListEnd (TokenSeparator)? ]
struct NameList : PEGTL_NS::sor<
    String,
    PEGTL_NS::if_must<
        LeftBracket,
        // Should this be optional?
        PEGTL_NS::pad<ListOf<String>, MultilinePadding>,
        RightBracket>> {};

// StringDictionaryItem = String (TokenSeparator)? : (TokenSeparator)? String
struct StringDictionaryItem :
    PEGTL_NS::seq<String, PEGTL_NS::pad<NamespaceSeparator, InlinePadding>,
                  String> {};

// StringDictionary = { (NewLines)? ( (TokenSeparator)? StringDictionaryItem 
// (TokenSeparator)? (ListSeparator (TokenSeparator)? StringDictionaryItem 
// (TokenSeparator)?)* ListEnd)? (TokenSeparator)? }
struct StringDictionaryOpen : LeftBrace {};
struct StringDictionaryClose : RightBrace {};
struct StringDictionary : PEGTL_NS::must<
    StringDictionaryOpen,
    PEGTL_NS::pad_opt<ListOf<StringDictionaryItem>, MultilinePadding>,
    StringDictionaryClose> {};

// time samples
// TimeSample = Number (TokenSeparator)? : (TokenSeparator)? None /
//              Number (TokenSeparator)? : (TokenSeparator)? TypedValue
struct TimeSample : PEGTL_NS::seq<
    Number,
    PEGTL_NS::pad<NamespaceSeparator, InlinePadding>,
    PEGTL_NS::sor<
        KeywordNone,
        TypedValue>> {};

// TimeSampleMap = { (NewLines)? ((TokenSeparator)? TimeSample 
// (TokenSeparator)? (ListSeparator (TokenSeparator)? TimeSample 
// (TokenSeparator)?)* ListEnd)? (TokenSeparator)? }
struct TimeSampleMap : PEGTL_NS::seq<
    LeftBrace,
    PEGTL_NS::pad_opt<ListOf<TimeSample>, MultilinePadding>,
    RightBrace> {};

// splines
    
// SplineCurveTypeItem = BEZIER / HERMITE
struct SplineCurveTypeItem : PEGTL_NS::sor<
    KeywordHermite,
    KeywordBezier> {};

struct SlopeValue : Number {};
// SplineExtrapolationType = NONE / HELD / LINEAR / 
// SLOPED '(' (TokenSeparator)? SloveValue (TokenSeparator)? ')' / 
// LOOP TokenSeparator REPEAT / LOOP TokenSeparator RESET / 
// LOOP TokenSeparator OSCILLATE
struct SplineExtrapolationType : PEGTL_NS::sor<
    KeywordNone_LC,
    KeywordHeld,
    KeywordLinear,
    PEGTL_NS::seq<KeywordSloped, 
                  PEGTL_NS::pad<LeftParen, InlinePadding>, 
                  PEGTL_NS::pad<SlopeValue, InlinePadding>, 
                  RightParen>,
    PEGTL_NS::seq<KeywordLoop, TokenSeparator, KeywordRepeat>,
    PEGTL_NS::seq<KeywordLoop, TokenSeparator, KeywordReset>,
    PEGTL_NS::seq<KeywordLoop, TokenSeparator, KeywordOscillate>> {};

// SplinePreExtrapItem = pre (TokenSeparator)? Colon (TokenSeparator)? 
// SplineExtrapolation
struct SplinePreExtrapItem : PEGTL_NS::seq<
    KeywordPre,
    PEGTL_NS::pad<Colon, InlinePadding>,
    SplineExtrapolationType> {};

// SplinePostExtrapItem = post (TokenSeparator)? Colon (TokenSeparator)? 
// SplineExtrapolation
struct SplinePostExtrapItem : PEGTL_NS::seq<
    KeywordPost,
    PEGTL_NS::pad<Colon, InlinePadding>,
    SplineExtrapolationType> {};

struct SplineLoopItemProtoStart : Number {};
struct SplineLoopItemProtoEnd : Number {};
struct SplineLoopItemNumPreLoops : Number {};
struct SplineLoopItemNumPostLoops : Number {};
struct SplineLoopItemValueOffset : Number {};
// SplineLoopItem = loop (TokenSeparator)? Colon 
// (TokenSeparator)? ( 
// (TokenSeparator)? SplineLoopItemProtoStart (TokenSeparator)?, 
// (TokenSeparator)? SplineLoopItemProtoEnd (TokenSeparator)?, 
// (TokenSeparator)? SplineLoopItemNumPreLoops (TokenSeparator)?, 
// (TokenSeparator)? SplineLoopItemNumPostLoops (TokenSeparator)?, 
// (TokenSeparator)? SplineLoopItemValueOffset (TokenSeparator)? 
// )
struct SplineLoopItem : PEGTL_NS::seq<
    KeywordLoop,
    PEGTL_NS::pad<Colon, InlinePadding>,
    PEGTL_NS::pad<LeftParen, InlinePadding>,
    PEGTL_NS::pad<SplineLoopItemProtoStart, InlinePadding>,
    PEGTL_NS::pad<ListSeparator, InlinePadding>,
    PEGTL_NS::pad<SplineLoopItemProtoEnd, InlinePadding>,
    PEGTL_NS::pad<ListSeparator, InlinePadding>,
    PEGTL_NS::pad<SplineLoopItemNumPreLoops, InlinePadding>,
    PEGTL_NS::pad<ListSeparator, InlinePadding>,
    PEGTL_NS::pad<SplineLoopItemNumPostLoops, InlinePadding>,
    PEGTL_NS::pad<ListSeparator, InlinePadding>,
    PEGTL_NS::pad<SplineLoopItemValueOffset, InlinePadding>,
    RightParen> {};

struct SplineTangentValue : Number {};
struct SplineTangentWidth : Number {};
// Helper rule to parse SplineTangentWithWidth
// SplineTangentWithWidthValue = Number (TokenSeparator)? ListSeparator 
// (TokenSeparator)? Number
struct SplineTangentWithWidthValue : PEGTL_NS::seq<
    SplineTangentWidth,
    PEGTL_NS::pad<ListSeparator, InlinePadding>,
    SplineTangentValue> {};
// SplineTangentWithoutWidthValue = Number (TokenSeparator)? 
// (not at SplineKnotPreValueSeparator)
struct SplineTangentWithoutWidthValue : PEGTL_NS::seq<
    PEGTL_NS::pad<SplineTangentValue, InlinePadding>,
    PEGTL_NS::not_at<ListSeparator>> {};

// SplineTangent = Identifier (TokenSeparator)? ( (TokenSeparator)? 
// SplineTangentWithoutWidthValue (TokenSeparator)? ) /
// Identifier (TokenSeparator)? ( (TokenSeparator)? 
// SplineTangentWithWidthValue (TokenSeparator)? )
struct SplineTangent : PEGTL_NS::sor<
    PEGTL_NS::seq<Identifier, 
                  PEGTL_NS::pad<LeftParen, InlinePadding>,
                  PEGTL_NS::pad<SplineTangentWithoutWidthValue, InlinePadding>,
                  RightParen>,
    PEGTL_NS::seq<Identifier, 
                  PEGTL_NS::pad<LeftParen, InlinePadding>,
                  PEGTL_NS::pad<SplineTangentWithWidthValue, InlinePadding>,
                  RightParen>> {};

// SplineInterpMode = NONE / HELD / LINEAR / CURVE
struct SplineInterpMode : PEGTL_NS::sor<
    KeywordNone_LC,
    KeywordHeld,
    KeywordLinear,
    KeywordCurve> {};

// SplinePreTan = pre TokenSeparator SplineTangent
struct SplinePreTan : PEGTL_NS::seq<
    KeywordPre, 
    TokenSeparator,
    SplineTangent> {};

// SplinePostShaping = post (TokenSeparator)? SplineInterpMode (TokenSeparator)?
// (SplineTangent)?
struct SplinePostShaping : PEGTL_NS::seq<
    KeywordPost,
    PEGTL_NS::pad<SplineInterpMode, InlinePadding>,
    PEGTL_NS::opt<SplineTangent>> {};

// SplineKnotParam = SplinePreTan / SplinePostShaping / DictionaryValue
struct SplineKnotParam : PEGTL_NS::sor<
    SplinePreTan,
    SplinePostShaping,
    DictionaryValue> {};

struct SplineKnotParamSeparator : StatementSeparator {}; 
// SplineKnotParamList = StatementSeparator (TokenSeparator)? 
//  (SplineKnotParam (TokenSeparator)?)* StatementEnd )
struct SplineKnotParamList : PEGTL_NS::opt<
    PEGTL_NS::if_must<
        SplineKnotParamSeparator,
        PEGTL_NS::seq<
            PEGTL_NS::pad<
                StatementSequenceOf<SplineKnotParam>, InlinePadding>,
            PEGTL_NS::not_at<StatementSeparator>>>> {};

struct SplineKnotValue : Number {};
struct SplineKnotPreValue : Number {};
struct SplineKnotPreValueSeparator : Ampersand {};
// SplineKnotValueWithoutPreValue = SplineKnotValue (TokenSeparator)? 
// (not at SplineKnotPreValueSeparator)
struct SplineKnotValueWithoutPreValue : PEGTL_NS::seq<
    SplineKnotValue,
    PEGTL_NS::pad<PEGTL_NS::not_at<SplineKnotPreValueSeparator>, 
        InlinePadding>> {};
// SplineKnotValueWithPreValue = SplineKnotPreValue (TokenSeparator)? 
// SplineKnotPreValueSeparator SplineKnotValue
struct SplineKnotValueWithPreValue : PEGTL_NS::seq<
    SplineKnotPreValue,
    PEGTL_NS::pad<SplineKnotPreValueSeparator, InlinePadding>,
    SplineKnotValue> {};

// SplineKnotValues = SplineKnotValueWithoutPreValue / 
// SplineKnotValueWithPreValue
struct SplineKnotValues : PEGTL_NS::sor<
    SplineKnotValueWithoutPreValue,
    SplineKnotValueWithPreValue> {};

struct SplineKnotTime : Number {};
// SplineKnotItem = Number (TokenSeparator)? : (TokenSeparator)?
// SplineKnotValues (SplineKnotParamList)?
struct SplineKnotItem : PEGTL_NS::seq<
    SplineKnotTime,
    PEGTL_NS::pad<Colon, InlinePadding>,
    SplineKnotValues,
    SplineKnotParamList> {};

// SplineItem = SplineCurveTypeItem / SplinePreExtrapItem / 
// SplinePostExtrapItem / SplineLoopItem / SplineKnotItem
struct SplineItem : PEGTL_NS::sor<
    SplineCurveTypeItem,
    SplinePreExtrapItem,
    SplinePostExtrapItem,
    SplineLoopItem,
    SplineKnotItem> {};

// SplineValue = { (TokenSeparator)? (SplineItem (TokenSeparator)?)* }
struct SplineValue : PEGTL_NS::if_must<
    LeftBrace,
    PEGTL_NS::pad<ListOf<SplineItem>, MultilinePadding>,
    RightBrace> {};

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

// AttributeMetadata = ( (NewLines)? ((TokenSeparator)? AttributeMetadataItem 
// (TokenSeparator)? (StatementSeparator (TokenSeparator)? AttributeMetadataItem 
// (TokenSeparator)?)* StatementEnd)? (TokenSeparator)? )
struct AttributeMetadata : MetadataBlock<AttributeMetadataItem>{};

// prim attribute definition
// AttributeVariability = config / uniform
struct AttributeVariability : PEGTL_NS::sor<
    KeywordConfig,
    KeywordUniform> {};

// AttributeType = (AttributeVariability TokenSeparator)? Identifier 
// (TokenSeparator)? ([ (TokenSeparator)? ])?
struct AttributeType : PEGTL_NS::seq<
    PEGTL_NS::opt<AttributeVariability, TokenSeparator>,
    Identifier,
    PEGTL_NS::opt<PEGTL_NS::star<InlinePadding>, ArrayType>> {};

// AttributeDeclaration = AttributeType TokenSeparator NamespacedName
struct AttributeDeclaration : PEGTL_NS::seq<
    PEGTL_NS::opt<KeywordCustom, TokenSeparator>,
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
//                [ (NewLines)? ((TokenSeparator)? (PathRef) (TokenSeparator)? 
//                (ListSeparator (TokenSeparator)? (PathRef) (TokenSeparator)?)* 
//                ListEnd)? (TokenSeparator)?] 
struct ConnectValue : PEGTL_NS::sor<
    KeywordNone,
    PathRef,
    PEGTL_NS::if_must<
        LeftBracket,
        PEGTL_NS::pad_opt<ListOf<PathRef>, MultilinePadding>,
        RightBracket>> {};

// AttributeSpec = ConnectListOp (parsed as part of PropertySpec)
//                 AttributeDeclaration (AttributeAssignment)? (TokenSeparator)? 
//                 (AttributeMetadata)? /
//                 AttributeDeclaration (TokenSeparator)? Dot (TokenSeparator)? 
//                 TimeSamples Assignment TimeSampleMap /
//                 AttributeDeclaration (TokenSeparator)? Dot (TokenSeparator)? 
//                 Connect Assignment ConnectValue
struct AttributeSpec : PEGTL_NS::seq<
    AttributeDeclaration,
    PEGTL_NS::if_must_else<
        PEGTL_NS::pad<Sdf_PathParser::Dot, InlinePadding>,
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
                    ConnectValue>>,
            PEGTL_NS::if_must<
                KeywordSpline,
                PEGTL_NS::seq<
                    Assignment,
                    SplineValue>>>,
        PEGTL_NS::seq<
            AttributeAssignmentOptional,
            PEGTL_NS::pad_opt<AttributeMetadata, InlinePadding>
        >>
    > {};

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

// RelationshipMetadataList = RelationshipMetadataList StatementSeparator 
// (TokenSeparator)? RelationshipMetadataItem (TokenSeparator)? /
//                         (TokenSeparator)? RelationshipMetadataItem 
//                         (TokenSeparator)?
// RelationshipMetadata = ( (NewLines)? (RelationshipMetadataList StatementEnd)? 
// (TokenSeparator)? 
struct RelationshipMetadata : MetadataBlock<RelationshipMetadataItem>{};

// prim relationship definition
// RelationshipAssignment = Assignment None /
//                          Assignment PathRef /
//                          Assignment [ ((NewLines)? ((TokenSeparator)? PathRef 
//                          (TokenSeparator)? (ListSeparator (TokenSeparator)? 
//                          PathRef (TokenSeparator)?)* ListEnd)? 
//                          (TokenSeparator)?)? ]
struct RelationshipAssignmentOpen : LeftBracket {};
struct RelationshipAssignmentClose : RightBracket {};
struct RelationshipAssignment : PEGTL_NS::seq<
    Assignment,
    PEGTL_NS::sor<
        KeywordNone,
        PathRef,
        PEGTL_NS::if_must<
            RelationshipAssignmentOpen,
            PEGTL_NS::pad_opt<ListOf<PathRef>,
                              MultilinePadding>,
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
//                    RelationshipType TokenSeparator NamespacedName
//                    (RelationshipAssignment)? (TokenSeparator)?
//                    (RelationshipMetadata)? /
//                    RelationshipType TokenSeparator NamespacedName
//                    (TokenSeparator)? [ (TokenSeparator)? PathRef
//                    (TokenSeparator)? ] /
//                    RelationshipType TokenSeparator NamespacedName
//                    (TokenSeparator)? . (TokenSeparator)? timeSamples
//                    Assignment TimeSampleMap / RelationshipType TokenSeparator
//                    NamespacedName (TokenSeparator)? . (TokenSeparator)?
//                    default Assignment PathRef
struct RelationshipTargetOpen : LeftBracket {};
struct RelationshipTargetClose : RightBracket {};
struct RelationshipAssignmentOptional : PEGTL_NS::opt<
    RelationshipAssignment> {};
struct RelationshipSpec : PEGTL_NS::seq<
    RelationshipType,
    TokenSeparator,
    NamespacedName,
    PEGTL_NS::if_must_else<
        PEGTL_NS::pad<Sdf_PathParser::Dot, InlinePadding>,
        PEGTL_NS::sor<
            PEGTL_NS::if_must<
                KeywordDefault,
                PEGTL_NS::seq<
                    Assignment,
                    PathRef>>>,
        PEGTL_NS::sor<
            PEGTL_NS::seq<
                PEGTL_NS::star<InlinePadding>,
                PEGTL_NS::if_must<
                    RelationshipTargetOpen,
                    PEGTL_NS::pad<PathRef, InlinePadding>,
                    RelationshipTargetClose>>,
            PEGTL_NS::seq<
                RelationshipAssignmentOptional,
                PEGTL_NS::star<InlinePadding>,
                PEGTL_NS::opt<RelationshipMetadata>>>>> {};

// prim metadata
// InheritsOrSpecializesList = None /
//                             PathRef /
//                             [ (NewLines)? ( (TokenSeparator)? PathRef
//                             (TokenSeparator)? (ListSeparator
//                             (TokenSeparator)? PathRef (TokenSeparator)?)*
//                             ListEnd)? (TokenSeparator)? ]
struct InheritsOrSpecializesList : PEGTL_NS::sor<
    KeywordNone,
    PathRef,
    PEGTL_NS::seq<
        LeftBracket,
        PEGTL_NS::pad_opt<ListOf<PathRef>,
                          MultilinePadding>,
        RightBracket>> {};

// SpecializesMetadata = specializes Assignment InheritsOrSpecializesList /
//                       add specializes Assignment InheritsOrSpecializesList /
//                       delete specializes Assignment InheritsOrSpecializesList
//                       / append specializes Assignment
//                       InheritsOrSpecializesList / prepend specializes
//                       Assignment InheritsOrSpecializesList / reorder
//                       specializes Assignment InheritsOrSpecializesList
struct SpecializesMetadata : PEGTL_NS::if_must<
    KeywordSpecializes,
    Assignment,
    InheritsOrSpecializesList> {};

// InheritsMetadata = inherits Assignment InheritsOrSpecializesList /
//                    add inherits Assignment InheritsOrSpecializesList /
//                    delete inherits Assignment InheritsOrSpecializesList /
//                    append inherits Assignment InheritsOrSpecializesList /
//                    prepend inherits Assignment InheritsOrSpecializesList /
//                    reorder inherits Assignment InheritsOrSpecializesList
struct InheritsMetadata : PEGTL_NS::if_must<
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

// ReferenceParameterList = (TokenSeparator)? ReferenceParameter
// (TokenSeparator)? (StatementSeparator (TokenSeparator)? ReferenceParameter
// (TokenSeparator)?)* ReferenceParameters = ( (NewLines)?
// (ReferenceParameterList StatementEnd)? (TokenSeparator)? ) ReferenceListItem
// = AssetRef (TokenSeparator)? (PathRef)? (TokenSeparator)?
// (ReferenceParameters)? /
//			           PathRef (TokenSeparator)?
//(ReferenceParameters)?
struct ReferenceParametersOpen : LeftParen {};
struct ReferenceParametersClose : RightParen {};
struct ReferenceListItem : PEGTL_NS::seq<
    PEGTL_NS::sor<
        PathRef,
        PEGTL_NS::seq<
            AssetRef,
            PEGTL_NS::star<InlinePadding>,
            PEGTL_NS::opt<PathRef>>
        >,
    PEGTL_NS::star<InlinePadding>,
    PEGTL_NS::opt<
        ReferenceParametersOpen,
        PEGTL_NS::pad_opt<StatementSequenceOf<ReferenceParameter>,
                          MultilinePadding>,
        ReferenceParametersClose>
    > {};

// 	ReferenceListItems = (TokenSeparator)? ReferenceListItem
// (TokenSeparator)? (ListSeparator (TokenSeparator)? ReferenceListItem
// (TokenSeparator)?)*
//	ReferenceList = None /
//			        ReferenceListItem /
//			        [ (NewLines)? (ReferenceListItems ListEnd)?
//(TokenSeparator)? ]
struct ReferenceList : PEGTL_NS::sor<
    KeywordNone,
    ReferenceListItem,
    PEGTL_NS::seq<
        LeftBracket,
        PEGTL_NS::pad_opt<ListOf<ReferenceListItem>, MultilinePadding>,
        RightBracket>> {};

// ReferencesMetadata = references Assignment ReferenceList /
//                      add references Assignment ReferenceList /
//                      delete references Assignment ReferenceList /
//                      append references Assignment ReferenceList /
//                      prepend references Assignment ReferenceList /
//                      reorder references Assignment ReferenceList
struct ReferencesMetadata : PEGTL_NS::if_must<
    KeywordReferences,
    Assignment,
    ReferenceList> {};

// PayloadParameters = ( (NewLines)? ((TokenSeparator)? LayerOffset
// (TokenSeparator)? (StatementSeparator (TokenSeparator)? LayerOffset
// (TokenSeparator)?)* StatementEnd)? (TokenSeparator)? ) PayloadListItem =
// AssetRef (TokenSeparator)? (PathRef)? (TokenSeparator)? (PayloadParameters)?
// /
//                   PathRef (TokenSeparator)? (PayloadParamaters)?
struct PayloadListItem : PEGTL_NS::seq<
    PEGTL_NS::sor<
        PathRef,
        PEGTL_NS::seq<
            AssetRef,
            PEGTL_NS::star<InlinePadding>,
            PEGTL_NS::opt<PathRef>>
        >,
    PEGTL_NS::star<InlinePadding>,
    PEGTL_NS::opt<
        LeftParen,
        PEGTL_NS::pad_opt<StatementSequenceOf<LayerOffset>,
                           MultilinePadding>,
        RightParen>> {};

// PayloadListItems = (TokenSeparator)? PayloadListItem (TokenSeparator)?
// (ListSeparator (TokenSeparator)? PayloadListItem (TokenSeparator)?)*
// PayloadList = None /
//               PayloadListItem /
//               [ (NewLines)? (PayloadListItems ListEnd)? (TokenSeparator)? ]
struct PayloadList : PEGTL_NS::sor<
    KeywordNone,
    PayloadListItem,
    PEGTL_NS::seq<
        LeftBracket,
        PEGTL_NS::pad_opt<ListOf<PayloadListItem>, MultilinePadding>,
        RightBracket>> {};

// PayloadMetadata = payload Assignment PayloadList /
//                   add payload Assignment PayloadList /
//                   delete payload Assignment PayloadList /
//                   append payload Assignment PayloadList /
//                   prepend payload Assignment PayloadList /
//                   reorder payload Assignment PayloadList
struct PayloadMetadata : PEGTL_NS::if_must<
    KeywordPayload,
    Assignment,
    PayloadList> {};

// RelocatesItem = PathRef (TokenSeparator)? : (TokenSeparator)? PathRef
// RelocatesMetadata = relocates Assignment { (NewLines)? ((TokenSeparator)?
// RelocatesItem (TokenSeparator)? (ListSeparator (TokenSeparator)?
// RelocatesItem (TokenSeparator)?)* ListEnd)? (TokenSeparator)? }
struct RelocatesMapOpen : LeftBrace {};
struct RelocatesMapClose : RightBrace {};
struct RelocatesMetadata : PEGTL_NS::if_must<
    KeywordRelocates,
    Assignment,
    PEGTL_NS::must<
        RelocatesMapOpen,
        PEGTL_NS::pad_opt<
            ListOf<
                PEGTL_NS::seq<
                    PathRef,
                    PEGTL_NS::pad<NamespaceSeparator, InlinePadding>,
                    PathRef>>,
            MultilinePadding>,
        RelocatesMapClose>> {};

// VariantsMetadata = variants Assignment DictionaryValue
struct VariantsMetadata : PEGTL_NS::if_must<
    KeywordVariants,
    Assignment,
    DictionaryValue> {};

// VariantSetsMetadata = variantSets Assignment NameList /
//                       add variantSets Assignment NameList /
//                       delete variantSets Assignment NameList /
//                       append variantSets Assignment NameList /
//                       prepend variantSets Assignment NameList /
//                       reorder variantSets Assignment NameList
struct VariantSetsMetadata : PEGTL_NS::if_must<
    KeywordVariantSets,
    Assignment,
    NameList> {};

// KindMetadata = kind Assignment String
struct KindMetadata : PEGTL_NS::if_must<
    KeywordKind,
    Assignment,
    String> {};

// PrefixOrSuffixSubstitutionsMetadata = prefixSubstitutions Assignment
// StringDictionary /
//                                       suffixSubstitutions Assignment
//                                       StringDictionary
struct PrefixOrSuffixSubstitutionsMetadata : PEGTL_NS::if_must<
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
    SharedMetadata,
    KindMetadata,
    ReferencesMetadata,
    PayloadMetadata,
    VariantsMetadata,
    VariantSetsMetadata,
    InheritsMetadata,
    SpecializesMetadata,
    RelocatesMetadata,
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
    PrefixOrSuffixSubstitutionsMetadata,
    PermissionMetadata,
    SymmetryFunctionMetadata> {};

// PrimMetadata = (NewLines)? (TokenSeparator)? (( (NewLines)?
// ((TokenSeparator)? PrimMetadataItem (TokenSeparator)? (StatementSeparator
// (TokenSeparator)? PrimMetadatItem (TokenSeparator)?)* StatementEnd)?
// (TokenSeparator)? ))?
struct PrimMetadata : MetadataBlock<PrimMetadataItem> {};

// prim definition
// PropertySpec = AttributeSpec /
//                RelationshipSpec
// ConnectListOp = add TokenSeparator AttributeDeclaration (TokenSeparator)? .
// (TokenSeparator)? connect Assignment ConnectValue /
//                 delete TokenSeparator AttributeDeclaration (TokenSeparator)?
//                 . (TokenSeparator)? connect Assignment ConnectValue / append
//                 TokenSeparator AttributeDeclaration (TokenSeparator)? .
//                 (TokenSeparator)? connect Assignment ConnectValue / prepend
//                 TokenSeparator AttributeDeclaration (TokenSeparator)? .
//                 (TokenSeparator)? connect Assignment ConnectValue / reorder
//                 TokenSeparator AttributeDeclaration (TokenSeparator)? .
//                 (TokenSeparator)? connect Assignment ConnectValue
// RelationshipListOp = add TokenSeparator RelationshipType TokenSeparator
// NamespacedName (RelationshipAssignment)? /
//                      delete TokenSeparator RelationshipType TokenSeparator
//                      NamespaceName (RelationshipAssignment)? / append
//                      TokenSeparator RelationshipType TokenSeparator
//                      NamespaceName (RelationshipAssignment)? / prepend
//                      TokenSeparator RelationshipType TokenSeparator
//                      NamespaceName (RelationshipAssignment)? / reorder
//                      TokenSeparator RelationshipType TokenSeparator
//                      NamespaceName (RelationshipAssignment)?
// Note this is not a direct translation - in order to greedily optimize
// we take note that attribute specs can contain connect list ops
// so we separate out list ops for the two specs
// and parse them separately as an additional production
// additionally, the keyword 'reorder' can start either list op
// statements or the child / property order ones, so we have to greedily take
// that into account here as well (which aren't truly PropertySpecs but captured
// here for optimization)
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
                PEGTL_NS::pad<Sdf_PathParser::Dot, InlinePadding>,
                KeywordConnect,
                Assignment,
                ConnectValue>>>> {};

// ChildOrderStatement = reorder TokenSeparator nameChildren Assignment NameList
// PropertyOrderStatement = reorder TokenSeparator properties Assignment
// NameList
struct ChildOrPropertyOrderStatement : PEGTL_NS::seq<
    KeywordReorder,
    TokenSeparator,
    PEGTL_NS::sor<
        KeywordNameChildren,
        KeywordProperties>,
    Assignment,
    NameList> {};

// 	VariantStatement = String (TokenSeparator)? (PrimMetadata)? (NewLines)?
// (TokenSeparator)? { (NewLines)? (TokenSeparator)? (PrimContentsList)?
// (TokenSeparator)? } (NewLines)?
struct PrimContents;
struct VariantStatementOpen : LeftBrace {};
struct VariantStatementClose : RightBrace {};
struct VariantStatement : PEGTL_NS::seq<
    String,
    PEGTL_NS::pad_opt<PrimMetadata, MultilinePadding>,
    PEGTL_NS::must<
        VariantStatementOpen,
        PrimContents,
        VariantStatementClose>
    > {};

// 	VariantSetStatement = variantSet TokenSeparator String Assignment
// (NewLines)? (TokenSeparator)? { (NewLines)? ((TokenSeparator)?
// VariantStatement)+ (TokenSeparator)? }
struct VariantStatementListOpen : LeftBrace {};
struct VariantStatementListClose : RightBrace {};
struct VariantSetStatement : PEGTL_NS::seq<
    KeywordVariantSet,
    TokenSeparator,
    String,
    PEGTL_NS::pad<Equals, InlinePadding, MultilinePadding>,
    PEGTL_NS::must<
        VariantStatementListOpen,
        PEGTL_NS::star<PEGTL_NS::pad<VariantStatement, MultilinePadding>>,
        VariantStatementListClose>> {};

// PrimItem = ChildOrderStatement /
//		      PropertyOrderStatement /
//		      PropertySpec /
//            PrimSpec /
//		      VariantSetStatement
struct PrimSpec;
struct PrimItem : PEGTL_NS::sor<
    PEGTL_NS::seq<
        PEGTL_NS::sor<ChildOrPropertyOrderStatement, PropertySpec>,
        PEGTL_NS::star<SinglelinePadding>,
        StatementSeparator>,
    PEGTL_NS::seq<
        PEGTL_NS::sor<PrimSpec, VariantSetStatement>,
        PEGTL_NS::star<SinglelinePadding>,
        Eol>>{};

// PrimContents = (NewLines)? ((TokenSeparator)? PrimItem)*
struct PrimContents :
    PEGTL_NS::seq<
        PEGTL_NS::star<MultilinePadding>,
        PEGTL_NS::star<PrimItem, PEGTL_NS::star<MultilinePadding>>> {};

// PrimTypeName = Identifier ((TokenSeparator)? . (TokenSeparator)? Identifier)* 
struct PrimTypeName :
    PEGTL_NS::list<Identifier, Sdf_PathParser::Dot, InlinePadding> {};

// PrimSpec = def TokenSeparator (PrimTypeName TokenSeparator)? String
// (TokenSeparator)? (PrimMetadata)? (NewLines)? (TokenSeparator)? {
// PrimContents (TokenSeparator)? } /
//            over TokenSeparator (PrimTypeName TokenSeparator)? String
//            (TokenSeparator)? (PrimMetadata)? (NewLines)? (TokenSeparator)? {
//            PrimContents (TokenSeparator)? } / class TokenSeparator
//            (PrimTypeName TokenSeparator)? String (TokenSeparator)?
//            (PrimMetadata)? (NewLines)? (TokenSeparator)? { PrimContents
//            (TokenSeparator)? }
struct PrimMetadataOptional : 
    PEGTL_NS::pad_opt<PrimMetadata, MultilinePadding> {};
struct PrimSpec : PEGTL_NS::seq<
    PEGTL_NS::sor<
        KeywordDef,
        KeywordOver,
        KeywordClass>,
    TokenSeparator,
    PEGTL_NS::opt<PrimTypeName, TokenSeparator>,
    String,
    PrimMetadataOptional,
    LeftBrace,
    PEGTL_NS::must<
        PrimContents,
        RightBrace>
    > {};

// LayerOffsetList = ( (TokenSeparator)? LayerOffset (TokenSeparator)?
// (StatementSeparator (TokenSeparator)? LayerOffset (TokenSeparator)?)*
// StatementEnd ) SublayerItem = AssetRef (TokenSeparator)? (LayerOffsetList)?
struct SublayerItem : PEGTL_NS::seq<
    AssetRef,
    PEGTL_NS::star<InlinePadding>,
    PEGTL_NS::opt<
        LeftParen,
        PEGTL_NS::pad_opt<StatementSequenceOf<LayerOffset>, MultilinePadding>,
        PEGTL_NS::must<RightParen>>
    > {};

// SublayerMetadata = subLayers Assignment [ (NewLines)? ((TokenSeparator)?
// SublayerItem (TokenSeparator)? (ListSeparator (TokenSeparator)? SublayerItem
// (TokenSeparator)?)* ListEnd)? (TokenSeparator)? ]
struct SublayerListOpen : LeftBracket {};
struct SublayerListClose : RightBracket {};
struct SublayerMetadata : PEGTL_NS::seq<
    KeywordSubLayers,
    Assignment,
    SublayerListOpen,
    PEGTL_NS::pad_opt<ListOf<SublayerItem>, MultilinePadding>,
    PEGTL_NS::must<SublayerListClose>> {};

// LayerMetadataItem = SharedMetadata /
//                     SublayerMetadata
struct LayerMetadataItem : PEGTL_NS::sor<
    LayerSharedWithListOpMetadata,
    RelocatesMetadata,
    SublayerMetadata> {};

// LayerMetadata = (NewLines)? (TokenSeparator)? ( ( (NewLines)?
// (TokenSeparator)? LayerMetadataItem (TokenSeparator)? (StatementSeparator
// (TokenSeparator)? LayerMetadataItem (TokenSeparator)?)* StatementEnd)? ) )?
// (NewLines)?
struct LayerMetadata : MetadataBlock<LayerMetadataItem> {};

// LayerHeader = # (!(CrLf) AnyChar)*
struct LayerHeader :
    PEGTL_NS::must<PEGTL_NS::one<'#'>, PEGTL_NS::until<Eolf>>{};

// PrimList = PrimList NewLines (TokenSeparator)? PrimStatement
// (TokenSeparator)? /
//		      (TokenSeparator)? PrimStatement (TokenSeparator)?
// LayerSpec = LayerHeader (LayerMetadata)? (NewLines)? (TokenSeparator PrimList
// NewLines)? (EolWhitspace)?
struct LayerSpec : PEGTL_NS::seq<
    LayerHeader,
    PEGTL_NS::pad_opt<LayerMetadata, MultilinePadding>,
    PEGTL_NS::star<
        PEGTL_NS::sor<
            PEGTL_NS::seq<
                KeywordReorder,
                TokenSeparator,
                KeywordRootPrims,
                Assignment,
                NameList,
                PEGTL_NS::star<SinglelinePadding>,
                PEGTL_NS::sor<StatementSeparator, PEGTL_NS::eof>
            >,
            PEGTL_NS::seq<
                PrimSpec,
                PEGTL_NS::star<SinglelinePadding>,
                Eolf
            >,
        MultilinePadding>
    >> {};

// LayerMetadataOnly = LayerHeader (LayerMetadata)?
// production used to interrogate layer metadata without reading the entire
// layer
struct LayerMetadataOnly : PEGTL_NS::seq<
    LayerHeader, 
    PEGTL_NS::pad_opt<LayerMetadata, MultilinePadding>> {};

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
