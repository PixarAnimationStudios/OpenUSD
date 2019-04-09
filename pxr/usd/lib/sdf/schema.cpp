//
// Copyright 2016 Pixar
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
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/parserValueContext.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/schemaTypeRegistration.h"
#include "pxr/usd/sdf/tokens.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/valueTypeRegistry.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/vt/dictionary.h"

#include <deque>
#include <map>
#include <set>
#include <vector>

using std::map;
using std::set;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((Default, "default"))
    ((DisplayGroup,"displayGroup"))
    ((Type,"type"))
    ((AppliesTo,"appliesTo"))
);

//
// SdfSchemaBase::FieldDefinition
//

SdfSchemaBase::FieldDefinition::FieldDefinition(
    const SdfSchemaBase& schema,
    const TfToken& name, 
    const VtValue& fallbackValue)
    : _schema(schema),
      _name(name),
      _fallbackValue(fallbackValue),
      _isPlugin(false),
      _isReadOnly(false),
      _holdsChildren(false),
      _valueValidator(nullptr),
      _listValueValidator(nullptr),
      _mapKeyValidator(nullptr),
      _mapValueValidator(nullptr)
{
}

const TfToken& 
SdfSchemaBase::FieldDefinition::GetName() const
{
    return _name;
}

const VtValue& 
SdfSchemaBase::FieldDefinition::GetFallbackValue() const
{
    return _fallbackValue;
}

const SdfSchemaBase::FieldDefinition::InfoVec&
SdfSchemaBase::FieldDefinition::GetInfo() const {
    return _info;
}

bool
SdfSchemaBase::FieldDefinition::IsPlugin() const
{
    return _isPlugin;
}

bool
SdfSchemaBase::FieldDefinition::IsReadOnly() const
{
    return _isReadOnly;
}

bool 
SdfSchemaBase::FieldDefinition::HoldsChildren() const
{
    return _holdsChildren;
}

SdfSchemaBase::FieldDefinition& 
SdfSchemaBase::FieldDefinition::FallbackValue(const VtValue& v)
{
    _fallbackValue = v;
    return *this;
}

SdfSchemaBase::FieldDefinition& 
SdfSchemaBase::FieldDefinition::Plugin()
{
    _isPlugin = true;
    return *this;
}

SdfSchemaBase::FieldDefinition& 
SdfSchemaBase::FieldDefinition::ReadOnly()
{
    _isReadOnly = true;
    return *this;
}

SdfSchemaBase::FieldDefinition& 
SdfSchemaBase::FieldDefinition::Children()
{
    _holdsChildren = true;
    _isReadOnly = true;
    return *this;
}

SdfSchemaBase::FieldDefinition& 
SdfSchemaBase::FieldDefinition::AddInfo(const TfToken& tok, const JsValue& val) {
    _info.push_back(std::make_pair(tok, val));
    return *this;
}

SdfSchemaBase::FieldDefinition& 
SdfSchemaBase::FieldDefinition::ValueValidator(Validator v)
{
    _valueValidator = v;
    return *this;
}

SdfSchemaBase::FieldDefinition& 
SdfSchemaBase::FieldDefinition::ListValueValidator(Validator v)
{
    _listValueValidator = v;
    return *this;
}

SdfSchemaBase::FieldDefinition& 
SdfSchemaBase::FieldDefinition::MapKeyValidator(Validator v)
{
    _mapKeyValidator = v;
    return *this;
}

SdfSchemaBase::FieldDefinition& 
SdfSchemaBase::FieldDefinition::MapValueValidator(Validator v)
{
    _mapValueValidator = v;
    return *this;
}

//
// SdfSchemaBase::SpecDefinition
//

TfTokenVector 
SdfSchemaBase::SpecDefinition::GetFields() const
{
    TRACE_FUNCTION();

    TfTokenVector rval(_fields.size());
    TfToken *cur = rval.data();
    for (auto const &p: _fields) {
        *cur++ = p.first;
    }
    return rval;
}

TfTokenVector 
SdfSchemaBase::SpecDefinition::GetRequiredFields() const
{
    TRACE_FUNCTION();

    TfTokenVector rval;
    TF_FOR_ALL(field, _fields) {
        if (field->second.required) {
            rval.push_back(field->first);
        }
    }

    return rval;
}

TfTokenVector 
SdfSchemaBase::SpecDefinition::GetMetadataFields() const
{
    TRACE_FUNCTION();

    TfTokenVector rval;
    TF_FOR_ALL(field, _fields) {
        if (field->second.metadata) {
            rval.push_back(field->first);
        }
    }

    return rval;
}

bool
SdfSchemaBase::SpecDefinition::IsMetadataField(const TfToken& name) const
{
    if (const _FieldInfo* fieldInfo = TfMapLookupPtr(_fields, name)) {
        return fieldInfo->metadata;
    }
    return false;
}

TfToken
SdfSchemaBase::SpecDefinition
::GetMetadataFieldDisplayGroup(const TfToken& name) const
{
    if (const _FieldInfo* fieldInfo = TfMapLookupPtr(_fields, name)) {
        return fieldInfo->metadata ?
            fieldInfo->metadataDisplayGroup : TfToken();
    }
    return TfToken();
}

bool 
SdfSchemaBase::SpecDefinition::IsRequiredField(const TfToken& name) const
{
    if (const _FieldInfo* fieldInfo = TfMapLookupPtr(_fields, name)) {
        return fieldInfo->required;
    }
    return false;
}

bool 
SdfSchemaBase::SpecDefinition::IsValidField(const TfToken& name) const
{
    return _fields.find(name) != _fields.end();
}

SdfSchemaBase::_SpecDefiner&
SdfSchemaBase::_SpecDefiner::MetadataField(const TfToken& name, bool required)
{
    return MetadataField(name, TfToken(), required);
}

SdfSchemaBase::_SpecDefiner&
SdfSchemaBase::_SpecDefiner::MetadataField(const TfToken& name,
                                      const TfToken& displayGroup,
                                      bool required)
{
    _FieldInfo fieldInfo;
    fieldInfo.metadata = true;
    fieldInfo.metadataDisplayGroup = displayGroup;
    fieldInfo.required = required;

    _definition->_AddField(name, fieldInfo);
    if (required)
        _schema->_AddRequiredFieldName(name);
    return *this;
}

SdfSchemaBase::_SpecDefiner& 
SdfSchemaBase::_SpecDefiner::Field(const TfToken& name, bool required)
{ 
    _FieldInfo fieldInfo;
    fieldInfo.required = required;

    _definition->_AddField(name, fieldInfo);
    if (required)
        _schema->_AddRequiredFieldName(name);
    return *this;
}

void 
SdfSchemaBase::SpecDefinition::_AddField(
    const TfToken& name, const _FieldInfo& fieldInfo)
{
    std::pair<_FieldMap::iterator, bool> insertStatus = 
        _fields.insert(std::make_pair(name, fieldInfo));
    if (!insertStatus.second) {
        TF_CODING_ERROR("Duplicate registration for field '%s'", 
                        name.GetText());
    }
}

SdfSchemaBase::_SpecDefiner &
SdfSchemaBase::_SpecDefiner::CopyFrom(const SpecDefinition &other)
{
    *_definition = other;
    return *this;
}


//
// Validation helpers
//

static
SdfAllowed
_ValidateFramesPerSecond(const SdfSchemaBase&, const VtValue& value)
{
    if (!value.IsHolding<double>()) {
        return SdfAllowed("Expected value of type double");
    }

    return SdfAllowed(value.Get<double>() > 0.0,
                     "Value must be greater than 0");
}

static SdfAllowed
_ValidateIsString(const SdfSchemaBase&, const VtValue& value)
{
    if (!value.IsHolding<std::string>()) {
        return SdfAllowed("Expected value of type string");
    }
    return true;
}

static SdfAllowed
_ValidateIsNonEmptyString(const SdfSchemaBase& schema, const VtValue& value)
{
    SdfAllowed result = _ValidateIsString(schema, value);
    if (result && value.Get<std::string>().empty()) {
        result = SdfAllowed("Expected non-empty string");
    }
    return result;
}

static SdfAllowed
_ValidateIdentifierToken(const SdfSchemaBase&, const VtValue& value)
{
    if (!value.IsHolding<TfToken>()) {
        return SdfAllowed("Expected value of type TfToken");
    }
    return SdfSchemaBase::IsValidIdentifier(value.Get<TfToken>());
}

static SdfAllowed
_ValidateNamespacedIdentifierToken(const SdfSchemaBase&, const VtValue& value)
{
    if (!value.IsHolding<TfToken>()) {
        return SdfAllowed("Expected value of type TfToken");
    }
    return SdfSchemaBase::IsValidNamespacedIdentifier(value.Get<TfToken>());
}

