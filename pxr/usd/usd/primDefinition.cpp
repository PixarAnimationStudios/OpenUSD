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

void 
UsdPrimDefinition::_IntializeForTypedSchema(
    const SdfLayerHandle &schematicsLayer,
    const SdfPath &schematicsPrimPath, 
    const VtTokenArray &propertiesToIgnore)
{
    _primLayerAndPath = {get_pointer(schematicsLayer), schematicsPrimPath};

    if (_MapSchematicsPropertyPaths(propertiesToIgnore)) {
        // Prim definition for Typed schemas use the prim spec to provide prim 
        // level metadata, so we map the empty property name to the prim  path
        // in the schematics for the field accessor functions. This mapping aids
        // the efficiency of value resolution by allowing UsdStage to access
        // fallback metadata from both prims and properties through the same
        // code path without extra conditionals.
        // Note that API schema prim definitions do not provide prim level
        // metadata so they exclude this mapping
        _propLayerAndPathMap.emplace(TfToken(), _primLayerAndPath);
    }
}

void 
UsdPrimDefinition::_IntializeForAPISchema(
    const TfToken &apiSchemaName,
    const SdfLayerHandle &schematicsLayer,
    const SdfPath &schematicsPrimPath, 
    const VtTokenArray &propertiesToIgnore)
{
    // We always include the API schema itself as the first applied API 
    // schema in its prim definition.
    _appliedAPISchemas = {apiSchemaName};

    _primLayerAndPath = {get_pointer(schematicsLayer), schematicsPrimPath};

    _MapSchematicsPropertyPaths(propertiesToIgnore);
}

SdfPropertySpecHandle 
UsdPrimDefinition::GetSchemaPropertySpec(const TfToken& propName) const
{
    if (const _LayerAndPath *layerAndPath = _GetPropertyLayerAndPath(propName)) {
        // XXX: We have to const cast because the schematics layers really
        // shouldn't be editable via the prim definitions. But these methods 
        // already exist and return an editable property spec. These methods
        // should one day be deprecated and replaced. 
        return const_cast<SdfLayer *>(layerAndPath->layer)->GetPropertyAtPath(
            layerAndPath->path);
    }
    return TfNullPtr;
}

SdfAttributeSpecHandle 
UsdPrimDefinition::GetSchemaAttributeSpec(const TfToken& attrName) const
{
    if (const _LayerAndPath *layerAndPath = _GetPropertyLayerAndPath(attrName)) {
        // XXX: We have to const cast because the schematics layers really
        // shouldn't be editable via the prim definitions. But these methods 
        // already exist and return an editable property spec. These methods
        // should one day be deprecated and replaced. 
        return const_cast<SdfLayer *>(layerAndPath->layer)->GetAttributeAtPath(
            layerAndPath->path);
    }
    return TfNullPtr;
}

SdfRelationshipSpecHandle 
UsdPrimDefinition::GetSchemaRelationshipSpec(const TfToken& relName) const
{
    if (const _LayerAndPath *layerAndPath = _GetPropertyLayerAndPath(relName)) {
        // XXX: We have to const cast because the schematics layers really
        // shouldn't be editable via the prim definitions. But these methods 
        // already exist and return an editable property spec. These methods
        // should one day be deprecated and replaced. 
        return const_cast<SdfLayer *>(layerAndPath->layer)->GetRelationshipAtPath(
            layerAndPath->path);
    }
    return TfNullPtr;
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
    _primLayerAndPath.HasField(SdfFieldKeys->Documentation, &docString);
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
    if (const _LayerAndPath *layerAndPath = _GetPropertyLayerAndPath(propName)) {
        return layerAndPath->ListMetadataFields();
    }
    return TfTokenVector();
}

TfTokenVector 
UsdPrimDefinition::_LayerAndPath::ListMetadataFields() const
{
    // Get the list of fields from the schematics for the property (or prim)
    // path and remove the fields that we don't allow fallbacks for.
    TfTokenVector fields = layer->ListFields(path);
    fields.erase(
        std::remove_if(fields.begin(), fields.end(), 
                       &UsdSchemaRegistry::IsDisallowedField), 
        fields.end());
    return fields;
}

