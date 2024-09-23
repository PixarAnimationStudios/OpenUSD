//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file Sdf/textFileFormatParser.cpp

#include "pxr/pxr.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/pegtl/pegtl/contrib/trace.hpp"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/sdf/textParserContext.h"
#include "pxr/usd/sdf/textFileFormatParser.h"
#include "pxr/usd/sdf/debugCodes.h"
#include "pxr/usd/sdf/textParserHelpers.h"
#include "pxr/base/ts/raii.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/valueTypeDispatch.h"

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_TextFileFormatParser {

////////////////////////////////////////////////////////////////////////
// TextFileFormat customized errors

template<> constexpr auto errorMessage<SingleQuote> =
    "Expected '";
template<> constexpr auto errorMessage<DoubleQuote> =
    "Expected \"";
template<> constexpr auto errorMessage<LeftParen> =
    "Expected (";
template<> constexpr auto errorMessage<RightParen> =
    "Expected )";
template<> constexpr auto errorMessage<LeftBracket> =
    "Expected [";
template<> constexpr auto errorMessage<RightBracket> =
    "Expected ]";
template<> constexpr auto errorMessage<LeftBrace> =
    "Expected {";
template<> constexpr auto errorMessage<RightBrace> =
    "Expected }";
template<> constexpr auto errorMessage<LeftAngleBracket> =
    "Expected <";
template<> constexpr auto errorMessage<RightAngleBracket> =
    "Expected >";
template<> constexpr auto errorMessage<At> =
    "Expected @";
template<> constexpr auto errorMessage<Assignment> =
    "Expected =";
template<> constexpr auto errorMessage<Digit> =
    "Expected number [0-9]";
template<> constexpr auto errorMessage<Sdf_PathParser::Path> =
    "Expected Path";
template<> constexpr auto errorMessage<ListOf<TupleValueItem>> =
    "Expected list of number, identifier, string, asset ref, or tuples "
    "separated by ,";
template<> constexpr auto errorMessage<TupleValueItem> =
    "Expected number, identifier, string, asset ref, or tuple";
template<> constexpr auto errorMessage<ListOf<ListValueItem>> =
    "Expected list of number, identifier, string, asset ref, list, or tuples "
    "separated by ,";
template<> constexpr auto errorMessage<ListValueItem> =
    "Expected number, identifier, string, asset ref, list or tuple";
template<> constexpr auto errorMessage<TokenSeparator> =
    "Expected spaces";
template<> constexpr auto errorMessage<DictionaryKey> =
    "Expected string or identifier";
template<> constexpr auto errorMessage<DictionaryValue> =
    "Expected dictionary";
template<> constexpr auto errorMessage<DictionaryValueClose> =
    "Expected }";
template<> constexpr auto errorMessage<ListOf<String>> =
    "Expected list of strings separated by ,";
template<> constexpr auto errorMessage<String> =
    "Expected string";
template<> constexpr auto errorMessage<StringDictionaryOpen> =
    "Expected {";
template<> constexpr auto errorMessage<StringDictionaryClose> =
    "Expected }";
template<> constexpr auto errorMessage<StringDictionaryItem> =
    "Expected string : string";
template<> constexpr auto errorMessage<Identifier> =
    "Expected identifier";
template<> constexpr auto errorMessage<MetadataClose> =
    "Expected )";
template<> constexpr auto errorMessage<PathRef> =
    "Expected path reference";
template<> constexpr auto errorMessage<TimeSampleMap> =
    "Expected dictionary of time samples (x : y)";
template<> constexpr auto errorMessage<ConnectValue> =
    "Expected None, path ref, or list of path refs separated by ,";
template<> constexpr auto errorMessage<RelationshipAssignmentClose> =
    "Expected ]";
template<> constexpr auto errorMessage<RelationshipTargetClose> =
    "Expected ]";
template<> constexpr auto errorMessage<RelocatesMapOpen> =
    "Expected {";
template<> constexpr auto errorMessage<RelocatesMapClose> =
    "Expected }";
template<> constexpr auto errorMessage<ReferenceList> =
    "Expected None, reference, or list of references separated by ,";
template<> constexpr auto errorMessage<PayloadList> =
    "Expected None, payload, or list of payloads separated by ,";
template<> constexpr auto errorMessage<InheritsOrSpecializesList> =
    "Expected None, path ref, or list of path refs separated by ,";
template<> constexpr auto errorMessage<NameList> =
    "Expected string or list of strings enclosed in [] separated by ,";
template<> constexpr auto errorMessage<VariantStatementOpen> =
    "Expected {";
template<> constexpr auto errorMessage<VariantStatementClose> =
    "Expected }";
template<> constexpr auto errorMessage<VariantStatementListOpen> =
    "Expected {";
template<> constexpr auto errorMessage<VariantStatementListClose> =
    "Expected }";
template<> constexpr auto errorMessage<VariantStatement> =
    "Expected sequence of child order, property order, prim, property, "
    "or variant sets enclosed in { }";
template<> constexpr auto errorMessage<PrimContents> =
    "Expected child order, property order, prim, property, or variant set";
template<> constexpr auto errorMessage<SublayerListClose> =
    "Expected ]";
template<> constexpr auto errorMessage<NoneOrTypedListValue> =
    "Expected None or [";

////////////////////////////////////////////////////////////////////////
// Common Keyword actions

template <>
struct TextParserAction<KeywordNone>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        Sdf_TextParserCurrentParsingContext parsingContext =
            context.parsingContext.back();
        if (parsingContext ==
            Sdf_TextParserCurrentParsingContext::KeyValueMetadata ||
            parsingContext ==
            Sdf_TextParserCurrentParsingContext::ListOpMetadata)
        {
            // if the value is None, set the string
            // being recorded to None
            context.currentValue = VtValue();
            if (context.values.IsRecordingString())
            {
                context.values.SetRecordedString(std::string("None"));
            }

            // None was the end of that production, so pop back
            // out to the Metadata context
            _PopContext(context);
        }
        else if (parsingContext ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            _SetDefault(context.path, VtValue(SdfValueBlock()), context);
        }
        else if (parsingContext ==
            Sdf_TextParserCurrentParsingContext::RelationshipSpec ||
                parsingContext == 
            Sdf_TextParserCurrentParsingContext::RelationshipAssignment)
        {
            context.relParsingTargetPaths = SdfPathVector();
        }
        else if (parsingContext ==
            Sdf_TextParserCurrentParsingContext::TimeSamples)
        {
            context.timeSamples[context.timeSampleTime] 
                = VtValue(SdfValueBlock());
        }
    }
};

template <>
struct TextParserAction<KeywordCustomData>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // if the current context is Metadata, this signals we need
        // to start a key value metadata production
        // the context previous to the current one (which should be
        // metadata) will tell us the spec the metadata belongs to
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::Metadata)
        {
            Sdf_TextParserCurrentParsingContext specContext =
                context.parsingContext.rbegin()[1];
            SdfSpecType specType = _GetSpecTypeFromContext(specContext);

            _KeyValueMetadataStart(in.string(), specType, context);
            _PushContext(context, 
                         Sdf_TextParserCurrentParsingContext::KeyValueMetadata);
        }
    }
};

template <>
struct TextParserAction<KeywordSymmetryArguments>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // if the current context is Metadata, this signals we need
        // to start a key value metadata production
        // the context previous to the current one (which should be
        // metadata) will tell us the spec the metadata belongs to
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::Metadata)
        {
            Sdf_TextParserCurrentParsingContext specContext =
                context.parsingContext.rbegin()[1];
            SdfSpecType specType = _GetSpecTypeFromContext(specContext);
            _KeyValueMetadataStart(in.string(), specType, context);
            _PushContext(context, 
                         Sdf_TextParserCurrentParsingContext::KeyValueMetadata);
        }
    }
};

template <>
struct TextParserAction<KeywordPermission>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // if we are in metadata, this opens a new parsing context
        // for permission metadata
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::Metadata)
        {
            _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::PermissionMetadata);
        }
    }
};

template <>
struct TextParserAction<KeywordSymmetryFunction>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // if we are in metadata, this opens a new parsing context
        // for symmetry function metadata
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::Metadata)
        {
            _PushContext(context, 
                 Sdf_TextParserCurrentParsingContext::SymmetryFunctionMetadata);

            context.symmetryFunctionName.clear();
        }
    }
};

template <>
struct TextParserAction<KeywordDisplayUnit>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // if we are in metadata, this opens a new parsing context
        // for symmetry function metadata
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::Metadata)
        {
            _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::DisplayUnitMetadata);
        }
    }
};

template <>
struct TextParserAction<KeywordCustom>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.custom = true;
    }
};

////////////////////////////////////////////////////////////////////////
// Basic Type actions

template <>
struct TextParserAction<String>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // obtain the text inside of the quotes
        // we have to first check for multi-line quotes
        // so we know what to pass to Sdf_EvalQuotedString
        std::string evaluatedString;
        const std::string_view inputStrView = in.string_view();
        // firstThree will be clipped to inputStrView's size so no bound checks
        // are needed.
        if(const auto firstThree = inputStrView.substr(0, 3);
           firstThree == "\"\"\"" || firstThree == "'''")
        {
            evaluatedString = Sdf_EvalQuotedString(
                inputStrView.data(), inputStrView.length(), 3, nullptr);
        }
        else
        {
            evaluatedString = Sdf_EvalQuotedString(
                inputStrView.data(), inputStrView.length(), 1, nullptr);
        }

        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::DocMetadata)
        {
            context.data->Set(
                context.path,
                SdfFieldKeys->Documentation,
                VtValue::Take(evaluatedString));
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::Metadata)
        {
            context.data->Set(
                context.path,
                SdfFieldKeys->Comment,
                VtValue::Take(evaluatedString));
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::PrimSpec)
        {
            std::string errorMessage;
            if (!_CreatePrimSpec(evaluatedString, context, errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                    throw PEGTL_NS::parse_error(errorMessage, in);
            }
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::ReorderRootPrims ||
            context.parsingContext.back() == 
            Sdf_TextParserCurrentParsingContext::ReorderNameChildren ||
            context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::ReorderProperties ||
            context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::VariantSetsMetadata)
        {
            context.nameVector.emplace_back(std::move(evaluatedString));
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::VariantSetStatement)
        {
            const SdfAllowed allow = 
                SdfSchema::IsValidVariantIdentifier(evaluatedString);
            if (!allow)
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    allow.GetWhyNot()
                );

                throw PEGTL_NS::parse_error(allow.GetWhyNot(), in);
            }

            context.currentVariantSetNames.push_back(
                    std::move(evaluatedString));
            context.currentVariantNames.emplace_back();
            context.path = context.path.AppendVariantSelection(
                context.currentVariantSetNames.back(), "");
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::VariantStatementList)
        {
            const SdfAllowed allow = 
                SdfSchema::IsValidVariantIdentifier(evaluatedString);
            if (!allow)
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    allow.GetWhyNot()
                );

                throw PEGTL_NS::parse_error(allow.GetWhyNot(), in);
            }

            context.currentVariantNames.back().push_back(evaluatedString);

            // A variant is basically like a new pseudo-root, so we need to push
            // a new item onto our name children stack to store prims defined
            // within this variant.
            context.nameChildrenStack.emplace_back();
            context.propertiesStack.emplace_back();

            std::string variantSetName = context.currentVariantSetNames.back();
            context.path = context.path.GetParentPath()
                .AppendVariantSelection(variantSetName, evaluatedString);

            context.data->CreateSpec(context.path, SdfSpecTypeVariant);
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::KindMetadata)
        {
            context.data->Set(
                context.path,
                SdfFieldKeys->Kind,
                VtValue(TfToken(evaluatedString)));
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::PrefixSubstitutionsMetadata ||
            context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::SuffixSubstitutionsMetadata)
        {
            if (!context.seenStringDictionaryKey)
            {
                // this is the dictionary key
                context.seenStringDictionaryKey = true;
                context.stringDictionaryKey = evaluatedString;
            }
            else
            {
                // this is the dictionary value
                if (!context.values.SetupFactory(std::string("string")))
                {
                    std::string errorMessage =
                        "Unrecognized value typename 'string' for dictionary";
                    Sdf_TextFileFormatParser_Err(
                        context,
                        in.input(),
                        in.position(),
                        errorMessage);

                    throw PEGTL_NS::parse_error(errorMessage, in);
                }

                context.values.AppendValue(evaluatedString);

                std::string errorMessage;
                if (!_ValueSetAtomic(context, errorMessage))
                {
                    Sdf_TextFileFormatParser_Err(
                        context,
                        in.input(),
                        in.position(),
                        errorMessage);

                    throw PEGTL_NS::parse_error(errorMessage, in);
                }

                size_t n = context.currentDictionaries.size();
                context.currentDictionaries[n-2][context.stringDictionaryKey] = 
                    context.currentValue;

                context.seenStringDictionaryKey = false;
                context.stringDictionaryKey.clear();
            }
        }
    }
};