static SdfAllowed
_ValidateIsSceneDescriptionValue(const SdfSchemaBase& schema, const VtValue& value)
{
    return schema.IsValidValue(value);
}

#define SDF_VALIDATE_WRAPPER(name_, expectedType_)                      \
static SdfAllowed                                                       \
_Validate ## name_(const SdfSchemaBase& schema, const VtValue& value)   \
{                                                                       \
    if (!value.IsHolding<expectedType_>()) {                            \
        return SdfAllowed("Expected value of type " # expectedType_);   \
    }                                                                   \
    return SdfSchemaBase::IsValid ## name_(value.Get<expectedType_>()); \
}

SDF_VALIDATE_WRAPPER(AttributeConnectionPath, SdfPath);
SDF_VALIDATE_WRAPPER(Identifier, std::string);
SDF_VALIDATE_WRAPPER(InheritPath, SdfPath);
SDF_VALIDATE_WRAPPER(Payload, SdfPayload);
SDF_VALIDATE_WRAPPER(Reference, SdfReference);
SDF_VALIDATE_WRAPPER(RelationshipTargetPath, SdfPath);
SDF_VALIDATE_WRAPPER(RelocatesPath, SdfPath);
SDF_VALIDATE_WRAPPER(SpecializesPath, SdfPath);
SDF_VALIDATE_WRAPPER(SubLayer, std::string);
SDF_VALIDATE_WRAPPER(VariantIdentifier, std::string);

TF_DEFINE_PUBLIC_TOKENS(SdfChildrenKeys, SDF_CHILDREN_KEYS);
TF_DEFINE_PUBLIC_TOKENS(SdfFieldKeys, SDF_FIELD_KEYS);

//
// Registration for built-in fields for various spec types.
//

struct Sdf_SchemaFieldTypeRegistrar
{
public:
    Sdf_SchemaFieldTypeRegistrar(SdfSchemaBase* schema) : _schema(schema) { }

    template <class T>
    void RegisterField(const TfToken& fieldName)
    {
        _schema->_CreateField(fieldName, VtValue(T()));
    }

private:
    SdfSchemaBase* _schema;
};

SdfSchemaBase::_ValueTypeRegistrar::_ValueTypeRegistrar(
    Sdf_ValueTypeRegistry* registry) :
    _registry(registry)
{
    // Do nothing
}

static std::string
_GetTypeName(const TfType& type, const std::string& cppTypeName)
{
    return (cppTypeName.empty() ? 
            (type ? type.GetTypeName() : std::string()) :
            cppTypeName);
}

void
SdfSchemaBase::_ValueTypeRegistrar::AddType(const Type& type)
{
    if (!type._defaultValue.IsEmpty() || !type._defaultArrayValue.IsEmpty()) {
        _registry->AddType(
            type._name, type._defaultValue, type._defaultArrayValue, 
            _GetTypeName(type._defaultValue.GetType(), type._cppTypeName),
            _GetTypeName(type._defaultArrayValue.GetType(), 
                         type._arrayCppTypeName),
            type._unit, type._role, type._dimensions);
    }
    else {
        _registry->AddType(
            type._name, type._type, /* arrayType = */ TfType(), 
            _GetTypeName(type._type, type._cppTypeName),
            /* arrayCppTypeName = */ std::string(),
            type._unit, type._role, type._dimensions);
    }
}

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfSchemaBase>();
}

SdfSchemaBase::SdfSchemaBase() : _valueTypeRegistry(new Sdf_ValueTypeRegistry)
{
    // Do nothing
}

SdfSchemaBase::~SdfSchemaBase()
{
    // Do nothing
}

