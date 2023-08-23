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

UsdPrimDefinition::Property 
UsdPrimDefinition::GetPropertyDefinition(const TfToken& propName) const 
{
    // For Typed schemas, the empty property is mapped to the prim path to 
    // access prim metadata for the schema. We make sure that this can't be
    // accessed via the public GetPropertyDefinition since we only want this 
    // returning true properties.
    if (ARCH_UNLIKELY(propName.IsEmpty())) {
        return Property();
    }
    return Property(propName, _GetPropertyLayerAndPath(propName));
}

UsdPrimDefinition::Attribute 
UsdPrimDefinition::GetAttributeDefinition(const TfToken& attrName) const 
{
    return GetPropertyDefinition(attrName);
}

UsdPrimDefinition::Relationship 
UsdPrimDefinition::GetRelationshipDefinition(const TfToken& relName) const 
{
    return GetPropertyDefinition(relName);
}

SdfSpecType UsdPrimDefinition::GetSpecType(const TfToken &propName) const
{
    if (const Property prop = GetPropertyDefinition(propName)) {
        return prop.GetSpecType();
    }
    return SdfSpecTypeUnknown;
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
    return Property(&_primLayerAndPath).GetDocumentation();
}

TfTokenVector 
UsdPrimDefinition::ListPropertyMetadataFields(const TfToken &propName) const 
{
    if (const Property prop = GetPropertyDefinition(propName)) {
        return prop.ListMetadataFields();
    }
    return TfTokenVector();
}

std::string 
UsdPrimDefinition::GetPropertyDocumentation(const TfToken &propName) const 
{
    if (const Property prop = GetPropertyDefinition(propName)) {
        return prop.GetDocumentation();
    }
    return std::string();
}

TfTokenVector 
UsdPrimDefinition::ListMetadataFields() const
{
    // Prim metadata for Typed schema definitions is stored specially as an
    // empty named property which will not be returned by GetPropertyDefinition.
    // But we can still access it 
    if (const _LayerAndPath *layerAndPath = _GetPropertyLayerAndPath(TfToken())) {
        return Property(layerAndPath).ListMetadataFields();
    }
    return TfTokenVector();
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
    const Property &strongProp,
    const Property &weakProp)
{
    if (!TF_VERIFY(strongProp && weakProp)) {
        return false;
    }

    if (strongProp.IsRelationship()) {
        // Compare spec types (relationship vs attribute)
        if (!weakProp.IsRelationship()) {
            TF_WARN("Cannot compose schema specs: Schema relationship spec at "
                    "path '%s' in layer '%s' is a different spec type than "
                    "schema attribute spec at path '%s' in layer '%s'.",
                    strongProp._layerAndPath->path.GetText(),
                    strongProp._layerAndPath->layer->GetIdentifier().c_str(),
                    weakProp._layerAndPath->path.GetText(),
                    weakProp._layerAndPath->layer->GetIdentifier().c_str());
            return false;
        }
        return true;
    }

    Attribute strongAttr(strongProp);
    if (!TF_VERIFY(strongAttr)) {
        return false;
    }

    Attribute weakAttr(weakProp);
    if (!weakAttr) {
            TF_WARN("Cannot compose schema specs: Schema attribute spec at "
                    "path '%s' in layer '%s' is a different spec type than "
                    "schema relationship spec at path '%s' in layer '%s'.",
                    strongProp._layerAndPath->path.GetText(),
                    strongProp._layerAndPath->layer->GetIdentifier().c_str(),
                    weakProp._layerAndPath->path.GetText(),
                    weakProp._layerAndPath->layer->GetIdentifier().c_str());
        return false;
    }

    // Compare the type name field of the attributes.
    const TfToken strongTypeName = strongAttr.GetTypeNameToken();
    const TfToken weakTypeName = weakAttr.GetTypeNameToken();
    if (weakTypeName != strongTypeName) {
        TF_WARN("Cannot compose schema attribute specs: Mismatched type names."
                "Schema attribute spec at path '%s' in layer '%s' has type "
                "name '%s' while schema attribute spec at path '%s' in layer "
                "'%s' has type name '%s'.",
                strongProp._layerAndPath->path.GetText(),
                strongProp._layerAndPath->layer->GetIdentifier().c_str(),
                strongTypeName.GetText(),
                weakProp._layerAndPath->path.GetText(),
                weakProp._layerAndPath->layer->GetIdentifier().c_str(),
                weakTypeName.GetText());
        return false;
    }
    return true;
}

