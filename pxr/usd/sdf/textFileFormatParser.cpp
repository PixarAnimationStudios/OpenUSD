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

#include "pxr/pxr.h"
#include "pxr/base/trace/trace.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/sdf/textParserContext.h"
#include "pxr/usd/sdf/textParserHelpers.h"
#include "pxr/usd/sdf/textFileFormatParser.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_TextFileFormatParser {

////////////////////////////////////////////////////////////////////////
// TextFileFormat customized errors
template<> constexpr auto errorMessage<Digit> = "expected number";
template<> constexpr auto errorMessage<SingleQuote> = "expected '";
template<> constexpr auto errorMessage<DoubleQuote> = "expected \"";
template<> constexpr auto errorMessage<At> = "expected @";
template<> constexpr auto errorMessage<LeftCurlyBrace> = "expected {";
template<> constexpr auto errorMessage<RightCurlyBrace> = "expected }";
template<> constexpr auto errorMessage<LeftAngleBracket> = "expected <";
template<> constexpr auto errorMessage<RightAngleBracket> = "expected >";
template<> constexpr auto errorMessage<LeftBrace> = "expected [";
template<> constexpr auto errorMessage<RightBrace> = "expected ]";
template<> constexpr auto errorMessage<LeftParen> = "expected (";
template<> constexpr auto errorMessage<RightParen> = "expected )";
template<> constexpr auto errorMessage<ListValueClose> = "expected ]";
template<> constexpr auto errorMessage<TupleValueClose> = "expected )";
template<> constexpr auto errorMessage<DictionaryValueClose> = "expected }";
template<> constexpr auto errorMessage<TokenSpace> = "expected space";
template<> constexpr auto errorMessage<Sdf_PathParser::Path> = "expected Path";
template<> constexpr auto errorMessage<PathRef> = "expected Path reference";
template<> constexpr auto errorMessage<Utf8SingleQuoteCharacter> =
    "expected sequence of non-CRLF UTF-8 encoded characters followed by '";
template<> constexpr auto errorMessage<Utf8SingleQuoteMultilineCharacter> =
    "expected sequence of UTF-8 encoded characters followed by '";
template<> constexpr auto errorMessage<Utf8DoubleQuoteCharacter> =
    "expected sequence of non-CRLF UTF-8 encoded characters followed by \"";
template<> constexpr auto errorMessage<Utf8DoubleQuoteMultilineCharacter> =
    "expected sequence of UTF-8 encoded characters followed by \"";
template<> constexpr auto errorMessage<Utf8AssetPathCharacter> =
    "expected sequence of non-CRLF UTF-8 encoded characters followed by @";
template<> constexpr auto errorMessage<Utf8AssetPathEscapedCharacter> =
    "expected sequence of non-CRLF UTF-8 encoded characters followed by @@@";
template<> constexpr auto errorMessage<ListValueInterior> =
    "expected one or more simple, tuple, or list values separated by ,";
template<> constexpr auto errorMessage<TupleValueInterior> =
    "expected on or more simple or tuple values separated by ,";
template<> constexpr auto errorMessage<DictionaryKey> =
    "expected string or identifier";
template<> constexpr auto errorMessage<Assignment> = "expected =";
template<> constexpr auto errorMessage<DictionaryValue> =
    "expected dictionary value";
template<> constexpr auto errorMessage<TypedValue> =
    "expected simple, tuple, list, or path ref value";
template<> constexpr auto errorMessage<TimeSamplesEnd> = "expected }";
template<> constexpr auto errorMessage<TimeSample> = 
    "expected number or identifier followed by"
    " : followed by None or typed value";
template<> constexpr auto errorMessage<DocString> =
    "expected string value";
template<> constexpr auto errorMessage<PermissionIdentifier> =
    "expected identifier";
template<> constexpr auto errorMessage<NameListEnd> = "expected ]";
template<> constexpr auto errorMessage<PrimMetadataListOpList> =
    "expected None or list value";
template<> constexpr auto errorMessage<PrimAttributeMetadataListOpList> =
    "expected None or list value";
template<> constexpr auto errorMessage<PrimAttributeMetadataValue> =
    "expected None, simple, tuple, list, dictionary, or path ref value";
template<> constexpr auto 
errorMessage<PrimAttributeMetadataDisplayUnitIdentifier> =
    "expected identifier";
template<> constexpr auto errorMessage<PrimAttributeMetadataListInterior> =
    "expected sequence of metadata items separated by ,";
template<> constexpr auto errorMessage<ReferenceList> =
    "expected None, single layer / path ref, or list of layer / path refs";
template<> constexpr auto errorMessage<PrimAttributeValue> =
    "expected None, simple, tuple, list, or path ref value";
template<> constexpr auto errorMessage<PrimAttributeFullType> =
    "expected attribute type";
template<> constexpr auto errorMessage<PrimAttributeFallbackNamespacedName> =
    "expected namespaced identifier";
template<> constexpr auto errorMessage<PrimAttributeConnectName> =
    "expected namespaced name followed by . followed by connect";
template<> constexpr auto errorMessage<PrimAttributeConnectList> =
    "expected list of path refs separated by ,";
template<> constexpr auto errorMessage<PrimAttributeConnectValue> =
    "expected None, path ref, or list of path refs";
template<> constexpr auto errorMessage<PrimAttributeAddConnectValue> =
    "expected None, path ref, or list of path refs";
template<> constexpr auto errorMessage<PrimAttributeDeleteConnectValue> =
    "expected None, path ref, or list of path refs";
template<> constexpr auto errorMessage<PrimAttributeAppendConnectValue> =
    "expected None, path ref, or list of path refs";
template<> constexpr auto errorMessage<PrimAttributePrependConnectValue> =
    "expected None, path ref, or list of path refs";
template<> constexpr auto errorMessage<PrimAttributeReorderConnectValue> =
    "expected None, path ref, or list of path refs";
template<> constexpr auto errorMessage<PrimAttributeTimeSamplesValue> =
    "expected list of time samples separated by ,";
template<> constexpr auto errorMessage<PrimRelationshipMetadataListOpList> =
    "expected None or list value";
template<> constexpr auto errorMessage<PrimRelationshipMetadataValue> =
    "expected None, simple, tuple, list, dictionary, or path ref value";
template<> constexpr auto errorMessage<PrimRelationshipMetadataListInterior> =
    "expected sequence of metadata items separated by ,";
template<> constexpr auto errorMessage<PrimRelationshipTarget> =
    "expected path reference";
template<> constexpr auto errorMessage<PrimRelationshipTargetNone> =
    "expected None or []";
template<> constexpr auto errorMessage<PrimRelationshipTargetList> =
    "expected list of path refs separated by ,";
template<> constexpr auto errorMessage<LayerRefOffsetValue> =
    "expected number";
template<> constexpr auto errorMessage<LayerRefScaleValue> =
    "expected number";
template<> constexpr auto errorMessage<LayerMetadataList> =
    "expected sequence of metadata items separated by ,";
template<> constexpr auto errorMessage<SublayerList> =
    "expected empty list or sequence of sublayer statements separated by ,";
template<> constexpr auto errorMessage<LayerOffsetList> =
    "expected sequence of layer offset statements separated by ,";
template<> constexpr auto errorMessage<LayerMetadataValue> =
    "expected None, simple, tuple, list, dictionary, or path ref value";
template<> constexpr auto errorMessage<LayerMetadataListOpList> =
    "expected None or list value";
template<> constexpr auto errorMessage<PrimReorderNameList> =
    "expected string or list";
template<> constexpr auto errorMessage<PrimTypeName> =
    "expected identifier";
template<> constexpr auto errorMessage<PrimStatementInterior> =
    "expected identifier followed by prim interior";
template<> constexpr auto errorMessage<NameList> =
    "expected string or list";
template<> constexpr auto errorMessage<PrimVariantSetName> =
    "expected string";
