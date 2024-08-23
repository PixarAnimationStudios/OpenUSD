//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/debug.h"
#include "pxr/usd/sdf/debugCodes.h"
#include "pxr/usd/sdf/textParserHelpers.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_TextFileFormatParser {

bool
_ValueSetAtomic(Sdf_TextParserContext& context,
    std::string& errorMessage)
{
    if (!context.values.IsRecordingString()) {
        if (context.values.valueIsShaped) {
            errorMessage = "Type name has [] for non-shaped value!\n";
            return false;
        }
    }

    std::string errStr;
    context.currentValue = context.values.ProduceValue(&errStr);
    if (context.currentValue.IsEmpty()) {
        errorMessage = "Error parsing simple value: " + errStr;
        return false;
    }

    return true;
}

bool
_ValueSetTuple(Sdf_TextParserContext& context,
    std::string& errorMessage)
{
    if (!context.values.IsRecordingString()) {
        if (context.values.valueIsShaped) {
            errorMessage = "Type name has [] for non-shaped value.\n";
            return false;
        }
    }

    std::string errStr;
    context.currentValue = context.values.ProduceValue(&errStr);
    if (context.currentValue == VtValue()) {
        errorMessage = "Error parsing tuple value: " + errStr;
        return false;
    }

    return true;
}

bool
_ValueSetList(Sdf_TextParserContext& context,
    std::string& errorMessage)
{
    if (!context.values.IsRecordingString()) {
        if (!context.values.valueIsShaped) {
            errorMessage = "Type name missing [] for shaped value.";
            return false;
        }
    }

    std::string errStr;
    context.currentValue = context.values.ProduceValue(&errStr);
    if (context.currentValue == VtValue()) {
        errorMessage = "Error parsing shaped value: " + errStr;
        return false;
    }

    return true;
}

bool
_ValueSetShaped(Sdf_TextParserContext& context,
    std::string& errorMessage)
{
    if (!context.values.IsRecordingString()) {
        if (!context.values.valueIsShaped) {
            errorMessage = "Type name missing [] for shaped value.";
            return false;
        }
    }

    std::string errStr;
    context.currentValue = context.values.ProduceValue(&errStr);
    if (context.currentValue == VtValue()) {
        // The factory method ProduceValue() uses for shaped types
        // only returns empty VtArrays, not empty VtValues, so this
        // is impossible to hit currently.
        // CODE_COVERAGE_OFF
        errorMessage = "Error parsing shaped value: " + errStr;
        // CODE_COVERAGE_OFF_GCOV_BUG
        // The following line actually shows as executed (a ridiculous 
        // number of times) even though the line above shwos as 
        // not executed
        return false;
        // CODE_COVERAGE_ON_GCOV_BUG
        // CODE_COVERAGE_ON
    }

    return true;
}

void
_SetDefault(const SdfPath& path, VtValue val,
            Sdf_TextParserContext& context)
{
    // If is holding SdfPathExpression (or array of same), make absolute with
    // path.GetPrimPath() as anchor.
    if (val.IsHolding<SdfPathExpression>()) {
        val.UncheckedMutate<SdfPathExpression>([&](SdfPathExpression &pe) {
            pe = pe.MakeAbsolute(path.GetPrimPath());
        });
    }
    else if (val.IsHolding<VtArray<SdfPathExpression>>()) {
        val.UncheckedMutate<VtArray<SdfPathExpression>>(
            [&](VtArray<SdfPathExpression> &peArr) {
                for (SdfPathExpression &pe: peArr) {
                    pe = pe.MakeAbsolute(path.GetPrimPath());
                }
            });
    }
    context.data->Set(path, SdfFieldKeys->Default, val);
}