// SplineKnotValue Helpers
template <typename T>
struct _Bundler
{
    void operator()(
        const double valueIn,
        VtValue* const valueOut)
    {
        *valueOut = VtValue(static_cast<T>(valueIn));
    }
};

static VtValue
_BundleSplineValue(
    Sdf_TextParserContext& context,
    const Sdf_ParserHelpers::Value &value)
{
    VtValue result;
    TsDispatchToValueTypeTemplate<_Bundler>(
        context.spline.GetValueType(),
        value.Get<double>(),
        &result);
    return result;
}

static bool
_SetSplineTanWithWidth(
    Sdf_TextParserContext& context,
    const std::string &encoding,
    const double width,
    const VtValue &slopeOrHeight)
{
    if (encoding == "ws") {
        if (context.splineTanIsPre) {
            context.splineKnot.SetPreTanWidth(width);
            context.splineKnot.SetPreTanSlope(slopeOrHeight);
        } else {
            context.splineKnot.SetPostTanWidth(width);
            context.splineKnot.SetPostTanSlope(slopeOrHeight);
        }
        return true;
    } 
    if (encoding == "wh") {
        if (context.splineTanIsPre) {
            context.splineKnot.SetMayaPreTanWidth(width);
            context.splineKnot.SetMayaPreTanHeight(slopeOrHeight);
        } else {
            context.splineKnot.SetMayaPostTanWidth(width);
            context.splineKnot.SetMayaPostTanHeight(slopeOrHeight);
        }
        return true;
    } 
    return false;
}

static bool
_SetSplineTanWithoutWidth(
    Sdf_TextParserContext& context,
    const std::string &encoding,
    const VtValue &slopeOrHeight)
{
    if (encoding == "s") {
        if (context.splineTanIsPre) {
            context.splineKnot.SetPreTanSlope(slopeOrHeight);
        } else {
            context.splineKnot.SetPostTanSlope(slopeOrHeight);
        }
        return true;
    } else if (encoding == "h") {
        if (context.splineTanIsPre) {
            context.splineKnot.SetMayaPreTanHeight(slopeOrHeight);
        } else {
            context.splineKnot.SetMayaPostTanHeight(slopeOrHeight);
        }
        return true;
    } 
    return false;
}

template <class Input>
std::pair<bool, Sdf_ParserHelpers::Value>
_HelperGetNumericValueFromString(const Input &in,
                                 Sdf_TextParserContext &context)
{
    const std::pair<bool, Sdf_ParserHelpers::Value> result =
        _GetNumericValueFromString(in.string_view());
    if (result.first)
    {
        TF_WARN("Integer literal '%s' on line %zu%s%s out of range, "
            "parsing as double.  Consider exponential notation for "
            "large floating point values.", in.string().c_str(), 
            in.position().line, 
            context.fileContext.empty() ? "" : " in file ",
            context.fileContext.empty() ? "" :
            context.fileContext.c_str());
    }
    return result;
}

template <>
struct TextParserAction<Number>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::TimeSamples)
        {
            context.timeSampleTime = result.second.Get<double>();
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::LayerOffset ||
            context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::LayerScale)
        {
            if (context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::LayerOffset)
            {
                context.layerRefOffset.SetOffset(result.second.Get<double>());
            }
            else
            {
                context.layerRefOffset.SetScale(result.second.Get<double>());
            }

            // in either case, we are done with this custom context
            _PopContext(context);
        }
    }
};

template <>
struct TextParserAction<Identifier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        Sdf_TextParserCurrentParsingContext parsingContext =
            context.parsingContext.back();

        if (parsingContext ==
            Sdf_TextParserCurrentParsingContext::DictionaryTypeName)
        {
            context.dictionaryTypeName += in.string();
        }
        else if (parsingContext == 
                Sdf_TextParserCurrentParsingContext::Metadata || 
            parsingContext == 
                Sdf_TextParserCurrentParsingContext::ListOpMetadata)
        {
            // if we are in a metadata context, the identifier
            // is the production in which we start the generic
            // metadata recording, but we need the spec context we are
            // in - for Metadata that's one level deeper on the stack,
            // for ListOpMetadata it's two levels
            Sdf_TextParserCurrentParsingContext specContext =
                parsingContext == 
                    Sdf_TextParserCurrentParsingContext::Metadata ?
                        context.parsingContext.rbegin()[1] :
                        context.parsingContext.rbegin()[2];

            SdfSpecType specType = _GetSpecTypeFromContext(specContext);
            _KeyValueMetadataStart(in.string(), specType, context);
            if (parsingContext == Sdf_TextParserCurrentParsingContext::Metadata)
            {
                // if we were already in a list op context, leave that one
                // as the current, otherwise start a key value context
                _PushContext(context, 
                         Sdf_TextParserCurrentParsingContext::KeyValueMetadata);
            }
        }
        else if (parsingContext ==
            Sdf_TextParserCurrentParsingContext::PermissionMetadata)
        {
            SdfPermission permission;
            std::string permissionStr = in.string();
            if (permissionStr == "public")
            {
                permission = SdfPermissionPublic;
            }
            else if (permissionStr == "private")
            {
                permission = SdfPermissionPrivate;
            }
            else
            {
                std::string errorMessage = "'" +
                    permissionStr + "' is not a valid permission constant";
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                throw PEGTL_NS::parse_error(errorMessage, in);
            }

            context.data->Set(
                context.path,
                SdfFieldKeys->Permission,
                VtValue(permission));

            // this signals the end of the permission
            // metadata context
            _PopContext(context);
        }
        else if (parsingContext ==
            Sdf_TextParserCurrentParsingContext::SymmetryFunctionMetadata)
        {
            context.symmetryFunctionName = in.string();
        }
        else if (parsingContext ==
            Sdf_TextParserCurrentParsingContext::DisplayUnitMetadata)
        {
            const TfEnum &unit = SdfGetUnitFromName(in.string());
            if (unit == TfEnum()) {
                std::string errorMessage = "'" +
                    in.string() + "' is not a valid display unit";
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);
                
                throw PEGTL_NS::parse_error(errorMessage, in);
            }
            context.data->Set(
                context.path,
                SdfFieldKeys->DisplayUnit,
                VtValue(unit));
            
            // this signals the end of the display unit
            // metadata context
            _PopContext(context);
        }
        else if (parsingContext ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            context.attributeTypeName += in.string();
        }
        else if (parsingContext ==
            Sdf_TextParserCurrentParsingContext::PrimSpec)
        {
            // this is broken into two actions (Identifier and Dot)
            // so that we don't have to reparse out the token spacing
            // between the identifier and . if we reduced on the full
            // PrimTypeName rule
            context.primTypeName += in.string();
        } else if (parsingContext==
                Sdf_TextParserCurrentParsingContext::SplineTangent) {
            context.splineTangentIdentifier = in.string();
        }
    }
};

template <>
struct TextParserAction<ArrayType>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        Sdf_TextParserCurrentParsingContext parsingContext =
            context.parsingContext.back();
        if (parsingContext ==
            Sdf_TextParserCurrentParsingContext::DictionaryTypeName)
        {
            context.dictionaryTypeName += "[]";
        }
        else if (parsingContext ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            // the [] is part of the attribute type name
            context.attributeTypeName += "[]";
        }
    }
};

template <>
struct TextParserAction<NamespacedName>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            std::string errorMessage;
            if (!_CreateAttributeSpec(in.string(), context, errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                    throw PEGTL_NS::parse_error(errorMessage, in);
            }

            if (!context.values.valueTypeIsValid) {
                context.values.StartRecordingString();
            }
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::RelationshipSpec)
        {
            std::string errorMessage;
            if (!_CreateRelationshipSpec(in.string(), context, errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                throw PEGTL_NS::parse_error(errorMessage, in);
            }
        }
    }
};