template<> constexpr auto errorMessage<VariantList> =
    "expected list of variants";
template<> constexpr auto errorMessage<KindValue> =
    "expected string";
template<> constexpr auto errorMessage<PayloadListInterior> =
    "expected sequence of layer or path references separated by ,";
template<> constexpr auto errorMessage<PayloadList> =
    "expected None, empty list, or list of layer or path references";
template<> constexpr auto errorMessage<InheritListInterior> =
    "expected sequence of layer or path references separated by ,";
template<> constexpr auto errorMessage<InheritList> =
    "expected None, empty list, or list of path references";
    template<> constexpr auto errorMessage<SpecializesListInterior> =
    "expected sequence of layer or path references separated by ,";
template<> constexpr auto errorMessage<SpecializesList> =
    "expected None, empty list, or list of path references";
template<> constexpr auto errorMessage<ReferenceListInterior> =
    "expected sequence of layer or path references separated by ,";
template<> constexpr auto errorMessage<NamespaceSeparator> =
    "expected :";
template<> constexpr auto errorMessage<RelocatesMap> =
    "expected dictionary or list of relocates statements";
template<> constexpr auto errorMessage<StringDictionary> =
    "expected dictionary of strings";

template<> constexpr bool emitRule<EolWhitespace> = false;

////////////////////////////////////////////////////////////////////////
// TextFileFormat actions

////////////////////////////////////////////////////////////////////////
// Shared actions

template <>
struct TextParserAction<MetadataListOpList>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // when a MetadataListOpList is consumed, it can
        // either be a None value or a ListValue
        // if it was a ListValue, the specialized action
        // for ListValue picked it up, so we only need
        // to handle None here
        constexpr std::string_view none {"None"};
        std::string value = TfStringTrim(in.string());
        if (value == none)
        {
            // if the value is None, set the string
            // being recorded to None
            context.currentValue = VtValue();
            if (context.values.IsRecordingString())
            {
                context.values.SetRecordedString(std::string(none));
            }
        }
    }
};

template <>
struct TextParserAction<NameListItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string value = _GetEvaluatedStringFromString(
            in.string(), context);
        context.nameVector.push_back(TfToken(value));
    }  
};

template <>
struct TextParserAction<TimeSamplesBegin>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.timeSamples = SdfTimeSampleMap();
    }
};

template <>
struct TextParserAction<TimeSampleExtendedNumber>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        Sdf_ParserHelpers::Value value = _GetValueFromString(
            in.string(), in.position().line, context);
        context.timeSampleTime = value.Get<double>();
    }
};

template <>
struct TextParserAction<TimeSampleExtendedNumberValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.timeSamples[context.timeSampleTime] = context.currentValue;
    }
};

template <>
struct TextParserAction<TimeSampleExtendedNumberNone>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.timeSamples[context.timeSampleTime] = VtValue(SdfValueBlock());
    }
};

template <>
struct TextParserAction<DocString>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->Documentation, 
            _GetEvaluatedStringFromString(in.string(), context), context);
    }
};

template <>
struct TextParserAction<PermissionIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        SdfPermission permission;
        if (!_GetPermissionFromString(in.string(), permission))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "'%s' is not a valid permission constant",
                in.string().c_str());
        }

        _SetField(context.path, SdfFieldKeys->Permission,
            permission, context);
    }
};

template <>
struct TextParserAction<SymmetryFunctionIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->SymmetryFunction,
            TfToken(_GetEvaluatedStringFromString(in.string(), context)),
            context);
    }
};

template <>
struct TextParserAction<SymmetryFunctionEmpty>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->SymmetryFunction,
            TfToken(), context);
    }
};

template <>
struct TextParserAction<StringValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string evaluatedString = _GetEvaluatedStringFromString(
            in.string(), context);
        _ValueAppendAtomic(evaluatedString, context);
    }
};

template <>
struct TextParserAction<IdentifierValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // the ParserValueContext needs identifiers to be stored
        // as TfToken instead of std::string to be able to distinguish
        // between them
        _ValueAppendAtomic(TfToken(in.string()), context);
    }
};

template <>
struct TextParserAction<NumberValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // super special case for negative 0
        // we have to store this as a double to preserve
        // the sign.  There is no negative zero integral
        // value, and we don't know at this point
        // what the final stored type will be.
        Sdf_ParserHelpers::Value value = _GetValueFromString(
            in.string(), in.position().line, context);
        _ValueAppendAtomic(value, context);
    }
};

template <>
struct TextParserAction<AssetRefValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // the ParserValueContext needs asset paths to be stored
        // as SdfAssetPath instead of std::string to be able to
        // distinguish between them
        std::string evaluatedAssetPath = _GetAssetRefFromString(in.string());
        _ValueAppendAtomic(SdfAssetPath(evaluatedAssetPath), context);
    }
};

template <>
struct TextParserAction<TupleValueOpen>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.values.BeginTuple();
    }
};

template <>
struct TextParserAction<TupleValueClose>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.values.EndTuple();
    }
};

template <>
struct TextParserAction<ListValueOpen>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.values.BeginList();
    }
};

template <>
struct TextParserAction<ListValueClose>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.values.EndList();
    }
};

template <>
struct TextParserAction<DictionaryValueOpen>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _DictionaryBegin(context);
    }
};

template <>
struct TextParserAction<DictionaryValueClose>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _DictionaryEnd(context);
    }
};

template <>
struct TextParserAction<TypedValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // we have to evaluate values at a higher level
        // for the final value set
        std::string errorMessage;
        std::string value = TfStringTrimLeft(in.string());
        if (TfStringStartsWith(value, "("))
        {
            // this is a list value and we are completely
            // finished reducing it
            if (!_ValueSetTuple(context, errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    "%s",
                    errorMessage.c_str());
            }
        }
        else if (TfStringStartsWith(value, "["))
        {
            // this is either a list or an empty array type
            // array type matches '[' and ']' with an
            // arbitrary number of spaces / tabs in between
            constexpr std::string_view arrayType {"[]"};
            std::string collapsedArrayType = TfStringReplace(value, " ", "");
            collapsedArrayType = TfStringReplace(value, "\t", "");
            if (collapsedArrayType != arrayType)
            {
                // it's a list
                if (!_ValueSetList(context, errorMessage))
                {
                    Sdf_TextFileFormatParser_Err(
                        context,
                        in.input(),
                        in.position(),
                        "%s",
                        errorMessage.c_str());
                }
            }
            else
            {
                // it's an array type
                // Set the recorded string on the ParserValueContext. Normally
                // 'values' is able to keep track of the parsed string, but in this
                // case it doesn't get the BeginList() and EndList() calls so the
                // recorded string would have been "". We want "[]" instead.
                if (context.values.IsRecordingString()) {
                    context.values.SetRecordedString("[]");
                }

                if(!_ValueSetShaped(context, errorMessage))
                {
                    Sdf_TextFileFormatParser_Err(
                        context,
                        in.input(),
                        in.position(),
                        "%s",
                        errorMessage.c_str());
                }
            }
        }
        else if (TfStringStartsWith(value, "<"))
        {
            std::string pathRef = TfStringTrimRight(value);
            std::string evaluatedPath = Sdf_EvalQuotedString(
                pathRef.c_str(), pathRef.length(), 1);
            _ValueSetCurrentToSdfPath(evaluatedPath, context);
        }
        else
        {
            // this is the atomic value and we are completely
            // finished reducing it
            if (!_ValueSetAtomic(context, errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    "%s",
                    errorMessage.c_str());
            }
        }
    }
};