void
_KeyValueMetadataStart(const std::string& key, SdfSpecType specType,
    Sdf_TextParserContext& context)
{
    TF_DEBUG(SDF_TEXT_FILE_FORMAT_CONTEXT).Msg(
        "Starting metadata for key: " + key +
        "(List op: " + _ListOpTypeToString(context.listOpType) + ")\n");

    context.genericMetadataKey = TfToken(key);

    const SdfSchema& schema = SdfSchema::GetInstance();
    const SdfSchema::SpecDefinition &specDef = 
        *schema.GetSpecDefinition(specType);
    if (specDef.IsMetadataField(context.genericMetadataKey)) {
        // Prepare to parse a known field
        const SdfSchema::FieldDefinition &fieldDef = 
            *schema.GetFieldDefinition(context.genericMetadataKey);
        const TfType fieldType = fieldDef.GetFallbackValue().GetType();

        // For list op-valued metadata fields, set up the parser as if
        // we were parsing an array of the list op's underlying type.
        // In _KeyValueMetadataEnd, we'll produce this list and set it
        // into the appropriate place in the list op.
        TfType itemArrayType;
        if (_IsGenericMetadataListOpType(fieldType, &itemArrayType)) {
            context.values.SetupFactory(schema.FindType(itemArrayType).
                GetAsToken().GetString());
        }
        else {
            context.values.SetupFactory(schema.FindType(
                fieldDef.GetFallbackValue()).GetAsToken().GetString());
        }
    } else {
        // Prepare to parse only the string representation of this metadata
        // value, since it's an unregistered field.
        context.values.StartRecordingString();
    }
}

bool
_KeyValueMetadataEnd(SdfSpecType specType, Sdf_TextParserContext& context,
    std::string& errorMessage)
{
    const SdfSchema& schema = SdfSchema::GetInstance();
    const SdfSchema::SpecDefinition &specDef = 
        *schema.GetSpecDefinition(specType);
    if (specDef.IsMetadataField(context.genericMetadataKey)) {
        // Validate known fields before storing them
        const SdfSchema::FieldDefinition &fieldDef = 
            *schema.GetFieldDefinition(context.genericMetadataKey);
        const TfType fieldType = fieldDef.GetFallbackValue().GetType();

        if (_IsGenericMetadataListOpType(fieldType)) {
            if (!fieldDef.IsValidListValue(context.currentValue)) {
                errorMessage = "invalid value for field \"" +
                    context.genericMetadataKey.GetString() + "\"";
                return false;
            }
            _SetGenericMetadataListOpItems(fieldType, context);
        }
        else {
            if (!fieldDef.IsValidValue(context.currentValue) ||
                context.currentValue.IsEmpty()) {
                errorMessage = "invalid value for field \"" +
                    context.genericMetadataKey.GetString() + "\"";
                return false;
            }
            context.data->Set(context.path,
                context.genericMetadataKey,
                VtValue(context.currentValue));
        }
    } else if (specDef.IsValidField(context.genericMetadataKey)) {
        // Prevent the user from overwriting fields that aren't metadata
        errorMessage = "\"" + context.genericMetadataKey.GetString() +
            "\" is registered as a non-metadata field";
        return false;
    } else {
        // Stuff unknown fields into a SdfUnregisteredValue so they can pass
        // through loading and saving unmodified
        VtValue value;
        if (context.currentValue.IsHolding<VtDictionary>()) {
            // If we parsed a dictionary, store it's actual value. Dictionaries
            // can be parsed fully because they contain type information.
            value = 
                SdfUnregisteredValue(context.currentValue.Get<VtDictionary>());
        } else {
            // Otherwise, we parsed a simple value or a shaped list of simple
            // values. We want to store the parsed string, but we need to
            // determine whether to unpack it into an SdfUnregisteredListOp
            // or to just store the string directly.
            auto getOldValue = [context]() {
                VtValue v;
                if (context.data->Has(context.path,
                    context.genericMetadataKey, &v)
                    && TF_VERIFY(v.IsHolding<SdfUnregisteredValue>())) {
                    v = v.UncheckedGet<SdfUnregisteredValue>().GetValue();
                }
                else {
                    v = VtValue();
                }
                return v;
            };

            auto getRecordedStringAsUnregisteredValue = [context]() {
                std::string s = context.values.GetRecordedString();
                if (s == "None") { 
                    return std::vector<SdfUnregisteredValue>(); 
                }

                // Put the entire string representation of this list into
                // a single SdfUnregisteredValue, but strip off the enclosing
                // brackets so that we don't write out two sets of brackets
                // when serializing out the list op.
                if (!s.empty() && s.front() == '[') { s.erase(0, 1); }
                if (!s.empty() && s.back() == ']') { s.pop_back(); }
                return std::vector<SdfUnregisteredValue>(
                    { SdfUnregisteredValue(s) });
            };

            VtValue oldValue = getOldValue();
            if (context.listOpType == SdfListOpTypeExplicit) {
                // In this case, we can't determine whether the we've parsed
                // an explicit list op statement or a simple value.
                // We just store the recorded string directly, as that's the
                // simplest thing to do.
                value = 
                    SdfUnregisteredValue(context.values.GetRecordedString());
            }
            else if (oldValue.IsEmpty()
                     || oldValue.IsHolding<SdfUnregisteredValueListOp>()) {
                // In this case, we've parsed a list op statement so unpack
                // it into a list op unless we've already parsed something
                // for this field that *isn't* a list op.
                SdfUnregisteredValueListOp listOp = 
                    oldValue.GetWithDefault<SdfUnregisteredValueListOp>();
                listOp.SetItems(getRecordedStringAsUnregisteredValue(), 
                                context.listOpType);
                value = SdfUnregisteredValue(listOp);
            }
            else {
                // If we've parsed a list op statement but have a non-list op
                // stored in this field, leave that value in place and ignore
                // the new value. We should only encounter this case if someone
                // hand-edited the layer in an unexpected or invalid way, so
                // just keeping the first value we find should be OK.
            }
        }

        if (!value.IsEmpty()) {
            context.data->Set(context.path, context.genericMetadataKey,
                VtValue(value));
        }
    }

    context.values.Clear();
    context.currentValue = VtValue();

    return true;
}