template <>
struct TextParserAction<PathRef>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::ConnectAttribute)
        {
            std::string pathStr = Sdf_EvalQuotedString(
                in.string().c_str(), 
                in.string().length(),
                1);
            
            context.savedPath = SdfPath(pathStr);

            // Valid paths are prim or property paths that do not contain
            // variant selections.
            SdfPath const &path = context.savedPath;
            bool pathValid = (path.IsPrimPath() || path.IsPropertyPath()) &&
                !path.ContainsPrimVariantSelection();
            if (!pathValid) {
                std::string errorMessage = "'" +
                    pathStr + "' is not a valid prim or property scene path";
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                throw PEGTL_NS::parse_error(errorMessage, in);
            }

            // Expand paths relative to the containing prim.
            //
            // This strips any variant selections from the containing prim
            // path before expanding the relative path, which is what we
            // want.  Connection paths never point into the variant namespace.
            SdfPath absPath = context.savedPath.MakeAbsolutePath(
                    context.path.GetPrimPath());

            // XXX Workaround for bug 68132:
            // Prior to the fix to bug 67916, FilterGenVariantBase was authoring
            // connection paths.  As a migration measure, we discard those
            // variant selections here.
            if (absPath.ContainsPrimVariantSelection())
            {
                TF_WARN("Connection path <%s> (in file @%s@, line %zu) has a "
                    "variant selection, but variant selections are not "
                    "meaningful in connection paths.  Stripping the variant "
                    "selection and using <%s> instead.  Resaving the file will "
                    "fix this issue.", absPath.GetText(), 
                    context.fileContext.c_str(),
                    in.position().line,
                    absPath.StripAllVariantSelections().GetText());

                absPath = absPath.StripAllVariantSelections();
            }
            context.connParsingTargetPaths.push_back(absPath);
        }
        else if(context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::InheritsListOpMetadata ||
            context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::SpecializesListOpMetadata)
        {
            const std::string pathStr = Sdf_EvalQuotedString(
                in.string().c_str(), 
                in.string().length(),
                1);
            
            context.savedPath = SdfPath(pathStr);
            if (!context.savedPath.IsPrimPath()) {
                std::string errorMessage = "'" +
                    pathStr + "' is not a valid prim path";
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                throw PEGTL_NS::parse_error(errorMessage, in);
            }

            // Expand paths relative to the containing prim.
            //
            // This strips any variant selections from the containing prim
            // path before expanding the relative path, which is what we
            // want.  Inherit paths are not allowed to be variants.
            SdfPath absPath = 
                context.savedPath.MakeAbsolutePath(context.path.GetPrimPath());

            if (context.parsingContext.back() == 
                Sdf_TextParserCurrentParsingContext::InheritsListOpMetadata)
            {
                context.inheritParsingTargetPaths.push_back(absPath);
            }
            else
            {
                context.specializesParsingTargetPaths.push_back(absPath);
            }
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::RelationshipAssignment ||
            context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::RelationshipTarget ||
            context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::RelationshipSpec)
        {
            std::string pathStr = Sdf_EvalQuotedString(
                in.string().c_str(), 
                in.string().length(),
                1);

            SdfPath path(pathStr);
            if (!path.IsAbsolutePath()) {
                // Expand paths relative to the containing prim.
                //
                // This strips any variant selections from the containing prim
                // path before expanding the relative path, which is what we
                // want.  Target paths never point into the variant namespace.
                path = path.MakeAbsolutePath(context.path.GetPrimPath());
            }

            if (!context.relParsingTargetPaths) {
                // This is the first target we've seen for this relationship.
                // Start tracking them in a vector.
                context.relParsingTargetPaths = SdfPathVector();
            }
            context.relParsingTargetPaths->push_back(path);
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::RelationshipDefault)
        {
            std::string pathStr = Sdf_EvalQuotedString(
                in.string().c_str(), 
                in.string().length(),
                1);

            // If path is empty, use default c'tor to construct empty path.
            // XXX: 08/04/08 Would be nice if SdfPath would allow 
            // SdfPath("") without throwing a warning.
            SdfPath path = pathStr.empty() ? SdfPath() : SdfPath(pathStr);
            context.data->Set(context.path, SdfFieldKeys->Default, 
                              VtValue(path));

            if (!context.relParsingNewTargetChildren.empty()) {
                std::vector<SdfPath> children = 
                    context.data->GetAs<std::vector<SdfPath> >(
                        context.path, 
                            SdfChildrenKeys->RelationshipTargetChildren);

                children.insert(children.end(), 
                                context.relParsingNewTargetChildren.begin(),
                                context.relParsingNewTargetChildren.end());
                
                context.data->Set(
                    context.path,
                    SdfChildrenKeys->RelationshipTargetChildren,
                    VtValue(children));
            }

            _PopContext(context);
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::RelocatesMetadata)
        {
            std::string pathStr = Sdf_EvalQuotedString(
                in.string().c_str(), 
                in.string().length(),
                1);

            SdfPath path = SdfPath(pathStr);
            if (!context.seenFirstRelocatesPath)
            {
                // this is the first relocates path 
                // (corresponds to the srcPath)
                // store it - we will use it when we see
                // the relocates value
                context.relocatesKey = path;

                // Verify we have a valid sourcePath for relocates.
                if (!SdfSchema::IsValidRelocatesSourcePath(
                        context.relocatesKey)) {
                    const std::string errorMessage = "'" + 
                            context.relocatesKey.GetAsString() + 
                            "' is not a valid relocates source path";
                    Sdf_TextFileFormatParser_Err(
                        context,
                        in.input(),
                        in.position(),
                        errorMessage);

                    throw PEGTL_NS::parse_error(errorMessage, in);
                }
                context.seenFirstRelocatesPath = true;
            }
            else
            {
                // We have our srcPath saved from previous iteration, get
                // relocate targetPath

                // Target paths can be empty but the corresponding string must 
                // be explicitly empty, if not it indicates a malformed path
                // which is never valid.
                if ( (path.IsEmpty() && !pathStr.empty()) ||
                     !SdfSchema::IsValidRelocatesTargetPath(path)) {
                    const std::string errorMessage = "'" +
                        context.relocatesKey.GetAsString() +
                        "' is not a valid relocates target path";
                    Sdf_TextFileFormatParser_Err(
                        context,
                        in.input(),
                        in.position(),
                        errorMessage);

                    throw PEGTL_NS::parse_error(errorMessage, in);
                }

                // The relocates map is expected to only hold absolute paths.
                // The SdRelocatesMapProxy ensures that all paths are made
                // absolute when editing, but since we're bypassing that proxy
                // and setting the map directly into the underlying SdfData, we
                // need to explicitly absolutize paths here.
                const SdfPath srcPath = 
                    context.relocatesKey.MakeAbsolutePath(context.path);
                const SdfPath targetPath =
                    path.MakeAbsolutePath(context.path);

                context.relocatesParsing.emplace_back(std::move(srcPath), 
                    std::move(targetPath));
                context.layerHints.mightHaveRelocates = true;

                context.relocatesKey = SdfPath();
                context.seenFirstRelocatesPath = false;
            }
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::ReferencesListOpMetadata ||
            context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::PayloadListOpMetadata)
        {
            std::string pathStr = Sdf_EvalQuotedString(
                in.string().c_str(), 
                in.string().length(),
                1);

            SdfPath path = SdfPath(pathStr);
            if (path.IsEmpty())
            {
                context.savedPath = SdfPath::EmptyPath();
            }
            else
            {
                if (!path.IsPrimPath())
                {
                    std::string errorMessage = "'" +
                        pathStr + "' is not a valid prim path";
                    Sdf_TextFileFormatParser_Err(
                        context,
                        in.input(),
                        in.position(),
                        errorMessage);

                    throw PEGTL_NS::parse_error(errorMessage, in);
                }

                context.savedPath = path;
            }
        }
    }
};

template <>
struct TextParserAction<AssetRef>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::ReferencesListOpMetadata ||
            context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::PayloadListOpMetadata)
        {
            const std::string_view inputStrView = in.string_view();
            const bool isTripleDelimited = (inputStrView.substr(0, 3) == "@@@");
            std::string evaluatedAssetPath = Sdf_EvalAssetPath(
                inputStrView.data(),
                inputStrView.length(),
                isTripleDelimited);

            if (evaluatedAssetPath.empty())
            {
                std::string errorMessage =
                    "Reference / payload asset path must not be empty. If this "
                    "is intended to be an internal reference / payload, "
                    "remove the '@' delimiters.";
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                    throw PEGTL_NS::parse_error(errorMessage, in);
            }

            context.layerRefPath = std::move(evaluatedAssetPath);
            context.layerRefOffset = SdfLayerOffset();
            context.savedPath = SdfPath::EmptyPath();
        }
        else if(context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::SubLayerMetadata)
        {
            const std::string_view inputStrView = in.string_view();
            const bool isTripleDelimited = (inputStrView.substr(0, 3) == "@@@");
            std::string evaluatedAssetPath = Sdf_EvalAssetPath(
                inputStrView.data(),
                inputStrView.length(),
                isTripleDelimited);

            context.layerRefPath = std::move(evaluatedAssetPath);
            context.layerRefOffset = SdfLayerOffset();
        }
    }
};

////////////////////////////////////////////////////////////////////////
// Value actions

template <>
struct TextParserAction<NumberValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        context.values.AppendValue(result.second);
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
        context.values.AppendValue(TfToken(in.string()));
    }
};

template <>
struct TextParserAction<StringValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::string_view inputStrView = in.string_view();
        size_t numDelimeters = 1;
        // firstThree will be clipped to inputStrView's size so no bound checks
        // are needed.
        if(const auto firstThree = inputStrView.substr(0, 3);
           firstThree == "\"\"\"" || firstThree == "'''")
        {
            numDelimeters = 3;
        }

        std::string evaluatedString = Sdf_EvalQuotedString(
            inputStrView.data(),
            inputStrView.length(),
            numDelimeters);

        TF_DEBUG(SDF_TEXT_FILE_FORMAT_CONTEXT).Msg(
            "String value: " + evaluatedString + "\n");

        context.values.AppendValue(evaluatedString);
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
        const std::string_view inputStrView = in.string_view();
        const bool isTripleDelimited = (inputStrView.substr(0, 3) == "@@@");
        std::string evaluatedAssetPath = Sdf_EvalAssetPath(
            inputStrView.data(),
            inputStrView.length(),
            isTripleDelimited);

        context.values.AppendValue(SdfAssetPath(evaluatedAssetPath));
    }
};

template <>
struct TextParserAction<AtomicValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this is the atomic value and we are completely
        // finished reducing it
        std::string errorMessage;
        if (!_ValueSetAtomic(context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }
    }
};

template <>
struct TextParserAction<PathRefValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string inputString = in.string();
        std::string pathRef = Sdf_EvalQuotedString(
            inputString.c_str(),
            inputString.length(),
            1);

        context.currentValue = pathRef.empty() ? SdfPath() :
            SdfPath(pathRef);
    }
};

template <>
struct TextParserAction<TypedValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::KeyValueMetadata)
        {
            _PopContext(context);
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            _SetDefault(context.path, context.currentValue, context);
        }
        else if(context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::TimeSamples)
        {
            context.timeSamples[context.timeSampleTime] =
                context.currentValue;
        }
    }
};

template <>
struct TextParserAction<NameList>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::ReorderRootPrims)
        {
            context.data->Set(context.path,
                SdfFieldKeys->PrimOrder,
                VtValue(context.nameVector));

            context.nameVector.clear();

            _PopContext(context);
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::VariantSetsMetadata)
        {
            // this evaluation is done here rather than in a reduction for
            // VariantSetsMetadata because list op keyword are greedy and
            // when there is a list op keyword there is no production for
            // VariantSetsMetadata, even though the interior productions
            // are the same
            std::vector<std::string> names;
            names.reserve(context.nameVector.size());
            for (const auto& name : context.nameVector) {
                const SdfAllowed allow =
                        SdfSchema::IsValidVariantIdentifier(name);
                    if (!allow)
                    {
                        Sdf_TextFileFormatParser_Err(
                            context,
                            in.input(),
                            in.position(),
                            allow.GetWhyNot());

                        throw PEGTL_NS::parse_error(allow.GetWhyNot(), in);
                    }
                names.push_back(name);
            }

            std::string errorMessage;
            if (!_SetListOpItemsWithError(SdfFieldKeys->VariantSetNames,
                context.listOpType, names, context, errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                throw PEGTL_NS::parse_error(errorMessage, in);
            }

            // If the op type is added or explicit, create the variant sets
            if (context.listOpType == SdfListOpTypeAdded ||
                context.listOpType == SdfListOpTypeExplicit) {
                for (const auto& i : context.nameVector) {
                    context.data->CreateSpec(
                        context.path.AppendVariantSelection(i, ""),
                        SdfSpecTypeVariantSet);
                }

                context.data->Set(
                    context.path,
                    SdfChildrenKeys->VariantSetChildren,
                    VtValue(context.nameVector));
            }

            context.nameVector.clear();
            context.listOpType = SdfListOpTypeExplicit;

            // all done parsing the variant sets metadata
            _PopContext(context);

            // if the operation was a list op, there is a listopMetadata
            // context on the stack that also needs to be popped
            if (context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::ListOpMetadata)
            {
                _PopContext(context);
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////
// Tuple actions

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
struct TextParserAction<TypedTupleValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_ValueSetTuple(context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }
    }
};

////////////////////////////////////////////////////////////////////////
// List actions

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
struct TextParserAction<TypedListValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string errorMessage;
        if (!_ValueSetList(context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }

        // if were were parsing list op metadata, this signals the end
        // of that context
        Sdf_TextParserCurrentParsingContext parsingContext =
            context.parsingContext.back();
        if (parsingContext == 
                Sdf_TextParserCurrentParsingContext::ListOpMetadata)
        {
          // pop back out to the metadata context - the list op type will get
          // reset when we reduce the final list op metadata production
          _PopContext(context);
        }
    }
};

template <>
struct TextParserAction<EmptyListValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // it's an array type
        // Set the recorded string on the ParserValueContext. Normally
        // 'values' is able to keep track of the parsed string, but in this
        // case it doesn't get the BeginList() and EndList() calls so the
        // recorded string would have been "". We want "[]" instead.
        if (context.values.IsRecordingString()) {
            context.values.SetRecordedString("[]");
        }

        std::string errorMessage;
        if(!_ValueSetShaped(context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }
    }
};

////////////////////////////////////////////////////////////////////////
// Dictionary actions

