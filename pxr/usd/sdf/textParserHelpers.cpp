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

#include "pxr/usd/sdf/textParserHelpers.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_TextFileFormatParser {

bool
_SetupValue(const std::string& typeName, Sdf_TextParserContext& context)
{
    return context.values.SetupFactory(typeName);
}

bool
_GetPermissionFromString(const std::string& str, 
                         SdfPermission& permission)
{
    if (str == "public")
    {
        permission = SdfPermissionPublic;
        return true;
    }
    else if(str == "private")
    {
        permission = SdfPermissionPrivate;
        return true;
    }

    return false;
}

bool
_GetDisplayUnitFromString(const std::string & name,
                          TfEnum& value)
{
    const TfEnum &unit = SdfGetUnitFromName(name);
    if (unit == TfEnum()) {
        return false;
    }
    value = unit;
    return true;
}

void
_ValueAppendAtomic(const Sdf_ParserHelpers::Value& arg1,
    Sdf_TextParserContext& context)
{
    context.values.AppendValue(arg1);
}

bool
_ValueSetAtomic(Sdf_TextParserContext& context, std::string& errorMessage)
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
_PrimSetInheritListItems(SdfListOpType opType, Sdf_TextParserContext& context,
    std::string& errorMessage) 
{
    if (context.inheritParsingTargetPaths.empty() &&
        opType != SdfListOpTypeExplicit) {
        errorMessage =
            "Setting inherit paths to None (or empty list) is only allowed "
            "when setting explicit inherit paths, not for list editing";
        return false;
    }

    TF_FOR_ALL(path, context.inheritParsingTargetPaths) {
        const SdfAllowed allow = SdfSchema::IsValidInheritPath(*path);
        if (!allow)
        {
            errorMessage = allow.GetWhyNot();
            return false;
        }
    }

    return _SetListOpItems(SdfFieldKeys->InheritPaths, opType, 
            context.inheritParsingTargetPaths, context, errorMessage);
}

void
_InheritAppendPath(Sdf_TextParserContext& context)
{
    // Expand paths relative to the containing prim.
    //
    // This strips any variant selections from the containing prim
    // path before expanding the relative path, which is what we
    // want.  Inherit paths are not allowed to be variants.
    SdfPath absPath = 
        context.savedPath.MakeAbsolutePath(context.path.GetPrimPath());

    context.inheritParsingTargetPaths.push_back(absPath);
}

bool
_PrimSetSpecializesListItems(SdfListOpType opType,
    Sdf_TextParserContext& context, std::string& errorMessage) 
{
    if (context.specializesParsingTargetPaths.empty() &&
        opType != SdfListOpTypeExplicit) {
        errorMessage = 
            "Setting specializes paths to None (or empty list) is only allowed "
            "when setting explicit specializes paths, not for list editing";
        return false;
    }

    TF_FOR_ALL(path, context.specializesParsingTargetPaths) {
        const SdfAllowed allow = SdfSchema::IsValidSpecializesPath(*path);
        if (!allow)
        {
            errorMessage = allow.GetWhyNot();
            return false;
        }
    }

    return _SetListOpItems(SdfFieldKeys->Specializes, opType, 
            context.specializesParsingTargetPaths, context, errorMessage);
}

void
_SpecializesAppendPath(Sdf_TextParserContext& context)
{
    // Expand paths relative to the containing prim.
    //
    // This strips any variant selections from the containing prim
    // path before expanding the relative path, which is what we
    // want.  Specializes paths are not allowed to be variants.
    SdfPath absPath = 
        context.savedPath.MakeAbsolutePath(context.path.GetPrimPath());

    context.specializesParsingTargetPaths.push_back(absPath);
}