void 
UsdPrimDefinition::_ComposePropertiesFromPrimDef(
    const UsdPrimDefinition &weakerPrimDef)
{
    _properties.reserve(_properties.size() + weakerPrimDef._properties.size());

    // Copy over property to path mappings from the weaker prim definition,
    // possibly creating composed definitions for properties that already exist.
    for (const auto &it : weakerPrimDef._propLayerAndPathMap) {
        _AddOrComposeProperty(it.first, it.second);
    }
}

void 
UsdPrimDefinition::_ComposePropertiesFromPrimDefInstance(
    const UsdPrimDefinition &weakerPrimDef, 
    const std::string &instanceName)
{
    _properties.reserve(_properties.size() + weakerPrimDef._properties.size());

    // Copy over property to path mappings from the weaker prim definition,
    // possibly creating composed definitions for properties that already exist.
    for (const auto &it : weakerPrimDef._propLayerAndPathMap) {
        // Apply the prefix to each property name before adding it.
        const TfToken instancedPropName = 
            UsdSchemaRegistry::MakeMultipleApplyNameInstance(
                it.first, instanceName);
        _AddOrComposeProperty(instancedPropName, it.second);
    }
}

void UsdPrimDefinition::_AddOrComposeProperty(
    const TfToken &propName,
    const _LayerAndPath &layerAndPath)
{
    // Note that the prop name may be empty as we use the empty path to
    // map to the spec containing the prim level metadata. We need to 
    // make sure we don't add the empty name to properties list if 
    // we successfully insert a metadata mapping.
    auto insertResult = _propLayerAndPathMap.emplace(propName, layerAndPath);
    if (insertResult.second){
        if (!propName.IsEmpty()) {
            _properties.push_back(propName);
        }
    } else {
        // The property exists already. Some fields may be able to be 
        // composed in from the new weaker property definition so we try to
        // do that here.
        _LayerAndPath &existingProp = insertResult.first->second;
        if (SdfPropertySpecHandle composedPropSpec = 
                _CreateComposedPropertyIfNeeded(
                    propName, existingProp, layerAndPath)) {
            // If a composed property was created, replace the existing
            // property definition. Otherwise, we just leave the existing 
            // property as is.
            existingProp = {
                get_pointer(composedPropSpec->GetLayer()), 
                composedPropSpec->GetPath()};
        }
    }
}

// We limit which fields are allowed to be composed in from a property defined
// in a weaker prim definition when a prim definition already has a property
// with the same name. 
static 
const TfTokenVector &_GetAllowedComposeFromWeakerPropertyFields()
{
    // Right now we only allow the "default" value (of attributes) and the 
    // "hidden" field to be composed from a weaker property. We may selectively
    // expand this set of fields if it becomes necessary.
    static const TfTokenVector fields = {
        SdfFieldKeys->Default, 
        SdfFieldKeys->Hidden
    };
    return fields;
}