template <>
struct TextParserAction<DictionaryValueOpen>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // reset dictionary state
        // dictionaries can be nested, so the expectation
        // of a dictionary value as well as the current key
        // are stack-based
        context.expectDictionaryValue.push_back(false);
        context.dictionaryTypeName.clear();

        // set context to expect to parse a type name
        // (unless we see the dictionary keyword)
        _PushContext(context, Sdf_TextParserCurrentParsingContext::Dictionary);
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::DictionaryTypeName);

        context.currentDictionaries.push_back(VtDictionary());

        // Whenever we parse a value for an unregistered generic metadata field,
        // the parser value context records the string representation only,
        // because we don't have enough type information to generate a C++
        // value. However, dictionaries are a special case because we have all
        // the type information we need to generate C++ values. So, override the
        // previous setting.
        if (context.values.IsRecordingString()) {
            context.values.StopRecordingString();
        }
    }
};

template <>
struct TextParserAction<DictionaryValueClose>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // when we hit a dictionary close, we need to pop the
        // expectation of a dictionary value
        context.expectDictionaryValue.pop_back();
        
        // we also need to pop off two contexts
        // the first is the DictionaryTypeName, which we pushed
        // expecting the next key / value pair
        // the second is the Dictionary context, which is now done
        context.currentDictionaries.pop_back();
        _PopContext(context);
        _PopContext(context);
    }
};

template <>
struct TextParserAction<KeywordDictionary>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // our initial guess is that a dictionary value won't be expected
        // so we pushed false onto the stack, however, here we have to
        // replace that value
        context.expectDictionaryValue.pop_back();
        context.expectDictionaryValue.push_back(true);

        // pop off the type name state as we won't be parsing that
        _PopContext(context);

        // push the dictionary key state
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::DictionaryKey);
    }
};

template <>
struct TextParserAction<DictionaryType>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // pop off the type name state as we are done
        _PopContext(context);

        if (!context.values.SetupFactory(context.dictionaryTypeName))
        {
            std::string errorMessage = "Unrecognized value typename '" +
                context.dictionaryTypeName + "' for dictionary";
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }

        // push the dictionary key context
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::DictionaryKey);
    }
};

template <>
struct TextParserAction<DictionaryKey>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        std::string dictionaryKey = in.string();
        if (TfStringStartsWith(dictionaryKey, "\"") || 
            TfStringStartsWith(dictionaryKey, "'"))
        {
            size_t numDelimeters = 1;
            if(TfStringStartsWith(dictionaryKey, "\"\"\"") ||
                TfStringStartsWith(dictionaryKey, "'''"))
            {
                numDelimeters = 3;
            }

            dictionaryKey = Sdf_EvalQuotedString(
                dictionaryKey.c_str(),
                dictionaryKey.length(),
                numDelimeters);
        }

        context.currentDictionaryKey.push_back(dictionaryKey);

        // done the dictionary key context
        _PopContext(context);
    }
};

template <>
struct TextParserAction<DictionaryValueItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // we've matched the value at this point, and we know whether
        // or not the value should be a dictionary or typed value
        // the last thing we need to do is insert the key / value pair
        if (context.expectDictionaryValue.back())
        {
            size_t n = context.currentDictionaries.size();
            // Insert the parsed dictionary into the parent dictionary.
            context.currentDictionaries[n-2][context.currentDictionaryKey.back()].Swap(
                context.currentDictionaries[n-1]);
            // Clear out the last dictionary (there can be more dictionaries on
            // the same nesting level).
            context.currentDictionaries[n-1].clear();
        }
        else
        {
            size_t n = context.currentDictionaries.size();
            context.currentDictionaries[n-2][context.currentDictionaryKey.back()] = 
                context.currentValue;
        }

        // expect the next dictionary type name
        // and reset dictionary state
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::DictionaryTypeName);

        context.expectDictionaryValue.pop_back();
        context.currentDictionaryKey.pop_back();
        context.dictionaryTypeName.clear();

        // expect the next value to be a typed value
        context.expectDictionaryValue.push_back(false);
    }
};

template <>
struct TextParserAction<DictionaryValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::KeyValueMetadata)
        {
            // it's a dictionary, we need to ensure the current
            // value that gets set in the context is the dictionary
            // we've been parsing
            context.currentValue.Swap(context.currentDictionaries[0]);
            context.currentDictionaries[0].clear();

            _PopContext(context);
        }
    }
};

template <>
struct TextParserAction<StringDictionaryOpen>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.currentDictionaries.emplace_back();

        // Whenever we parse a value for an unregistered generic metadata field,
        // the parser value context records the string representation only,
        // because we don't have enough type information to generate a C++
        // value. However, dictionaries are a special case because we have all
        // the type information we need to generate C++ values. So, override the
        // previous setting.
        if (context.values.IsRecordingString()) {
            context.values.StopRecordingString();
        }

        context.seenStringDictionaryKey = false;
        context.stringDictionaryKey.clear();
    }
};

template <>
struct TextParserAction<StringDictionaryClose>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.currentDictionaries.pop_back();
    }
};

////////////////////////////////////////////////////////////////////////
// Common Metadata actions

template <>
struct TextParserAction<MetadataOpen>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, Sdf_TextParserCurrentParsingContext::Metadata);

        context.listOpType = SdfListOpTypeExplicit;
    }
};

template <>
struct TextParserAction<MetadataClose>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PopContext(context);
    }
};

template <>
struct TextParserAction<KeyValueMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        Sdf_TextParserCurrentParsingContext specContext =
            context.parsingContext.rbegin()[1];
        SdfSpecType specType = _GetSpecTypeFromContext(specContext);

        std::string errorMessage;
        if(!_KeyValueMetadataEnd(specType, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }
        
        // no need to pop the parsing context as it was already popped in
        // the individual reductions for None, TypedValue, and DictionaryValue
        context.listOpType = SdfListOpTypeExplicit;
    }
};

template <>
struct TextParserAction<GeneralListOpMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        Sdf_TextParserCurrentParsingContext specContext =
            context.parsingContext.rbegin()[1];
        SdfSpecType specType = _GetSpecTypeFromContext(specContext);

        std::string errorMessage;
        if (!_KeyValueMetadataEnd(specType, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }
        
        // no need to pop the parsing context as it was already popped in
        // the individual reductions for None, TypedValue, and DictionaryValue
        // but we do need to reset the list op type (we couldn't reset it 
        // before because _KeyValueMetadataEnd needed it and this reduces
        // after the TypedValue)
        context.listOpType = SdfListOpTypeExplicit;
    }
};

template <>
struct TextParserAction<ListOpKeyValueMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        Sdf_TextParserCurrentParsingContext specContext =
            context.parsingContext.rbegin()[1];
        SdfSpecType specType = _GetSpecTypeFromContext(specContext);

        std::string errorMessage;
        if (!_KeyValueMetadataEnd(specType, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }
        
        // no need to pop the parsing context as it was already popped in
        // the individual reductions for None, TypedValue, and DictionaryValue
        // but we do need to reset the list op type (we couldn't reset it 
        // before because _KeyValueMetadataEnd needed it and this reduces
        // after the TypedValue)
        context.listOpType = SdfListOpTypeExplicit;
    }
};

template <>
struct TextParserAction<KeywordDoc>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, Sdf_TextParserCurrentParsingContext::DocMetadata);
    }
};

template <>
struct TextParserAction<DocMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // finished the DocMetadata context
        _PopContext(context);
    }
};

template <>
struct TextParserAction<SymmetryFunctionMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this signals the end of the symmetry function
        // metadata context - if symmetryFunctionName is empty
        // we set it as empty
        context.data->Set(
            context.path,
            SdfFieldKeys->SymmetryFunction,
            context.symmetryFunctionName.empty() ? VtValue(TfToken()) :
            VtValue(TfToken(context.symmetryFunctionName)));

        _PopContext(context);
    }
};

////////////////////////////////////////////////////////////////////////
// Listop actions

template <>
struct TextParserAction<KeywordAdd>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::Metadata)
        {
            context.listOpType = SdfListOpTypeAdded;
            _PushContext(context, 
                         Sdf_TextParserCurrentParsingContext::ListOpMetadata);
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            context.listOpType = SdfListOpTypeAdded;
        }
    }
};

template <>
struct TextParserAction<KeywordDelete>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::Metadata)
        {
            context.listOpType = SdfListOpTypeDeleted;
            _PushContext(context, 
                         Sdf_TextParserCurrentParsingContext::ListOpMetadata);
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            context.listOpType = SdfListOpTypeDeleted;
        }
    }
};

template <>
struct TextParserAction<KeywordAppend>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::Metadata)
        {
            context.listOpType = SdfListOpTypeAppended;
            _PushContext(context, 
                         Sdf_TextParserCurrentParsingContext::ListOpMetadata);
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            context.listOpType = SdfListOpTypeAppended;
        }
    }
};

template <>
struct TextParserAction<KeywordPrepend>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::Metadata)
        {
            context.listOpType = SdfListOpTypePrepended;
            _PushContext(context, 
                         Sdf_TextParserCurrentParsingContext::ListOpMetadata);
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            context.listOpType = SdfListOpTypePrepended;
        }
    }
};

template <>
struct TextParserAction<KeywordReorder>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::Metadata)
        {
            context.listOpType = SdfListOpTypeOrdered;
            _PushContext(context, 
                         Sdf_TextParserCurrentParsingContext::ListOpMetadata);
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            context.listOpType = SdfListOpTypeOrdered;
        }
    }
};

////////////////////////////////////////////////////////////////////////
// Attribute actions

template <>
struct TextParserAction<Sdf_PathParser::Dot>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::PrimSpec)
        {
            context.primTypeName += ".";
        }
    }
};

template <>
struct TextParserAction<KeywordVarying>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.variability = VtValue(SdfVariabilityVarying);
    }
};

template <>
struct TextParserAction<KeywordConfig>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // convert legacy "config" variability to SdfVariabilityUniform
        // this value was never officially used in USD but we handle
        // this just in case the value was written outiform = "uniform";
        context.variability = VtValue(SdfVariabilityUniform);
    }
};

template <>
struct TextParserAction<KeywordUniform>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.variability = VtValue(SdfVariabilityUniform);
    }
};

template <>
struct TextParserAction<AttributeVariability>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.assoc = VtValue();
    }
};

template <>
struct TextParserAction<AttributeType>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // once the AttributeType is reduced, we have the full
        // type name of the attribute stored in attributeTypeName
        // so we know what type the attribute is at this point
        // (Note: we build the type name by Identifier and ArrayType
        // reductions rather than here because otherwise we'd have
        // to parse out the AttributeVariability as well)
        context.values.SetupFactory(context.attributeTypeName);
    }  
};

template <>
struct TextParserAction<AttributeAssignmentOptional>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (!context.values.valueTypeIsValid) {
            context.values.StopRecordingString();
        }
    }
};

template <>
struct TextParserAction<AttributeSpec>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // finished parsing an attribute spec
        // reset the assumption that we are going to parse an
        // attribute next unless a keyword tells us otherwise
        // note that the parsing context on the stack simply remains
        // it will get removed by e.g., relation if it's the wrong one
        context.custom = false;
        context.variability = VtValue();
        context.attributeTypeName.clear();
    }
};

template <>
struct TextParserAction<KeywordTimeSamples>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.timeSamples = SdfTimeSampleMap();

        _PushContext(context, Sdf_TextParserCurrentParsingContext::TimeSamples);
    }
};

template <>
struct TextParserAction<TimeSampleMap>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.data->Set(
                context.path,
                SdfFieldKeys->TimeSamples,
                VtValue(context.timeSamples));

        _PopContext(context);
    }
};