template <>
struct TextParserAction<DictionaryElementDictionaryValueAssignment>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // the input string contains the full assignment, but we
        // only want what is before the '=' - we had to match the
        // value first which is why this is a combined rule apply
        // TODO: what if the key itself has a = in it?
        std::vector<std::string> assignmentPieces = 
            TfStringSplit(in.string(), "=");
        if (assignmentPieces.size() < 2)
        {
            // there has to be at least two pieces!
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "invalid assignment statement '%s'",
                in.string());
        }
        std::string dictionaryKey = TfStringTrim(assignmentPieces[0]);
        if (TfStringStartsWith(dictionaryKey, "\""))
        {
            dictionaryKey = 
                _GetEvaluatedStringFromString(dictionaryKey, context);
        }

        _DictionaryInsertDictionary(dictionaryKey, context);
    }
};

template <>
struct TextParserAction<DictionaryElementTypedValueAssignment>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // the input string contains the full assignment, but we
        // only want what is before the '=' - we had to match the
        // value first which is why this is a combined rule apply
        // what if the key itself has a = in it?
        std::vector<std::string> assignmentPieces =
            TfStringSplit(in.string(), "=");
        if (assignmentPieces.size() < 2)
        {
            // there has to be at least two pieces!
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "invalid assignment statement '%s'",
                in.string().c_str());
        }

        // if the key was a string, we want what is inside
        std::string dictionaryKey = TfStringTrim(assignmentPieces[0]);
        if (TfStringStartsWith(dictionaryKey, "\""))
        {
            dictionaryKey = _GetEvaluatedStringFromString(dictionaryKey, context);
        }
        _DictionaryInsertValue(dictionaryKey, context);
    }
};

template <>
struct TextParserAction<DictionaryValueScalarType>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_DictionaryInitScalarFactory(in.string(), context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<DictionaryValueShapedType>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this matches the entire sequence of type + []
        // with possible spaces padding the array type
        // we only want the identifier part, so we strip off
        // everything before the first brace and trim
        std::vector<std::string> typePieces = TfStringSplit(in.string(), "[");
        if (typePieces.size() < 2)
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "invalid shaped type identifier '%s'",
                in.string());
        }

        std::string errorMessage;
        if (!_DictionaryInitShapedFactory(
            TfStringTrim(typePieces[0]), context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<StringDictionaryElementKey>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_DictionaryInitScalarFactory(
            Sdf_ParserHelpers::Value(std::string("string")),
            context,
            errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<StringDictionaryElementValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _ValueAppendAtomic(
            _GetEvaluatedStringFromString(in.string(), context), context);
        std::string errorMessage;
        if (!_ValueSetAtomic(context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<StringDictionaryElement>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // the input string contains the full key : value, but we
        // only want what is before the ':' - we had to match the
        // value first which is why this is a combined rule apply
        // what if the key itself has a : in it?
        std::vector<std::string> assignmentPieces =
            TfStringSplit(in.string(), ":");
        if (assignmentPieces.size() < 2)
        {
            // there has to be at least two pieces!
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "invalid dictionary element '%s'",
                in.string().c_str());
        }
        std::string dictionaryKey = TfStringTrim(assignmentPieces[0]);
        _DictionaryInsertValue(dictionaryKey, context);
    }
};

////////////////////////////////////////////////////////////////////////
// Layer actions

template <>
struct TextParserAction<LayerHeader>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::string cookie = TfStringTrimRight(in.string());
        const std::string expected = "#" +  context.magicIdentifierToken + " ";
        if (TfStringStartsWith(cookie, expected))
        {
            if (!context.versionString.empty() && 
                !TfStringEndsWith(cookie, context.versionString))
            {
                TF_WARN("File '%s' is not the latest %s version (found '%s', "
                    "expected '%s'). The file may parse correctly and yield "
                    "incorrect results.",
                    context.fileContext.c_str(),
                    context.magicIdentifierToken.c_str(),
                    cookie.substr(expected.length()).c_str(),
                    context.versionString.c_str());
            }
        }
        else
        {
            // throw error
            std::string errorMessage = TfStringPrintf(
                "Magic Cookie '%s'.  Expected prefix of '%s'",
                TfStringTrim(cookie).c_str(),
                expected.c_str());

            throw PEGTL_NS::parse_error(errorMessage, in);
        }

        context.nameChildrenStack.push_back(std::vector<TfToken>());
        context.data->CreateSpec(
            SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot);
    }
};

template <>
struct TextParserAction<LayerContent>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // store the names of the root prims
        context.data->Set(SdfPath::AbsoluteRootPath(), 
            SdfChildrenKeys->PrimChildren,
            VtValue(context.nameChildrenStack.back()));

        context.nameChildrenStack.pop_back();
    }
};

template <>
struct TextParserAction<LayerMetadataString>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->Comment,
            _GetEvaluatedStringFromString(in.string(), context), context);
    }
};

template <>
struct TextParserAction<LayerMetadataKey>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypePseudoRoot, context);
    }
};

template <>
struct TextParserAction<LayerMetadataValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // a MetadataValue can be either a dictionary, typed value
        // or None, but we only need to take specific actions when
        // it's a dictionary or None
        constexpr std::string_view none {"None"};
        std::string value = TfStringTrim(in.string());
        if (value == none)
        {
            // if the value is None, set the string
            // being recorded to None
            context.currentValue = VtValue();
            if (context.values.IsRecordingString())
            {
                context.values.SetRecordedString(std::string(none));
            }
        }
        else if(TfStringStartsWith(value, "{"))
        {
            // it's a dictionary, we need to ensure the current
            // value that gets set in the context is the dictionary
            // we've been parsing
            context.currentValue.Swap(context.currentDictionaries[0]);
            context.currentDictionaries[0].clear();
        }

        std::string errorMessage;
        if (!_GenericMetadataEnd(SdfSpecTypePseudoRoot, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<LayerMetadataListOpAddIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypePseudoRoot, context);
        context.listOpType = SdfListOpTypeAdded;
    }
};

template <>
struct TextParserAction<LayerMetadataListOpDeleteIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypePseudoRoot, context);
        context.listOpType = SdfListOpTypeDeleted;
    }
};

template <>
struct TextParserAction<LayerMetadataListOpAppendIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypePseudoRoot, context);
        context.listOpType = SdfListOpTypeAppended;
    }
};

template <>
struct TextParserAction<LayerMetadataListOpPrependIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypePseudoRoot, context);
        context.listOpType = SdfListOpTypePrepended;
    }
};

template <>
struct TextParserAction<LayerMetadataListOpReorderIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypePseudoRoot, context);
        context.listOpType = SdfListOpTypeOrdered;
    }
};

template <>
struct TextParserAction<LayerMetadataListOpList>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_GenericMetadataEnd(SdfSpecTypePseudoRoot, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<SublayerList>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayers,
            context.subLayerPaths, context);
        _SetField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayerOffsets,
            context.subLayerOffsets, context);
        
        context.subLayerPaths.clear();
        context.subLayerOffsets.clear();
    }
};

template <>
struct TextParserAction<SublayerStatement>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.subLayerPaths.push_back(context.layerRefPath);
        context.subLayerOffsets.push_back(context.layerRefOffset);
    }
};

template <>
struct TextParserAction<LayerRef>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string evaluatedAssetPath = _GetAssetRefFromString(in.string());
        context.layerRefPath = evaluatedAssetPath;
        context.layerRefOffset = SdfLayerOffset();
    }
};

template <>
struct TextParserAction<LayerRefOffsetValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        Sdf_ParserHelpers::Value value =
            _GetValueFromString(in.string(), in.position().line, context);
        context.layerRefOffset.SetOffset(value.Get<double>());
    }
};

template <>
struct TextParserAction<LayerRefScaleValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        Sdf_ParserHelpers::Value value =
            _GetValueFromString(in.string(), in.position().line, context);
        context.layerRefOffset.SetScale(value.Get<double>());
    }
};

////////////////////////////////////////////////////////////////////////
// Prim actions

template <>
struct TextParserAction<PrimMetadataListOpAddIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypePrim, context);
        context.listOpType = SdfListOpTypeAdded;
    }
};

template <>
struct TextParserAction<PrimMetadataListOpDeleteIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypePrim, context);
        context.listOpType = SdfListOpTypeDeleted;
    }
};