bool 
UsdPrimDefinition::_MapSchematicsPropertyPaths(
    const VtTokenArray &propertiesToIgnore)
{
    // Get the names of all the properties defined in the prim spec.
    TfTokenVector specPropertyNames;
    if (!_primLayerAndPath.HasField(
            SdfChildrenKeys->PropertyChildren, &specPropertyNames)) {
        if (!_primLayerAndPath.layer->HasSpec(_primLayerAndPath.path)) {
            // While its possible for the spec to have no properties, we expect 
            // the prim spec itself to exist.
            TF_WARN("No prim spec exists at path '%s' in schematics layer %s.",
                    _primLayerAndPath.path.GetText(),
                    _primLayerAndPath.layer->GetIdentifier().c_str());
            return false;
        }
        return true;
    }

    auto addPropFn = [&](TfToken &propName) {
        _LayerAndPath propLayerAndPath {
            _primLayerAndPath.layer, 
            _primLayerAndPath.path.AppendProperty(propName)};
        auto insertIt = _propLayerAndPathMap.emplace(
            std::move(propName), std::move(propLayerAndPath));
        if (insertIt.second) {
            _properties.push_back(insertIt.first->first);
        }
    };

    _properties.reserve(specPropertyNames.size());
    // If there are no properties to ignore just simply add all the properties
    // found in the spec. Otherwise, we need to do the check to skip adding
    // any ignored properties.
    if (propertiesToIgnore.empty()) {
        for (TfToken &propName : specPropertyNames) {
            addPropFn(propName);
        }
    } else {
        for (TfToken &propName : specPropertyNames) {
            // Note the propertiesToIgnore list is expected to be extremely 
            // small (like a few entries at most) so linear search should be
            // efficient enough.
            if (std::find(propertiesToIgnore.begin(),
                          propertiesToIgnore.end(), 
                          propName) == propertiesToIgnore.end()) {
                addPropFn(propName);
            }
        }
    }
    
    return true;
}

// Returns true if the property with the given name in these two separate prim
// definitions have the same type. "Same type" here means that they are both
// the same kind of property (attribute or relationship) and if they are 
// attributes, that their attributes type names are the same.
/*static*/
bool UsdPrimDefinition::_PropertyTypesMatch(
    const _LayerAndPath &strongLayerAndPath,
    const _LayerAndPath &weakLayerAndPath)
{
    const SdfSpecType specType = strongLayerAndPath.GetSpecType();
    const bool specIsAttribute = (specType == SdfSpecTypeAttribute);

    // Compare spec types (relationship vs attribute)
    if (specType != weakLayerAndPath.GetSpecType()) {
        TF_WARN("%s at path '%s' from stronger schema failed to override %s at "
                "'%s' from weaker schema during schema prim definition "
                "composition because of the property spec types do not match.",
                specIsAttribute ? "Attribute" : "Relationsip",
                strongLayerAndPath.path.GetText(),
                specIsAttribute ? "relationsip" : "attribute",
                weakLayerAndPath.path.GetText());
        return false;
    }

    // Compare variability
    SdfVariability strongVariability, weakVariability;
    strongLayerAndPath.HasField(SdfFieldKeys->Variability, &strongVariability);
    weakLayerAndPath.HasField(SdfFieldKeys->Variability, &weakVariability);
    if (weakVariability != strongVariability) {
        TF_WARN("Property at path '%s' from stronger schema failed to override "
                "property at path '%s' from weaker schema during schema prim "
                "definition composition because their variability does not "
                "match.",
                strongLayerAndPath.path.GetText(),
                weakLayerAndPath.path.GetText());
        return false;
    }

    // Done comparing if it's not an attribute.
    if (!specIsAttribute) {
        return true;
    }

    // Compare the type name field of the attributes.
    TfToken strongTypeName, weakTypeName;
    strongLayerAndPath.HasField(SdfFieldKeys->TypeName, &strongTypeName);
    weakLayerAndPath.HasField(SdfFieldKeys->TypeName, &weakTypeName);
    if (weakTypeName != strongTypeName) {
        TF_WARN("Attribute at path '%s' with type name '%s' from stronger "
                "schema failed to override attribute at path '%s' with type "
                "name '%s' from weaker schema during schema prim definition "
                "composition because of the attribute type names do not match.",
                strongLayerAndPath.path.GetText(),
                strongTypeName.GetText(),
                weakLayerAndPath.path.GetText(),
                weakTypeName.GetText());
        return false;
    }
    return true;
}