bool
_CreatePrimSpec(const std::string& primIdentifierString,
    Sdf_TextParserContext& context, std::string& errorMessage)
{
    TfToken primIdentifier(primIdentifierString);
    if (!SdfPath::IsValidIdentifier(primIdentifier))
    {
        errorMessage = "'" + primIdentifierString + "' is not a valid prim name";
        return false;
    }
    context.path = context.path.AppendChild(primIdentifier);

    // create the spec
    if(context.data->HasSpec(context.path))
    {
        errorMessage = "Duplicate prim '" + primIdentifierString + "'";
        return false;
    }
    TF_DEBUG(SDF_TEXT_FILE_FORMAT_CONTEXT).Msg(
        "Creating prim spec: " + primIdentifierString + "\n");
    context.data->CreateSpec(context.path, SdfSpecTypePrim);
    context.nameChildrenStack.back().push_back(primIdentifier);

    // set the context for this prim's name children and properties
    context.nameChildrenStack.emplace_back();
    context.propertiesStack.emplace_back();
    
    // set the specifier, typeName, and clear the isParsingPrimName context
    context.data->Set(context.path,
        SdfFieldKeys->Specifier, VtValue(context.specifier));
    if (!context.primTypeName.empty())
    {
        TfToken typeName = TfToken(context.primTypeName);
        context.data->Set(context.path,
            SdfFieldKeys->TypeName, VtValue(typeName));
    }
    
    context.primTypeName.clear();

    return true;
}