void
SdfSchemaBase::_RegisterStandardFields()
{
    // Ensure that entries for all scene description fields
    // are created with an appropriately-typed fallback value.
    // Then register additional information for each field; doing so
    // for a field that hasn't been created will cause a fatal error
    // to be emitted.
    //
    // This ensures that the field registration stays in sync with
    // the field types defined in SchemaTypeRegistration.h
    Sdf_SchemaFieldTypeRegistrar r(this);
    SdfRegisterFields(&r);

    // Regular Fields
    _DoRegisterField(SdfFieldKeys->Active, true);
    _DoRegisterField(SdfFieldKeys->AllowedTokens, VtTokenArray());
    _DoRegisterField(SdfFieldKeys->AssetInfo, VtDictionary())
        .MapKeyValidator(&_ValidateIdentifier)
        .MapValueValidator(&_ValidateIsSceneDescriptionValue);
    _DoRegisterField(SdfFieldKeys->TimeSamples, SdfTimeSampleMap());
    _DoRegisterField(SdfFieldKeys->ColorConfiguration, SdfAssetPath());
    _DoRegisterField(SdfFieldKeys->ColorManagementSystem, TfToken());
    _DoRegisterField(SdfFieldKeys->ColorSpace, TfToken());
    _DoRegisterField(SdfFieldKeys->Comment, "");
    
    // Connection paths are marked read-only because adding/removing 
    // connections requires adding/removing children specs, which we are
    // disallowing via the Info API.
    _DoRegisterField(SdfFieldKeys->ConnectionPaths, SdfPathListOp())
        .ReadOnly()
        .ListValueValidator(&_ValidateAttributeConnectionPath);

    _DoRegisterField(SdfFieldKeys->Custom, false);
    _DoRegisterField(SdfFieldKeys->CustomData, VtDictionary())
        .MapKeyValidator(&_ValidateIdentifier)
        .MapValueValidator(&_ValidateIsSceneDescriptionValue);
    _DoRegisterField(SdfFieldKeys->CustomLayerData, VtDictionary())
        .MapKeyValidator(&_ValidateIdentifier)
        .MapValueValidator(&_ValidateIsSceneDescriptionValue);
    _DoRegisterField(SdfFieldKeys->Default, VtValue())
        .ValueValidator(&_ValidateIsSceneDescriptionValue);
    _DoRegisterField(SdfFieldKeys->DisplayGroup, "");
    _DoRegisterField(SdfFieldKeys->DisplayName, "");
    _DoRegisterField(SdfFieldKeys->DisplayUnit,
                   TfEnum(SdfDimensionlessUnitDefault));
    _DoRegisterField(SdfFieldKeys->Documentation, "");
    _DoRegisterField(SdfFieldKeys->DefaultPrim, TfToken());
    _DoRegisterField(SdfFieldKeys->EndFrame, 0.0);
    _DoRegisterField(SdfFieldKeys->EndTimeCode, 0.0);
    _DoRegisterField(SdfFieldKeys->FramePrecision, 3);
    _DoRegisterField(SdfFieldKeys->FramesPerSecond, 24.0)
        .ValueValidator(&_ValidateFramesPerSecond);
    _DoRegisterField(SdfFieldKeys->Hidden, false);
    _DoRegisterField(SdfFieldKeys->HasOwnedSubLayers, false);
    _DoRegisterField(SdfFieldKeys->Instanceable, false);
    _DoRegisterField(SdfFieldKeys->InheritPaths, SdfPathListOp())
        .ListValueValidator(&_ValidateInheritPath);
    _DoRegisterField(SdfFieldKeys->Kind, TfToken());
    _DoRegisterField(SdfFieldKeys->MapperArgValue, VtValue())
        .ValueValidator(&_ValidateIsSceneDescriptionValue);
    _DoRegisterField(SdfFieldKeys->Owner, "");
    _DoRegisterField(SdfFieldKeys->PrimOrder, std::vector<TfToken>())
        .ListValueValidator(&_ValidateIdentifierToken);
    _DoRegisterField(SdfFieldKeys->NoLoadHint, false);
    _DoRegisterField(SdfFieldKeys->Payload, SdfPayloadListOp())
        .ListValueValidator(&_ValidatePayload);
    _DoRegisterField(SdfFieldKeys->Permission, SdfPermissionPublic);
    _DoRegisterField(SdfFieldKeys->Prefix, "");
    _DoRegisterField(SdfFieldKeys->PrefixSubstitutions, VtDictionary())
        .MapKeyValidator(&_ValidateIsNonEmptyString)
        .MapValueValidator(&_ValidateIsString);
    _DoRegisterField(SdfFieldKeys->PropertyOrder, std::vector<TfToken>())
        .ListValueValidator(&_ValidateNamespacedIdentifierToken);
    _DoRegisterField(SdfFieldKeys->References, SdfReferenceListOp()) 
        .ListValueValidator(&_ValidateReference);
    _DoRegisterField(SdfFieldKeys->SessionOwner, "");
    _DoRegisterField(SdfFieldKeys->Specializes, SdfPathListOp())
        .ListValueValidator(&_ValidateSpecializesPath);
    _DoRegisterField(SdfFieldKeys->Suffix, "");
    _DoRegisterField(SdfFieldKeys->SuffixSubstitutions, VtDictionary())
        .MapKeyValidator(&_ValidateIsNonEmptyString)
        .MapValueValidator(&_ValidateIsString);

    // See comment on SdfFieldKeys->ConnectionPaths for why this is read-only.
    _DoRegisterField(SdfFieldKeys->TargetPaths,  SdfPathListOp())
        .ReadOnly()
        .ListValueValidator(&_ValidateRelationshipTargetPath);

    _DoRegisterField(SdfFieldKeys->Relocates, SdfRelocatesMap())
        .MapKeyValidator(&_ValidateRelocatesPath)
        .MapValueValidator(&_ValidateRelocatesPath);
    _DoRegisterField(SdfFieldKeys->Specifier, SdfSpecifierOver);
    _DoRegisterField(SdfFieldKeys->StartFrame, 0.0);
    _DoRegisterField(SdfFieldKeys->StartTimeCode, 0.0);
    _DoRegisterField(SdfFieldKeys->SubLayers, std::vector<std::string>())
        .ListValueValidator(&_ValidateSubLayer);
    _DoRegisterField(SdfFieldKeys->SubLayerOffsets, std::vector<SdfLayerOffset>());
    _DoRegisterField(SdfFieldKeys->SymmetricPeer, "");
    _DoRegisterField(SdfFieldKeys->SymmetryArgs, VtDictionary())
        .MapKeyValidator(&_ValidateIdentifier)
        .MapValueValidator(&_ValidateIsSceneDescriptionValue);
    _DoRegisterField(SdfFieldKeys->SymmetryArguments, VtDictionary())
        .MapKeyValidator(&_ValidateIdentifier)
        .MapValueValidator(&_ValidateIsSceneDescriptionValue);
    _DoRegisterField(SdfFieldKeys->SymmetryFunction, TfToken());
    _DoRegisterField(SdfFieldKeys->TimeCodesPerSecond, 24.0);
    _DoRegisterField(SdfFieldKeys->TypeName, TfToken());
    _DoRegisterField(SdfFieldKeys->VariantSetNames, SdfStringListOp())
        .ListValueValidator(&_ValidateIdentifier);
    _DoRegisterField(SdfFieldKeys->VariantSelection, SdfVariantSelectionMap())
        .MapValueValidator(&_ValidateVariantIdentifier);
    _DoRegisterField(SdfFieldKeys->Variability, SdfVariabilityVarying);
    
    // Children fields.
    _DoRegisterField(SdfChildrenKeys->ConnectionChildren, std::vector<SdfPath>())
        .Children()
        .ListValueValidator(&_ValidateAttributeConnectionPath);
    _DoRegisterField(SdfChildrenKeys->ExpressionChildren, std::vector<TfToken>())
        .Children();
    _DoRegisterField(SdfChildrenKeys->MapperArgChildren, std::vector<TfToken>())
        .Children()
        .ListValueValidator(&_ValidateIdentifier);
    _DoRegisterField(SdfChildrenKeys->MapperChildren, std::vector<SdfPath>())
        .Children()
        .ListValueValidator(&_ValidateAttributeConnectionPath);
    _DoRegisterField(SdfChildrenKeys->PrimChildren, std::vector<TfToken>())
        .Children()
        .ListValueValidator(&_ValidateIdentifier);
    _DoRegisterField(SdfChildrenKeys->PropertyChildren, std::vector<TfToken>())
        .Children()
        .ListValueValidator(&_ValidateIdentifier);
    _DoRegisterField(SdfChildrenKeys->RelationshipTargetChildren,
                   std::vector<SdfPath>())
        .Children()
        .ListValueValidator(&_ValidateRelationshipTargetPath);
    _DoRegisterField(SdfChildrenKeys->VariantChildren, std::vector<TfToken>())
        .Children()
        .ListValueValidator(&_ValidateVariantIdentifier);
    _DoRegisterField(SdfChildrenKeys->VariantSetChildren, std::vector<TfToken>())
        .Children()
        .ListValueValidator(&_ValidateIdentifier);

    //
    // Spec definitions
    //

    _Define(SdfSpecTypePseudoRoot)
        .MetadataField(SdfFieldKeys->ColorConfiguration)
        .MetadataField(SdfFieldKeys->ColorManagementSystem)
        .Field(SdfFieldKeys->Comment)
        .MetadataField(SdfFieldKeys->CustomLayerData)
        .MetadataField(SdfFieldKeys->DefaultPrim)
        .MetadataField(SdfFieldKeys->Documentation)
        .MetadataField(SdfFieldKeys->EndTimeCode)
        .MetadataField(SdfFieldKeys->FramesPerSecond)
        .MetadataField(SdfFieldKeys->FramePrecision)
        .MetadataField(SdfFieldKeys->HasOwnedSubLayers)
        .MetadataField(SdfFieldKeys->Owner)
        .MetadataField(SdfFieldKeys->SessionOwner)
        .MetadataField(SdfFieldKeys->StartTimeCode)
        .MetadataField(SdfFieldKeys->TimeCodesPerSecond)
        .MetadataField(SdfFieldKeys->EndFrame)
        .MetadataField(SdfFieldKeys->StartFrame)

        .Field(SdfChildrenKeys->PrimChildren)
        .Field(SdfFieldKeys->PrimOrder)
        .Field(SdfFieldKeys->SubLayers)
        .Field(SdfFieldKeys->SubLayerOffsets);

    _Define(SdfSpecTypePrim)
        .Field(SdfFieldKeys->Specifier, /* required = */ true)

        .Field(SdfFieldKeys->Comment)
        .Field(SdfFieldKeys->InheritPaths)
        .Field(SdfFieldKeys->Specializes)
        .Field(SdfChildrenKeys->PrimChildren)
        .Field(SdfFieldKeys->PrimOrder)
        .Field(SdfChildrenKeys->PropertyChildren)
        .Field(SdfFieldKeys->PropertyOrder)
        .Field(SdfFieldKeys->References)
        .Field(SdfFieldKeys->Relocates)
        .Field(SdfFieldKeys->VariantSelection)
        .Field(SdfChildrenKeys->VariantSetChildren)
        .Field(SdfFieldKeys->VariantSetNames)

        .MetadataField(SdfFieldKeys->Active,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->AssetInfo,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->CustomData,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Documentation,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Hidden,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Instanceable,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Kind,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Payload,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Permission,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Prefix,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->PrefixSubstitutions,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Suffix,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->SuffixSubstitutions,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->SymmetricPeer,
                       SdfMetadataDisplayGroupTokens->symmetry)
        .MetadataField(SdfFieldKeys->SymmetryArguments,
                       SdfMetadataDisplayGroupTokens->symmetry)
        .MetadataField(SdfFieldKeys->SymmetryFunction,
                       SdfMetadataDisplayGroupTokens->symmetry)
        .MetadataField(SdfFieldKeys->TypeName,
                       SdfMetadataDisplayGroupTokens->core)
        ;

    // The property spec definition will be used as the basis for the
    // attribute and relationship spec definitions.
    SpecDefinition property;
    _Define(&property)
        .Field(SdfFieldKeys->Custom,      /* required = */ true)
        .Field(SdfFieldKeys->Variability, /* required = */ true)

        .Field(SdfFieldKeys->Comment)
        .Field(SdfFieldKeys->Default)
        .Field(SdfFieldKeys->TimeSamples)

        .MetadataField(SdfFieldKeys->AssetInfo,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->CustomData,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->DisplayGroup,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->DisplayName,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Documentation,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Hidden,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Permission,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Prefix,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->Suffix,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->SymmetricPeer,
                       SdfMetadataDisplayGroupTokens->symmetry)
        .MetadataField(SdfFieldKeys->SymmetryArguments,
                       SdfMetadataDisplayGroupTokens->symmetry)
        .MetadataField(SdfFieldKeys->SymmetryFunction,
                       SdfMetadataDisplayGroupTokens->symmetry)
        ;

    _Define(SdfSpecTypeAttribute)
        .CopyFrom(property)
        .Field(SdfFieldKeys->TypeName,                /* required = */ true)

        .Field(SdfChildrenKeys->ConnectionChildren)
        .Field(SdfChildrenKeys->MapperChildren)
        .Field(SdfFieldKeys->ConnectionPaths)
        .Field(SdfFieldKeys->DisplayUnit)
        .MetadataField(SdfFieldKeys->AllowedTokens,
                       SdfMetadataDisplayGroupTokens->core)
        .MetadataField(SdfFieldKeys->ColorSpace, 
                       SdfMetadataDisplayGroupTokens->core)
        ;

    _Define(SdfSpecTypeConnection);

    _Define(SdfSpecTypeMapper)
        .Field(SdfFieldKeys->TypeName, /* required = */ true)
        .Field(SdfChildrenKeys->MapperArgChildren)
        .MetadataField(SdfFieldKeys->SymmetryArguments,
                       SdfMetadataDisplayGroupTokens->symmetry);

    _Define(SdfSpecTypeMapperArg)
        .Field(SdfFieldKeys->MapperArgValue);

    _Define(SdfSpecTypeExpression);

    _Define(SdfSpecTypeRelationship)
        .CopyFrom(property)
        .Field(SdfChildrenKeys->RelationshipTargetChildren)
        .Field(SdfFieldKeys->TargetPaths)

        .MetadataField(SdfFieldKeys->NoLoadHint,
                       SdfMetadataDisplayGroupTokens->core);

    _Define(SdfSpecTypeRelationshipTarget)
        .Field(SdfChildrenKeys->PropertyChildren)
        .Field(SdfFieldKeys->PropertyOrder);

    _Define(SdfSpecTypeVariantSet)
        .Field(SdfChildrenKeys->VariantChildren);

    _Define(SdfSpecTypeVariant)
        .CopyFrom(*GetSpecDefinition(SdfSpecTypePrim));

}

