//
// Copyright 2018 Pixar
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
#include "usdMaya/adaptor.h"

#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/readUtil.h"
#include "usdMaya/util.h"
#include "usdMaya/writeUtil.h"

#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/tokens.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPlug.h>
#include <maya/MStringArray.h>

PXR_NAMESPACE_OPEN_SCOPE

static std::string
_GetMayaAttrNameForMetadataKey(const TfToken& key)
{
    return TfStringPrintf("USD_%s",
            TfMakeValidIdentifier(key.GetString()).c_str());
}

static std::string
_GetMayaAttrNameForAttrName(const TfToken& attrName)
{
    return TfStringPrintf("USD_ATTR_%s",
            TfMakeValidIdentifier(attrName.GetString()).c_str());
}

static VtValue
_GetListOpForTokenVector(const TfTokenVector& vector)
{
    SdfTokenListOp op;
    op.SetPrependedItems(vector);
    return VtValue(op);
}



std::map<std::string, TfType> UsdMayaAdaptor::_schemaLookup;
std::map<TfToken, std::vector<std::string>>
    UsdMayaAdaptor::_attributeAliases;

UsdMayaAdaptor::UsdMayaAdaptor(const MObject& obj) : _handle(obj)
{
}

UsdMayaAdaptor::operator bool() const
{
    if (!_handle.isValid()) {
        return false;
    }

    MStatus status;
    MFnDependencyNode node(_handle.object(), &status);
    return status;
}

std::string
UsdMayaAdaptor::GetMayaNodeName() const
{
    if (!*this) {
        return std::string();
    }

    if (_handle.object().hasFn(MFn::kDagNode)) {
        MFnDagNode dagNode(_handle.object());
        return dagNode.fullPathName().asChar();
    }
    else {
        MFnDependencyNode depNode(_handle.object());
        return depNode.name().asChar();
    }
}

TfToken
UsdMayaAdaptor::GetUsdTypeName() const
{
    if (!*this) {
        return TfToken();
    }

    const TfType ty = GetUsdType();
    const SdfPrimSpecHandle primDef = UsdSchemaRegistry::GetInstance()
            .GetPrimDefinition(ty);
    if (!primDef) {
        return TfToken();
    }

    return primDef->GetNameToken();
}

TfType
UsdMayaAdaptor::GetUsdType() const
{
    if (!*this) {
        return TfType();
    }

    MObject object = _handle.object();
    MFnDependencyNode depNode(object);

    // The adaptor type mapping might be registered externally in a prim writer
    // plugin. This simply pokes the prim writer registry to load the prim
    // writer plugin in order to pull in the adaptor mapping.
    UsdMayaPrimWriterRegistry::Find(depNode.typeName().asChar());
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaAdaptor>();

    const auto iter = _schemaLookup.find(depNode.typeName().asChar());
    if (iter != _schemaLookup.end()) {
        return iter->second;
    }
    else {
        return TfType();
    }
}

TfTokenVector
UsdMayaAdaptor::GetAppliedSchemas() const
{
    if (!*this) {
        return TfTokenVector();
    }

    VtValue appliedSchemas;
    if (GetMetadata(UsdTokens->apiSchemas, &appliedSchemas)) {
        TfTokenVector result;
        appliedSchemas.Get<SdfTokenListOp>().ApplyOperations(&result);
        return result;
    }

    return TfTokenVector();
}

UsdMayaAdaptor::SchemaAdaptor
UsdMayaAdaptor::GetSchema(const TfType& ty) const
{
    const SdfPrimSpecHandle primDef = UsdSchemaRegistry::GetInstance()
            .GetPrimDefinition(ty);
    if (!primDef) {
        return SchemaAdaptor();
    }

    return GetSchemaByName(primDef->GetNameToken());
}