bool
_CreateRelationshipSpec(const std::string& relationshipName,
    Sdf_TextParserContext& context,
    std::string& errorMessage)
{
    TfToken name(relationshipName);
    if (!SdfPath::IsValidNamespacedIdentifier(name)) {
        errorMessage = "'" + name.GetString() + 
            " is not a valid relationship name";

        return false;
    }

    TF_DEBUG(SDF_TEXT_FILE_FORMAT_CONTEXT).Msg(
        "Creating relationship spec for " + relationshipName +
        ", current path is: " + context.path.GetAsString() + "\n");
    
    context.path = context.path.AppendProperty(name);

    if (!context.data->HasSpec(context.path)) {
        context.propertiesStack.back().push_back(name);
        context.data->CreateSpec(context.path, SdfSpecTypeRelationship);
    }

    context.data->Set(
        context.path, SdfFieldKeys->Variability, 
        VtValue(context.variability));

    if (context.custom) {
        context.data->Set(
            context.path,
            SdfFieldKeys->Custom,
            VtValue(true));
    }

    context.relParsingTargetPaths.reset();
    context.relParsingNewTargetChildren.clear();

    return true;
}

bool
_CreateAttributeSpec(const std::string& attributeName, 
    Sdf_TextParserContext& context,
    std::string& errorMessage)
{
    TfToken name(attributeName);
    if (!SdfPath::IsValidNamespacedIdentifier(name))
    {
        errorMessage = "'" + name.GetString() + 
            "' is not a valid attribute name";

        return false;
    }

    TF_DEBUG(SDF_TEXT_FILE_FORMAT_CONTEXT).Msg(
        "Creating attribute spec for " + attributeName +
        ", current path is: " + context.path.GetAsString() + "\n");

    context.path = context.path.AppendProperty(name);

    // If we haven't seen this attribute before, then set the object type
    // and add it to the parent's list of properties. Otherwise both have
    // already been done, so we don't need to do anything.
    if (!context.data->HasSpec(context.path))
    {
        context.propertiesStack.back().push_back(name);
        context.data->CreateSpec(context.path, SdfSpecTypeAttribute);
        context.data->Set(context.path,
            SdfFieldKeys->Custom,
            VtValue(false));
    }
    
    // this may be a redefinition, and custom may have changed on the spec
    // so we set that here
    if(context.custom)
    {
        context.data->Set(context.path,
            SdfFieldKeys->Custom,
            VtValue(true));
    }

    // If the type was previously set, check that it matches. Otherwise set it.
    const TfToken newType(context.values.valueTypeName);
    VtValue oldTypeValue;
    if (context.data->Has(context.path,
        SdfFieldKeys->TypeName,
        &oldTypeValue))
    {
        const TfToken& oldType = oldTypeValue.Get<TfToken>();
        if (newType != oldType)
        {
            errorMessage = "attribute '" + context.path.GetName() + 
                "' already has type '" + oldType.GetString() + 
                "', cannot change to '" + newType.GetString() + "'";

            return false;
        }
    }
    else
    {
        context.data->Set(context.path,
            SdfFieldKeys->TypeName,
            VtValue(newType));
    }

    // If the variability was previously set, check that it matches. Otherwise
    // set it.  If the 'variability' VtValue is empty, that indicates varying
    // variability.
    SdfVariability variability = context.variability.IsEmpty() ? 
        SdfVariabilityVarying : context.variability.Get<SdfVariability>();
    VtValue oldVariability;
    if (context.data->Has(context.path,
        SdfFieldKeys->Variability,
        &oldVariability))
    {
        if (variability != oldVariability.Get<SdfVariability>())
        {
            errorMessage = "attribute '" + context.path.GetName() +
                "' already has variability '" +
                TfEnum::GetName(oldVariability.Get<SdfVariability>()) +
                "', cannot change to '" +
                TfEnum::GetName(variability) + "'";

            return false;
        }
    }
    else
    {
        context.data->Set(context.path,
            SdfFieldKeys->Variability,
            VtValue(variability));
    }

    return true;
}