SdfSchemaBase::FieldDefinition& 
SdfSchemaBase::_CreateField(const TfToken &key, const VtValue &value, bool plugin)
{
    FieldDefinition def(*this, key, value);
    if (plugin) {
        def.Plugin();
    }

    const std::pair<_FieldDefinitionMap::iterator, bool> insertStatus = 
        _fieldDefinitions.insert(std::make_pair(key, def));
    if (!insertStatus.second) {
        TF_CODING_ERROR("Duplicate creation for field '%s'", key.GetText());
    }
    
    return insertStatus.first->second;
}

SdfSchemaBase::FieldDefinition&
SdfSchemaBase::_DoRegisterField(const TfToken &key, const VtValue &v)
{
    // The field for which we're trying to register extra schema
    // information must have already been created with a call to
    // _CreateField. See comment in SdfSchemaBase::_RegisterStandardFields.
    _FieldDefinitionMap::iterator fieldIt = _fieldDefinitions.find(key);
    if (fieldIt == _fieldDefinitions.end()) {
        TF_FATAL_ERROR("Field '%s' has not been created.", key.GetText());
    }

    FieldDefinition& fieldDef = fieldIt->second;

    // The new fallback value's type must match the type of
    // the fallback value the field was created with. This ensures
    // we stay in sync with the fields in SchemaTypeRegistration.h.
    if (!TfSafeTypeCompare(fieldDef.GetFallbackValue().GetTypeid(),
                              v.GetTypeid())) {
        TF_FATAL_ERROR("Registered fallback value for field '%s' does "
                       "not match field type definition. "
                       "(expected: %s, got: %s)",
                       key.GetText(),
                       fieldDef.GetFallbackValue().GetTypeName().c_str(),
                       v.GetTypeName().c_str());
    }

    fieldDef.FallbackValue(v);
    return fieldDef;
}

SdfSchemaBase::_SpecDefiner
SdfSchemaBase::_ExtendSpecDefinition(SdfSpecType specType)
{
    SpecDefinition* specDef = TfMapLookupPtr(_specDefinitions, specType);
    if (!specDef) {
        TF_FATAL_ERROR("No definition for spec type %s",
                       TfEnum::GetName(specType).c_str());
    }
    return _SpecDefiner(this, specDef);
}

const SdfSchemaBase::FieldDefinition* 
SdfSchemaBase::GetFieldDefinition(const TfToken &fieldKey) const
{
    return TfMapLookupPtr(_fieldDefinitions, fieldKey);
}

const SdfSchemaBase::SpecDefinition* 
SdfSchemaBase::GetSpecDefinition(SdfSpecType specType) const
{
    return TfMapLookupPtr(_specDefinitions, specType);
}

const VtValue&
SdfSchemaBase::GetFallback(const TfToken &fieldKey) const
{
    static VtValue empty;

    const FieldDefinition* def = GetFieldDefinition(fieldKey);
    return def ? def->GetFallbackValue() : empty;
}

bool 
SdfSchemaBase::IsRegistered(const TfToken &fieldKey, VtValue *fallback) const
{
    const FieldDefinition* def = GetFieldDefinition(fieldKey);
    if (!def) {
        return false;
    }

    if (fallback) {
        *fallback = def->GetFallbackValue();
    }

    return true;
}

bool 
SdfSchemaBase::HoldsChildren(const TfToken &fieldKey) const
{
    const FieldDefinition* def = GetFieldDefinition(fieldKey);
    return (def ? def->HoldsChildren() : false);
}

VtValue 
SdfSchemaBase::CastToTypeOf(const TfToken &fieldKey, const VtValue &value) const
{
    VtValue fallback;
    if (!SdfSchemaBase::IsRegistered(fieldKey, &fallback)) {
        return VtValue();
    }
    
    if (fallback.IsEmpty()) {
        return value;
    }

    return VtValue::CastToTypeOf(value, fallback);
}

const SdfSchemaBase::SpecDefinition* 
SdfSchemaBase::_CheckAndGetSpecDefinition(SdfSpecType specType) const
{
    const SpecDefinition* def = GetSpecDefinition(specType);
    if (!def) {
        TF_CODING_ERROR("No definition for spec type %s", 
                        TfStringify(specType).c_str());
    }

    return def;
}

bool 
SdfSchemaBase::IsValidFieldForSpec(const TfToken &fieldKey, 
                               SdfSpecType specType) const
{
    const SpecDefinition* def = _CheckAndGetSpecDefinition(specType);
    return (def ? def->IsValidField(fieldKey) : false);
}

TfTokenVector 
SdfSchemaBase::GetFields(SdfSpecType specType) const
{
    const SpecDefinition* def = _CheckAndGetSpecDefinition(specType);
    return (def ? def->GetFields() : TfTokenVector());
}

TfTokenVector 
SdfSchemaBase::GetMetadataFields(SdfSpecType specType) const
{
    const SpecDefinition* def = _CheckAndGetSpecDefinition(specType);
    return (def ? def->GetMetadataFields() : TfTokenVector());
}

TfToken
SdfSchemaBase::GetMetadataFieldDisplayGroup(SdfSpecType specType,
                                   TfToken const &metadataField) const
{
    const SpecDefinition* def = _CheckAndGetSpecDefinition(specType);
    return (def ? def->GetMetadataFieldDisplayGroup(metadataField) : TfToken());
}

TfTokenVector 
SdfSchemaBase::GetRequiredFields(SdfSpecType specType) const
{
    const SpecDefinition* def = _CheckAndGetSpecDefinition(specType);
    return (def ? def->GetRequiredFields() : TfTokenVector());
}

SdfAllowed
SdfSchemaBase::IsValidValue(const VtValue& value) const
{
    if (value.IsEmpty()) {
        return true;
    }

    if (value.IsHolding<VtDictionary>()) {
        // Although dictionaries are not explicitly registered as a value
        // type, they are valid scene description and can be written/read 
        // to/from layers as long as each individual value is valid scene
        // description. Note that we don't have to check keys because
        // VtDictionary's keys are always strings.
        //
        TF_FOR_ALL(it, value.UncheckedGet<VtDictionary>()) {
            if (SdfAllowed valueStatus = IsValidValue(it->second)) {
                // Value is OK, so do nothing.
            }
            else {
                const std::string error = TfStringPrintf(
                    "Value for key '%s' does not have a valid scene "
                    "description type (%s)", 
                    it->first.c_str(), it->second.GetTypeName().c_str());
                return SdfAllowed(error);
            }
        }
    }
    else if (!FindType(value)) {
        return SdfAllowed(
            "Value does not have a valid scene description type "
            "(" + value.GetTypeName() + ")");
    }
    
    return true;
}

std::vector<SdfValueTypeName>
SdfSchemaBase::GetAllTypes() const
{
    return _valueTypeRegistry->GetAllTypes();
}

SdfValueTypeName
SdfSchemaBase::FindType(const std::string& typeName) const
{
    return _valueTypeRegistry->FindType(typeName);
}

SdfValueTypeName
SdfSchemaBase::FindType(const TfType& type, const TfToken& role) const
{
    return _valueTypeRegistry->FindType(type, role);
}