bool
_PrimSetReferenceListItems(SdfListOpType opType, Sdf_TextParserContext& context,
    std::string& errorMessage) 
{
    if (context.referenceParsingRefs.empty() &&
        opType != SdfListOpTypeExplicit) {
        errorMessage = 
            "Setting references to None (or an empty list) is only allowed "
            "when setting explicit references, not for list editing";
        return false;
    }

    TF_FOR_ALL(ref, context.referenceParsingRefs) {
        const SdfAllowed allow = SdfSchema::IsValidReference(*ref);
        if (!allow)
        {
            errorMessage = allow.GetWhyNot();
            return false;
        }
    }

    return _SetListOpItems(SdfFieldKeys->References, opType, 
        context.referenceParsingRefs, context, errorMessage);
}

bool
_PrimSetPayloadListItems(SdfListOpType opType, Sdf_TextParserContext& context,
    std::string& errorMessage) 
{
    if (context.payloadParsingRefs.empty() &&
        opType != SdfListOpTypeExplicit) {
        errorMessage = 
            "Setting payload to None (or an empty list) is only allowed "
            "when setting explicit payloads, not for list editing";
        return false;
    }

    TF_FOR_ALL(ref, context.payloadParsingRefs) {
        const SdfAllowed allow = SdfSchema::IsValidPayload(*ref);
        if (!allow)
        {
            errorMessage = allow.GetWhyNot();
            return false;
        }
    }

    return _SetListOpItems(SdfFieldKeys->Payload, opType, 
        context.payloadParsingRefs, context, errorMessage);
}

bool
_PrimSetVariantSetNamesListItems(SdfListOpType opType, 
                                 Sdf_TextParserContext& context,
                                 std::string& errorMessage)
{
    std::vector<std::string> names;
    names.reserve(context.nameVector.size());
    TF_FOR_ALL(name, context.nameVector) {
        const SdfAllowed allow = SdfSchema::IsValidVariantIdentifier(*name);
        if (!allow)
        {
            errorMessage = allow.GetWhyNot();
            return false;
        }
        names.push_back(name->GetText());
    }

    if(!_SetListOpItems(SdfFieldKeys->VariantSetNames, opType,
        names, context, errorMessage))
    {
        return false;
    }

    // If the op type is added or explicit, create the variant sets
    if (opType == SdfListOpTypeAdded || opType == SdfListOpTypeExplicit) {
        TF_FOR_ALL(i, context.nameVector) {
            _CreateSpec(
                context.path.AppendVariantSelection(*i,""),
                SdfSpecTypeVariantSet, context);
        }

        _SetField(
            context.path, SdfChildrenKeys->VariantSetChildren, 
            context.nameVector, context);
    }

    return true;
}

void
_RelationshipInitTarget(const SdfPath& targetPath,
                        Sdf_TextParserContext& context)
{
    SdfPath path = context.path.AppendTarget(targetPath);

    if (!_HasSpec(path, context)) {
        // Create relationship target spec by setting the appropriate 
        // object type flag.
        _CreateSpec(path, SdfSpecTypeRelationshipTarget, context);

        // Add the target path to the owning relationship's list of target 
        // children.
        context.relParsingNewTargetChildren.push_back(targetPath);
    }
}

bool
_RelationshipSetTargetsList(SdfListOpType opType, 
                            Sdf_TextParserContext& context,
                            std::string& errorMessage)
{
    if (!context.relParsingTargetPaths) {
        // No target paths were encountered.
        return true;
    }

    if (context.relParsingTargetPaths->empty() &&
        opType != SdfListOpTypeExplicit) {
        errorMessage = 
            "Setting relationship targets to None (or empty list) is only "
            "allowed when setting explicit targets, not for list editing";
        return false;
    }

    TF_FOR_ALL(path, *context.relParsingTargetPaths) {
        const SdfAllowed allow = SdfSchema::IsValidRelationshipTargetPath(*path);
        if (!allow)
        {
            errorMessage = allow.GetWhyNot();
            return false;
        }
    }

    if (opType == SdfListOpTypeAdded || 
        opType == SdfListOpTypeExplicit) {

        // Initialize relationship target specs for each target path that
        // is added in this layer.
        TF_FOR_ALL(pathIter, *context.relParsingTargetPaths) {
            _RelationshipInitTarget(*pathIter, context);
        }
    }

    return _SetListOpItems(SdfFieldKeys->TargetPaths, opType, 
        *context.relParsingTargetPaths, context, errorMessage);
}