std::pair<bool, Sdf_ParserHelpers::Value>
_GetNumericValueFromString(const std::string_view in)
{
    bool didOverflow = false;
    Sdf_ParserHelpers::Value value;
    constexpr std::string_view negativeZero = "-0";
    constexpr std::string_view negativeInfinity = "-inf";
    constexpr std::string_view positiveInfinity = "inf";
    constexpr std::string_view nan = "nan";

    auto mustBeDouble = [](const std::string_view str) {
        return str.find('.') != str.npos ||
               str.find('e') != str.npos ||
               str.find('E') != str.npos;
    };

    if (in == negativeZero)
    {
        value = double(-0.0);
    }
    else if(in == negativeInfinity)
    {
        value = -std::numeric_limits<double>::infinity();
    }
    
    else if (in == positiveInfinity)
    {
        value = std::numeric_limits<double>::infinity();
    }
    else if (in == nan)
    {
        value = std::numeric_limits<double>::quiet_NaN();
    }
    else if (mustBeDouble(in))
    {
        value = TfStringToDouble(in.data(), in.length());
    }
    else
    {
        // positive and negative integers are stored as long
        // unless out of range
        bool outOfRange = false;
        if (!in.empty() && in.front() == '-')
        {
            value = TfStringToInt64(std::string(in), &outOfRange);
        }
        else
        {
            value = TfStringToUInt64(std::string(in), &outOfRange);
        }

        if (outOfRange)
        {
            didOverflow = true;
            value = TfStringToDouble(in.data(), in.length());
        }
    }

    return {didOverflow, value};
}

void
_SetGenericMetadataListOpItems(const TfType& fieldType, 
                               Sdf_TextParserContext& context)
{
    // Chain together attempts to set list op items using 'or' to bail
    // out as soon as we successfully write out the list op we're holding.
    // we don't return the error message here because some of these can be
    // unsuccessful
    std::string errorMessage;
    _SetItemsIfListOpWithError<SdfIntListOp>(fieldType, context, 
                                             errorMessage) ||
    _SetItemsIfListOpWithError<SdfInt64ListOp>(fieldType, context, 
                                               errorMessage) ||
    _SetItemsIfListOpWithError<SdfUIntListOp>(fieldType, context, 
                                              errorMessage) ||
    _SetItemsIfListOpWithError<SdfUInt64ListOp>(fieldType, context, 
                                                errorMessage) ||
    _SetItemsIfListOpWithError<SdfStringListOp>(fieldType, context, 
                                                errorMessage) ||
    _SetItemsIfListOpWithError<SdfTokenListOp>(fieldType, context, 
                                               errorMessage);
}

bool
_IsGenericMetadataListOpType(const TfType& type,
                             TfType* itemArrayType)
{
    static std::pair<TfType, TfType> listOpAndArrayTypes[] = {
        _GetListOpAndArrayTfTypes<SdfIntListOp>(),
        _GetListOpAndArrayTfTypes<SdfInt64ListOp>(),
        _GetListOpAndArrayTfTypes<SdfUIntListOp>(),
        _GetListOpAndArrayTfTypes<SdfUInt64ListOp>(),
        _GetListOpAndArrayTfTypes<SdfStringListOp>(),
        _GetListOpAndArrayTfTypes<SdfTokenListOp>(),
    };

    auto iter = std::find_if(std::begin(listOpAndArrayTypes),
                             std::end(listOpAndArrayTypes),
                             [&type](auto const &p) {
                                 return p.first == type;
                             });

    if (iter == std::end(listOpAndArrayTypes)) {
        return false;
    }

    if (itemArrayType) {
        *itemArrayType = iter->second;
    }
    
    return true;
}