void 
UsdPrimDefinition::_ComposePropertiesFromPrimDef(
    const UsdPrimDefinition &weakerPrimDef, 
    bool useWeakerPropertyForTypeConflict)
{
    _properties.reserve(_properties.size() + weakerPrimDef._properties.size());

    // Copy over property to path mappings from the weaker prim definition that 
    // aren't already in this prim definition.
    for (const auto &it : weakerPrimDef._propLayerAndPathMap) {
        // Note that the prop name may be empty as we use the empty path to
        // map to the spec containing the prim level metadata. We need to 
        // make sure we don't add the empty name to properties list if 
        // we successfully insert a metadata mapping.
        auto insertResult = _propLayerAndPathMap.insert(it);
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
                !_PropertyTypesMatch(insertResult.first->second, it.second)) {
                insertResult.first->second = it.second;
            }
        }
    }
}

void 
UsdPrimDefinition::_ComposePropertiesFromPrimDefInstance(
    const UsdPrimDefinition &weakerPrimDef, 
    const std::string &instanceName)
{
    _properties.reserve(_properties.size() + weakerPrimDef._properties.size());

    // Copy over property to path mappings from the weaker prim definition that 
    // aren't already in this prim definition.
    for (const auto &it : weakerPrimDef._propLayerAndPathMap) {
        // Apply the prefix to each property name before adding it.
        const TfToken instancedPropName = 
            UsdSchemaRegistry::MakeMultipleApplyNameInstance(
                it.first, instanceName);
        auto insertResult = _propLayerAndPathMap.emplace(
            instancedPropName, it.second);
        if (insertResult.second) {
            _properties.push_back(instancedPropName);
        }
    }
}

void
UsdPrimDefinition::_ComposeOverAndReplaceExistingProperty(
    const TfToken &propName,
    const SdfLayerRefPtr &overLayer,
    const SdfPath &overPrimPath)
{
    // Get the path to the property in the prim definition that the 
    // override property applies to. If no such property exists, we ignore
    // the override.
    UsdPrimDefinition::_LayerAndPath *defLayerAndPath = 
        _GetPropertyLayerAndPath(propName);
    if (!defLayerAndPath) {
        return;
    }

    // Property overrides are not allowed to change the type of a property
    // from its defining spec.
    _LayerAndPath overLayerAndPath{
        get_pointer(overLayer), overPrimPath.AppendProperty(propName)};
    if (!_PropertyTypesMatch(overLayerAndPath, *defLayerAndPath)) {
        return;
    }

    // Compose the defined property into the override property spec to 
    // get the property spec with the overrides applied. Any fields that
    // are defined in the override spec are stronger so we copy the defined
    // spec fields that aren't already in the override spec.
    for (const TfToken srcField : defLayerAndPath->ListMetadataFields()) {
        if (!overLayerAndPath.HasField<VtValue>(srcField, nullptr)) {
            VtValue value;
            if (defLayerAndPath->HasField(srcField, &value)) {
                overLayer->SetField(overLayerAndPath.path, srcField, value);
            }
        }
    }

    // With the override spec composed, set the definition's path for the 
    // property to the composed override spec path.
    *defLayerAndPath = std::move(overLayerAndPath);
}