bool
_PrimSetVariantSelection(Sdf_TextParserContext& context, std::string& errorMessage)
{
    SdfVariantSelectionMap refVars;

    // The previous parser implementation allowed multiple variant selection
    // dictionaries in prim metadata to be merged, so we do the same here.
    VtValue oldVars;
    if (_HasField(
            context.path, SdfFieldKeys->VariantSelection, &oldVars, context)) {
        refVars = oldVars.Get<SdfVariantSelectionMap>();
    }

    TF_FOR_ALL(it, context.currentDictionaries[0]) {
        if (!it->second.IsHolding<std::string>()) {
            errorMessage = "variant name must be a string";
            return false;
        } else {
            const std::string variantName = it->second.Get<std::string>();
            const SdfAllowed allow = SdfSchema::IsValidVariantSelection(variantName);
            if (!allow)
            {
                errorMessage = allow.GetWhyNot();
                return false;
            }

            refVars[it->first] = variantName;
        }
    }

    _SetField(context.path, SdfFieldKeys->VariantSelection, refVars, context);
    context.currentDictionaries[0].clear();

    return true;
}

bool
_RelocatesAdd(const Sdf_ParserHelpers::Value& arg1, 
    const Sdf_ParserHelpers::Value& arg2, Sdf_TextParserContext& context,
    std::string& errorMessage)
{
    const std::string& srcStr    = arg1.Get<std::string>();
    const std::string& targetStr = arg2.Get<std::string>();

    SdfPath srcPath(srcStr);
    SdfPath targetPath(targetStr);

    if (!SdfSchema::IsValidRelocatesPath(srcPath)) {
        errorMessage = srcStr + " is not a valid relocates path";
        return false;
    }
    if (!SdfSchema::IsValidRelocatesPath(targetPath)) {
        errorMessage = targetStr + " is not a valid relocates path";
        return false;
    }

    // The relocates map is expected to only hold absolute paths. The
    // SdRelocatesMapProxy ensures that all paths are made absolute when
    // editing, but since we're bypassing that proxy and setting the map
    // directly into the underlying SdfData, we need to explicitly absolutize
    // paths here.
    const SdfPath srcAbsPath = srcPath.MakeAbsolutePath(context.path);
    const SdfPath targetAbsPath = targetPath.MakeAbsolutePath(context.path);

    context.relocatesParsingMap.insert(std::make_pair(srcAbsPath, 
                                                      targetAbsPath));
    context.layerHints.mightHaveRelocates = true;

    return true;
}

bool
_AttributeSetConnectionTargetsList(SdfListOpType opType, 
                                   Sdf_TextParserContext& context,
                                   std::string& errorMessage)
{
    if (context.connParsingTargetPaths.empty() &&
        opType != SdfListOpTypeExplicit) {
        errorMessage = "Setting connection paths to None (or an empty list) "
            "is only allowed when setting explicit connection paths, "
            "not for list editing";
        return false;
    }

    TF_FOR_ALL(path, context.connParsingTargetPaths) {
        const SdfAllowed allow = 
            SdfSchema::IsValidAttributeConnectionPath(*path);
        if (!allow)
        {
            errorMessage = allow.GetWhyNot();
            return false;
        }
    }

    if (opType == SdfListOpTypeAdded || 
        opType == SdfListOpTypeExplicit) {

        TF_FOR_ALL(pathIter, context.connParsingTargetPaths) {
            SdfPath path = context.path.AppendTarget(*pathIter);
            if (!_HasSpec(path, context)) {
                _CreateSpec(path, SdfSpecTypeConnection, context);
            }
        }

        _SetField(
            context.path, SdfChildrenKeys->ConnectionChildren,
            context.connParsingTargetPaths, context);
    }

    return _SetListOpItems(SdfFieldKeys->ConnectionPaths, opType, 
        context.connParsingTargetPaths, context, errorMessage);
}