UsdMayaAdaptor::SchemaAdaptor
UsdMayaAdaptor::GetSchemaByName(const TfToken& schemaName) const
{
    if (!*this) {
        return SchemaAdaptor();
    }

    // Get the schema definition. If it's registered, there should be a def.
    SdfPrimSpecHandle primDef =
            UsdSchemaRegistry::GetInstance().GetPrimDefinition(schemaName);
    if (!primDef) {
        return SchemaAdaptor();
    }

    // Get the schema's TfType; its name should be registered as an alias.
    const TfType schemaType =
            TfType::Find<UsdSchemaBase>().FindDerivedByName(schemaName);

    // Is this an API schema?
    if (schemaType.IsA<UsdAPISchemaBase>()) {
        return SchemaAdaptor(_handle.object(), primDef);
    }
    // Is this a typed schema?
    else if (schemaType.IsA<UsdSchemaBase>()) {
        // XXX
        // We currently require an exact type match instead of the polymorphic
        // behavior that actual USD schema classes implement. This is because
        // we can't currently get the prim definition from the schema registry
        // for non-concrete schemas like Imageable (see bug 160436). Ideally, 
        // once that's resolved, we would cache a mapping of Maya types to all
        // compatible USD type names based on schema inheritance.
        // (In that future world, we'll also want to special case some schemas
        // like UsdGeomImageable to be "compatible" with all Maya nodes.)
        const TfToken objectTypeName = GetUsdTypeName();
        if (schemaName == objectTypeName) {
            // There's an exact MFn::Type match? Easy-peasy.
            return SchemaAdaptor(_handle.object(), primDef);
        }
        else {
            // If no match, do not allow usage of the typed-schema adaptor
            // mechanism. The importer/exporter have not declared that they
            // will use the adaptor mechanism to handle this type.
            return SchemaAdaptor();
        }
    }

    // We shouldn't be able to reach this (everything is either typed or API).
    TF_CODING_ERROR("'%s' isn't a known API or typed schema",
            schemaName.GetText());
    return SchemaAdaptor();
}

UsdMayaAdaptor::SchemaAdaptor
UsdMayaAdaptor::GetSchemaOrInheritedSchema(const TfType& ty) const
{
    if (!*this) {
        return SchemaAdaptor();
    }

    if (ty.IsA<UsdAPISchemaBase>()) {
        // No "promotion" for API schemas.
        return GetSchema(ty);
    }
    else if (ty.IsA<UsdSchemaBase>()) {
        // Can "promote" typed schemas based on inheritance.
        const TfType objectType = GetUsdType();
        if (objectType.IsA(ty)) {
            return GetSchema(objectType);
        }
    }

    return SchemaAdaptor();
}

UsdMayaAdaptor::SchemaAdaptor
UsdMayaAdaptor::ApplySchema(const TfType& ty)
{
    MDGModifier modifier;
    return ApplySchema(ty, modifier);
}

UsdMayaAdaptor::SchemaAdaptor
UsdMayaAdaptor::ApplySchema(const TfType& ty, MDGModifier& modifier)
{
    const SdfPrimSpecHandle primDef = UsdSchemaRegistry::GetInstance()
            .GetPrimDefinition(ty);
    if (!primDef) {
        TF_CODING_ERROR("Can't find schema definition for type '%s'",
                ty.GetTypeName().c_str());
        return SchemaAdaptor();
    }

    return ApplySchemaByName(primDef->GetNameToken(), modifier);
}

UsdMayaAdaptor::SchemaAdaptor
UsdMayaAdaptor::ApplySchemaByName(const TfToken& schemaName)
{
    MDGModifier modifier;
    return ApplySchemaByName(schemaName, modifier);
}

UsdMayaAdaptor::SchemaAdaptor
UsdMayaAdaptor::ApplySchemaByName(
    const TfToken& schemaName,
    MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Adaptor is not valid");
        return SchemaAdaptor();
    }

    // Get the schema's TfType; its name should be registered as an alias.
    const TfType schemaType =
            TfType::Find<UsdSchemaBase>().FindDerivedByName(schemaName);

    // Make sure that this is an API schema. Only API schemas can be applied.
    if (!schemaType.IsA<UsdAPISchemaBase>()) {
        TF_CODING_ERROR("'%s' is not a registered API schema",
                schemaName.GetText());
        return SchemaAdaptor();
    }

    // Make sure that this is an "apply" schema.
    if (!UsdSchemaRegistry::GetInstance().IsAppliedAPISchema(schemaType)) {
        TF_CODING_ERROR("'%s' is not an applied API schema",
                schemaName.GetText());
        return SchemaAdaptor();
    }

    // Get the schema definition. If it's registered, there should be a def.
    SdfPrimSpecHandle primDef =
            UsdSchemaRegistry::GetInstance().GetPrimDefinition(schemaName);
    if (!primDef) {
        TF_CODING_ERROR("Can't find schema definition for name '%s'",
                schemaName.GetText());
        return SchemaAdaptor();
    }

    // Add to schema list (if not yet present).
    TfTokenVector currentSchemas = GetAppliedSchemas();
    if (std::find(currentSchemas.begin(), currentSchemas.end(), schemaName) ==
            currentSchemas.end()) {
        currentSchemas.push_back(schemaName);
        SetMetadata(
                UsdTokens->apiSchemas,
                _GetListOpForTokenVector(currentSchemas),
                modifier);
    }

    return SchemaAdaptor(_handle.object(), primDef);
}