bool 
UsdPrimDefinition::_ComposeWeakerAPIPrimDefinition(
    const UsdPrimDefinition &apiPrimDef,
    const TfToken &instanceName,
    _FamilyAndInstanceToVersionMap *alreadyAppliedSchemaFamilyVersions,
    bool allowDupes)
{
    // Helper for appending the given schemas to our applied schemas list while
    // checking for version conflicts. Returns true if the schemas are appended
    // which only happens if there are no conflicts.
    auto appendSchemasFn = [&](const TfTokenVector &apiSchemaNamesToAppend) {
        // We store some information so that we can undo any schemas added by
        // this function if we run into a version conflict partway through.
        const size_t startingNumAppliedSchemas = _appliedAPISchemas.size();
        std::vector<std::pair<TfToken, TfToken>> newlyAddedFamilies;

        _appliedAPISchemas.reserve(
            startingNumAppliedSchemas + apiSchemaNamesToAppend.size());
        // Append each API schema name to this definition's applied API schemas
        // list checking each for vesion conflicts with the already applied 
        // schemas. If any conflicts are found, NONE of these schemas will be
        // applied. 
        for (const TfToken &apiSchemaName : apiSchemaNamesToAppend) {
            // Applied schema names may be a single apply schema or an instance 
            // of a multiple apply schema so we have to parse the full schema 
            // name into a schema identifier and possibly an instance name.
            const std::pair<TfToken, TfToken> identifierAndInstance = 
                UsdSchemaRegistry::GetTypeNameAndInstance(apiSchemaName);

            // Use the identifier to get the schema family. The family and 
            // instance name are the key into the already applied family 
            // versions.
            const UsdSchemaRegistry::SchemaInfo *schemaInfo = 
                UsdSchemaRegistry::FindSchemaInfo(identifierAndInstance.first);
            std::pair<TfToken, TfToken> familyAndInstance(
                schemaInfo->family, identifierAndInstance.second);

            // Try to add the family and instance's version to the applied map
            // to check if we have a version conflict.
            const auto result = alreadyAppliedSchemaFamilyVersions->emplace(
                familyAndInstance, schemaInfo->version);
            if (result.second) {
                // The family and instance were not already in the map so we
                // can add the schema name to the applied list. We also store 
                // that this is a newly added family and instance so that we
                // can undo the addition if we have to.
                _appliedAPISchemas.push_back(std::move(apiSchemaName));
                newlyAddedFamilies.push_back(std::move(familyAndInstance));
            } else if (result.first->second == schemaInfo->version) {
                // The family and instance were already added but the versions
                // are the same. This is allowed. If we allow the API schema
                // name to show up in the list more than once, we add it. 
                // Otherwise we safely skip it.
                if (allowDupes) {
                    _appliedAPISchemas.push_back(std::move(apiSchemaName));
                }
            } else {
                // If we get here, the family and instance name were already 
                // added with a different version of the schema. This is a 
                // conflict and we will not add ANY of the schemas that are 
                // included by the API schema definition. Since we may have 
                // added some of the included schemas, we need to undo that
                // here before returning.
                _appliedAPISchemas.resize(startingNumAppliedSchemas);
                for (const auto &key : newlyAddedFamilies) {
                    alreadyAppliedSchemaFamilyVersions->erase(key);
                }

                if (apiSchemaNamesToAppend.front() == apiSchemaName) {
                   TF_WARN("Failure composing the API schema definition for "
                        "'%s' into another prim definition. Adding this schema "
                        "would cause a version conflict with an already "
                        "composed in API schema definition with family '%s' "
                        "and version %u.",
                    apiSchemaName.GetText(),
                    result.first->first.first.GetText(),
                    result.first->second);
                } else {
                    TF_WARN("Failure composing the API schema definition for "
                        "'%s' into another prim definition. Adding API schema "
                        "'%s', which is built in to this schema definition "
                        "would cause a version conflict with an already "
                        "composed in API schema definition with family '%s' "
                        "and version %u.",
                    apiSchemaNamesToAppend.front().GetText(),
                    apiSchemaName.GetText(),
                    result.first->first.first.GetText(),
                    result.first->second);
                }
                return false;
            }
        }

        // All schemas were successfully included.
        return true;
    };

    // Append all the API schemas included in the schema def to the 
    // prim def's API schemas list. This list will always include the 
    // schema itself followed by all other API schemas that were 
    // composed into its definition.
    const TfTokenVector &apiSchemaNamesToAppend = 
        apiPrimDef.GetAppliedAPISchemas();

    if (instanceName.IsEmpty()) {
        if (!appendSchemasFn(apiSchemaNamesToAppend)) {
            return false;
        }
        _ComposePropertiesFromPrimDef(apiPrimDef);
    } else {
        // If an instance name is provided, the API schema definition is a 
        // multiple apply template that needs the instance name applied to it
        // and all the other multiple apply schema templates it may include.
        TfTokenVector instancedAPISchemaNames;
        instancedAPISchemaNames.reserve(apiSchemaNamesToAppend.size());
        for (const TfToken &apiSchemaName : apiSchemaNamesToAppend) {
            instancedAPISchemaNames.push_back(
                UsdSchemaRegistry::MakeMultipleApplyNameInstance(
                    apiSchemaName, instanceName));
        }
        if (!appendSchemasFn(instancedAPISchemaNames)) {
            return false;
        }
        _ComposePropertiesFromPrimDefInstance(apiPrimDef, instanceName);
    }

    return true;
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