void
_AttributeAppendConnectionPath(Sdf_TextParserContext& context,
size_t lineNumber)
{
    // Expand paths relative to the containing prim.
    //
    // This strips any variant selections from the containing prim
    // path before expanding the relative path, which is what we
    // want.  Connection paths never point into the variant namespace.
    SdfPath absPath = 
        context.savedPath.MakeAbsolutePath(context.path.GetPrimPath());

    // XXX Workaround for bug 68132:
    // Prior to the fix to bug 67916, FilterGenVariantBase was authoring
    // invalid connection paths containing variant selections (which
    // Sd was failing to report as erroneous).  As a result, there's
    // a fair number of assets out there with these broken forms of
    // connection paths.  As a migration measure, we discard those
    // variant selections here.
    if (absPath.ContainsPrimVariantSelection()) {
        TF_WARN("Connection path <%s> (in file @%s@, line %zu) has a variant "
                "selection, but variant selections are not meaningful in "
                "connection paths.  Stripping the variant selection and "
                "using <%s> instead.  Resaving the file will fix this issue.",
                absPath.GetText(),
                context.fileContext.c_str(),
                lineNumber,
                absPath.StripAllVariantSelections().GetText());
        absPath = absPath.StripAllVariantSelections();
    }

    context.connParsingTargetPaths.push_back(absPath);
}

bool
_PrimInitAttribute(const Sdf_ParserHelpers::Value &arg1,
    Sdf_TextParserContext& context, std::string& errorMessage)  
{
    TfToken name(arg1.Get<std::string>());
    if (!SdfPath::IsValidNamespacedIdentifier(name)) {
        errorMessage = "'" + name.GetString() + 
            "' is not a valid attribute name";
        return false;
    }

    context.path = context.path.AppendProperty(name);

    // If we haven't seen this attribute before, then set the object type
    // and add it to the parent's list of properties. Otherwise both have
    // already been done, so we don't need to do anything.
    if (!_HasSpec(context.path, context)) {
        context.propertiesStack.back().push_back(name);
        _CreateSpec(context.path, SdfSpecTypeAttribute, context);
        _SetField(context.path, SdfFieldKeys->Custom, false, context);
    }

    if(context.custom)
        _SetField(context.path, SdfFieldKeys->Custom, true, context);

    // If the type was previously set, check that it matches. Otherwise set it.
    const TfToken newType(context.values.valueTypeName);

    VtValue oldTypeValue;
    if (_HasField(
            context.path, SdfFieldKeys->TypeName, &oldTypeValue, context)) {
        const TfToken& oldType = oldTypeValue.Get<TfToken>();

        if (newType != oldType) {
            errorMessage = "attribute '" + context.path.GetName() + 
                "' already has type '" + oldType.GetString() + 
                "', cannot change to '" + newType.GetString() + "'";

            return false;
        }
    }
    else {
        _SetField(context.path, SdfFieldKeys->TypeName, newType, context);
    }

    // If the variability was previously set, check that it matches. Otherwise
    // set it.  If the 'variability' VtValue is empty, that indicates varying
    // variability.
    SdfVariability variability = context.variability.IsEmpty() ? 
        SdfVariabilityVarying : context.variability.Get<SdfVariability>();
    VtValue oldVariability;
    if (_HasField(
            context.path, SdfFieldKeys->Variability, &oldVariability, 
            context)) {
        if (variability != oldVariability.Get<SdfVariability>()) {
            errorMessage = "attribute '" + context.path.GetName() +
                "' already has variability '" +
                TfEnum::GetName(oldVariability.Get<SdfVariability>()) +
                "', cannot change to '" +
                TfEnum::GetName(variability) + "'";
            return false;
        }
    } else {
        _SetField(
            context.path, SdfFieldKeys->Variability, variability, context);
    }

    return true;
}