void
UsdMayaAdaptor::UnapplySchema(const TfType& ty)
{
    MDGModifier modifier;
    UnapplySchema(ty, modifier);
}

void
UsdMayaAdaptor::UnapplySchema(const TfType& ty, MDGModifier& modifier)
{
    const SdfPrimSpecHandle primDef = UsdSchemaRegistry::GetInstance()
            .GetPrimDefinition(ty);
    if (!primDef) {
        TF_CODING_ERROR("Can't find schema definition for type '%s'",
                ty.GetTypeName().c_str());
        return;
    }

    UnapplySchemaByName(primDef->GetNameToken(), modifier);
}

void
UsdMayaAdaptor::UnapplySchemaByName(const TfToken& schemaName)
{
    MDGModifier modifier;
    UnapplySchemaByName(schemaName, modifier);
}

void
UsdMayaAdaptor::UnapplySchemaByName(
    const TfToken& schemaName,
    MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Adaptor is not valid");
        return;
    }

    // Remove from schema list.
    TfTokenVector currentSchemas = GetAppliedSchemas();
    currentSchemas.erase(
            std::remove(
                currentSchemas.begin(), currentSchemas.end(), schemaName),
            currentSchemas.end());
    if (currentSchemas.empty()) {
        ClearMetadata(UsdTokens->apiSchemas, modifier);
    }
    else {
        SetMetadata(
                UsdTokens->apiSchemas,
                _GetListOpForTokenVector(currentSchemas),
                modifier);
    }
}

static bool
_GetMetadataUnchecked(
    const MFnDependencyNode& node,
    const TfToken& key,
    VtValue* value)
{
    VtValue fallback = SdfSchema::GetInstance().GetFallback(key);
    if (fallback.IsEmpty()) {
        return false;
    }

    std::string mayaAttrName = _GetMayaAttrNameForMetadataKey(key);
    MPlug plug = node.findPlug(mayaAttrName.c_str());
    if (plug.isNull()) {
        return false;
    }

    TfType ty = fallback.GetType();
    VtValue result = UsdMayaWriteUtil::GetVtValue(plug, ty, TfToken());
    if (result.IsEmpty()) {
        TF_RUNTIME_ERROR(
                "Cannot convert plug '%s' into metadata '%s' (%s)",
                plug.name().asChar(),
                key.GetText(),
                ty.GetTypeName().c_str());
        return false;
    }

    *value = result;
    return true;
}

UsdMetadataValueMap
UsdMayaAdaptor::GetAllAuthoredMetadata() const
{
    if (!*this) {
        return UsdMetadataValueMap();
    }

    MFnDependencyNode node(_handle.object());
    UsdMetadataValueMap metaMap;
    for (const TfToken& key : GetPrimMetadataFields()) {
        VtValue value;
        if (_GetMetadataUnchecked(node, key, &value)) {
            metaMap[key] = value;
        }
    }

    return metaMap;
}

bool
UsdMayaAdaptor::GetMetadata(const TfToken& key, VtValue* value) const
{
    if (!*this) {
        return false;
    }

    if (!SdfSchema::GetInstance().IsRegistered(key)) {
        TF_CODING_ERROR("Metadata key '%s' is not registered", key.GetText());
        return false;
    }

    MFnDependencyNode node(_handle.object());
    return _GetMetadataUnchecked(node, key, value);
}

bool
UsdMayaAdaptor::SetMetadata(const TfToken& key, const VtValue& value)
{
    MDGModifier modifier;
    return SetMetadata(key, value, modifier);
}