SdfPropertySpecHandle 
UsdPrimDefinition::_FindOrCreatePropertySpecForComposition(
    const TfToken &propName,
    const _LayerAndPath &srcLayerAndPath)
{
    // Arbitrary prim path for this definition's composed property specs. Only
    // this prim definition will use the layer we find or create here so we 
    // don't need unique prim spec names/paths.
    static const SdfPath primPath("/ComposedProperties");

    SdfPropertySpecHandle destProp;

    // If we have a composed layer, we can check if we've already created
    // a spec for the composed property and return it if we have. Otherwise, 
    // we create a new layer for this prim definition to write its composed
    // properties.
    if (_composedPropertyLayer) {
        if (destProp = _composedPropertyLayer->GetPropertyAtPath(
                primPath.AppendProperty(propName))) {
            return destProp;
        }
    } else {
        _composedPropertyLayer = SdfLayer::CreateAnonymous(
            "schema-composed-properties");
    }

    SdfChangeBlock block;

    // Find or create the prim spec that will hold the composed property specs.
    SdfPrimSpecHandle destPrim = _composedPropertyLayer->GetPrimAtPath(primPath);
    if (!destPrim) {
        destPrim = SdfPrimSpec::New(
            _composedPropertyLayer, primPath.GetName(), SdfSpecifierDef);
    }

    // Create a copy of the source attribute or relationship spec. We do this 
    // manually as the copy utils for Sdf specs are more generalized than what 
    // we need here.
    const Property srcProp(&srcLayerAndPath);
    if (srcProp.IsAttribute()) {
        const Attribute srcAttr(srcProp);
        destProp = SdfAttributeSpec::New(
            destPrim, 
            propName,
            srcAttr.GetTypeName(), 
            srcAttr.GetVariability());
    } else if (srcProp.IsRelationship()) {
        destProp = SdfRelationshipSpec::New(
            destPrim, 
            propName, 
            srcProp.GetVariability());
    } else {
        TF_CODING_ERROR("Cannot create a property spec from spec at layer "
            "'%s' and path '%s'. The spec type is not an attribute or "
            "relationship.",
            srcLayerAndPath.layer->GetIdentifier().c_str(),
            srcLayerAndPath.path.GetText());
        return destProp;
    }

    // Copy all the metadata fields from the source spec to the new spec.
    for (const TfToken &field : srcProp.ListMetadataFields()) {
        VtValue value;
        srcLayerAndPath.HasField(field, &value);
        destProp->SetField(field, value);
    }

    return destProp;
}

SdfPropertySpecHandle 
UsdPrimDefinition::_CreateComposedPropertyIfNeeded(
    const TfToken &propName,
    const _LayerAndPath &strongProp, 
    const _LayerAndPath &weakProp)
{
    SdfPropertySpecHandle destProp;

    // If the property types don't match, then we can't compose the properties
    // together.
    if (!_PropertyTypesMatch(Property(&strongProp), Property(&weakProp))) {
        return destProp;
    }

    for (const TfToken &field : _GetAllowedComposeFromWeakerPropertyFields()) {
        // If the stronger property already has the field, skip it.
        if (strongProp.HasField<VtValue>(field, nullptr)) {
            continue;
        }

        // Get the field's value from the weaker property. If it doesn't have
        // the field, we skip it too.
        VtValue weakValue;
        if (!weakProp.HasField(field, &weakValue)) {
            continue;
        }
 
        // If we get here we need to compose a property definition so create a
        // a copy of the stronger property if we haven't already and add the
        // field.
        if (!destProp) {
            destProp = _FindOrCreatePropertySpecForComposition(
                propName, strongProp);
        }
        destProp->SetField(field, weakValue);
    }

    return destProp;
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

    const Property overProp(&overLayerAndPath);
    const Property defProp(defLayerAndPath);
    if (!_PropertyTypesMatch(overProp, defProp)) {
        return;
    }

    // Compose the defined property into the override property spec to 
    // get the property spec with the overrides applied. Any fields that
    // are defined in the override spec are stronger so we copy the defined
    // spec fields that aren't already in the override spec.
    for (const TfToken srcField : defProp.ListMetadataFields()) {
        if (!overLayerAndPath.HasField<VtValue>(srcField, nullptr)) {
            VtValue value;
            if (defLayerAndPath->HasField(srcField, &value)) {
                overLayer->SetField(overLayerAndPath.path, srcField, value);
            }
        }
    }

    // There's one exception to override fields being stonger; an override
    // cannot change the defined property's variability. So we may have to 
    // set the variability to match the defined property.
    SdfVariability variability = defProp.GetVariability();
    if (overProp.GetVariability() != variability) {
        overLayer->SetField(
            overLayerAndPath.path, SdfFieldKeys->Variability, variability);
    }

    // With the override spec composed, set the definition's path for the 
    // property to the composed override spec path.
    *defLayerAndPath = std::move(overLayerAndPath);
}