template <>
struct TextParserAction<PrimMetadataListOpAppendIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypePrim, context);
        context.listOpType = SdfListOpTypeAppended;
    }
};

template <>
struct TextParserAction<PrimMetadataListOpPrependIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypePrim, context);
        context.listOpType = SdfListOpTypePrepended;
    }
};

template <>
struct TextParserAction<PrimMetadataListOpReorderIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypePrim, context);
        context.listOpType = SdfListOpTypeOrdered;
    }
};

template <>
struct TextParserAction<PrimMetadataListOpList>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // when a MetadataListOpList is consumed, it can
        // either be a None value or a ListValue
        constexpr std::string_view none {"None"};
        std::string value = TfStringTrim(in.string());
        std::string errorMessage;
        if (value == none)
        {
            // if the value is None, set the string
            // being recorded to None
            context.currentValue = VtValue();
            if (context.values.IsRecordingString())
            {
                context.values.SetRecordedString(std::string(none));
            }
        }
        else if (TfStringStartsWith(value, "["))
        {
            if (!_ValueSetList(context, errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    "%s",
                    errorMessage.c_str());
            }
        }

        if (!_GenericMetadataEnd(SdfSpecTypePrim, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<KindValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, 
            SdfFieldKeys->Kind,
           TfToken(_GetEvaluatedStringFromString(in.string(), context)), 
           context);
    }
};

template <>
struct TextParserAction<PrefixSubstitutionsMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->PrefixSubstitutions,
            context.currentDictionaries[0], context);
        
        context.currentDictionaries[0].clear();
    }
};

template <>
struct TextParserAction<SuffixSubstitutionsMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->SuffixSubstitutions,
            context.currentDictionaries[0], context);
        
        context.currentDictionaries[0].clear();
    }
};

template <>
struct TextParserAction<PayloadMetadataKeyword>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.layerRefPath = std::string();
        context.savedPath = SdfPath();
        context.payloadParsingRefs.clear();
    }
};

template <>
struct TextParserAction<PayloadListOp>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetPayloadListItems(
            SdfListOpTypeExplicit, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PayloadListOpAdd>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetPayloadListItems(
            SdfListOpTypeAdded, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PayloadListOpDelete>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetPayloadListItems(
            SdfListOpTypeDeleted, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PayloadListOpAppend>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetPayloadListItems(
            SdfListOpTypeAppended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PayloadListOpPrepend>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetPayloadListItems(
            SdfListOpTypePrepended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PayloadListOpReorder>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetPayloadListItems(
            SdfListOpTypeOrdered, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PayloadLayerRefItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.layerRefPath.empty())
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "Payload asset path must not be empty.  If this "
                "is intended to be an internal payload, remove the "
                "'@' delimeters.");
        }

        SdfPayload payload(context.layerRefPath, context.savedPath,
            context.layerRefOffset);

        context.payloadParsingRefs.push_back(payload);
    }  
};

template <>
struct TextParserAction<OptionalPayloadPrimPath>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (in.string().length() == 0)
        {
            // this matched the optional rule
            context.savedPath = SdfPath();
        }
        else
        {
            // this matched an actual prim path
            std::string path = Sdf_EvalQuotedString(
                in.string().c_str(), in.string().length(), 1);
            if (path.length() == 0)
            {
                context.savedPath = SdfPath();
            }
            else
            {
                std::string errorMessage;
                if (!_PathSetPrim(path, context, errorMessage))
                {
                    Sdf_TextFileFormatParser_Err(
                        context,
                        in.input(),
                        in.position(),
                        "%s",
                        errorMessage.c_str());
                }
            }
        }
    }
};

template <>
struct TextParserAction<PayloadPathRef>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.layerRefPath.clear();
        context.layerRefOffset = SdfLayerOffset();

        std::string pathRef = Sdf_EvalQuotedString(
            in.string().c_str(), in.string().length(), 1);
        if (pathRef.length() == 0)
        {
            context.savedPath = SdfPath::EmptyPath();
        }
        else
        {
            std::string errorMessage;
            if (!_PathSetPrim(pathRef, context, errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    "%s",
                    errorMessage.c_str());
            }
        }
    }
};

template <>
struct TextParserAction<PayloadPathRefItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        SdfPayload payload(std::string(), context.savedPath,
            context.layerRefOffset);
        context.payloadParsingRefs.push_back(payload);
    }
};

template <>
struct TextParserAction<InheritsMetadataKeyword>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.inheritParsingTargetPaths.clear();
    }
};

template <>
struct TextParserAction<InheritsListOp>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetInheritListItems(
            SdfListOpTypeExplicit, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<InheritsListOpAdd>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetInheritListItems(
            SdfListOpTypeAdded, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<InheritsListOpDelete>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetInheritListItems(
            SdfListOpTypeDeleted, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<InheritsListOpAppend>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetInheritListItems(
            SdfListOpTypeAppended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<InheritsListOpPrepend>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetInheritListItems(
            SdfListOpTypePrepended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<InheritsListOpReorder>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetInheritListItems(
            SdfListOpTypeOrdered, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<InheritListItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_PathSetPrim(
            Sdf_EvalQuotedString(in.string().c_str(), in.string().length(), 1),
            context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }

        _InheritAppendPath(context);
    }
};

template <>
struct TextParserAction<SpecializesMetadataKeyword>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.specializesParsingTargetPaths.clear();
    }
};

template <>
struct TextParserAction<SpecializesListOp>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetSpecializesListItems(
            SdfListOpTypeExplicit, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<SpecializesListOpAdd>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetSpecializesListItems(
            SdfListOpTypeAdded, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<SpecializesListOpDelete>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetSpecializesListItems(
            SdfListOpTypeDeleted, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<SpecializesListOpAppend>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetSpecializesListItems(
            SdfListOpTypeAppended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<SpecializesListOpPrepend>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetSpecializesListItems(
            SdfListOpTypePrepended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<SpecializesListOpReorder>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetSpecializesListItems(
            SdfListOpTypeOrdered, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<SpecializesListItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_PathSetPrim(
            Sdf_EvalQuotedString(in.string().c_str(), in.string().length(), 1),
            context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }

        _SpecializesAppendPath(context);
    }
};

template<>
struct TextParserAction<RelocatesMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->Relocates,
            context.relocatesParsingMap, context);

        context.relocatesParsingMap.clear();
    }
};

template <>
struct TextParserAction<RelocatesStatement>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this statement is accepted in its entirety, but we need
        // access to both path refs here to add the relocates operation
        // the incoming string has both separated by a ':' character
        std::vector<std::string> pathRefs = TfStringSplit(in.string(), ":");
        if (pathRefs.size() < 2)
        {
            // there has to be at least two path refs!
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "invalid relocates statement '%s'",
                in.string().c_str());
        }
        std::string relocatesSource = 
            _GetEvaluatedStringFromString(TfStringTrim(pathRefs[0]), context);
        std::string relocatesTarget = 
            _GetEvaluatedStringFromString(TfStringTrim(pathRefs[1]), context);
        std::string errorMessage;
        if (!_RelocatesAdd(relocatesSource, relocatesTarget, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<VariantSetsListOpAdd>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetVariantSetNamesListItems(
            SdfListOpTypeAdded, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        context.nameVector.clear();
    }
};

template <>
struct TextParserAction<VariantSetsListOpDelete>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetVariantSetNamesListItems(
            SdfListOpTypeDeleted, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        context.nameVector.clear();
    }
};

template <>
struct TextParserAction<VariantSetsListOpAppend>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetVariantSetNamesListItems(
            SdfListOpTypeAppended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        context.nameVector.clear();
    }
};

template <>
struct TextParserAction<VariantSetsListOpPrepend>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetVariantSetNamesListItems(
            SdfListOpTypePrepended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        context.nameVector.clear();
    }
};