bool
UsdMayaAdaptor::SetMetadata(
    const TfToken& key,
    const VtValue& value,
    MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Adaptor is not valid");
        return false;
    }

    VtValue fallback;
    if (!SdfSchema::GetInstance().IsRegistered(key, &fallback)) {
        TF_CODING_ERROR("Metadata key '%s' is not registered", key.GetText());
        return false;
    }

    if (fallback.IsEmpty()) {
        return false;
    }

    VtValue castValue = VtValue::CastToTypeOf(value, fallback);
    if (castValue.IsEmpty()) {
        TF_CODING_ERROR("Can't cast value to type '%s'",
                fallback.GetTypeName().c_str());
        return false;
    }

    std::string mayaAttrName = _GetMayaAttrNameForMetadataKey(key);
    std::string mayaNiceAttrName = key.GetText();
    MFnDependencyNode node(_handle.object());
    TfType ty = fallback.GetType();
    MObject attrObj = UsdMayaReadUtil::FindOrCreateMayaAttr(
            ty, TfToken(), SdfVariabilityUniform,
            node, mayaAttrName, mayaNiceAttrName, modifier);
    if (attrObj.isNull()) {
        return false;
    }

    MPlug plug = node.findPlug(attrObj);
    if (!UsdMayaReadUtil::SetMayaAttr(plug, castValue, modifier)) {
        return false;
    }

    return true;
}

void
UsdMayaAdaptor::ClearMetadata(const TfToken& key)
{
    MDGModifier modifier;
    ClearMetadata(key, modifier);
}

void
UsdMayaAdaptor::ClearMetadata(const TfToken& key, MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Adaptor is not valid");
        return;
    }

    MFnDependencyNode node(_handle.object());
    std::string mayaAttrName = _GetMayaAttrNameForMetadataKey(key);
    if (node.hasAttribute(mayaAttrName.c_str())) {
        MObject attr = node.attribute(mayaAttrName.c_str());
        modifier.removeAttribute(_handle.object(), attr);
        modifier.doIt();
    }
}

/* static */
TfTokenVector
UsdMayaAdaptor::GetPrimMetadataFields()
{
    return SdfSchema::GetInstance().GetMetadataFields(SdfSpecTypePrim);
}

template <typename T>
static TfToken::Set _GetRegisteredSchemas()
{
    TfToken::Set schemas;
    std::set<TfType> derivedTypes;
    TfType::Find<T>().GetAllDerivedTypes(&derivedTypes);

    UsdSchemaRegistry registry = UsdSchemaRegistry::GetInstance();
    for (const TfType& ty : derivedTypes) {
        SdfPrimSpecHandle primDef = registry.GetPrimDefinition(ty);
        if (!primDef) {
            continue;
        }

        schemas.insert(primDef->GetNameToken());
    }

    return schemas;
}

/* static */
TfToken::Set
UsdMayaAdaptor::GetRegisteredAPISchemas()
{
    return _GetRegisteredSchemas<UsdAPISchemaBase>();
}

/* static */
TfToken::Set
UsdMayaAdaptor::GetRegisteredTypedSchemas()
{
    return _GetRegisteredSchemas<UsdSchemaBase>();
}

/* static */
void
UsdMayaAdaptor::RegisterTypedSchemaConversion(
    const std::string& nodeTypeName,
    const TfType& usdType)
{
    _schemaLookup[nodeTypeName] = usdType;
}

/* static */
void
UsdMayaAdaptor::RegisterAttributeAlias(
    const TfToken& attributeName,
    const std::string& alias)
{
    _attributeAliases[attributeName].push_back(alias);
}

/* static */
std::vector<std::string>
UsdMayaAdaptor::GetAttributeAliases(const TfToken& attributeName)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaAdaptor>();

    std::vector<std::string> result;
    result.push_back(_GetMayaAttrNameForAttrName(attributeName));

    auto iter = _attributeAliases.find(attributeName);
    if (iter != _attributeAliases.end()) {
        const std::vector<std::string>& aliases = iter->second;
        result.insert(result.end(), aliases.begin(), aliases.end());
    }

    return result;
}



UsdMayaAdaptor::SchemaAdaptor::SchemaAdaptor()
    : _handle(), _schemaDef(nullptr)
{
}

UsdMayaAdaptor::SchemaAdaptor::SchemaAdaptor(
    const MObjectHandle& handle, SdfPrimSpecHandle schemaDef)
    : _handle(handle), _schemaDef(schemaDef)
{
}

UsdMayaAdaptor::SchemaAdaptor::operator bool() const
{
    if (!_handle.isValid() || !_schemaDef) {
        return false;
    }

    MStatus status;
    MFnDependencyNode node(_handle.object(), &status);
    return status;
}