template <>
struct TextParserAction<KeywordSpline>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const TfType valueType =
            SdfGetTypeForValueTypeName(
                TfToken(context.values.valueTypeName));
        if (valueType == TfType::Find<SdfTimeCode>()) {
          // Special case for timecode-valued attributes: physically use double,
          // but set the flag that causes layer offsets to be applied to values
          // as well as times.
          context.splineValid = true;
          context.spline = TsSpline(TfType::Find<double>());
          context.spline.SetTimeValued(true);
        } else {
            // Are splines valid for this value type?
            context.splineValid = TsSpline::IsSupportedValueType(valueType);
            if (context.splineValid) {
                // Normal case.  Set up a spline to parse into.
                context.spline = TsSpline(valueType);
            }
            else {
                std::string errorMessage = "Unsupported spline value type " + 
                        valueType.GetTypeName() + "for context value: " + 
                        context.values.valueTypeName 
                    + "and attribute time name: " + context.attributeTypeName;
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);
                throw PEGTL_NS::parse_error(errorMessage, in);
            }
        }
        context.splineKnotMap.clear();
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::SplineValues);
        // Assume we will get a SplineKnotItem, and if we get other SplineItem
        // we pop this there, by checking
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::SplineKnotItem);
    }
};

template <>
struct TextParserAction<SplineItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // We are done with this SplineItem, anticipate another SplineKnotItem
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::SplineKnotItem);
    }
};

template <>
struct TextParserAction<KeywordBezier>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // We landed on a curve item, pop the anticipated SplineItem context
        _PopContext(context);
        context.spline.SetCurveType(TsCurveTypeBezier);
    }
};

template <>
struct TextParserAction<KeywordHermite>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // We landed on a curve item, pop the anticipated SplineItem context
        _PopContext(context);
        context.spline.SetCurveType(TsCurveTypeHermite);
    }
};

template <>
struct TextParserAction<KeywordNone_LC>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() == 
                Sdf_TextParserCurrentParsingContext::SplinePostExtrapItem ||
            context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::SplinePreExtrapItem) {
            context.splineExtrap = TsExtrapolation(TsExtrapValueBlock);
        } else if (context.parsingContext.back() == 
            Sdf_TextParserCurrentParsingContext::SplineInterpMode) {
            context.splineKnot.SetNextInterpolation(TsInterpValueBlock);
            // SplineInterpMode context will be popped in its action
        }
    }
};

template <>
struct TextParserAction<KeywordHeld>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() == 
                Sdf_TextParserCurrentParsingContext::SplinePostExtrapItem ||
            context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::SplinePreExtrapItem) {
            context.splineExtrap = TsExtrapolation(TsExtrapHeld);
        } else if (context.parsingContext.back() == 
            Sdf_TextParserCurrentParsingContext::SplineInterpMode) {
            context.splineKnot.SetNextInterpolation(TsInterpHeld);
            // SplineInterpMode context will be popped in its action
        }
    }
};

template <>
struct TextParserAction<KeywordLinear>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() == 
                Sdf_TextParserCurrentParsingContext::SplinePostExtrapItem ||
            context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::SplinePreExtrapItem) {
            context.splineExtrap = TsExtrapolation(TsExtrapLinear);
        } else if (context.parsingContext.back() == 
            Sdf_TextParserCurrentParsingContext::SplineInterpMode) {
            context.splineKnot.SetNextInterpolation(TsInterpLinear);
            // SplineInterpMode context will be popped in its action
        }
    }
};

template <>
struct TextParserAction<KeywordCurve>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() == 
            Sdf_TextParserCurrentParsingContext::SplineInterpMode) {
            context.splineKnot.SetNextInterpolation(TsInterpCurve);
            // SplineInterpMode context will be popped in its action
        }
    }
};

template <>
struct TextParserAction<KeywordSloped>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::SplineExtrapSloped);
    }
};

template <>
struct TextParserAction<SlopeValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        context.splineExtrap = TsExtrapolation(TsExtrapSloped);
        context.splineExtrap.slope = result.second.Get<double>();
        // Pop SplineExtrapSloped context
        _PopContext(context);
    }
};

template <>
struct TextParserAction<SplineLoopItemProtoStart>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        context.splineLoopItem[0] = result.second.Get<double>();
    }
};

template <>
struct TextParserAction<SplineLoopItemProtoEnd>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        context.splineLoopItem[1] = result.second.Get<double>();
    }
};

template <>
struct TextParserAction<SplineLoopItemNumPreLoops>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        context.splineLoopItem[2] = result.second.Get<double>();
    }
};

template <>
struct TextParserAction<SplineLoopItemNumPostLoops>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        context.splineLoopItem[3] = result.second.Get<double>();
    }
};

template <>
struct TextParserAction<SplineLoopItemValueOffset>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        context.splineLoopItem[4] = result.second.Get<double>();
    }
};

template <>
struct TextParserAction<KeywordLoop>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::SplineKnotItem) {
            // We had anticipated getting a SplineKnotItem, but we did not get
            // that; pop it.
            _PopContext(context);
        }

        if (context.parsingContext.back() == 
                Sdf_TextParserCurrentParsingContext::SplinePostExtrapItem ||
            context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::SplinePreExtrapItem) {
            _PushContext(context,
                     Sdf_TextParserCurrentParsingContext::SplineKeywordLoop);
        }
    }
};

template <>
struct TextParserAction<SplineLoopItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const double numPreLoops = context.splineLoopItem[2];
        const double numPostLoops = context.splineLoopItem[3];
        if (std::trunc(numPreLoops) != numPreLoops
                || std::trunc(numPostLoops) != numPostLoops) {
            std::string errorMessage =
                "SplineLoopItem: Non-integer loop count";
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);
            throw PEGTL_NS::parse_error(errorMessage, in);
        }
        TsLoopParams lp;
        lp.protoStart = context.splineLoopItem[0];
        lp.protoEnd = context.splineLoopItem[1];
        lp.numPreLoops = context.splineLoopItem[2];
        lp.numPostLoops = context.splineLoopItem[3];
        lp.valueOffset = context.splineLoopItem[4];
        context.spline.SetInnerLoopParams(lp);
        context.splineLoopItem = {};
    }
};

template <>
struct TextParserAction<KeywordRepeat>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() == 
            Sdf_TextParserCurrentParsingContext::SplineKeywordLoop)
        {
            context.splineExtrap = TsExtrapolation(TsExtrapLoopRepeat);
            // Pop the SplineKeywordLoop context
            _PopContext(context);
        }
    }
};

template <>
struct TextParserAction<KeywordReset>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() == 
            Sdf_TextParserCurrentParsingContext::SplineKeywordLoop)
        {
            context.splineExtrap = TsExtrapolation(TsExtrapLoopReset);
            // Pop the SplineKeywordLoop context
            _PopContext(context);
        }
    }
};

template <>
struct TextParserAction<KeywordOscillate>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() == 
            Sdf_TextParserCurrentParsingContext::SplineKeywordLoop)
        {
            context.splineExtrap = TsExtrapolation(TsExtrapLoopOscillate);
            // Pop the SplineKeywordLoop context
            _PopContext(context);
        }
    }
};

template <>
struct TextParserAction<SplineKnotValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        context.splineKnotValue = result.second;
    }
};

template <>
struct TextParserAction<SplineKnotPreValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        context.splineKnotPreValue = result.second;
    }
};

template <>
struct TextParserAction<SplineKnotTime>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        context.splineKnot = TsKnot(
                context.spline.GetValueType(), 
                context.spline.GetCurveType());
        context.splineKnot.SetTime(result.second.Get<double>());
        // We should get SplineKnotValue next;
        context.splineKnotValue = Sdf_ParserHelpers::Value();
        context.splineKnotPreValue = Sdf_ParserHelpers::Value();
    }
};

template <>
struct TextParserAction<SplineKnotItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // Possible we will have a context of SplineKnotParam, which needs
        // to be popped also.
        if (context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::SplineKnotParam) {
            _PopContext(context);
        }
        // Done with this SplineKnotItem insert it.
        _PopContext(context);
        context.splineKnotMap.insert(context.splineKnot);
    }
};

template <>
struct TextParserAction<SplineKnotValueWithoutPreValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.splineKnot.SetValue(
            _BundleSplineValue(context, context.splineKnotValue));
    }
};

template <>
struct TextParserAction<SplineKnotValueWithPreValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.splineKnot.SetPreValue(
            _BundleSplineValue(context, context.splineKnotPreValue));
        context.splineKnot.SetValue(
            _BundleSplineValue(context, context.splineKnotValue));
    }
};

template <>
struct TextParserAction<SplineKnotValues>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // Done with SplineKnotValue
        // Anticipate SplineKnotParam now, so push its context.
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::SplineKnotParam);
    }
};

template <>
struct TextParserAction<SplineKnotParam>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // Possible we parsed a dictionary
        context.splineKnot.SetCustomData(context.currentDictionaries[0]);
        context.currentDictionaries[0].clear();
        // Done with this SplineKnotParam, but we anticipate another
        // SplineKnotParam, so keep the context.
    }
};

template <>
struct TextParserAction<SplineTangentWithoutWidthValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // Set the parsed tangent value
        if (!_SetSplineTanWithoutWidth(context, context.splineTangentIdentifier, 
                _BundleSplineValue(context, context.splineTangentValue))) {
            const std::string errorMessage = "Unrecognized spline tangent "
                "encoding " + context.splineTangentIdentifier;
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);
            throw PEGTL_NS::parse_error(errorMessage, in);
        }
    }
};

template <>
struct TextParserAction<SplineTangentWithWidthValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (!_SetSplineTanWithWidth(context, context.splineTangentIdentifier, 
                context.splineTangentWidthValue.Get<double>(), 
                _BundleSplineValue(
                    context, context.splineTangentValue)) ) {
            const std::string errorMessage = "Unrecognized spline "
                "tangent encoding " + context.splineTangentIdentifier;
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);
            throw PEGTL_NS::parse_error(errorMessage, in);
        }
    }
};

template <>
struct TextParserAction<SplineTangentValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        context.splineTangentValue = result.second;
    }
};

template <>
struct TextParserAction<SplineTangentWidth>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        const std::pair<bool, Sdf_ParserHelpers::Value> result =
            _HelperGetNumericValueFromString(in, context);
        context.splineTangentWidthValue = result.second;
    }
};

template <>
struct TextParserAction<SplineTangent>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PopContext(context);
    }
};

template <>
struct TextParserAction<KeywordPre>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::SplineKnotItem) {
            // We had anticipated getting a SplineKnotItem, but we did not get
            // that; pop it.
            _PopContext(context);
        }
        if (context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::SplineValues) {
            // We are still in spline values and it seems we will be getting a
            // SplinePreExtrapItem next, push that context.
            _PushContext(context,
                     Sdf_TextParserCurrentParsingContext::SplinePreExtrapItem);
            return;
        }
        if (context.parsingContext.back() == 
            Sdf_TextParserCurrentParsingContext::SplineKnotParam) {
            context.splineTanIsPre = true;
            // We should get a SplineTangent now
            context.splineTangentValue = Sdf_ParserHelpers::Value();
            context.splineTangentWidthValue = Sdf_ParserHelpers::Value();
            _PushContext(context,
                         Sdf_TextParserCurrentParsingContext::SplineTangent);
        }
    }
};

template <>
struct TextParserAction<SplinePreExtrapItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.spline.SetPreExtrapolation(context.splineExtrap);
        // Done with SplinePreExtrapItem
        _PopContext(context);
    }
};