template <>
struct TextParserAction<VariantSetsListOpReorder>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetVariantSetNamesListItems(
            SdfListOpTypeOrdered, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        context.nameVector.clear();
    }
};

template <>
struct TextParserAction<VariantSetsListOp>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetVariantSetNamesListItems(
            SdfListOpTypeExplicit, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        context.nameVector.clear();
    }
};

template <>
struct TextParserAction<VariantsMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_PrimSetVariantSelection(context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<ReferencesMetadataKeyword>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.layerRefPath = std::string();
        context.savedPath = SdfPath();
        context.referenceParsingRefs.clear();
    }
};

template <>
struct TextParserAction<ReferencesListOp>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetReferenceListItems(
            SdfListOpTypeExplicit, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<ReferencesListOpAdd>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetReferenceListItems(
            SdfListOpTypeAdded, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<ReferencesListOpDelete>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetReferenceListItems(
            SdfListOpTypeDeleted, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<ReferencesListOpAppend>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetReferenceListItems(
            SdfListOpTypeAppended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<ReferencesListOpPrepend>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetReferenceListItems(
            SdfListOpTypePrepended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<ReferencesListOpReorder>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimSetReferenceListItems(
            SdfListOpTypeOrdered, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<ReferenceLayerRefItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.layerRefPath.empty())
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(), 
                "Reference asset path must not be empty.  If this "
                "is intended to be an internal reference, remove the "
                "'@' delimeters.");
        }

        SdfReference ref(context.layerRefPath, context.savedPath,
            context.layerRefOffset);

        ref.SwapCustomData(context.currentDictionaries[0]);
        context.referenceParsingRefs.push_back(ref);
    }  
};

template <>
struct TextParserAction<OptionalReferencePrimPath>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (in.string().length() == 0)
        {
            // this matched the optional rule
            context.savedPath = SdfPath();
        }
        else
        {
            // this matched an actual prim path
            std::string path = Sdf_EvalQuotedString(
                in.string().c_str(), in.string().length(), 1);
            if (path.length() == 0)
            {
                context.savedPath = SdfPath();
            }
            else
            {
                std::string errorMessage;
                if (!_PathSetPrim(path, context, errorMessage))
                {
                    Sdf_TextFileFormatParser_Err(
                        context,
                        in.input(),
                        in.position(),
                        "%s",
                        errorMessage.c_str());
                }
            }
        }
    }
};

template <>
struct TextParserAction<ReferencePathRef>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.layerRefPath.clear();
        context.layerRefOffset = SdfLayerOffset();

        std::string pathRef = Sdf_EvalQuotedString(
            in.string().c_str(), in.string().length(), 1);
        if (pathRef.length() == 0)
        {
            context.savedPath = SdfPath::EmptyPath();
        }
        else
        {
            std::string errorMessage;
            if (!_PathSetPrim(pathRef, context, errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    "%s",
                    errorMessage.c_str());
            }
        }
    }
};

template <>
struct TextParserAction<ReferencePathRefItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        SdfReference ref(std::string(), 
            context.savedPath,
            context.layerRefOffset);
        ref.SwapCustomData(context.currentDictionaries[0]);
        context.referenceParsingRefs.push_back(ref);
    }
};

template <>
struct TextParserAction<PrimDefSpecifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.specifier = SdfSpecifierDef;
        context.typeName = TfToken();
    }
};

template <>
struct TextParserAction<PrimClassSpecifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.specifier = SdfSpecifierClass;
        context.typeName = TfToken();
    }
};

template <>
struct TextParserAction<PrimOverSpecifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.specifier = SdfSpecifierOver;
        context.typeName = TfToken();
    }
};

template <>
struct TextParserAction<PrimTypeName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this matched the entirety of the prim type name
        // which could have spaces padding the separator
        // (in this case '.'), so we need to tokenize and trim
        // to form an unpadded type name
        std::vector<std::string> typeNameParts =
            TfStringTokenize(in.string(), ".");
        std::string typeName = typeNameParts[0];
        if (typeNameParts.size() > 1)
        {
            for (size_t i = 1; i < typeNameParts.size(); i++)
            {
                typeName = typeName + "." + TfStringTrim(typeNameParts[i]);
            }
        }

        context.typeName = TfToken(typeName);
    }
};

template <>
struct TextParserAction<PrimReorderNameList>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path,
            SdfFieldKeys->PrimOrder,
            context.nameVector,
            context);

        context.nameVector.clear();
    }
};

template <>
struct TextParserAction<PrimIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        TfToken name(_GetEvaluatedStringFromString(in.string(), context));
        if (!SdfPath::IsValidIdentifier(name))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "'%s' is not a valid prim name",
                name.GetText());
        }

        context.path = context.path.AppendChild(name);
        if (_HasSpec(context.path, context))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "Duplicate prim '%s'",
                context.path.GetText());
        }
        else
        {
            // record the existence of this prim
            _CreateSpec(context.path, SdfSpecTypePrim, context);

            // add this prim to its parent's name children
            context.nameChildrenStack.back().push_back(name);
        }

        // create our name children vector and properties vector
        context.nameChildrenStack.push_back(std::vector<TfToken>());
        context.propertiesStack.push_back(std::vector<TfToken>());

        _SetField(context.path,
            SdfFieldKeys->Specifier,
            context.specifier,
            context);

        if (!context.typeName.IsEmpty())
        {
            _SetField(context.path, SdfFieldKeys->TypeName,
                context.typeName, context);
        }
    }
};

template <>
struct TextParserAction<PrimStatementInterior>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // store the names of our children
        if (!context.nameChildrenStack.back().empty())
        {
            _SetField(context.path, SdfChildrenKeys->PrimChildren,
                context.nameChildrenStack.back(), context);
        }

        // store the names of our properties, if there are any
        if (!context.propertiesStack.back().empty())
        {
            _SetField(context.path, SdfChildrenKeys->PropertyChildren,
                context.propertiesStack.back(), context);
        }

        context.nameChildrenStack.pop_back();
        context.propertiesStack.pop_back();
        context.path = context.path.GetParentPath();
    }
};

template <>
struct TextParserAction<PrimChildOrderStatement>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->PrimOrder,
            context.nameVector, context);
        
        context.nameVector.clear();
    }
};

template <>
struct TextParserAction<PrimPropertyOrderStatement>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->PropertyOrder,
            context.nameVector, context);
        
        context.nameVector.clear();
    }
};

template <>
struct TextParserAction<PrimMetadataString>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->Comment,
            _GetEvaluatedStringFromString(in.string(), context), context);
    }
};

template <>
struct TextParserAction<PrimMetadataKey>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypePrim, context);
    }
};

template <>
struct TextParserAction<PrimMetadataValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // a MetadataValue can be either a dictionary, typed value
        // or None, but we only need to take specific actions when
        // it's a dictionary or None
        constexpr std::string_view none {"None"};
        std::string value = TfStringTrim(in.string());
        if (value == none)
        {
            // if the value is None, set the string
            // being recorded to None
            context.currentValue = VtValue();
            if (context.values.IsRecordingString())
            {
                context.values.SetRecordedString(std::string(none));
            }
        }
        else if(TfStringStartsWith(value, "{"))
        {
            // it's a dictionary, we need to ensure the current
            // value that gets set in the context is the dictionary
            // we've been parsing
            context.currentValue.Swap(context.currentDictionaries[0]);
            context.currentDictionaries[0].clear();
        }

        std::string errorMessage;
        if (!_GenericMetadataEnd(SdfSpecTypePrim, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PrimVariantSetName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string name = _GetEvaluatedStringFromString(in.string(), context);
        const SdfAllowed allow = SdfSchema::IsValidVariantIdentifier(name);
        if (!allow)
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                allow.GetWhyNot().c_str());
        }

        context.currentVariantSetNames.push_back(name);
        context.currentVariantNames.push_back(std::vector<std::string>());
        context.path = context.path.AppendVariantSelection(name, "");
    }
};