std::string
UsdMayaAdaptor::SchemaAdaptor::_GetMayaAttrNameOrAlias(
    const SdfAttributeSpecHandle& attrSpec) const
{
    if (!*this) {
        TF_CODING_ERROR("Schema adaptor is not valid");
        return std::string();
    }

    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaAdaptor>();

    const MObject thisObject = _handle.object();
    MFnDependencyNode depNode(thisObject);

    // If the generated name exists, it is the most preferred name,
    const TfToken name = attrSpec->GetNameToken();
    const std::string genName = _GetMayaAttrNameForAttrName(name);
    if (depNode.hasAttribute(genName.c_str())) {
        return genName;
    }

    // Otherwise, search for any aliases that may already exist.
    auto iter = _attributeAliases.find(name);
    if (iter != _attributeAliases.end()) {
        const std::vector<std::string>& aliases = iter->second;
        for (const std::string& alias : aliases) {
            if (depNode.hasAttribute(alias.c_str())) {
                return alias;
            }
        }
    }

    // No attribute exists for this USD attribute. When creating, always use
    // the generated name.
    return genName;
}

UsdMayaAdaptor
UsdMayaAdaptor::SchemaAdaptor::GetNodeAdaptor() const
{
    if (!*this) {
        return UsdMayaAdaptor(MObject::kNullObj);
    }

    return UsdMayaAdaptor(_handle.object());
}

TfToken
UsdMayaAdaptor::SchemaAdaptor::GetName() const
{
    if (!*this) {
        return TfToken();
    }

    return _schemaDef->GetNameToken();
}

UsdMayaAdaptor::AttributeAdaptor
UsdMayaAdaptor::SchemaAdaptor::GetAttribute(const TfToken& attrName) const
{
    if (!*this) {
        return AttributeAdaptor();
    }

    SdfAttributeSpecHandle attrDef = _schemaDef->GetAttributes()[attrName];
    if (!attrDef) {
        TF_CODING_ERROR("Attribute '%s' doesn't exist on schema '%s'",
                attrName.GetText(), _schemaDef->GetName().c_str());
        return AttributeAdaptor();
    }

    std::string mayaAttrName = _GetMayaAttrNameOrAlias(attrDef);
    MFnDependencyNode node(_handle.object());
    MPlug plug = node.findPlug(mayaAttrName.c_str());
    if (plug.isNull()) {
        return AttributeAdaptor();
    }

    return AttributeAdaptor(plug, attrDef);
}

UsdMayaAdaptor::AttributeAdaptor
UsdMayaAdaptor::SchemaAdaptor::CreateAttribute(const TfToken& attrName)
{
    MDGModifier modifier;
    return CreateAttribute(attrName, modifier);
}

UsdMayaAdaptor::AttributeAdaptor
UsdMayaAdaptor::SchemaAdaptor::CreateAttribute(
    const TfToken& attrName, MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Schema adaptor is not valid");
        return AttributeAdaptor();
    }

    SdfAttributeSpecHandle attrDef = _schemaDef->GetAttributes()[attrName];
    if (!attrDef) {
        TF_CODING_ERROR("Attribute '%s' doesn't exist on schema '%s'",
                attrName.GetText(), _schemaDef->GetName().c_str());
        return AttributeAdaptor();
    }

    std::string mayaAttrName = _GetMayaAttrNameOrAlias(attrDef);
    std::string mayaNiceAttrName = attrDef->GetName();
    MFnDependencyNode node(_handle.object());

    bool newAttr = !node.hasAttribute(mayaAttrName.c_str());
    MObject attrObj = UsdMayaReadUtil::FindOrCreateMayaAttr(
            attrDef->GetTypeName(), attrDef->GetVariability(),
            node, mayaAttrName, mayaNiceAttrName, modifier);
    if (attrObj.isNull()) {
        return AttributeAdaptor();
    }

    MPlug plug = node.findPlug(attrObj);
    if (newAttr && attrDef->HasDefaultValue()) {
        // Set the fallback value as the initial value of the attribute, if
        // it exists. (There's not much point in setting the "default" value in
        // Maya, because it won't behave like the fallback value in USD.)
        UsdMayaReadUtil::SetMayaAttr(
                plug, attrDef->GetDefaultValue(), modifier);
    }

    return AttributeAdaptor(plug, attrDef);
}

void
UsdMayaAdaptor::SchemaAdaptor::RemoveAttribute(const TfToken& attrName)
{
    MDGModifier modifier;
    RemoveAttribute(attrName, modifier);
}

