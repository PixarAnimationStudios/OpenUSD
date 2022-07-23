//
// Copyright 2020 Pixar
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
#include "pxr/usd/usd/primDefinition.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/copyUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdPrimDefinition::UsdPrimDefinition(
    const SdfPath &schematicsPrimPath, bool isAPISchema)
    : _schematicsPrimPath(schematicsPrimPath)
{
    // If this prim definition is not for an API schema, the primary prim spec 
    // will provide the prim metadata. map the empty property name to the prim 
    // path in the schematics for the field accessor functions.
    // Note that this mapping aids the efficiency of value resolution by 
    // allowing UsdStage to access fallback metadata from both prims and 
    // properties through the same code path without extra conditionals.
    if (!isAPISchema) {
        _propPathMap.emplace(TfToken(), _schematicsPrimPath);
    }
}

std::string 
UsdPrimDefinition::GetDocumentation() const 
{
    // Special case for prim documentation. Pure API schemas don't map their
    // prim spec paths to the empty token as they aren't meant to provide 
    // metadata fallbacks so _HasField will always return false. To get 
    // documentation for an API schema, we have to get the documentation
    // field from the schematics for the prim path (which we store for all 
    // definitions specifically to access the documentation).
    std::string docString;
    _GetSchematics()->HasField(
        _schematicsPrimPath, SdfFieldKeys->Documentation, &docString);
    return docString;
}

std::string 
UsdPrimDefinition::GetPropertyDocumentation(const TfToken &propName) const 
{
    if (propName.IsEmpty()) {
        return std::string();
    }
    std::string docString;
    _HasField(propName, SdfFieldKeys->Documentation, &docString);
    return docString;
}

TfTokenVector 
UsdPrimDefinition::_ListMetadataFields(const TfToken &propName) const
{
    if (const SdfPath *path = TfMapLookupPtr(_propPathMap, propName)) {
        // Get the list of fields from the schematics for the property (or prim)
        // path and remove the fields that we don't allow fallbacks for.
        TfTokenVector fields = _GetSchematics()->ListFields(*path);
        fields.erase(std::remove_if(fields.begin(), fields.end(), 
                                    &UsdSchemaRegistry::IsDisallowedField), 
                     fields.end());
        return fields;
    }
    return TfTokenVector();
}

void 
UsdPrimDefinition::_AddProperties(
    std::vector<std::pair<TfToken, SdfPath>> &&propNameToPathVec)
{
    _properties.reserve(_properties.size() + propNameToPathVec.size());

    for (auto &propNameAndPath : propNameToPathVec) {
        auto insertIt = _propPathMap.insert(std::move(propNameAndPath));
        if (insertIt.second) {
            _properties.push_back(insertIt.first->first);
        }
    }
}

// Returns true if the property with the given name in these two separate prim
// definitions have the same type. "Same type" here means that they are both
// the same kind of property (attribute or relationship) and if they are 
// attributes, that their attributes type names are the same.
static bool _PropertyTypesMatch(
    const UsdPrimDefinition &strongerPrimDef,
    const UsdPrimDefinition &weakerPrimDef,
    const TfToken &propName)
{
    // Empty prop name represents the schema's prim level metadata. This 
    // doesn't have a "property type" so it always matches.
    if (propName.IsEmpty()) {
        return true;
    }

    const SdfSpecType specType = strongerPrimDef.GetSpecType(propName);
    const bool specIsAttribute = (specType == SdfSpecTypeAttribute);

    // Compare spec types (relationship vs attribute)
    if (specType != weakerPrimDef.GetSpecType(propName)) {
        TF_WARN("%s '%s' from stronger schema failed to override %s '%s' "
                "from weaker schema during schema prim definition composition "
                "because of the property spec types do not match.",
                specIsAttribute ? "Attribute" : "Relationsip",
                propName.GetText(),
                specIsAttribute ? "relationsip" : "attribute",
                propName.GetText());
        return false;
    }

    // Done comparing if its not an attribute.
    if (!specIsAttribute) {
        return true;
    }

    // Compare the type name field of the attributes.
    TfToken strongerTypeName;
    strongerPrimDef.GetPropertyMetadata(
        propName, SdfFieldKeys->TypeName, &strongerTypeName);
    TfToken weakerTypeName;
    weakerPrimDef.GetPropertyMetadata(
        propName, SdfFieldKeys->TypeName, &weakerTypeName);
    if (weakerTypeName != strongerTypeName) {
        TF_WARN("Attribute '%s' with type name '%s' from stronger schema "
                "failed to override attribute '%s' with type name '%s' from "
                "weaker schema during schema prim definition composition "
                "because of the attribute type names do not match.",
                propName.GetText(),
                strongerTypeName.GetText(),
                propName.GetText(),
                weakerTypeName.GetText());
        return false;
    }
    return true;
}