template <>
struct TextParserAction<VariantSetStatement>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        SdfPath variantSetPath = context.path;
        context.path = context.path.GetParentPath();

        // create this VariantSetSpec if it does not already exist
        if (!_HasSpec(variantSetPath, context))
        {
            _CreateSpec(variantSetPath, SdfSpecTypeVariantSet, context);

            // add the name of this variant set to the VariantSets field
            _AppendVectorItem(SdfChildrenKeys->VariantSetChildren,
                TfToken(context.currentVariantSetNames.back()),
                context);
        }

        // author the variant set's variants
        _SetField(variantSetPath, SdfChildrenKeys->VariantChildren,
            TfToTokenVector(context.currentVariantNames.back()), context);

        context.currentVariantSetNames.pop_back();
        context.currentVariantNames.pop_back();
    }
};

template <>
struct TextParserAction<PrimVariantName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::string variantName =
            _GetEvaluatedStringFromString(in.string(), context);
        const SdfAllowed allow =
            SdfSchema::IsValidVariantIdentifier(variantName);
        if (!allow)
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                allow.GetWhyNot().c_str());
        }

        context.currentVariantNames.back().push_back(variantName);

        // a variant is basically like a new pseudo-root, so we need to push
        // a new item onto our name children stack to store prims defined
        // within this variant
        context.nameChildrenStack.push_back(std::vector<TfToken>());
        context.propertiesStack.push_back(std::vector<TfToken>());

        std::string variantSetName = context.currentVariantSetNames.back();
        context.path = context.path.GetParentPath()
            .AppendVariantSelection(variantSetName, variantName);

        _CreateSpec(context.path, SdfSpecTypeVariant, context);
    }
};

template <>
struct TextParserAction<VariantStatement>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // store the names of the prims and properties defined in this variant
        if (!context.nameChildrenStack.back().empty())
        {
            _SetField(context.path, SdfChildrenKeys->PrimChildren,
                context.nameChildrenStack.back(), context);
        }

        if (!context.propertiesStack.back().empty())
        {
            _SetField(context.path, SdfChildrenKeys->PropertyChildren,
                context.propertiesStack.back(), context);
        }

        context.nameChildrenStack.pop_back();
        context.propertiesStack.pop_back();

        std::string variantSet = context.path.GetVariantSelection().first;
        context.path =
            context.path.GetParentPath().AppendVariantSelection(variantSet, "");
    }
};

////////////////////////////////////////////////////////////////////////
// Prim Relationship actions

template <>
struct TextParserAction<PrimRelationshipMetadataString>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->Comment,
            _GetEvaluatedStringFromString(in.string(), context), context);
    }
};

template <>
struct TextParserAction<PrimRelationshipMetadataKey>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypeRelationship, context);
    }
};

template <>
struct TextParserAction<PrimRelationshipMetadataValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // a MetadataValue can be either a dictionary, typed value
        // or None, but we only need to take specific actions when
        // it's a dictionary or None
        constexpr std::string_view none {"None"};
        std::string value = TfStringTrim(in.string());
        if (value == none)
        {
            // if the value is None, set the string
            // being recorded to None
            context.currentValue = VtValue();
            if (context.values.IsRecordingString())
            {
                context.values.SetRecordedString(std::string(none));
            }
        }
        else if(TfStringStartsWith(value, "{"))
        {
            // it's a dictionary, we need to ensure the current
            // value that gets set in the context is the dictionary
            // we've been parsing
            context.currentValue.Swap(context.currentDictionaries[0]);
            context.currentDictionaries[0].clear();
        }

        std::string errorMessage;
        if (!_GenericMetadataEnd(
            SdfSpecTypeRelationship, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PrimRelationshipMetadataListOpAddIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypeRelationship, context);
        context.listOpType = SdfListOpTypeAdded;
    }
};

template <>
struct TextParserAction<PrimRelationshipMetadataListOpDeleteIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypeRelationship, context);
        context.listOpType = SdfListOpTypeDeleted;
    }
};

template <>
struct TextParserAction<PrimRelationshipMetadataListOpAppendIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypeRelationship, context);
        context.listOpType = SdfListOpTypeAppended;
    }
};

template <>
struct TextParserAction<PrimRelationshipMetadataListOpPrependIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypeRelationship, context);
        context.listOpType = SdfListOpTypePrepended;
    }
};

template <>
struct TextParserAction<PrimRelationshipMetadataListOpReorderIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypeRelationship, context);
        context.listOpType = SdfListOpTypeOrdered;
    }
};

template <>
struct TextParserAction<PrimRelationshipMetadataListOpList>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // when a MetadataListOpList is consumed, it can
        // either be a None value or a ListValue
        constexpr std::string_view none {"None"};
        std::string value = TfStringTrim(in.string());
        std::string errorMessage;
        if (value == none)
        {
            // if the value is None, set the string
            // being recorded to None
            context.currentValue = VtValue();
            if (context.values.IsRecordingString())
            {
                context.values.SetRecordedString(std::string(none));
            }
        }
        else if (TfStringStartsWith(value, "["))
        {
            if (!_ValueSetList(context, errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    "%s",
                    errorMessage.c_str());
            }
        }
        
        if (!_GenericMetadataEnd(
            SdfSpecTypeRelationship, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PrimRelationshipName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string namespacedName = _UnpadNamespacedName(in.string());
        std::string errorMessage;
        if (!_PrimInitRelationship(namespacedName, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PrimRelationshipTimesamplesName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this production matches the entire NamespacedName.timeSamples
        // so we need to parse out just the NamespacedName portion
        std::vector<std::string> timeSamplesNamePieces =
            TfStringSplit(in.string(), ".");
        if (timeSamplesNamePieces.size() < 2)
        {
            // there has to be at least two pieces!
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "invalid time samples relationship name '%s'",
                in.string().c_str());
        }

        std::string errorMessage;
        if (!_PrimInitRelationship(
            _UnpadNamespacedName(TfStringTrim(
                timeSamplesNamePieces[0])), context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PrimRelationshipDefaultName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this production matches the entire NamespacedName.default
        // so we need to parse out just the NamespacedName portion
        std::vector<std::string> defaultNamePieces =
            TfStringSplit(in.string(), ".");
        if (defaultNamePieces.size() < 2)
        {
            // there has to be at least two pieces!
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "invalid default relationship name '%s'",
                in.string().c_str());
        }

        std::string errorMessage;
        if (!_PrimInitRelationship(
            _UnpadNamespacedName(TfStringTrim(
                defaultNamePieces[0])), context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PrimRelationshipDefaultRef>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // if path is empty, use dfeault c'tor to construct empty path
        // XXX: 08/04/08 would be nice if SdfPath would allow
        // SdfPath("") without throwing a warning
        std::string pathString = _GetEvaluatedStringFromString(in.string(), context);
        SdfPath path = pathString.empty() ? SdfPath() : SdfPath(pathString);
        _SetDefault(context.path, VtValue(path), context);
    }
};

template <>
struct TextParserAction<PrimRelationshipDefault>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PrimEndRelationship(context);
    }
};

template <>
struct TextParserAction<PrimRelationshipTimeSamplesValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->TimeSamples,
            context.timeSamples, context);
        
        _PrimEndRelationship(context);
    }
};

template <>
struct TextParserAction<PrimRelationshipTypeUniform>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.custom = false;
        context.variability = VtValue(SdfVariabilityUniform);
    }
};

template <>
struct TextParserAction<PrimRelationshipTypeCustomUniform>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.custom = true;
        context.variability = VtValue(SdfVariabilityUniform);
    }
};

template <>
struct TextParserAction<PrimRelationshipTypeCustomVarying>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.custom = true;
        context.variability = VtValue(SdfVariabilityVarying);
    }
};

template <>
struct TextParserAction<PrimRelationshipTypeVarying>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.custom = false;
        context.variability = VtValue(SdfVariabilityVarying);
    }
};