void
_DictionaryBegin(Sdf_TextParserContext& context)
{
    context.currentDictionaries.push_back(VtDictionary());

    // Whenever we parse a value for an unregistered generic metadata field, 
    // the parser value context records the string representation only, because
    // we don't have enough type information to generate a C++ value. However,
    // dictionaries are a special case because we have all the type information
    // we need to generate C++ values. So, override the previous setting.
    if (context.values.IsRecordingString()) {
        context.values.StopRecordingString();
    }
}

void
_DictionaryEnd(Sdf_TextParserContext& context)
{
    context.currentDictionaries.pop_back();
}

void
_DictionaryInsertValue(const Sdf_ParserHelpers::Value& arg1,
    Sdf_TextParserContext& context)
{
    size_t n = context.currentDictionaries.size();
    context.currentDictionaries[n-2][arg1.Get<std::string>()] = 
        context.currentValue;
}

void
_DictionaryInsertDictionary(const Sdf_ParserHelpers::Value& arg1,
                            Sdf_TextParserContext& context)
{
    size_t n = context.currentDictionaries.size();
    // Insert the parsed dictionary into the parent dictionary.
    context.currentDictionaries[n-2][arg1.Get<std::string>()].Swap(
        context.currentDictionaries[n-1]);
    // Clear out the last dictionary (there can be more dictionaries on the
    // same nesting level).
    context.currentDictionaries[n-1].clear();
}

bool
_DictionaryInitScalarFactory(const Sdf_ParserHelpers::Value& arg1,
                             Sdf_TextParserContext& context,
                             std::string& errorMessage)
{
    const std::string& typeName = arg1.Get<std::string>();
    if (!_SetupValue(typeName, context)) {
        errorMessage = "Unrecognized value typename '" + typeName +
            "' for dictionary";
        return false;
    }

    return true;
}

bool
_DictionaryInitShapedFactory(const Sdf_ParserHelpers::Value& arg1,
                             Sdf_TextParserContext& context,
                             std::string& errorMessage)
{
    const std::string typeName = arg1.Get<std::string>() + "[]";
    if (!_SetupValue(typeName, context)) {
        errorMessage = "Unrecognized value typename '" + typeName +
            "' for dictionary";
        return false;
    }

    return true;
}

bool
_ValueSetTuple(Sdf_TextParserContext& context, std::string& errorMessage)
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
_ValueSetList(Sdf_TextParserContext& context, std::string& errorMessage)
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
_ValueSetShaped(Sdf_TextParserContext& context, std::string& errorMessage)
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
_ValueSetCurrentToSdfPath(const Sdf_ParserHelpers::Value& arg1,
                                     Sdf_TextParserContext& context)
{
    // make current Value an SdfPath of the given argument...
    std::string s = arg1.Get<std::string>();
    // If path is empty, use default c'tor to construct empty path.
    // XXX: 08/04/08 Would be nice if SdfPath would allow 
    // SdfPath("") without throwing a warning.
    context.currentValue = s.empty() ? SdfPath() : SdfPath(s);
}

bool
_PrimInitRelationship(const Sdf_ParserHelpers::Value& arg1,
                      Sdf_TextParserContext& context,
                      std::string& errorMessage)
{
    TfToken name( arg1.Get<std::string>() );
    if (!SdfPath::IsValidNamespacedIdentifier(name)) {
        errorMessage = std::string(name.GetText()) +
            " is not a valid relationship name";
        return false;
    }

    context.path = context.path.AppendProperty(name);

    if (!_HasSpec(context.path, context)) {
        context.propertiesStack.back().push_back(name);
        _CreateSpec(context.path, SdfSpecTypeRelationship, context);
    }

    _SetField(
        context.path, SdfFieldKeys->Variability, 
        context.variability, context);

    if (context.custom) {
        _SetField(context.path, SdfFieldKeys->Custom, context.custom, context);
    }

    context.relParsingTargetPaths.reset();
    context.relParsingNewTargetChildren.clear();

    return true;
}