SdfValueTypeName
SdfSchemaBase::FindType(const VtValue& value, const TfToken& role) const
{
    return _valueTypeRegistry->FindType(value, role);
}

SdfValueTypeName
SdfSchemaBase::FindOrCreateType(const std::string& typeName) const
{
    return _valueTypeRegistry->FindOrCreateTypeName(typeName);
}

SdfSchemaBase::_ValueTypeRegistrar
SdfSchemaBase::_GetTypeRegistrar() const
{
    return _ValueTypeRegistrar(_valueTypeRegistry.get());
}

SdfAllowed
SdfSchemaBase::IsValidIdentifier(const std::string& identifier)
{
    if (!SdfPath::IsValidIdentifier(identifier)) {
        return SdfAllowed("\"" + identifier +
                          "\" is not a valid identifier");
    }
    return true;
}

SdfAllowed
SdfSchemaBase::IsValidNamespacedIdentifier(const std::string& identifier)
{
    if (!SdfPath::IsValidNamespacedIdentifier(identifier)) {
        return SdfAllowed("\"" + identifier +
                          "\" is not a valid identifier");
    }
    return true;
}

SdfAllowed
SdfSchemaBase::IsValidVariantIdentifier(const std::string& identifier)
{
    // Allow [[:alnum:]_|\-]+ with an optional leading dot.

    std::string::const_iterator first = identifier.begin();
    std::string::const_iterator last = identifier.end();

    // Allow optional leading dot.
    if (first != last && *first == '.') {
        ++first;
    }

    for (; first != last; ++first) {
        char c = *first;
        if (!(isalnum(c) || (c == '_') || (c == '|') || (c == '-'))) {
            return SdfAllowed(TfStringPrintf(
                    "\"%s\" is not a valid variant "
                    "name due to '%c' at index %d",
                    identifier.c_str(),
                    c,
                    (int)(first - identifier.begin())));
        }
    }

    return true;
}

SdfAllowed 
SdfSchemaBase::IsValidRelocatesPath(const SdfPath& path)
{
    if (path == SdfPath::AbsoluteRootPath()) {
        return SdfAllowed("Root paths not allowed in relocates map");
    }

    return true;
}

SdfAllowed
SdfSchemaBase::IsValidInheritPath(const SdfPath& path)
{
    if (!(path.IsAbsolutePath() && path.IsPrimPath())) {
        return SdfAllowed("Inherit paths must be an absolute prim path");
    }
    return true;
}

SdfAllowed
SdfSchemaBase::IsValidSpecializesPath(const SdfPath& path)
{
    if (!(path.IsAbsolutePath() && path.IsPrimPath())) {
        return SdfAllowed("Specializes paths must be absolute prim path");
    }
    return true;
}

SdfAllowed 
SdfSchemaBase::IsValidAttributeConnectionPath(const SdfPath& path)
{
    if (path.ContainsPrimVariantSelection()) {
        return SdfAllowed("Attribute connection paths cannot contain "
                          "variant selections");
    }
    if (path.IsAbsolutePath() && (path.IsPropertyPath() || path.IsPrimPath())) {
        return true;
    }
    else {
        return SdfAllowed(
            TfStringPrintf("Connection paths must be absolute prim or "
                           "property paths: <%s>", path.GetText()));
    }
}

SdfAllowed
SdfSchemaBase::IsValidRelationshipTargetPath(const SdfPath& path)
{
    if (path.ContainsPrimVariantSelection()) {
        return SdfAllowed("Relationship target paths cannot contain "
                          "variant selections");
    }
    if (path.IsAbsolutePath() && 
        (path.IsPropertyPath() || path.IsPrimPath() || path.IsMapperPath())) {
        return true;
    }
    else {
        return SdfAllowed("Relationship target paths must be absolute prim, "
                          "property or mapper paths");
    }
}

SdfAllowed 
SdfSchemaBase::IsValidReference(const SdfReference& ref)
{
    const SdfPath& path = ref.GetPrimPath();
    if (!path.IsEmpty() &&
        !(path.IsAbsolutePath() && path.IsPrimPath())) {
        return SdfAllowed("Reference prim path <" + path.GetString() + 
                          "> must be either empty or an absolute prim path");
    }

    return true;
}

SdfAllowed
SdfSchemaBase::IsValidPayload(const SdfPayload& p)
{
    const SdfPath& path = p.GetPrimPath();
    if (!path.IsEmpty() &&
        !(path.IsAbsolutePath() && path.IsPrimPath())) {
        return SdfAllowed("Payload prim path <" + path.GetString() + 
                          "> must be either empty or an absolute prim path");
    }

    return true;
}

SdfAllowed 
SdfSchemaBase::IsValidSubLayer(const std::string& sublayer)
{
    if (sublayer.empty()) {
        return SdfAllowed("Sublayer paths must not be empty");
    }

    return true;
}

typedef Sdf_ParserHelpers::Value Value;

// Helper function that adds values of type T to the value list that are
// either stored directly or stored as elements of a vector<T>. Returns true
// on success and false on failure.
template <typename T>
static bool
_AccumulateTypedValues(const JsValue &value, std::deque<Value> *values) {
    if (value.IsArrayOf<T>()) {
        for (const T& v : value.GetArrayOf<T>()) {
            values->push_back(v);
        }
        return true;
    } else if (value.Is<T>()) {
        values->push_back(value.Get<T>());
        return true;
    }
    return false;
}

// Recursive helper function to feed the ParserValueContext with the correct
// calls to BeginTuple(), EndTuple(), and TupleItem() in between calls to
// AppendValue().
static void
_AddValuesToValueContext(std::deque<Value> *values, Sdf_ParserValueContext *context, int level = 0) {
    if (context->valueTupleDimensions.size == 0) {
        while (!values->empty()) {
            context->AppendValue(values->front());
            values->pop_front();
        }
    } else if (static_cast<size_t>(level) < context->valueTupleDimensions.size) {
        context->BeginTuple();
        for (size_t i = 0; i < context->valueTupleDimensions.d[level]; i++) {
            _AddValuesToValueContext(values, context, level + 1);
        }
        context->EndTuple();
    } else if (!values->empty()) {
        context->AppendValue(values->front());
        values->pop_front();
    }
}

// Uses the ParserValueContext to manufacture a VtValue of the correct type
// from a JsValue and a value typename. For example, this can manufacture a
// "Vec3d[]" from a JsValue containing vector<double>(1, 2, 3, 4, 5, 6) into
// VtValue(VtArray([2], Vec3d(1, 2, 3), Vec3d(4, 5, 6))). If an error occurs,
// an empty VtValue is returned and the error text is stored in *errorText.
static VtValue
_ParseValue(const std::string &valueTypeName, const JsValue &value,
            std::string *errorText)
{
    // Checks for strings, ints, doubles, and vectors of those types because
    // that's what _ConvertDict() in Plugin.cpp parses out of plugInfo.json
    std::deque<Value> values;
    if (!_AccumulateTypedValues<std::string>(value, &values) &&
        !_AccumulateTypedValues<int>(value, &values) &&
        !_AccumulateTypedValues<double>(value, &values)) {
        *errorText = "Value was not a string, an int, a double, or a "
                     "vector of those types";
        return VtValue();
    }

    // Initialize the ParserValueContext
    Sdf_ParserValueContext context;
    if (!context.SetupFactory(valueTypeName)) {
        *errorText = TfStringPrintf("\"%s\" is not a valid type", 
                                    valueTypeName.c_str());
        return VtValue();
    }

    // Feed the ParserValueContext the values in the correct format.
    // A better solution would be to have the default value be a string,
    // which is parsed using the menva file format syntax for typed values.
    // This would involve extracting the typed value rule out of the parser
    // and into a new parser.
    if (context.valueIsShaped)
        context.BeginList();
    while (!values.empty())
        _AddValuesToValueContext(&values, &context);
    if (context.valueIsShaped)
        context.EndList();

    // Return the produced value, or fill in errorText and return VtValue() 
    // on failure
    return context.ProduceValue(errorText);
}

// Helper function to make reading from dictionaries easier
template <typename T>
static bool
_GetKey(const JsObject &dict, const std::string &key, T *value)
{
    JsObject::const_iterator i = dict.find(key);
    if (i != dict.end() && i->second.Is<T>()) {
        *value = i->second.Get<T>();
        return true;
    }
    return false;
}

// Helper function to read and extract from dictionaries
template <typename T>
static bool
_ExtractKey(JsObject &dict, const std::string &key, T *value)
{
    JsObject::const_iterator i = dict.find(key);
    if (i != dict.end() && i->second.Is<T>()) {
        *value = i->second.Get<T>();
        dict.erase(i);
        return true;
    }
    return false;
}