template <>
struct TextParserAction<PrimRelationshipTarget>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string targetPath = 
            Sdf_EvalQuotedString(in.string().c_str(), in.string().length(), 1);
        _RelationshipAppendTargetPath(targetPath, context);
    }
};

template <>
struct TextParserAction<PrimRelationshipTargetNone>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.relParsingTargetPaths = SdfPathVector();
    }
};

template <>
struct TextParserAction<PrimRelationshipList>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _RelationshipInitTarget(
            context.relParsingTargetPaths->back(), context);
    }
};

template <>
struct TextParserAction<PrimRelationshipAddListOp>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_RelationshipSetTargetsList(
            SdfListOpTypeAdded, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage);
        }

        _PrimEndRelationship(context);
    }
};

template <>
struct TextParserAction<PrimRelationshipDeleteListOp>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_RelationshipSetTargetsList(
            SdfListOpTypeDeleted, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }

        _PrimEndRelationship(context);
    }
};

template <>
struct TextParserAction<PrimRelationshipPrependListOp>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_RelationshipSetTargetsList(
            SdfListOpTypePrepended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }

        _PrimEndRelationship(context);
    }
};

template <>
struct TextParserAction<PrimRelationshipAppendListOp>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_RelationshipSetTargetsList(
            SdfListOpTypeAppended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }

        _PrimEndRelationship(context);
    }
};

template <>
struct TextParserAction<PrimRelationshipReorderListOp>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_RelationshipSetTargetsList(
            SdfListOpTypeOrdered, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }

        _PrimEndRelationship(context);
    }
};

template <>
struct TextParserAction<PrimRelationshipStandard>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_RelationshipSetTargetsList(
            SdfListOpTypeExplicit, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }

        _PrimEndRelationship(context);
    }
};

////////////////////////////////////////////////////////////////////////
// Prim Attribute actions

template <>
struct TextParserAction<PrimAttributeMetadataString>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path,
            SdfFieldKeys->Comment,
            _GetEvaluatedStringFromString(in.string(), context),
            context);
    }
};

template <>
struct TextParserAction<PrimAttributeMetadataKey>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypeAttribute, context);
    }
};

template <>
struct TextParserAction<PrimAttributeMetadataValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // a MetadataValue can be either a dictionary, typed value
        // or None, but we only need to take specific actions when
        // it's a dictionary or None
        constexpr std::string_view none {"None"};
        std::string value = TfStringTrim(in.string());
        if (value == none)
        {
            // if the value is None, set the string
            // being recorded to None
            context.currentValue = VtValue();
            if (context.values.IsRecordingString())
            {
                context.values.SetRecordedString(std::string(none));
            }
        }
        else if(TfStringStartsWith(value, "{"))
        {
            // it's a dictionary, we need to ensure the current
            // value that gets set in the context is the dictionary
            // we've been parsing
            context.currentValue.Swap(context.currentDictionaries[0]);
            context.currentDictionaries[0].clear();
        }

        std::string errorMessage;
        if (!_GenericMetadataEnd(SdfSpecTypeAttribute, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PrimAttributeMetadataListOpAddIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypeAttribute, context);
        context.listOpType = SdfListOpTypeAdded;
    }
};

template <>
struct TextParserAction<PrimAttributeMetadataListOpDeleteIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypeAttribute, context);
        context.listOpType = SdfListOpTypeDeleted;
    }
};

template <>
struct TextParserAction<PrimAttributeMetadataListOpAppendIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypeAttribute, context);
        context.listOpType = SdfListOpTypeAppended;
    }
};

template <>
struct TextParserAction<PrimAttributeMetadataListOpPrependIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypeAttribute, context);
        context.listOpType = SdfListOpTypePrepended;
    }
};

template <>
struct TextParserAction<PrimAttributeMetadataListOpReorderIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _GenericMetadataStart(in.string(), SdfSpecTypeAttribute, context);
        context.listOpType = SdfListOpTypeOrdered;
    }
};

template <>
struct TextParserAction<PrimAttributeMetadataListOpList>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // when a MetadataListOpList is consumed, it can
        // either be a None value or a ListValue
        constexpr std::string_view none {"None"};
        std::string value = TfStringTrim(in.string());
        std::string errorMessage;
        if (value == none)
        {
            // if the value is None, set the string
            // being recorded to None
            context.currentValue = VtValue();
            if (context.values.IsRecordingString())
            {
                context.values.SetRecordedString(std::string(none));
            }
        }
        else if (TfStringStartsWith(value, "["))
        {
            if (!_ValueSetList(context, errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    "%s",
                    errorMessage.c_str());
            }
        }

        if (!_GenericMetadataEnd(SdfSpecTypeAttribute, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PrimAttributeMetadataDisplayUnitIdentifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        TfEnum displayUnit;
        if (!_GetDisplayUnitFromString(in.string(), displayUnit))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "'%s' is not a valid display unit",
                in.string());
        }

        _SetField(context.path, SdfFieldKeys->DisplayUnit,
            displayUnit, context);
    }
};

template <>
struct TextParserAction<PrimAttributeStandardType>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetupValue(in.string(), context);
    }
};

template <>
struct TextParserAction<PrimAttributeArrayType>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this rule matched the sequence identifier[] with possible
        // whitespace between the end of the identifier and the start of
        // the array type signal and between the braces
        // we need to trim out all of the space
        std::string arrayType = TfStringTrim(in.string());
        arrayType = TfStringReplace(arrayType, " ", "");
        arrayType = TfStringReplace(arrayType, "\t", "");
        _SetupValue(arrayType, context);
    }
};

template <>
struct TextParserAction<PrimAttributeVariability>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this handles matches for both KeywordUniform and KeywordConfig
        // which ultimately have the same variability
        // convert legacy "config" variability to SdfVariabilityUniform
        // this value was never officially used in USD but we handle
        // this just in case the value was written outiform = "uniform";
        context.variability = VtValue(SdfVariabilityUniform);
        context.assoc = VtValue();
    }
};

template <>
struct TextParserAction<PrimAttributeQualifiedType>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.custom = false;
    }
};

template <>
struct TextParserAction<PrimAttributeType>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.variability = VtValue();
        context.custom = false;
    }
};

template <>
struct TextParserAction<PrimAttributeFallback>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.path = context.path.GetParentPath();
    }
};

template <>
struct TextParserAction<PrimAttributeFallbackNamespacedName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.custom = true;
        std::string errorMessage;
        if (!_PrimInitAttribute(
            _UnpadNamespacedName(in.string()), context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PrimAttributeFallbackTypeName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (!context.values.valueTypeIsValid)
        {
            context.values.StartRecordingString();
        }
    }
};

template <>
struct TextParserAction<PrimAttributeAssignmentOptional>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (!context.values.valueTypeIsValid)
        {
            context.values.StopRecordingString();
        }
    }
};

template <>
struct TextParserAction<PrimAttributeDefaultNamespacedName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if(!_PrimInitAttribute(
            _UnpadNamespacedName(in.string()), context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PrimAttributeDefaultTypeName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (!context.values.valueTypeIsValid)
        {
            context.values.StartRecordingString();
        }
    }
};

template <>
struct TextParserAction<PrimAttributeDefault>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.path = context.path.GetParentPath();
    }
};

template <>
struct TextParserAction<PrimAttributeTimeSamplesName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this production matches the entire NamespacedName.timeSamples
        // so we need to parse out just the NamespacedName portion
        std::vector<std::string> timeSamplesNamePieces =
            TfStringSplit(in.string(), ".");
        if (timeSamplesNamePieces.size() < 2)
        {
            // there has to be at least two pieces!
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "invalid time samples attribute name '%s'",
                in.string().c_str());
        }

        std::string errorMessage;
        if (!_PrimInitAttribute(
            _UnpadNamespacedName(TfStringTrim(
                timeSamplesNamePieces[0])), context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PrimAttributeTimeSamplesValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _SetField(context.path, SdfFieldKeys->TimeSamples,
            context.timeSamples, context);
        
        context.path = context.path.GetParentPath();
    }
};