template <>
struct TextParserAction<KeywordPost>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::SplineKnotItem) {
            // We had anticipated getting a SplineKnotItem, but we did not get
            // that; pop it.
            _PopContext(context);
        }

        if (context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::SplineValues) {
            // We are still in spline values and it seems we will be getting a
            // SplinePostExtrapItem next, push that context.
            _PushContext(context,
                     Sdf_TextParserCurrentParsingContext::SplinePostExtrapItem);
            return;
        }
        if (context.parsingContext.back() == 
            Sdf_TextParserCurrentParsingContext::SplineKnotParam) {
            context.splineTanIsPre = false;
            // We anticipate a SplineInterpMode here now.
            _PushContext(context,
                         Sdf_TextParserCurrentParsingContext::SplineInterpMode);
        }
    }
};

template <>
struct TextParserAction<SplinePostExtrapItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.spline.SetPostExtrapolation(context.splineExtrap);
        // Done with SplinePostExtrapItem
        _PopContext(context);
    }
};

template <>
struct TextParserAction<SplineInterpMode>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // Done with this SplineInterpMode, Pop it.
        _PopContext(context);
        // Anticipate a SplineTangent, which could be empty, so we need to check
        // this in SplinePostShaping action, else SplineTangent will be popped
        // in its matching action.
        context.splineTangentValue = Sdf_ParserHelpers::Value();
        context.splineTangentWidthValue = Sdf_ParserHelpers::Value();
        _PushContext(context,
                     Sdf_TextParserCurrentParsingContext::SplineTangent);
    }
};

template <>
struct TextParserAction<SplinePostShaping>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // We had anticipated a SplineTangent but possible we never got it;
        // check and pop it out
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::SplineTangent) {
            _PopContext(context);
        }
    }
};

template <>
struct TextParserAction<SplineValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.splineValid) {
            // Transfer knots to spline.  Don't de-regress on read.
            if (!context.splineKnotMap.empty()) {
                TsAntiRegressionAuthoringSelector selector(
                        TsAntiRegressionNone);
                context.spline.SetKnots(context.splineKnotMap);
            }
            context.data->Set(
                context.path, 
                SdfFieldKeys->Spline,
                VtValue(context.spline));
        }
        // We are done with our splineValue, pop our assumed SplineKnotItem and
        // then pop SplineValues context
        _PopContext(context);
        _PopContext(context);
    }
};

template <>
struct TextParserAction<KeywordConnect>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::ConnectAttribute);

        context.connParsingTargetPaths.clear();
        context.connParsingAllowConnectionData = true;
        if (!context.values.valueTypeIsValid) {
            context.values.StopRecordingString();
        }
    }
};

template <>
struct TextParserAction<ConnectValue>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.connParsingTargetPaths.empty() &&
            context.listOpType != SdfListOpTypeExplicit)
        {
            std::string errorMessage =
                "Setting connection paths to None (or an empty list) "
                "is only allowed when setting explicit connection paths, "
                "not for list editing";
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }

        for (const auto& path : context.connParsingTargetPaths) {
            const SdfAllowed allow = 
                SdfSchema::IsValidAttributeConnectionPath(path);
            if (!allow)
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    allow.GetWhyNot());

                throw PEGTL_NS::parse_error(allow.GetWhyNot(), in);
            }
        }

        if (context.listOpType == SdfListOpTypeAdded || 
            context.listOpType == SdfListOpTypeExplicit) 
        {
            for (const auto& pathIter : context.connParsingTargetPaths) {
                SdfPath path = context.path.AppendTarget(pathIter);
                if (!context.data->HasSpec(path))
                {
                    context.data->CreateSpec(
                        path,
                        SdfSpecTypeConnection);
                }
            }

            context.data->Set(context.path,
                SdfChildrenKeys->ConnectionChildren,
                VtValue(context.connParsingTargetPaths));
        }

        std::string errorMessage;
        if (!_SetListOpItemsWithError(SdfFieldKeys->ConnectionPaths, 
            context.listOpType, 
            context.connParsingTargetPaths, 
            context,
            errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }

        context.listOpType = SdfListOpTypeExplicit;
        context.custom = false;
        context.variability = VtValue();
        context.attributeTypeName.clear();

        // done parsing the connection attribute context
        _PopContext(context);
    }
};

////////////////////////////////////////////////////////////////////////
// Relationship actions

template <>
struct TextParserAction<KeywordRel>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            // we assume attribute spec by default unless there
            // is an indication it isn't an attribute spec
            // the keyword "rel" is a relation, so we remove
            // the attribute spec context and replace it
            _PopContext(context);
        }

        // default variability for relationships is uniform but we may have seen
        // KeywordVarying prior to this keyword, so we check whether the value 
        // is empty (a reset default) or whether it was explicitly set to 
        // varying - if so we don't change it
        if (context.variability.IsEmpty())
        {
            context.variability = VtValue(SdfVariabilityUniform);
        }

        _PushContext(context,
                     Sdf_TextParserCurrentParsingContext::RelationshipSpec);
    }
};

template <>
struct TextParserAction<RelationshipSpec>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // finished parsing an relationship spec
        _PopContext(context);

        context.custom = false;
        context.variability = VtValue();

        // reset the assumption that we are going to parse an
        // attribute next unless a keyword tells us otherwise
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::AttributeSpec);
    }
};

template <>
struct TextParserAction<KeywordDefault>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::RelationshipSpec)
        {
            _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::RelationshipDefault);
        }
    }
};

template <>
struct TextParserAction<Assignment>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // if we hit an assignment in a relationship context
        // this is a different context than if we didn't
        // so we push that here
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::RelationshipSpec)
        {
            _PushContext(context, 
                 Sdf_TextParserCurrentParsingContext::RelationshipAssignment);
        }
    }
};

template <>
struct TextParserAction<RelationshipAssignmentOptional>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // at the end of the relationship assignment, we assign the targets
        // and pop the relationship assignment context
        std::string errorMessage;
        if (!context.relParsingTargetPaths) {
            // No target paths were encountered.
            
            // pop the relationship assignment context if we had entered it
            // since this is an optional assignment, we may never have
            // seen the '='
            if (context.parsingContext.back() ==
                Sdf_TextParserCurrentParsingContext::RelationshipAssignment)
            {
                _PopContext(context);
            }

            if (context.listOpType != SdfListOpTypeExplicit)
            {
                // in this case, we will never reduce a RelationshipSpec
                // so we have to do here what we would have done in
                // the RelationshipSpec reduction - this is an artifact
                // that results from trying to be greedy about consuming
                // list op keywords rather than a big sor on RelationshipSpec
                // that would result in more backtracking

                // pop the relationship spec context
                _PopContext(context);

                context.custom = false;
                context.variability = VtValue();

                _PushContext(context, 
                         Sdf_TextParserCurrentParsingContext::AttributeSpec);
            }

            context.listOpType = SdfListOpTypeExplicit;

            return;
        }

        if (context.relParsingTargetPaths->empty() &&
            context.listOpType != SdfListOpTypeExplicit) {
                std::string errorMessage =
                    "Setting relationship targets to None (or empty list) "
                    "is only allowed when setting explicit targets, not for "
                    "list editing";
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                throw PEGTL_NS::parse_error(errorMessage, in);
        }

        for (const auto& path : *context.relParsingTargetPaths) {
            const SdfAllowed allow =
                SdfSchema::IsValidRelationshipTargetPath(path);
            if (!allow)
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    allow.GetWhyNot());

                throw PEGTL_NS::parse_error(allow.GetWhyNot(), in);
            }
        }

        if (context.listOpType == SdfListOpTypeAdded || 
            context.listOpType == SdfListOpTypeExplicit) {

            // Initialize relationship target specs for each target path that
            // is added in this layer.
            for (const auto& pathIter : *context.relParsingTargetPaths) {
                SdfPath targetPath = context.path.AppendTarget(pathIter);
                if (!context.data->HasSpec(targetPath)) {
                    // Create relationship target spec by setting the 
                    // appropriate object type flag.
                    context.data->CreateSpec(
                        targetPath,
                        SdfSpecTypeRelationshipTarget);

                    // Add the target path to the owning relationship's list of
                    // target children.
                    context.relParsingNewTargetChildren.push_back(pathIter);
                }
            }
        }

        if(!_SetListOpItemsWithError(SdfFieldKeys->TargetPaths, 
                        context.listOpType, *context.relParsingTargetPaths, 
                        context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }

        if (!context.relParsingNewTargetChildren.empty())
        {
            std::vector<SdfPath> children =
                context.data->GetAs<std::vector<SdfPath>>(
                    context.path, SdfChildrenKeys->RelationshipTargetChildren);
            
            children.insert(children.end(),
                context.relParsingNewTargetChildren.begin(),
                context.relParsingNewTargetChildren.end());

            context.data->Set(
                context.path,
                SdfChildrenKeys->RelationshipTargetChildren,
                VtValue(children));
        }

        // pop the relationship assignment context if we had entered it
        // since this is an optional assignment, we may never have
        // seen the '='
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::RelationshipAssignment)
        {
            _PopContext(context);
        }

        if (context.listOpType != SdfListOpTypeExplicit)
        {
            // in this case, we will never reduce a RelationshipSpec
            // so we have to do here what we would have done in
            // the RelationshipSpec reduction - this is an artifact
            // that results from trying to be greedy about consuming
            // list op keywords rather than a big sor on RelationshipSpec
            // that would result in more backtracking

            // pop the relationship spec context
            _PopContext(context);

            context.custom = false;
            context.variability = VtValue();

            _PushContext(context, 
                         Sdf_TextParserCurrentParsingContext::AttributeSpec);
        }

        context.listOpType = SdfListOpTypeExplicit;
    }
};

template <>
struct TextParserAction<RelationshipAssignmentOpen>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.relParsingTargetPaths = SdfPathVector();
    }
};

template <>
struct TextParserAction<RelationshipTargetOpen>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::RelationshipTarget);
    }
};

template <>
struct TextParserAction<RelationshipTargetClose>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        SdfPath path = context.path.AppendTarget(
            context.relParsingTargetPaths->back());

        if (!context.data->HasSpec(path)) {
            // Create relationship target spec by setting the appropriate 
            // object type flag.
            context.data->CreateSpec(path, SdfSpecTypeRelationshipTarget);

            // Add the target path to the owning relationship's list of target 
            // children.
            context.relParsingNewTargetChildren.push_back(
                    context.relParsingTargetPaths->back());
        }

        // pop the relationship target context
        _PopContext(context);
    }
};

////////////////////////////////////////////////////////////////////////
// Prim actions

template <>
struct TextParserAction<PropertySpec>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this will reset the parent path for attributes, relations,
        // connect values, etc. - the reason we do it here instead of
        // the individual ends is because there is some ambiguity
        // that would have to be resolved between non-list op attributes
        // with connect values vs list op ones
        context.path = context.path.GetParentPath();
    }
};

template <>
struct TextParserAction<PrimMetadataOptional>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // assume when parsing a prim spec that the default
        // expectation is an attribute unless there is a
        // keyword indication otherwise
        context.custom = false;
        context.variability = VtValue();

        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::AttributeSpec);
    }
};

template <>
struct TextParserAction<PrimSpec>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // store the names of our children
        if (!context.nameChildrenStack.back().empty())
        {
            context.data->Set(context.path,
                SdfChildrenKeys->PrimChildren,
                VtValue(context.nameChildrenStack.back()));
        }

        // store the names of our properties, if there are any
        if (!context.propertiesStack.back().empty())
        {
            context.data->Set(context.path,
                SdfChildrenKeys->PropertyChildren,
                VtValue(context.propertiesStack.back()));
        }

        // done parsing the prim spec, restore context state
        // to parent context
        context.nameChildrenStack.pop_back();
        context.propertiesStack.pop_back();
        context.path = context.path.GetParentPath();

        // this will pop the default attribute context
        // that we expect when parsing prim contents
        _PopContext(context);

        // now we need to pop the prim spec itself
        _PopContext(context);

        // if after popping if we aren't in the context
        // of a layer spec, we are somewhere parsing inside
        // of a parent prim / variant statement, so by default we should
        // be looking for an attribute spec unless told otherwise
        if (context.parsingContext.back() !=
            Sdf_TextParserCurrentParsingContext::LayerSpec)
        {
            context.custom = false;
            context.variability = VtValue();

            _PushContext(context, 
                         Sdf_TextParserCurrentParsingContext::AttributeSpec);
        }
    }
};