static VtValue
_GetDefaultValueForListOp(const std::string& valueTypeName)
{
    if (valueTypeName == "intlistop") {
        return VtValue(SdfIntListOp());
    }
    else if (valueTypeName == "int64listop") {
        return VtValue(SdfInt64ListOp());
    }
    if (valueTypeName == "uintlistop") {
        return VtValue(SdfUIntListOp());
    }
    else if (valueTypeName == "uint64listop") {
        return VtValue(SdfUInt64ListOp());
    }
    if (valueTypeName == "stringlistop") {
        return VtValue(SdfStringListOp());
    }
    else if (valueTypeName == "tokenlistop") {
        return VtValue(SdfTokenListOp());
    }
    return VtValue();
}

static VtValue
_GetDefaultMetadataValue(const std::string& valueTypeName,
                         const JsValue& defaultValue)
{
    if (valueTypeName == "dictionary") {
        if (!defaultValue.IsNull()) {
            // Defaults aren't allowed for dictionaries because we have
            // no way of parsing them at the moment
            TF_CODING_ERROR("Default values are not allowed on fields "
                            "of type \"dictionary\", which will "
                            "always default to an empty dictionary.");
            return VtValue();
        }
        return VtValue(VtDictionary());
    }

    const VtValue listOpValue = _GetDefaultValueForListOp(valueTypeName);
    if (!listOpValue.IsEmpty()) {
        if (!defaultValue.IsNull()) {
            // Defaults aren't allowed for list ops because we have
            // no way of parsing them at the moment
            TF_CODING_ERROR("Default values are not allowed on fields "
                            "of type \"%s\", which will always"
                            "default to an empty list op.", 
                            valueTypeName.c_str());
            return VtValue();
        }
        return listOpValue;
    }

    // XXX: This is bogus but currently necessary.  When looking
    //      up types for metadata use the SdfSchema instead of
    //      this schema.  SdSchema wants to pull in sdf metadata
    //      but doesn't register all usd types.
    if (const SdfValueTypeName valueType = 
        SdfSchema::GetInstance().FindType(valueTypeName)) {

        if (defaultValue.IsNull()) {
            return valueType.GetDefaultValue();
        }
        else {
            std::string errorText;
            const VtValue parsedValue = _ParseValue(
                valueTypeName, defaultValue, &errorText);
            if (parsedValue.IsEmpty()) {
                TF_CODING_ERROR("Could not parse default value: %s",
                                errorText.c_str());
            }
            return parsedValue;
        }
    }

    TF_CODING_ERROR("\"%s\" is not a registered value type", 
                    valueTypeName.c_str());
    return VtValue();
}

// Reads and registers new fields from plugInfo.json files
const std::vector<const SdfSchemaBase::FieldDefinition *>
SdfSchemaBase::_UpdateMetadataFromPlugins(
    const PlugPluginPtrVector& plugins,
    const std::string& tag,
    const _DefaultValueFactoryFn& defFactory)
{
    static const std::string sdfMetadataTag = "SdfMetadata";
    const std::string& metadataTag = (tag.empty() ? sdfMetadataTag : tag);
    std::vector<const SdfSchemaBase::FieldDefinition *> metadataFieldsParsed;

    // Update the schema with new metadata fields from each plugin, if they 
    // contain any
    for (const PlugPluginPtr & plug : plugins) {
        // Get the top-level dictionary key specified by the metadata tag.
        JsObject fields;
        const JsObject &metadata = plug->GetMetadata();
        if (!_GetKey(metadata, metadataTag, &fields))
            continue;
        
        // Register new fields
        for (const std::pair<std::string, JsValue>& field : fields) {
            const TfToken fieldName(field.first);

            // Validate field
            JsObject fieldInfo;
            if (!_GetKey(fields, fieldName, &fieldInfo)) {
                TF_CODING_ERROR("Value must be a dictionary (at \"%s\" in "
                                "plugin \"%s\")", 
                                fieldName.GetText(),
                                plug->GetPath().c_str());
                continue;
            }

            std::string valueTypeName;
            if (!_ExtractKey(
                fieldInfo, _tokens->Type.GetString(), &valueTypeName)) {
                TF_CODING_ERROR("Could not read a string for \"type\" "
                                "(at \"%s\" in plugin \"%s\")",
                                fieldName.GetText(), plug->GetPath().c_str());
                continue;
            }

            if (IsRegistered(fieldName)) {
                TF_CODING_ERROR("\"%s\" is already a registered field "
                                "(in plugin \"%s\")",
                                fieldName.GetText(), 
                                plug->GetPath().c_str());
                continue;
            }

            // Parse plugin-defined default value for this field.
            VtValue defaultValue;
            {
                const JsValue pluginDefault = 
                    TfMapLookupByValue(fieldInfo,
                    _tokens->Default.GetString(), JsValue());

                TfErrorMark m;

                defaultValue = _GetDefaultMetadataValue(
                    valueTypeName, pluginDefault);
                if (defaultValue.IsEmpty() && defFactory) {
                    defaultValue = defFactory(valueTypeName, pluginDefault);
                }

                if (defaultValue.IsEmpty()) {
                    // If an error wasn't emitted but we still don't have a
                    // default value, emit an error indicating this.
                    //
                    // If an error was emitted, post a follow-up error that 
                    // provides more context about where that error was 
                    // encountered, since the default value factory isn't
                    // given enough info to do this itself.
                    if (m.IsClean()) {
                        TF_CODING_ERROR("No default value for metadata "
                                        "(at \"%s\" in plugin \"%s\")",
                                        fieldName.GetText(), 
                                        plug->GetPath().c_str());
                    }
                    else {
                        TF_CODING_ERROR("Error parsing default value for "
                                        "metadata (at \"%s\" in plugin \"%s\")",
                                        fieldName.GetText(), 
                                        plug->GetPath().c_str());
                    }
                    continue;
                }
                else {
                    // We can drop errors that had been issued from 
                    // _GetDefaultMetadataValue (e.g., due to this metadata 
                    // type not being recognized) if the passed-in factory
                    // was able to produce a default value.
                    m.Clear();
                }
            }

            // Use the supplied displayGroup, if set, otherwise 'uncategorized'.
            TfToken displayGroup;
            {
                std::string displayGroupString;
                if (_ExtractKey(fieldInfo,
                    _tokens->DisplayGroup.GetString(), &displayGroupString))
                    displayGroup = TfToken(displayGroupString);
            }

            FieldDefinition& fieldDef = 
                _RegisterField(fieldName, defaultValue, /* plugin = */ true);

            // Look for 'appliesTo', either a single string or a list of strings
            // specifying which spec types this metadatum should be registered
            // for.
            set<string> appliesTo;
            {
                const JsValue val = 
                    TfMapLookupByValue(fieldInfo,
                    _tokens->AppliesTo.GetString(), JsValue());
                if (val.IsArrayOf<string>()) {
                    const vector<string> vec = val.GetArrayOf<string>();
                    appliesTo.insert(vec.begin(), vec.end());
                } else if (val.Is<string>()) {
                    appliesTo.insert(val.Get<string>());
                }

                // this is so appliesTo does not show up in fieldDef's info
                fieldInfo.erase(_tokens->AppliesTo.GetString());
            }
            if (appliesTo.empty() || appliesTo.count("layers")) {
                _ExtendSpecDefinition(SdfSpecTypePseudoRoot)
                    .MetadataField(fieldName, displayGroup);
            }

            if (appliesTo.empty() || appliesTo.count("prims")) {
                _ExtendSpecDefinition(SdfSpecTypePrim)
                    .MetadataField(fieldName, displayGroup);
            }

            if (appliesTo.empty() || appliesTo.count("properties") ||
                appliesTo.count("attributes")) {
                _ExtendSpecDefinition(SdfSpecTypeAttribute)
                    .MetadataField(fieldName, displayGroup);
            }

            if (appliesTo.empty() || appliesTo.count("properties") ||
                appliesTo.count("relationships")) {
                _ExtendSpecDefinition(SdfSpecTypeRelationship)
                    .MetadataField(fieldName, displayGroup);
            }

            // All metadata on prims should also apply to variants.
            // This matches how the variant spec definition is copied
            // from the prim spec definition in _RegisterStandardFields.
            if (appliesTo.empty() || appliesTo.count("variants") || 
                appliesTo.count("prims")) {
                _ExtendSpecDefinition(SdfSpecTypeVariant)
                    .MetadataField(fieldName, displayGroup);
            }

            // All remaining values in the fieldInfo will are unknown to sdf,
            // so store them off in our field definitions for other libraries
            // to use.
            for (const std::pair<const std::string, JsValue>& it : fieldInfo) {
                const std::string& metadataInfoName = it.first;
                const JsValue& metadataInfoValue = it.second;

                TfToken metadataInfo (metadataInfoName);
                fieldDef.AddInfo(metadataInfo, metadataInfoValue);
            }
            metadataFieldsParsed.push_back(&fieldDef);
        }
    }
    return metadataFieldsParsed;
}