bool 
UsdPrimDefinition::_ComposeWeakerAPIPrimDefinition(
    const UsdPrimDefinition &apiPrimDef,
    const TfToken &instanceName,
    _FamilyAndInstanceToVersionMap *alreadyAppliedSchemaFamilyVersions)
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
            } else if (result.first->second != schemaInfo->version) {
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
        const _LayerAndPath *layerAndPath = _GetPropertyLayerAndPath(propName);

        if (TF_VERIFY(layerAndPath)) {
            // SdfCopySpec requires the source layer to be a non-const handle
            // even though it shouldn't be editing the source layer. Thus we
            // have to const cast here.
            const SdfLayerHandle srcLayer(
                const_cast<SdfLayer *>(layerAndPath->layer));
            if (!SdfCopySpec(srcLayer, layerAndPath->path,
                             layer, path.AppendProperty(propName))) {
                TF_WARN("Failed to copy prim definition property '%s' to prim "
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

const TfToken &
UsdPrimDefinition::Property::GetName() const
{
    return _name;
}

bool 
UsdPrimDefinition::Property::IsAttribute() const
{
    return _layerAndPath && GetSpecType() == SdfSpecTypeAttribute;
}

bool 
UsdPrimDefinition::Property::IsRelationship() const
{
    return _layerAndPath && GetSpecType() == SdfSpecTypeRelationship;
}

SdfSpecType 
UsdPrimDefinition::Property::GetSpecType() const {
    return _layerAndPath->layer->GetSpecType(_layerAndPath->path);
}

TfTokenVector 
UsdPrimDefinition::Property::ListMetadataFields() const 
{
    // Get the list of fields from the schematics for the property (or prim)
    // path and remove the fields that we don't allow fallbacks for.
    TfTokenVector fields = _layerAndPath->layer->ListFields(_layerAndPath->path);
    fields.erase(
        std::remove_if(fields.begin(), fields.end(), 
                       &UsdSchemaRegistry::IsDisallowedField), 
        fields.end());
    return fields;
}

SdfVariability 
UsdPrimDefinition::Property::GetVariability() const {
    SdfVariability variability;
    _layerAndPath->HasField(SdfFieldKeys->Variability, &variability);
    return variability;
}

std::string 
UsdPrimDefinition::Property::GetDocumentation() const {
    std::string docString;
    _layerAndPath->HasField(SdfFieldKeys->Documentation, &docString);
    return docString;
}

UsdPrimDefinition::Attribute::Attribute(const Property &property) 
    : Property(property) 
{
}

UsdPrimDefinition::Attribute::Attribute(Property &&property) 
    : Property(std::move(property))
{
}

SdfValueTypeName 
UsdPrimDefinition::Attribute::GetTypeName() const 
{
    return SdfSchema::GetInstance().FindType(GetTypeNameToken());
}


TfToken 
UsdPrimDefinition::Attribute::GetTypeNameToken() const 
{
    TfToken typeName;
    _layerAndPath->HasField(SdfFieldKeys->TypeName, &typeName);
    return typeName;
}

UsdPrimDefinition::Relationship::Relationship(const Property &property) 
    : Property(property) 
{
}

UsdPrimDefinition::Relationship::Relationship(Property &&property) 
    : Property(std::move(property))
{
}

PXR_NAMESPACE_CLOSE_SCOPE

