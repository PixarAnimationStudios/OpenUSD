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

UsdPrimDefinition::UsdPrimDefinition(const SdfPrimSpecHandle &primSpec, 
                                     bool isAPISchema) 
{
    // Cache the property names and the paths to each property spec from the
    // prim spec. For API schemas, the prim spec will not provide metadata for
    // the prim itself.
    _SetPrimSpec(primSpec, /*providesPrimMetadata =*/ !isAPISchema);
}

std::string 
UsdPrimDefinition::GetDocumentation() const 
{
    // Special case for prim documentation. Pure API schemas don't map their
    // prim spec paths to the empty token as they aren't meant to provide 
    // metadata fallbacks so _HasField will always return false. To get 
    // documentation for an API schema, we have to ask the prim spec (which
    // will work for all definitions).
    return _primSpec ? _primSpec->GetDocumentation() : std::string();
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
UsdPrimDefinition::_SetPrimSpec(
    const SdfPrimSpecHandle &primSpec, bool providesPrimMetadata)
{
    _primSpec = primSpec;

    // If there are no properties yet, we can just copy them all from the prim
    // spec without worrying about handling duplicates. Otherwise we have to
    // _AddProperty.
    if (_propPathMap.empty()) {
        for (SdfPropertySpecHandle prop: primSpec->GetProperties()) {
            _propPathMap[prop->GetNameToken()] = prop->GetPath();
            _properties.push_back(prop->GetNameToken());
        }
    } else {
        for (SdfPropertySpecHandle prop: primSpec->GetProperties()) {
            _AddProperty(prop->GetNameToken(), prop->GetPath());
        }
    }

    // If this prim spec will provide the prim metadata, map the empty property
    // name to the prim path for the field accessor functions.
    if (providesPrimMetadata) {
        _propPathMap[TfToken()] = primSpec->GetPath();
    }
}

void 
UsdPrimDefinition::_ApplyPropertiesFromPrimDef(
    const UsdPrimDefinition &primDef, const std::string &propPrefix)
{
    if (propPrefix.empty()) {
        for (const auto &it : primDef._propPathMap) {
            _AddProperty(it.first, it.second);
        }
    } else {
        for (const auto &it : primDef._propPathMap) {
            // Apply the prefix to each property name before adding it.
            const TfToken prefixedPropName(
                SdfPath::JoinIdentifier(propPrefix, it.first.GetString()));
            _AddProperty(prefixedPropName, it.second);
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