void
_PrimEndRelationship(Sdf_TextParserContext& context)
{
    if (!context.relParsingNewTargetChildren.empty()) {
        std::vector<SdfPath> children = 
            context.data->GetAs<std::vector<SdfPath> >(
                context.path, SdfChildrenKeys->RelationshipTargetChildren);

        children.insert(children.end(), 
                        context.relParsingNewTargetChildren.begin(),
                        context.relParsingNewTargetChildren.end());

        _SetField(
            context.path, SdfChildrenKeys->RelationshipTargetChildren, 
            children, context);
    }

    context.path = context.path.GetParentPath();
}

void
_RelationshipAppendTargetPath(const Sdf_ParserHelpers::Value& arg1,
                              Sdf_TextParserContext& context)
{
    // Add a new target to the current relationship
    const std::string& pathStr = arg1.Get<std::string>();
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

bool
_PathSetPrim(const Sdf_ParserHelpers::Value& arg1,
    Sdf_TextParserContext& context, std::string& errorMessage)
{
    const std::string& pathStr = arg1.Get<std::string>();
    context.savedPath = SdfPath(pathStr);
    if (!context.savedPath.IsPrimPath()) {
        errorMessage = pathStr + " is not a valid prim path";
        return false;
    }

    return true;
}

bool
_PathSetPrimOrPropertyScenePath(const Sdf_ParserHelpers::Value& arg1,
                                Sdf_TextParserContext& context,
                                std::string& errorMessage)
{
    const std::string& pathStr = arg1.Get<std::string>();
    context.savedPath = SdfPath(pathStr);
    // Valid paths are prim or property paths that do not contain variant
    // selections.
    SdfPath const &path = context.savedPath;
    bool pathValid = (path.IsPrimPath() || path.IsPropertyPath()) &&
        !path.ContainsPrimVariantSelection();
    if (!pathValid) {
        errorMessage = pathStr + " is not a valid prim or property scene path";
        return false;
    }

    return true;
}

void
_SetGenericMetadataListOpItems(const TfType& fieldType, 
                               Sdf_TextParserContext& context)
{
    // Chain together attempts to set list op items using 'or' to bail
    // out as soon as we successfully write out the list op we're holding.
    std::string errorMessage;
    _SetItemsIfListOp<SdfIntListOp>(fieldType, context, errorMessage)    ||
    _SetItemsIfListOp<SdfInt64ListOp>(fieldType, context, errorMessage)  ||
    _SetItemsIfListOp<SdfUIntListOp>(fieldType, context, errorMessage)   ||
    _SetItemsIfListOp<SdfUInt64ListOp>(fieldType, context, errorMessage) ||
    _SetItemsIfListOp<SdfStringListOp>(fieldType, context, errorMessage) ||
    _SetItemsIfListOp<SdfTokenListOp>(fieldType, context, errorMessage);
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

void
_GenericMetadataStart(const Sdf_ParserHelpers::Value &name, SdfSpecType specType,
                      Sdf_TextParserContext& context)
{
    context.genericMetadataKey = TfToken(name.Get<std::string>());
    context.listOpType = SdfListOpTypeExplicit;

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
        // In _GenericMetadataEnd, we'll produce this list and set it
        // into the appropriate place in the list op.
        TfType itemArrayType;
        if (_IsGenericMetadataListOpType(fieldType, &itemArrayType)) {
            _SetupValue(schema.FindType(itemArrayType).
                            GetAsToken().GetString(), context);
        }
        else {
            _SetupValue(schema.FindType(fieldDef.GetFallbackValue()).
		            GetAsToken().GetString(), context);
        }
    } else {
        // Prepare to parse only the string representation of this metadata
        // value, since it's an unregistered field.
        context.values.StartRecordingString();
    }
}

bool
_GenericMetadataEnd(SdfSpecType specType, Sdf_TextParserContext& context,
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
                errorMessage = "invalid value for field " + 
                    std::string(context.genericMetadataKey.GetText());

                return false;
            }
            else {
                _SetGenericMetadataListOpItems(fieldType, context);
            }
        }
        else {
            if (!fieldDef.IsValidValue(context.currentValue) ||
                context.currentValue.IsEmpty()) {
                errorMessage = "invalid value for field " +
                    std::string(context.genericMetadataKey.GetText());
                
                return false;
            }
            else {
                _SetField(
                    context.path, context.genericMetadataKey, 
                    context.currentValue, context);
            }
        }
    } else if (specDef.IsValidField(context.genericMetadataKey)) {
        // Prevent the user from overwriting fields that aren't metadata
        errorMessage = std::string(context.genericMetadataKey.GetText()) + 
            " is registered as a non-metadata field";

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
            auto getOldValue = [&context]() {
                VtValue v;
                if (_HasField(context.path, context.genericMetadataKey,
                              &v, context)
                    && TF_VERIFY(v.IsHolding<SdfUnregisteredValue>())) {
                    v = v.UncheckedGet<SdfUnregisteredValue>().GetValue();
                }
                else {
                    v = VtValue();
                }
                return v;
            };

            auto getRecordedStringAsUnregisteredValue = [&context]() {
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
            _SetField(context.path, context.genericMetadataKey, 
                      value, context);
        }
    }

    context.values.Clear();
    context.currentValue = VtValue();

    return true;
}