void
SdfSchemaBase::_AddRequiredFieldName(const TfToken &fieldName)
{
    if (find(_requiredFieldNames.begin(),
             _requiredFieldNames.end(), fieldName) == _requiredFieldNames.end())
        _requiredFieldNames.push_back(fieldName);
}

//
// SdfSchema
//

TF_INSTANTIATE_SINGLETON(SdfSchema);

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfSchema, TfType::Bases<SdfSchemaBase> >();
}

SdfSchema::SdfSchema()
{
    typedef SdfSchema This;

    _RegisterTypes(_GetTypeRegistrar());

    _RegisterStandardFields();

    // _UpdateMetadataFromPlugins may reenter schema ctor, so tell TfSingleton
    // that the instance is constructed.
    TfSingleton<This>::SetInstanceConstructed(*this);

    // Update generic metadata fields from all currently-registered plugins.
    // Set up notice handling so we'll check for new generic metadata as more
    // plugins are registered.
    _UpdateMetadataFromPlugins(PlugRegistry::GetInstance().GetAllPlugins());
    TfNotice::Register(TfCreateWeakPtr(this), &This::_OnDidRegisterPlugins);
}

SdfSchema::~SdfSchema()
{
    // Do nothing
}

void
SdfSchema::_RegisterTypes(_ValueTypeRegistrar r)
{
    using T = _ValueTypeRegistrar::Type;
    const TfEnum& length  = SdfDefaultUnit(TfEnum(SdfLengthUnit(0)));
    const TfToken& point  = SdfValueRoleNames->Point;
    const TfToken& vector = SdfValueRoleNames->Vector;
    const TfToken& normal = SdfValueRoleNames->Normal;
    const TfToken& color  = SdfValueRoleNames->Color;
    const TfToken& texCoord = SdfValueRoleNames->TextureCoordinate;

    // Make sure TfTypes are registered.
    TfRegistryManager::GetInstance().SubscribeTo<TfType>();

    // Simple types.
    r.AddType(T("bool",   bool()));
    // XXX: We also need to fix the VT_INTEGRAL_BUILTIN_VALUE_TYPES
    //       macro to use 'int8_t' if we add 'char'.
    //r.AddType(T("char",   int8_t());
    r.AddType(T("uchar",  uint8_t()).CPPTypeName("unsigned char"));
    //r.AddType(T("short",  int16_t());
    //r.AddType(T("ushort", uint16_t());
    r.AddType(T("int",    int32_t()).CPPTypeName("int"));
    r.AddType(T("uint",   uint32_t()).CPPTypeName("unsigned int"));
    r.AddType(T("int64",  int64_t()).CPPTypeName("int64_t"));
    r.AddType(T("uint64", uint64_t()).CPPTypeName("uint64_t"));
    r.AddType(T("half",   GfHalf(0.0)).CPPTypeName("GfHalf"));
    r.AddType(T("float",  float()));
    r.AddType(T("double", double()));
    // TfType reports "string" as the typename for "std::string", but we want
    // the fully-qualified name for documentation purposes.
    r.AddType(T("string", std::string()).CPPTypeName("std::string"));
    r.AddType(T("token",  TfToken()));
    r.AddType(T("asset",  SdfAssetPath()));

    // Compound types.
    r.AddType(T("double2",  GfVec2d(0.0)).Dimensions(2));
    r.AddType(T("double3",  GfVec3d(0.0)).Dimensions(3));
    r.AddType(T("double4",  GfVec4d(0.0)).Dimensions(4));
    r.AddType(T("float2",   GfVec2f(0.0)).Dimensions(2));
    r.AddType(T("float3",   GfVec3f(0.0)).Dimensions(3));
    r.AddType(T("float4",   GfVec4f(0.0)).Dimensions(4));
    r.AddType(T("half2",    GfVec2h(0.0)).Dimensions(2));
    r.AddType(T("half3",    GfVec3h(0.0)).Dimensions(3));
    r.AddType(T("half4",    GfVec4h(0.0)).Dimensions(4));
    r.AddType(T("int2",     GfVec2i(0.0)).Dimensions(2));
    r.AddType(T("int3",     GfVec3i(0.0)).Dimensions(3));
    r.AddType(T("int4",     GfVec4i(0.0)).Dimensions(4));
    r.AddType(T("point3h",  GfVec3h(0.0)).DefaultUnit(length).Role(point)
                                         .Dimensions(3));
    r.AddType(T("point3f",  GfVec3f(0.0)).DefaultUnit(length).Role(point)
                                         .Dimensions(3));
    r.AddType(T("point3d",  GfVec3d(0.0)).DefaultUnit(length).Role(point)
                                         .Dimensions(3));
    r.AddType(T("vector3h", GfVec3h(0.0)).DefaultUnit(length).Role(vector)
                                         .Dimensions(3));
    r.AddType(T("vector3f", GfVec3f(0.0)).DefaultUnit(length).Role(vector)
                                         .Dimensions(3));
    r.AddType(T("vector3d", GfVec3d(0.0)).DefaultUnit(length).Role(vector)
                                         .Dimensions(3));
    r.AddType(T("normal3h", GfVec3h(0.0)).DefaultUnit(length).Role(normal)
                                         .Dimensions(3));
    r.AddType(T("normal3f", GfVec3f(0.0)).DefaultUnit(length).Role(normal)
                                         .Dimensions(3));
    r.AddType(T("normal3d", GfVec3d(0.0)).DefaultUnit(length).Role(normal)
                                         .Dimensions(3));
    r.AddType(T("color3h",  GfVec3h(0.0)).Role(color).Dimensions(3));
    r.AddType(T("color3f",  GfVec3f(0.0)).Role(color).Dimensions(3));
    r.AddType(T("color3d",  GfVec3d(0.0)).Role(color).Dimensions(3));
    r.AddType(T("color4h",  GfVec4h(0.0)).Role(color).Dimensions(4));
    r.AddType(T("color4f",  GfVec4f(0.0)).Role(color).Dimensions(4));
    r.AddType(T("color4d",  GfVec4d(0.0)).Role(color).Dimensions(4));
    r.AddType(T("quath",    GfQuath(1.0)).Dimensions(4));
    r.AddType(T("quatf",    GfQuatf(1.0)).Dimensions(4));
    r.AddType(T("quatd",    GfQuatd(1.0)).Dimensions(4));
    r.AddType(T("matrix2d", GfMatrix2d(1.0)).Dimensions({2, 2}));
    r.AddType(T("matrix3d", GfMatrix3d(1.0)).Dimensions({3, 3}));
    r.AddType(T("matrix4d", GfMatrix4d(1.0)).Dimensions({4, 4}));
    r.AddType(T("frame4d",  GfMatrix4d(1.0)).Role(SdfValueRoleNames->Frame)
                                            .Dimensions({4, 4}));
    r.AddType(T("texCoord2f", GfVec2f(0.0)).Role(texCoord).Dimensions(2));
    r.AddType(T("texCoord2d", GfVec2d(0.0)).Role(texCoord).Dimensions(2));
    r.AddType(T("texCoord2h", GfVec2h(0.0)).Role(texCoord).Dimensions(2));
    r.AddType(T("texCoord3f", GfVec3f(0.0)).Role(texCoord).Dimensions(3));
    r.AddType(T("texCoord3d", GfVec3d(0.0)).Role(texCoord).Dimensions(3));
    r.AddType(T("texCoord3h", GfVec3h(0.0)).Role(texCoord).Dimensions(3));

    // XXX: Legacy types.  We can remove these when assets are
    //      updated.  parserHelpers.cpp adds support for reading
    //      old text Usd files but we also need support for binary
    //      files.  We also need these for places we confuse Sdf
    //      and Sd.
    r.AddType(T("Vec2i",      GfVec2i(0.0)).Dimensions(2));
    r.AddType(T("Vec2h",      GfVec2h(0.0)).Dimensions(2));
    r.AddType(T("Vec2f",      GfVec2f(0.0)).Dimensions(2));
    r.AddType(T("Vec2d",      GfVec2d(0.0)).Dimensions(2));
    r.AddType(T("Vec3i",      GfVec3i(0.0)).Dimensions(3));
    r.AddType(T("Vec3h",      GfVec3h(0.0)).Dimensions(3));
    r.AddType(T("Vec3f",      GfVec3f(0.0)).Dimensions(3));
    r.AddType(T("Vec3d",      GfVec3d(0.0)).Dimensions(3));
    r.AddType(T("Vec4i",      GfVec4i(0.0)).Dimensions(4));
    r.AddType(T("Vec4h",      GfVec4h(0.0)).Dimensions(4));
    r.AddType(T("Vec4f",      GfVec4f(0.0)).Dimensions(4));
    r.AddType(T("Vec4d",      GfVec4d(0.0)).Dimensions(4));
    r.AddType(T("Point",      GfVec3d(0.0)).DefaultUnit(length).Role(point)
                                           .Dimensions(3));
    r.AddType(T("PointFloat", GfVec3f(0.0)).DefaultUnit(length).Role(point)
                                           .Dimensions(3));
    r.AddType(T("Normal",     GfVec3d(0.0)).DefaultUnit(length).Role(normal)
                                           .Dimensions(3));
    r.AddType(T("NormalFloat",GfVec3f(0.0)).DefaultUnit(length).Role(normal)
                                           .Dimensions(3));
    r.AddType(T("Vector",     GfVec3d(0.0)).DefaultUnit(length).Role(vector)
                                           .Dimensions(3));
    r.AddType(T("VectorFloat",GfVec3f(0.0)).DefaultUnit(length).Role(vector)
                                           .Dimensions(3));
    r.AddType(T("Color",      GfVec3d(0.0)).Role(color).Dimensions(3));
    r.AddType(T("ColorFloat", GfVec3f(0.0)).Role(color).Dimensions(3));
    r.AddType(T("Quath",      GfQuath(1.0)).Dimensions(4));
    r.AddType(T("Quatf",      GfQuatf(1.0)).Dimensions(4));
    r.AddType(T("Quatd",      GfQuatd(1.0)).Dimensions(4));
    r.AddType(T("Matrix2d",   GfMatrix2d(1.0)).Dimensions({2, 2}));
    r.AddType(T("Matrix3d",   GfMatrix3d(1.0)).Dimensions({3, 3}));
    r.AddType(T("Matrix4d",   GfMatrix4d(1.0)).Dimensions({4, 4}));
    r.AddType(T("Frame",      GfMatrix4d(1.0)).Role(SdfValueRoleNames->Frame)
                                              .Dimensions({4, 4}));
    r.AddType(T("Transform",  GfMatrix4d(1.0)).Role(SdfValueRoleNames->Transform)
                                              .Dimensions({4, 4}));
    r.AddType(T("PointIndex", int()).Role(SdfValueRoleNames->PointIndex));
    r.AddType(T("EdgeIndex",  int()).Role(SdfValueRoleNames->EdgeIndex));
    r.AddType(T("FaceIndex",  int()).Role(SdfValueRoleNames->FaceIndex));
}