template <>
struct TextParserAction<KeywordDef>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.specifier = SdfSpecifierDef;

        // if we are inside a prim spec or variant statement, we are expecting
        // an attribute, but got a prim instead, so pop off the attribute
        // context before pushing the prim context
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            _PopContext(context);
        }

        _PushContext(context, Sdf_TextParserCurrentParsingContext::PrimSpec);
    }
};

template <>
struct TextParserAction<KeywordClass>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.specifier = SdfSpecifierClass;

        // if we are inside a prim spec or variant statement, we are expecting
        // an attribute, but got a prim instead, so pop off the attribute
        // context before pushing the prim context
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            _PopContext(context);
        }

        _PushContext(context, Sdf_TextParserCurrentParsingContext::PrimSpec);
    }
};

template <>
struct TextParserAction<KeywordOver>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.specifier = SdfSpecifierOver;

        // if we are inside a prim spec or variant statement, we are expecting
        // an attribute, but got a prim instead, so pop off the attribute
        // context before pushing the prim context
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            _PopContext(context);
        }

        _PushContext(context, Sdf_TextParserCurrentParsingContext::PrimSpec);
    }
};

template <>
struct TextParserAction<KeywordKind>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, Sdf_TextParserCurrentParsingContext::KindMetadata);
    }
};

template <>
struct TextParserAction<KindMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // done with the kind metadata context
        _PopContext(context);
    }
};

template <>
struct TextParserAction<KeywordRelocates>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.relocatesKey = SdfPath();
        context.seenFirstRelocatesPath = false;

        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::RelocatesMetadata);
    }
};

template <>
struct TextParserAction<RelocatesMapClose>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // are we in a prim or a layer context?
        // relocates metadata is top of the stack
        // below it would be metadata and then the
        // entity we want
        Sdf_TextParserCurrentParsingContext specContext =
                context.parsingContext.rbegin()[2];
        if (specContext ==
            Sdf_TextParserCurrentParsingContext::PrimSpec)
        {
            SdfRelocatesMap relocatesParsingMap(
                std::make_move_iterator(context.relocatesParsing.begin()),
                std::make_move_iterator(context.relocatesParsing.end()));
            
            context.data->Set(
                context.path,
                SdfFieldKeys->Relocates,
                VtValue(relocatesParsingMap));
        }
        else if (specContext ==
            Sdf_TextParserCurrentParsingContext::LayerSpec)
        {
            context.data->Set(
                context.path,
                SdfFieldKeys->LayerRelocates,
                VtValue(context.relocatesParsing));
        }
        else
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                "Unable to parse relocates data, unknown context!");
        }

        context.relocatesParsing.clear();

        // finished with relocates metadata
        _PopContext(context);
    }
};

template <>
struct TextParserAction<KeywordPayload>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // we may have seen a list op keyword, which
        // would have put us in the listop parsing
        // context, we replace that here with
        // a references list op context
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::ListOpMetadata)
        {
            _PopContext(context);
        }

        _PushContext(context, 
                 Sdf_TextParserCurrentParsingContext::PayloadListOpMetadata);

        context.layerRefPath = std::string();
        context.savedPath = SdfPath();
        context.layerRefOffset = SdfLayerOffset();
        context.payloadParsingRefs.clear();
    }
};

template <>
struct TextParserAction<PayloadList>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.payloadParsingRefs.empty() &&
            context.listOpType != SdfListOpTypeExplicit)
        {
            std::string errorMessage =
                "Setting payload to None (or an empty list)"
                "is only allowed when setting explicit payloads,"
                " not for list editing";
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }


        for (const auto& ref : context.payloadParsingRefs) {
            const SdfAllowed allow = SdfSchema::IsValidPayload(ref);
            if (!allow)
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    allow.GetWhyNot());

                throw PEGTL_NS::parse_error(allow.GetWhyNot(), in);
            }
        }

        std::string errorMessage;
        if (!_SetListOpItemsWithError(SdfFieldKeys->Payload,
            context.listOpType,
            context.payloadParsingRefs,
            context,
            errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }

        context.listOpType = SdfListOpTypeExplicit;

        // all done parsing the payload list
        _PopContext(context);
    }
};

template <>
struct TextParserAction<PayloadListItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        SdfPayload payload(context.layerRefPath,
                         context.savedPath,
                         context.layerRefOffset);
        context.payloadParsingRefs.push_back(payload);

        context.layerRefPath.clear();
        context.savedPath = SdfPath::EmptyPath();
        context.layerRefOffset = SdfLayerOffset();
    }
};

template <>
struct TextParserAction<KeywordReferences>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // we may have seen a list op keyword, which
        // would have put us in the listop parsing
        // context, we replace that here with
        // a references list op context
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::ListOpMetadata)
        {
            _PopContext(context);
        }

        _PushContext(context, 
                 Sdf_TextParserCurrentParsingContext::ReferencesListOpMetadata);

        context.layerRefPath = std::string();
        context.savedPath = SdfPath();
        context.layerRefOffset = SdfLayerOffset();
        context.referenceParsingRefs.clear();
    }
};

template <>
struct TextParserAction<ReferenceParametersOpen>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::ReferenceParameters);
    }
};

template <>
struct TextParserAction<ReferenceParametersClose>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // all done parsing the reference parameters
        _PopContext(context);
    }
};

template <>
struct TextParserAction<ReferenceListItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        SdfReference ref(context.layerRefPath,
                         context.savedPath,
                         context.layerRefOffset);
        ref.SwapCustomData(context.currentDictionaries[0]);
        context.referenceParsingRefs.push_back(ref);

        context.layerRefPath.clear();
        context.savedPath = SdfPath::EmptyPath();
        context.layerRefOffset = SdfLayerOffset();
    }
};

template <>
struct TextParserAction<KeywordOffset>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, Sdf_TextParserCurrentParsingContext::LayerOffset);
    }
};

template <>
struct TextParserAction<KeywordScale>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, Sdf_TextParserCurrentParsingContext::LayerScale);
    }
};

template <>
struct TextParserAction<ReferenceList>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.referenceParsingRefs.empty() &&
            context.listOpType != SdfListOpTypeExplicit) {
                std::string errorMessage =
                   "Setting references to None (or an empty list)"
                    "is only allowed when setting explicit references,"
                    " not for list editing";
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                throw PEGTL_NS::parse_error(errorMessage, in);
        }

        for (const auto& ref : context.referenceParsingRefs) {
            const SdfAllowed allow = SdfSchema::IsValidReference(ref);
            if (!allow)
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    allow.GetWhyNot());

                throw PEGTL_NS::parse_error(allow.GetWhyNot(), in);
            }
        }

        std::string errorMessage;
        if (!_SetListOpItemsWithError(SdfFieldKeys->References,
            context.listOpType,
            context.referenceParsingRefs,
            context,
            errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }

        context.listOpType = SdfListOpTypeExplicit;

        // all done parsing the references list
        _PopContext(context);
    }
};

template <>
struct TextParserAction<KeywordSpecializes>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // we may have seen a list op keyword, which would
        // have put us in the listop parsing context
        // we replace that with a specializes list op context
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::ListOpMetadata)
        {
            _PopContext(context);
        }

        context.specializesParsingTargetPaths.clear();
        
        _PushContext(context, 
             Sdf_TextParserCurrentParsingContext::SpecializesListOpMetadata);
    }
};

template <>
struct TextParserAction<KeywordInherits>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // we may have seen a list op keyword, which would
        // have put us in the listop parsing context
        // we replace that with a inherits list op context
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::ListOpMetadata)
        {
            _PopContext(context);
        }

        context.inheritParsingTargetPaths.clear();

        _PushContext(context, 
                 Sdf_TextParserCurrentParsingContext::InheritsListOpMetadata);
    }
};

template <>
struct TextParserAction<InheritsOrSpecializesList>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::InheritsListOpMetadata)
        {
            if (context.inheritParsingTargetPaths.empty() &&
                context.listOpType != SdfListOpTypeExplicit) {
                std::string errorMessage =
                    "Setting inherit paths to None (or empty list) is only "
                    "allowed when setting explicit inherit paths, not for list "
                    "editing";
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);
                
                throw PEGTL_NS::parse_error(errorMessage, in);
            }

            for (const auto& path : context.inheritParsingTargetPaths) {
                const SdfAllowed allow = SdfSchema::IsValidInheritPath(path);
                if (!allow)
                {
                    Sdf_TextFileFormatParser_Err(
                        context,
                        in.input(),
                        in.position(),
                        allow.GetWhyNot());

                    throw PEGTL_NS::parse_error(allow.GetWhyNot(), in);
                }
            }

            std::string errorMessage;
            if (!_SetListOpItemsWithError(
                SdfFieldKeys->InheritPaths,
                context.listOpType, 
                context.inheritParsingTargetPaths,
                context,
                errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                throw PEGTL_NS::parse_error(errorMessage, in);
            }
        }
        else if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::SpecializesListOpMetadata)
        {
            if (context.specializesParsingTargetPaths.empty() &&
                context.listOpType != SdfListOpTypeExplicit) {
                std::string errorMessage =
                    "Setting specializes paths to None (or empty list) is only "
                    "allowed when setting explicit specializes paths, not for "
                    "list editing";
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);
                
                throw PEGTL_NS::parse_error(errorMessage, in);
            }

            for (const auto& path : context.specializesParsingTargetPaths) {
                const SdfAllowed allow = 
                        SdfSchema::IsValidSpecializesPath(path);
                if (!allow)
                {
                    Sdf_TextFileFormatParser_Err(
                        context,
                        in.input(),
                        in.position(),
                        allow.GetWhyNot());

                    throw PEGTL_NS::parse_error(allow.GetWhyNot(), in);
                }
            }

            std::string errorMessage;
            if (!_SetListOpItemsWithError(
                SdfFieldKeys->Specializes,
                context.listOpType, 
                context.specializesParsingTargetPaths,
                context,
                errorMessage))
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                throw PEGTL_NS::parse_error(errorMessage, in);
            }
        }
        
        context.listOpType = SdfListOpTypeExplicit;

        // all done parsing the inherits / specializes list
        _PopContext(context);
    }
};

template <>
struct TextParserAction<KeywordVariants>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::VariantsMetadata);
    }
};

template <>
struct TextParserAction<VariantsMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        SdfVariantSelectionMap refVars;

        // The previous parser implementation allowed multiple variant selection
        // dictionaries in prim metadata to be merged, so we do the same here.
        VtValue oldVars;
        if (context.data->Has(context.path, SdfFieldKeys->VariantSelection,
            &oldVars))
        {
            refVars = oldVars.Get<SdfVariantSelectionMap>();
        }

        for (const auto& it : context.currentDictionaries[0]) {
            if (!it.second.IsHolding<std::string>()) {
                std::string errorMessage = "variant name must be a string";
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    errorMessage);

                throw PEGTL_NS::parse_error(errorMessage, in);
            } 
            const std::string variantName = it.second.Get<std::string>();
            const SdfAllowed allow =
                SdfSchema::IsValidVariantSelection(variantName);
            if (!allow)
            {
                Sdf_TextFileFormatParser_Err(
                    context,
                    in.input(),
                    in.position(),
                    allow.GetWhyNot());

                throw PEGTL_NS::parse_error(allow.GetWhyNot(), in);
            }

            refVars[it.first] = variantName;
        }

        context.data->Set(
            context.path,
            SdfFieldKeys->VariantSelection,
            VtValue(refVars));

        context.currentDictionaries[0].clear();

        // all done parsing the variants metadata
        _PopContext(context);
    }
};