template <>
struct TextParserAction<PrimAttributeValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        constexpr std::string_view none {"None"};
        std::string value = TfStringTrim(in.string());
        if (value == none)
        {
            _SetDefault(context.path, VtValue(SdfValueBlock()), context);
        }
        else
        {
            _SetDefault(context.path, context.currentValue, context);
        }
    }
};

template <>
struct TextParserAction<PrimAttributeConnectName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this production matches the entire NamespacedName.connect
        // so we need to parse out just the NamespacedName portion
        std::vector<std::string> connectNamePieces =
            TfStringSplit(in.string(), ".");
        if (connectNamePieces.size() < 2)
        {
            // there has to be at least two pieces!
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "invalid connect attribute name '%s'",
                in.string().c_str());
        }

        std::string errorMessage;
        if (!_PrimInitAttribute(
            _UnpadNamespacedName(TfStringTrim(
                connectNamePieces[0])), context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
    }
};

template <>
struct TextParserAction<PrimAttributeConnectAssignment>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.connParsingTargetPaths.clear();
        context.connParsingAllowConnectionData = true;
    }
};

template <>
struct TextParserAction<PrimAttributeConnectItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string path = Sdf_EvalQuotedString(
            in.string().c_str(), in.string().length(), 1);
        std::string errorMessage;
        if (!_PathSetPrimOrPropertyScenePath(path, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        _AttributeAppendConnectionPath(context, in.position().line);
    }
};

template <>
struct TextParserAction<PrimAttributeConnectValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_AttributeSetConnectionTargetsList(
            SdfListOpTypeExplicit, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        context.path = context.path.GetParentPath();
    }
};

template <>
struct TextParserAction<PrimAttributeAddConnectValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_AttributeSetConnectionTargetsList(
            SdfListOpTypeAdded, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        context.path = context.path.GetParentPath();
    }
};

template <>
struct TextParserAction<PrimAttributeDeleteConnectValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_AttributeSetConnectionTargetsList(
            SdfListOpTypeDeleted, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        context.path = context.path.GetParentPath();
    }
};

template <>
struct TextParserAction<PrimAttributeAppendConnectValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_AttributeSetConnectionTargetsList(
            SdfListOpTypeAppended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        context.path = context.path.GetParentPath();
    }
};

template <>
struct TextParserAction<PrimAttributePrependConnectValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_AttributeSetConnectionTargetsList(
            SdfListOpTypePrepended, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        context.path = context.path.GetParentPath();
    }
};

template <>
struct TextParserAction<PrimAttributeReorderConnectValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_AttributeSetConnectionTargetsList(
            SdfListOpTypeOrdered, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "%s",
                errorMessage.c_str());
        }
        context.path = context.path.GetParentPath();
    }
};

template <>
struct TextParserAction<PrimAttributeAddConnectAssignment>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.connParsingTargetPaths.clear();
        context.connParsingAllowConnectionData = true;
    }
};

template <>
struct TextParserAction<PrimAttributeDeleteConnectAssignment>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.connParsingTargetPaths.clear();
        context.connParsingAllowConnectionData = false;
    }
};

template <>
struct TextParserAction<PrimAttributeAppendConnectAssignment>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.connParsingTargetPaths.clear();
        context.connParsingAllowConnectionData = true;
    }
};

template <>
struct TextParserAction<PrimAttributePrependConnectAssignment>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.connParsingTargetPaths.clear();
        context.connParsingAllowConnectionData = true;
    }
};

template <>
struct TextParserAction<PrimAttributeReorderConnectAssignment>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.connParsingTargetPaths.clear();
        context.connParsingAllowConnectionData = false;
    }
};

////////////////////////////////////////////////////////////////////////
// Parsing entry-point methods

/// Parse a text layer into an SdfData
bool 
Sdf_ParseLayer(
    const std::string& fileContext, 
    const std::shared_ptr<ArAsset>& asset,
    const std::string& magicId,
    const std::string& versionString,
    bool metadataOnly,
    SdfDataRefPtr data,
    SdfLayerHints *hints)
{
    TfAutoMallocTag2 tag("Sdf", "Sdf_ParseLayer");

    TRACE_FUNCTION();

    // Configure for input file.
    Sdf_TextParserContext context;

    context.data = data;
    context.fileContext = fileContext;
    context.magicIdentifierToken = magicId;
    context.versionString = versionString;
    context.metadataOnly = metadataOnly;

    // read the entire file into memory
    size_t size = asset->GetSize();
    std::string buffer(size, ' ');
    if (asset->Read(&buffer[0], size, 0) != size)
    {
        TF_RUNTIME_ERROR("Failed to read asset contents @%s@: "
            "an error occurred while reading",
            fileContext.c_str());
    }

    PEGTL_NS::string_input<> content { std::move(buffer), fileContext};
    context.values.errorReporter =
        std::bind(_ReportParseError<PEGTL_NS::string_input<>>,
        std::ref(context), std::cref(content), std::placeholders::_1);
    bool status = false;
    try
    {
        if (!metadataOnly)
        {
            status = PEGTL_NS::parse<
                PEGTL_NS::must<Layer, PEGTL_NS::internal::eof>,
                TextParserAction,
                TextParserControl>(content, context);
            *hints = context.layerHints;
        }
        else
        {
            // note the absence of the eof here - there will be more
            // content in the layer and we don't know what that content is,
            // so we stop at reduction of LayerMetadataOnly
            status = PEGTL_NS::parse<
                PEGTL_NS::must<LayerMetadataOnly>,
                TextParserAction,
                TextParserControl>(content, context);
            *hints = context.layerHints;
        }
    }
    catch (boost::bad_get const &)
    {
        TF_CODING_ERROR("Bad boost:get<T>() in layer parser.");
        Sdf_TextFileFormatParser_Err(
            context,
            content,
            content.position(),
            "Internal layer parser error.");
    }
    catch (const PEGTL_NS::parse_error& e)
    {
        Sdf_TextFileFormatParser_Err(
            context,
            content,
            e.positions.size() == 0 ? content.position() : e.positions[0],
            e.what());
    }

    return status;
}

/// Parse a layer text string into an SdfData
bool
Sdf_ParseLayerFromString(
    const std::string & layerString, 
    const std::string & magicId,
    const std::string & versionString,
    SdfDataRefPtr data,
    SdfLayerHints *hints)
{
    TfAutoMallocTag2 tag("Sdf", "Sdf_ParseLayerFromString");

    TRACE_FUNCTION();

    // Configure for input string.
    Sdf_TextParserContext context;

    context.data = data;
    context.magicIdentifierToken = magicId;
    context.versionString = versionString;

    PEGTL_NS::string_input<> content { std::move(layerString), ""};
    context.values.errorReporter =
        std::bind(_ReportParseError<PEGTL_NS::string_input<>>,
        std::ref(context), std::cref(content), std::placeholders::_1);
    bool status;
    try
    {
        status = PEGTL_NS::parse<
            PEGTL_NS::must<Layer, PEGTL_NS::internal::eof>,
            TextParserAction,
            TextParserControl>(content, context);
    }
    catch (boost::bad_get const &)
    {
        TF_CODING_ERROR("Bad boost:get<T>() in layer parser.");
        Sdf_TextFileFormatParser_Err(
            context,
            content,
            content.position(),
            "Internal layer parser error.");
    }
    catch (const PEGTL_NS::parse_error& e)
    {
        Sdf_TextFileFormatParser_Err(
            context,
            content,
            e.positions.size() == 0 ? content.position() : e.positions[0],
            e.what());
    }

    return status;
}

}  // end namespace Sdf_TextFileFormatParser

PXR_NAMESPACE_CLOSE_SCOPE