void 
SdfSchema::_OnDidRegisterPlugins(const PlugNotice::DidRegisterPlugins& n)
{
    _UpdateMetadataFromPlugins(n.GetNewPlugins());
}

const Sdf_ValueTypeNamesType*
SdfSchema::_NewValueTypeNames() const
{
    Sdf_ValueTypeNamesType* n = new Sdf_ValueTypeNamesType;

    n->Bool          = FindType("bool");
    n->UChar         = FindType("uchar");
    n->Int           = FindType("int");
    n->UInt          = FindType("uint");
    n->Int64         = FindType("int64");
    n->UInt64        = FindType("uint64");
    n->Half          = FindType("half");
    n->Float         = FindType("float");
    n->Double        = FindType("double");
    n->String        = FindType("string");
    n->Token         = FindType("token");
    n->Asset         = FindType("asset");
    n->Int2          = FindType("int2");
    n->Int3          = FindType("int3");
    n->Int4          = FindType("int4");
    n->Half2         = FindType("half2");
    n->Half3         = FindType("half3");
    n->Half4         = FindType("half4");
    n->Float2        = FindType("float2");
    n->Float3        = FindType("float3");
    n->Float4        = FindType("float4");
    n->Double2       = FindType("double2");
    n->Double3       = FindType("double3");
    n->Double4       = FindType("double4");
    n->Point3h       = FindType("point3h");
    n->Point3f       = FindType("point3f");
    n->Point3d       = FindType("point3d");
    n->Vector3h      = FindType("vector3h");
    n->Vector3f      = FindType("vector3f");
    n->Vector3d      = FindType("vector3d");
    n->Normal3h      = FindType("normal3h");
    n->Normal3f      = FindType("normal3f");
    n->Normal3d      = FindType("normal3d");
    n->Color3h       = FindType("color3h");
    n->Color3f       = FindType("color3f");
    n->Color3d       = FindType("color3d");
    n->Color4h       = FindType("color4h");
    n->Color4f       = FindType("color4f");
    n->Color4d       = FindType("color4d");
    n->Quath         = FindType("quath");
    n->Quatf         = FindType("quatf");
    n->Quatd         = FindType("quatd");
    n->Matrix2d      = FindType("matrix2d");
    n->Matrix3d      = FindType("matrix3d");
    n->Matrix4d      = FindType("matrix4d");
    n->Frame4d       = FindType("frame4d");
    n->TexCoord2f    = FindType("texCoord2f");
    n->TexCoord2d    = FindType("texCoord2d");
    n->TexCoord2h    = FindType("texCoord2h");
    n->TexCoord3f    = FindType("texCoord3f");
    n->TexCoord3d    = FindType("texCoord3d");
    n->TexCoord3h    = FindType("texCoord3h");

    n->BoolArray     = FindType("bool[]");
    n->UCharArray    = FindType("uchar[]");
    n->IntArray      = FindType("int[]");
    n->UIntArray     = FindType("uint[]");
    n->Int64Array    = FindType("int64[]");
    n->UInt64Array   = FindType("uint64[]");
    n->HalfArray     = FindType("half[]");
    n->FloatArray    = FindType("float[]");
    n->DoubleArray   = FindType("double[]");
    n->StringArray   = FindType("string[]");
    n->TokenArray    = FindType("token[]");
    n->AssetArray    = FindType("asset[]");
    n->Int2Array     = FindType("int2[]");
    n->Int3Array     = FindType("int3[]");
    n->Int4Array     = FindType("int4[]");
    n->Half2Array    = FindType("half2[]");
    n->Half3Array    = FindType("half3[]");
    n->Half4Array    = FindType("half4[]");
    n->Float2Array   = FindType("float2[]");
    n->Float3Array   = FindType("float3[]");
    n->Float4Array   = FindType("float4[]");
    n->Double2Array  = FindType("double2[]");
    n->Double3Array  = FindType("double3[]");
    n->Double4Array  = FindType("double4[]");
    n->Point3hArray  = FindType("point3h[]");
    n->Point3fArray  = FindType("point3f[]");
    n->Point3dArray  = FindType("point3d[]");
    n->Vector3hArray = FindType("vector3h[]");
    n->Vector3fArray = FindType("vector3f[]");
    n->Vector3dArray = FindType("vector3d[]");
    n->Normal3hArray = FindType("normal3h[]");
    n->Normal3fArray = FindType("normal3f[]");
    n->Normal3dArray = FindType("normal3d[]");
    n->Color3hArray  = FindType("color3h[]");
    n->Color3fArray  = FindType("color3f[]");
    n->Color3dArray  = FindType("color3d[]");
    n->Color4hArray  = FindType("color4h[]");
    n->Color4fArray  = FindType("color4f[]");
    n->Color4dArray  = FindType("color4d[]");
    n->QuathArray    = FindType("quath[]");
    n->QuatfArray    = FindType("quatf[]");
    n->QuatdArray    = FindType("quatd[]");
    n->Matrix2dArray = FindType("matrix2d[]");
    n->Matrix3dArray = FindType("matrix3d[]");
    n->Matrix4dArray = FindType("matrix4d[]");
    n->Frame4dArray  = FindType("frame4d[]");
    n->TexCoord2fArray = FindType("texCoord2f[]");
    n->TexCoord2dArray = FindType("texCoord2d[]");
    n->TexCoord2hArray = FindType("texCoord2h[]");
    n->TexCoord3fArray = FindType("texCoord3f[]");
    n->TexCoord3dArray = FindType("texCoord3d[]");
    n->TexCoord3hArray = FindType("texCoord3h[]");

    return n;
}

PXR_NAMESPACE_CLOSE_SCOPE