std::string
_UnpadNamespacedName(const std::string& in)
{
    // namespaced names are resolved by the lexer rules with
    // spaces accepted around the namespaced delimeter `::`
    // so we have to remove those spaces to get an unpadded name
    std::vector<std::string> nameParts = TfStringSplit(in, "::");
    std::string namespacedName = nameParts[0];
    if (nameParts.size() > 1)
    {
        for (size_t i = 1; i < nameParts.size(); i++)
        {
            namespacedName = namespacedName + "::" + TfStringTrim(nameParts[i]);
        }
    }

    return namespacedName;
}

Sdf_ParserHelpers::Value
_GetValueFromString(const std::string& in,
    size_t lineNumber,
    Sdf_TextParserContext& context)
{
    Sdf_ParserHelpers::Value value;
    const std::string negativeZero = "-0";
    const std::string negativeInfinity = "-inf";
    const std::string positiveInfinity = "inf";
    const std::string nan = "nan";
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
    else if (TfStringContains(in, ".") || TfStringContains(in, "e") ||
        TfStringContains(in, "E"))
    {
        value = TfStringToDouble(in);
    }
    else
    {
        // positive and negative integers are stored as long
        // unless out of range
        bool outOfRange = false;
        if (TfStringStartsWith(in, "-"))
        {
            value = TfStringToInt64(in, &outOfRange);
        }
        else
        {
            value = TfStringToUInt64(in, &outOfRange);
        }

        if (outOfRange)
        {
            TF_WARN("Integer literal '%s' on line %zu%s%s out of range, parsing "
                "as double.  Consider exponential notation for large "
                "floating point values.", in.c_str(), lineNumber,
                context.fileContext.empty() ? "" : " in file ",
                context.fileContext.empty() ? "" :
                context.fileContext.c_str());

            value = TfStringToDouble(in);
        }
    }

    return value;
}

std::string
_GetAssetRefFromString(const std::string& in)
{
    bool isTripleDelimited = TfStringStartsWith(in, "@@@");
    return Sdf_EvalAssetPath(in.c_str(), in.length(), isTripleDelimited);
}

std::string
_GetEvaluatedStringFromString(const std::string& in,
    Sdf_TextParserContext& context)
{
    unsigned int numLines = 0;
    size_t numDelimeters = 1;
    if(TfStringStartsWith(in, "\"\"\"") || TfStringStartsWith(in, "'''"))
    {
        numDelimeters = 3;
    }

    std::string evaluatedString =
        Sdf_EvalQuotedString(in.c_str(), in.length(), numDelimeters, &numLines);

    return evaluatedString;
}

} // end namespace Sdf_TextFileFormatParser

PXR_NAMESPACE_CLOSE_SCOPE