std::string
_ContextToString(Sdf_TextParserCurrentParsingContext parsingContext)
{
    switch(parsingContext)
    {
        case Sdf_TextParserCurrentParsingContext::LayerSpec:
        {
            return "LayerSpec";
        }
        case Sdf_TextParserCurrentParsingContext::PrimSpec:
        {
            return "PrimSpec";
        }
        case Sdf_TextParserCurrentParsingContext::AttributeSpec:
        {
            return "AttributeSpec";
        }
        case Sdf_TextParserCurrentParsingContext::RelationshipSpec:
        {
            return "RelationshipSpec";
        }
        case Sdf_TextParserCurrentParsingContext::Metadata:
        {
            return "Metadata";
        }
        case Sdf_TextParserCurrentParsingContext::KeyValueMetadata:
        {
            return "KeyValueMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::ListOpMetadata:
        {
            return "ListOpMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::DocMetadata:
        {
            return "DocMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::PermissionMetadata:
        {
            return "PermissionMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::SymmetryFunctionMetadata:
        {
            return "SymmetryFunctionMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::DisplayUnitMetadata:
        {
            return "DisplayUnitMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::Dictionary:
        {
            return "Dictionary";
        }
        case Sdf_TextParserCurrentParsingContext::DictionaryTypeName:
        {
            return "DictionaryTypeName";
        }
        case Sdf_TextParserCurrentParsingContext::DictionaryKey:
        {
            return "DictionaryKey";
        }
        case Sdf_TextParserCurrentParsingContext::ConnectAttribute:
        {
            return "ConnectAttribute";
        }
        case Sdf_TextParserCurrentParsingContext::TimeSamples:
        {
            return "TimeSamples";
        }
        case Sdf_TextParserCurrentParsingContext::SplineValues:
        {
            return "SplineValues";
        }
        case Sdf_TextParserCurrentParsingContext::SplineKnotItem:
        {
            return "SplineKnotItem";
        }
        case Sdf_TextParserCurrentParsingContext::SplinePostExtrapItem:
        {
            return "SplinePostExtrapItem";
        }
        case Sdf_TextParserCurrentParsingContext::SplinePreExtrapItem:
        {
            return "SplinePreExtrapItem";
        }
        case Sdf_TextParserCurrentParsingContext::SplineExtrapSloped:
        {
            return "SplineExtrapSloped";
        }
        case Sdf_TextParserCurrentParsingContext::SplineKeywordLoop:
        {
            return "SplineKeywordLoop";
        }
        case Sdf_TextParserCurrentParsingContext::SplineKnotParam:
        {
            return "SplineKnotParam";
        }
        case Sdf_TextParserCurrentParsingContext::SplineTangent:
        {
            return "SplineTangent";
        }
        case Sdf_TextParserCurrentParsingContext::ReorderRootPrims:
        {
            return "ReorderRootPrims";
        }
        case Sdf_TextParserCurrentParsingContext::ReorderNameChildren:
        {
            return "ReorderNameChildren";
        }
        case Sdf_TextParserCurrentParsingContext::ReorderProperties:
        {
            return "ReorderProperties";
        }
        case Sdf_TextParserCurrentParsingContext::InheritsListOpMetadata:
        {
            return "InheritsListOpMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::SpecializesListOpMetadata:
        {
            return "SpecializesListOpMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::ReferencesListOpMetadata:
        {
            return "ReferencesListOpMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::PayloadListOpMetadata:
        {
            return "PayloadListOpMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::VariantSetsMetadata:
        {
            return "VariantSetsMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::ReferenceParameters:
        {
            return "ReferenceParameters";
        }
        case Sdf_TextParserCurrentParsingContext::RelocatesMetadata:
        {
            return "RelocatesMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::KindMetadata:
        {
            return "KindMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::RelationshipAssignment:
        {
            return "RelationshipAssignment";
        }
        case Sdf_TextParserCurrentParsingContext::RelationshipTarget:
        {
            return "RelationshipTarget";
        }
        case Sdf_TextParserCurrentParsingContext::RelationshipDefault:
        {
            return "RelationshipDefault";
        }
        case Sdf_TextParserCurrentParsingContext::LayerOffset:
        {
            return "LayerOffset";
        }
        case Sdf_TextParserCurrentParsingContext::LayerScale:
        {
            return "LayerScale";
        }
        case Sdf_TextParserCurrentParsingContext::VariantsMetadata:
        {
            return "VariantsMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::VariantSetStatement:
        {
            return "VariantSetStatement";
        }
        case Sdf_TextParserCurrentParsingContext::VariantStatementList:
        {
            return "VariantStatementList";
        }
        case Sdf_TextParserCurrentParsingContext::PrefixSubstitutionsMetadata:
        {
            return "PrefixSubstitutionsMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::SuffixSubstitutionsMetadata:
        {
            return "SuffixSubstitutionsMetadata";
        }
        case Sdf_TextParserCurrentParsingContext::SubLayerMetadata:
        {
            return "SubLayerMetadata";
        }
        default:
        {
            return "";
        }
    }
}

std::string
_ListOpTypeToString(SdfListOpType listOpType)
{
    if (listOpType == SdfListOpTypeExplicit)
    {  
        return "explicit";
    }
    if (listOpType == SdfListOpTypeAdded)
    {
        return "add";
    }
    if (listOpType == SdfListOpTypeDeleted)
    {
        return "delete";
    }
    if (listOpType == SdfListOpTypeAppended)
    {
        return "append";
    }
    if (listOpType == SdfListOpTypePrepended)
    {
        return "prepend";
    }
    if (listOpType == SdfListOpTypeOrdered)
    {
        return "reorder";
    }
    return "unknown";
}

SdfSpecType
_GetSpecTypeFromContext(Sdf_TextParserCurrentParsingContext parsingContext)
{
    if (parsingContext ==
        Sdf_TextParserCurrentParsingContext::AttributeSpec)
    {
        return SdfSpecTypeAttribute;
    }
    if (parsingContext ==
        Sdf_TextParserCurrentParsingContext::RelationshipSpec)
    {
        return SdfSpecTypeRelationship;
    }
    if (parsingContext == Sdf_TextParserCurrentParsingContext::PrimSpec)
    {
        return SdfSpecTypePrim;
    }
    if(parsingContext == Sdf_TextParserCurrentParsingContext::LayerSpec)
    {
        return SdfSpecTypePseudoRoot;
    }
    if (parsingContext == 
            Sdf_TextParserCurrentParsingContext::VariantStatementList)
    {
        return SdfSpecTypePrim;
    }
    return SdfSpecTypeUnknown;
}

void
_PushContext(Sdf_TextParserContext& context,
    Sdf_TextParserCurrentParsingContext newParsingContext)
{
    TF_DEBUG(SDF_TEXT_FILE_FORMAT_CONTEXT).Msg(
        "Pushing context: " + _ContextToString(newParsingContext) + "\n");
    if (!context.parsingContext.empty())
    {
        TF_DEBUG(SDF_TEXT_FILE_FORMAT_CONTEXT).Msg(
            "Parent: " + _ContextToString(context.parsingContext.back()) + "\n");
    }

    TF_DEBUG(SDF_TEXT_FILE_FORMAT_CONTEXT).Msg(
        "Current path: " + context.path.GetAsString() + "\n\n");

    context.parsingContext.push_back(newParsingContext);
}

void
_PopContext(Sdf_TextParserContext& context)
{
    TF_DEBUG(SDF_TEXT_FILE_FORMAT_CONTEXT).Msg(
        "Popping context: " + 
            _ContextToString(context.parsingContext.back()) + "\n");
    
    context.parsingContext.pop_back();
    
    if (!context.parsingContext.empty())
    {
        TF_DEBUG(SDF_TEXT_FILE_FORMAT_CONTEXT).Msg(
            "Return to: " + 
                _ContextToString(context.parsingContext.back()) + "\n");
    }
}

} // end namespace Sdf_TextFileFormatParser

PXR_NAMESPACE_CLOSE_SCOPE