void
UsdMayaAdaptor::SchemaAdaptor::RemoveAttribute(
    const TfToken& attrName, MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Schema adaptor is not valid");
        return;
    }

    SdfAttributeSpecHandle attrDef = _schemaDef->GetAttributes()[attrName];
    if (!attrDef) {
        TF_CODING_ERROR("Attribute '%s' doesn't exist on schema '%s'",
                attrName.GetText(), _schemaDef->GetName().c_str());
        return;
    }

    std::string mayaAttrName = _GetMayaAttrNameOrAlias(attrDef);
    MFnDependencyNode node(_handle.object());
    if (node.hasAttribute(mayaAttrName.c_str())) {
        MObject attr = node.attribute(mayaAttrName.c_str());
        modifier.removeAttribute(_handle.object(), attr);
        modifier.doIt();
    }
}

TfTokenVector
UsdMayaAdaptor::SchemaAdaptor::GetAuthoredAttributeNames() const
{
    if (!*this) {
        return TfTokenVector();
    }

    MFnDependencyNode node(_handle.object());
    TfTokenVector result;
    for (const SdfAttributeSpecHandle& attr : _schemaDef->GetAttributes()) {
        std::string mayaAttrName = _GetMayaAttrNameOrAlias(attr);
        if (node.hasAttribute(mayaAttrName.c_str())) {
            result.push_back(attr->GetNameToken());
        }
    }

    return result;
}

TfTokenVector
UsdMayaAdaptor::SchemaAdaptor::GetAttributeNames() const
{
    if (!*this) {
        return TfTokenVector();
    }

    TfTokenVector attrNames;
    for (const SdfAttributeSpecHandle attr : _schemaDef->GetAttributes()) {
        attrNames.push_back(attr->GetNameToken());
    }

    return attrNames;
}

const SdfPrimSpecHandle
UsdMayaAdaptor::SchemaAdaptor::GetSchemaDefinition() const
{
    return _schemaDef;
}



UsdMayaAdaptor::AttributeAdaptor::AttributeAdaptor()
    : _plug(), _node(), _attr(), _attrDef(nullptr)
{
}

UsdMayaAdaptor::AttributeAdaptor::AttributeAdaptor(
    const MPlug& plug, SdfAttributeSpecHandle attrDef)
    : _plug(plug), _node(plug.node()), _attr(plug.attribute()),
      _attrDef(attrDef)
{
}

UsdMayaAdaptor::AttributeAdaptor::operator bool() const
{
    if (_plug.isNull() || !_node.isValid() || !_attr.isValid() || !_attrDef) {
        return false;
    }

    MStatus status;
    MFnDependencyNode depNode(_node.object(), &status);
    if (!status) {
        return false;
    }

    MFnAttribute attr(_attr.object(), &status);
    if (!status) {
        return false;
    }

    return depNode.hasAttribute(attr.name());
}

UsdMayaAdaptor
UsdMayaAdaptor::AttributeAdaptor::GetNodeAdaptor() const
{
    if (!*this) {
        return UsdMayaAdaptor(MObject::kNullObj);
    }

    return UsdMayaAdaptor(_plug.node());
}

TfToken
UsdMayaAdaptor::AttributeAdaptor::GetName() const
{
    if (!*this) {
        return TfToken();
    }

    return _attrDef->GetNameToken();
}

bool
UsdMayaAdaptor::AttributeAdaptor::Get(VtValue* value) const
{
    if (!*this) {
        return false;
    }

    VtValue result = UsdMayaWriteUtil::GetVtValue(_plug,
            _attrDef->GetTypeName());
    if (result.IsEmpty()) {
        return false;
    }

    *value = result;
    return true;
}

bool
UsdMayaAdaptor::AttributeAdaptor::Set(const VtValue& newValue)
{
    MDGModifier modifier;
    return Set(newValue, modifier);
}

bool
UsdMayaAdaptor::AttributeAdaptor::Set(
    const VtValue& newValue,
    MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Attribute adaptor is not valid");
        return false;
    }

    return UsdMayaReadUtil::SetMayaAttr(_plug, newValue, modifier);
}

const SdfAttributeSpecHandle
UsdMayaAdaptor::AttributeAdaptor::GetAttributeDefinition() const
{
    return _attrDef;
}

PXR_NAMESPACE_CLOSE_SCOPE