template <>
struct TextParserAction<KeywordVariantSets>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::VariantSetsMetadata);
    }
};

template <>
struct TextParserAction<KeywordVariantSet>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // need to remove the attribute context that was pushed on before
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::AttributeSpec)
        {
            _PopContext(context);
        }

        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::VariantSetStatement);
    }
};

template <>
struct TextParserAction<VariantStatementOpen>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // parsing the contents of a variant statement is like
        // parsing the contents of a prim, so we have to assume
        // that the first thing we will see is an attribute spec
        // until keywords contextualize us otherwise
        context.custom = false;
        context.variability = VtValue();
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::AttributeSpec);
    }
};

template <>
struct TextParserAction<VariantStatementClose>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // this should be an attribute spec context
        _PopContext(context);
    }
};

template <>
struct TextParserAction<VariantStatementListOpen>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::VariantStatementList);
    }
};

template <>
struct TextParserAction<VariantStatementListClose>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // all done parsing variant statement list
        _PopContext(context);
    }
};

template <>
struct TextParserAction<VariantStatement>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        // Store the names of the prims and properties defined in this variant.
        if (!context.nameChildrenStack.back().empty()) {
            context.data->Set(
                context.path,
                SdfChildrenKeys->PrimChildren,
                VtValue(context.nameChildrenStack.back()));
        }
        if (!context.propertiesStack.back().empty()) {
            context.data->Set(
                context.path,
                SdfChildrenKeys->PropertyChildren,
                VtValue(context.propertiesStack.back()));
        }

        context.nameChildrenStack.pop_back();
        context.propertiesStack.pop_back();

        std::string variantSet = context.path.GetVariantSelection().first;
        context.path = 
            context.path.GetParentPath().AppendVariantSelection(variantSet, "");
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

        // Create this VariantSetSpec if it does not already exist.
        if (!context.data->HasSpec(variantSetPath))
        {
            context.data->CreateSpec(variantSetPath,
                SdfSpecTypeVariantSet);

            // add the name of this variant set to the VariantSets field
            std::vector<TfToken> vec =
                context.data->GetAs<std::vector<TfToken>>(context.path,
                    SdfChildrenKeys->VariantSetChildren);

            vec.push_back(TfToken(context.currentVariantSetNames.back()));
            context.data->Set(context.path,
                SdfChildrenKeys->VariantSetChildren,
                VtValue(vec));
        }

        // Author the variant set's variants
        context.data->Set(variantSetPath,
            SdfChildrenKeys->VariantChildren,
            VtValue(TfToTokenVector(context.currentVariantNames.back())));

        context.currentVariantSetNames.pop_back();
        context.currentVariantNames.pop_back();

        // all done parsing variant set statement
        _PopContext(context);

        // at the end of this context, we jump back into the prim context
        // which means by default we need to expect an attribute
        context.custom = false;
        context.variability = VtValue();
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::AttributeSpec);
    }
};

template <>
struct TextParserAction<KeywordNameChildren>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::ReorderNameChildren);
    }
};

template <>
struct TextParserAction<KeywordProperties>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::ReorderProperties);
    }
};

template <>
struct TextParserAction<ChildOrPropertyOrderStatement>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::ReorderNameChildren)
        {
            context.data->Set(
                context.path,
                SdfFieldKeys->PrimOrder,
                VtValue(context.nameVector));

            _PopContext(context);
        }
        else if(context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::ReorderProperties)
        {
            context.data->Set(
                context.path,
                SdfFieldKeys->PropertyOrder,
                VtValue(context.nameVector));

            _PopContext(context);
        }
        
        // the list op type got set by the reorder keyword so reset it here
        context.listOpType = SdfListOpTypeExplicit;
        context.nameVector.clear();
    }
};

template <>
struct TextParserAction<KeywordPrefixSubstitutions>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, 
             Sdf_TextParserCurrentParsingContext::PrefixSubstitutionsMetadata);
    }
};

template <>
struct TextParserAction<KeywordSuffixSubstitutions>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, 
             Sdf_TextParserCurrentParsingContext::SuffixSubstitutionsMetadata);
    }
};

template <>
struct TextParserAction<PrefixOrSuffixSubstitutionsMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (context.parsingContext.back() ==
            Sdf_TextParserCurrentParsingContext::PrefixSubstitutionsMetadata)
        {
            context.data->Set(
                context.path,
                SdfFieldKeys->PrefixSubstitutions,
                VtValue(context.currentDictionaries[0]));
        }
        else
        {
            // suffix substitutions
            context.data->Set(
                context.path,
                SdfFieldKeys->SuffixSubstitutions,
                VtValue(context.currentDictionaries[0]));
        }

        context.currentDictionaries[0].clear();

        // done with this context
        _PopContext(context);
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

        context.nameChildrenStack.emplace_back();
        context.data->CreateSpec(
            SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot);

        _PushContext(context, Sdf_TextParserCurrentParsingContext::LayerSpec);
    }
};

template <>
struct TextParserAction<LayerSpec>
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
struct TextParserAction<KeywordRootPrims>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::ReorderRootPrims);
    }
};

template <>
struct TextParserAction<LayerKeyValueMetadata>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        Sdf_TextParserCurrentParsingContext specContext =
            context.parsingContext.rbegin()[1];
        SdfSpecType specType = _GetSpecTypeFromContext(specContext);

        std::string errorMessage;
        if (!_KeyValueMetadataEnd(specType, context, errorMessage))
        {
            Sdf_TextFileFormatParser_Err(
                context,
                in.input(),
                in.position(),
                errorMessage);

            throw PEGTL_NS::parse_error(errorMessage, in);
        }
        
        // no need to pop the parsing context as it was already popped in
        // the individual reductions for None, TypedValue, and DictionaryValue
        context.listOpType = SdfListOpTypeExplicit;
    }
};

template <>
struct TextParserAction<KeywordSubLayers>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.subLayerPaths.clear();
        context.subLayerOffsets.clear();

        _PushContext(context, 
                     Sdf_TextParserCurrentParsingContext::SubLayerMetadata);
    }
};

template <>
struct TextParserAction<SublayerItem>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        context.subLayerPaths.push_back(context.layerRefPath);
        context.subLayerOffsets.push_back(context.layerRefOffset);
    }
};

template <>
struct TextParserAction<SublayerListClose>
{
    template <class Input>
    static void apply(const Input& in, Sdf_TextParserContext& context)
    {
        if (!context.subLayerPaths.empty())
        {
            context.data->Set(SdfPath::AbsoluteRootPath(),
                SdfFieldKeys->SubLayers,
                VtValue(context.subLayerPaths));
            
            context.data->Set(SdfPath::AbsoluteRootPath(),
                SdfFieldKeys->SubLayerOffsets,
                VtValue(context.subLayerOffsets));

            context.subLayerPaths.clear();
            context.subLayerOffsets.clear();
        }

        _PopContext(context);
    }
};

} // end namespace Sdf_TextFileFormatParser

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

    // read the entire file into memory
    size_t size = asset->GetSize();
    std::string buffer(size, ' ');
    if (asset->Read(&buffer[0], size, 0) != size)
    {
        TF_RUNTIME_ERROR("Failed to read asset contents @%s@: "
            "an error occurred while reading",
            fileContext.c_str());
    }

    Sdf_TextFileFormatParser::PEGTL_NS::string_input<> content { 
        std::move(buffer), fileContext};
    context.values.errorReporter =
        [&context, capture0 = std::cref(content)](auto && PH1) { 
            return Sdf_TextFileFormatParser::_ReportParseError<
            Sdf_TextFileFormatParser::PEGTL_NS::string_input<>>(
                context, capture0, std::forward<decltype(PH1)>(PH1)); };
    bool status = false;
    try
    {
        if (!metadataOnly)
        {
            if (TfDebug::IsEnabled(SDF_TEXT_FILE_FORMAT_PEGTL_TRACE)) {
                status = Sdf_TextFileFormatParser::PEGTL_NS::standard_trace<
                    Sdf_TextFileFormatParser::PEGTL_NS::must<
                            Sdf_TextFileFormatParser::LayerSpec, 
                        Sdf_TextFileFormatParser::PEGTL_NS::internal::eof>,
                    Sdf_TextFileFormatParser::TextParserAction>(content, 
                                                                context);
                *hints = context.layerHints;
            } else {
                status = Sdf_TextFileFormatParser::PEGTL_NS::parse<
                    Sdf_TextFileFormatParser::PEGTL_NS::must<
                            Sdf_TextFileFormatParser::LayerSpec, 
                        Sdf_TextFileFormatParser::PEGTL_NS::internal::eof>,
                    Sdf_TextFileFormatParser::TextParserAction,
                    Sdf_TextFileFormatParser::TextParserControl>(content, 
                                                                 context);
                *hints = context.layerHints;
            }
        }
        else
        {
            // note the absence of the eof here - there will be more
            // content in the layer and we don't know what that content is,
            // so we stop at reduction of LayerMetadataOnly
            status = Sdf_TextFileFormatParser::PEGTL_NS::parse<
                Sdf_TextFileFormatParser::PEGTL_NS::must<
                    Sdf_TextFileFormatParser::LayerMetadataOnly>,
                Sdf_TextFileFormatParser::TextParserAction,
                Sdf_TextFileFormatParser::TextParserControl>(content, context);
            *hints = context.layerHints;
        }
    }
    catch (std::bad_variant_access const &)
    {
        TF_CODING_ERROR("Bad variant access in layer parser.");
        Sdf_TextFileFormatParser::Sdf_TextFileFormatParser_Err(
            context,
            content,
            content.position(),
            "Internal layer parser error.");

        throw Sdf_TextFileFormatParser::PEGTL_NS::parse_error(
            "Internal layer parser error", content);
    }
    catch (const Sdf_TextFileFormatParser::PEGTL_NS::parse_error& e)
    {
        Sdf_TextFileFormatParser::Sdf_TextFileFormatParser_Err(
            context,
            content,
            e.positions().empty() ? content.position() : e.positions()[0],
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

    Sdf_TextFileFormatParser::PEGTL_NS::memory_input<> content {
        layerString.data(), layerString.size(), ""};

    context.values.errorReporter = 
        [&context, capture0 = std::cref(content)](auto && PH1) { 
            return Sdf_TextFileFormatParser::_ReportParseError<
                Sdf_TextFileFormatParser::PEGTL_NS::memory_input<>>(
                    context, capture0, std::forward<decltype(PH1)>(PH1)); };
    bool status = false;
    try
    {
        status = Sdf_TextFileFormatParser::PEGTL_NS::parse<
            Sdf_TextFileFormatParser::PEGTL_NS::must<
                    Sdf_TextFileFormatParser::LayerSpec,
                Sdf_TextFileFormatParser::PEGTL_NS::internal::eof>,
            Sdf_TextFileFormatParser::TextParserAction,
            Sdf_TextFileFormatParser::TextParserControl>(content, context);
    }
    catch (std::bad_variant_access const &)
    {
        TF_CODING_ERROR("Bad variant access in layer parser.");
        Sdf_TextFileFormatParser::Sdf_TextFileFormatParser_Err(
            context,
            content,
            content.position(),
            "Internal layer parser error.");

        throw Sdf_TextFileFormatParser::PEGTL_NS::parse_error(
            "Internal layer parser error", content);
    }
    catch (const Sdf_TextFileFormatParser::PEGTL_NS::parse_error& e)
    {
        Sdf_TextFileFormatParser::Sdf_TextFileFormatParser_Err(
            context,
            content,
            e.positions().empty() ? content.position() : e.positions()[0],
            e.what());
    }

    return status;
}

PXR_NAMESPACE_CLOSE_SCOPE