void 
UsdPrimDefinition::_ComposePropertiesFromPrimDef(
    const UsdPrimDefinition &weakerPrimDef, 
    bool useWeakerPropertyForTypeConflict,
    const std::string &instanceName)
{
    _properties.reserve(_properties.size() + weakerPrimDef._properties.size());

    // Copy over property to path mappings from the weaker prim definition that 
    // aren't already in this prim definition.
    if (instanceName.empty()) {
        for (const auto &it : weakerPrimDef._propPathMap) {
            // Note that the prop name may be empty as we use the empty path to
            // map to the spec containing the prim level metadata. We need to 
            // make sure we don't add the empty name to properties list if 
            // we successfully insert a metadata mapping.
            auto insertResult = _propPathMap.insert(it);
            if (insertResult.second){
                if (!it.first.IsEmpty()) {
                    _properties.push_back(it.first);
                }
            } else {
                // The property exists already. If we need to use the weaker 
                // property in the event of a property type conflict, then we
                // check if the weaker property's type matches the existing, and
                // replace the existing with the weaker property if the types
                // do not match.
                if (useWeakerPropertyForTypeConflict &&
                    !_PropertyTypesMatch(*this, weakerPrimDef, it.first)) {
                    insertResult.first->second = it.second;
                }
            }
        }
    } else {
        for (const auto &it : weakerPrimDef._propPathMap) {
            // Apply the prefix to each property name before adding it.
            const TfToken instancedPropName = 
                UsdSchemaRegistry::MakeMultipleApplyNameInstance(
                    it.first, instanceName);
            auto insertResult = _propPathMap.emplace(
                instancedPropName, it.second);
            if (insertResult.second) {
                _properties.push_back(instancedPropName);
            } else {
                // The property exists already. If we need to use the weaker 
                // property in the event of a property type conflict, then we
                // check if the weaker property's type matches the existing, and
                // replace the existing with the weaker property if the types
                // do not match.
                if (useWeakerPropertyForTypeConflict && 
                    !_PropertyTypesMatch(
                        *this, weakerPrimDef, instancedPropName)) {
                    insertResult.first->second = it.second;
                }
            }
        }
    }
}

bool 
UsdPrimDefinition::FlattenTo(const SdfLayerHandle &layer, 
                             const SdfPath &path,  
                             SdfSpecifier newSpecSpecifier) const
{
    SdfChangeBlock block;

    // Find or create the target prim spec at the target layer.
    SdfPrimSpecHandle targetSpec = layer->GetPrimAtPath(path);
    if (targetSpec) {
        // If the target spec already exists, clear its properties and schema 
        // allowed metadata. This does not clear non-schema metadata fields like 
        // children, composition arc, clips, specifier, etc.
        targetSpec->SetProperties(SdfPropertySpecHandleVector());
        for (const TfToken &fieldName : targetSpec->ListInfoKeys()) {
            if (!UsdSchemaRegistry::IsDisallowedField(fieldName)) {
                targetSpec->ClearInfo(fieldName);
            }
        }
    } else {
        // Otherwise create a new target spec and set its specifier.
        targetSpec = SdfCreatePrimInLayer(layer, path);
        if (!targetSpec) {
            TF_WARN("Failed to create prim spec at path '%s' in layer '%s'",
                    path.GetText(), layer->GetIdentifier().c_str());
            return false;
        }
        targetSpec->SetSpecifier(newSpecSpecifier);
    }

    // Copy all properties.
    for (const TfToken &propName : GetPropertyNames()) {
        SdfPropertySpecHandle propSpec = GetSchemaPropertySpec(propName);
        if (TF_VERIFY(propSpec)) {
            if (!SdfCopySpec(propSpec->GetLayer(), propSpec->GetPath(),
                             layer, path.AppendProperty(propName))) {
                TF_WARN("Failed to copy prim defintion property '%s' to prim "
                        "spec at path '%s' in layer '%s'.", propName.GetText(),
                        path.GetText(), layer->GetIdentifier().c_str());
            }
        }
    }

    // Copy prim metadata
    for (const TfToken &fieldName : ListMetadataFields()) {
        VtValue fieldValue;
        if (GetMetadata(fieldName, &fieldValue)) {
            layer->SetField(path, fieldName, fieldValue);
        }
    }

    // Explicitly set the full list of applied API schemas in metadata as the 
    // the apiSchemas field copied from prim metadata will only contain the 
    // built-in API schemas of the underlying typed schemas but not any 
    // additional API schemas that may have been applied to this definition.
    layer->SetField(path, UsdTokens->apiSchemas, 
        VtValue(SdfTokenListOp::CreateExplicit(_appliedAPISchemas)));

    // Also explicitly set the documentation string. This is necessary when
    // flattening an API schema prim definition as GetMetadata doesn't return
    // the documentation as metadata for API schemas.
    targetSpec->SetDocumentation(GetDocumentation());

    return true;
}

UsdPrim 
UsdPrimDefinition::FlattenTo(const UsdPrim &parent, 
                             const TfToken &name,
                             SdfSpecifier newSpecSpecifier) const
{
    // Create the path of the prim we're flattening to.
    const SdfPath primPath = parent.GetPath().AppendChild(name);

    // Map the target prim to the edit target.
    const UsdEditTarget &editTarget = parent.GetStage()->GetEditTarget();
    const SdfLayerHandle &targetLayer = editTarget.GetLayer();
    const SdfPath &targetPath = editTarget.MapToSpecPath(primPath);
    if (targetPath.IsEmpty()) {
        return UsdPrim();
    }

    FlattenTo(targetLayer, targetPath, newSpecSpecifier);

    return parent.GetStage()->GetPrimAtPath(primPath);
}

UsdPrim 
UsdPrimDefinition::FlattenTo(const UsdPrim &prim,
                             SdfSpecifier newSpecSpecifier) const
{
    return FlattenTo(prim.GetParent(), prim.GetName(), newSpecSpecifier);
}

PXR_NAMESPACE_CLOSE_SCOPE

