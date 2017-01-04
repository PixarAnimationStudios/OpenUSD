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
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/specType.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/cleanupTracker.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/type.h"
#include <boost/python/extract.hpp>
#include <boost/python/object.hpp>
#include <map>

SDF_DEFINE_BASE_SPEC(SdfSpec);

//
// SdfSpec
//

SdfSpec &SdfSpec::operator=(const SdfSpec &other)
{
    _id = other._id;
    return *this;
}

SdfSpec::~SdfSpec()
{
}

const SdfSchemaBase& 
SdfSpec::GetSchema() const
{
    return _id->GetLayer()->GetSchema();
}

SdfLayerHandle
SdfSpec::GetLayer() const
{
    return _id ? _id->GetLayer() : SdfLayerHandle();
}

SdfPath
SdfSpec::GetPath() const
{
    return _id ? _id->GetPath() : SdfPath();
}

SdfSpecType
SdfSpec::GetSpecType() const
{
    // We can't retrieve an object type for a dormant spec.
    if (!_id || !_id->GetLayer()) {
	return SdfSpecTypeUnknown;
    }
    const SdfPath & path = _id->GetPath();
    if (path.IsEmpty()) {
	return SdfSpecTypeUnknown;
    }
    return GetLayer()->GetSpecType(path);
}

bool
SdfSpec::IsDormant() const
{
    // If we have no id, we're dormant.
    if (!_id)
        return true;

    // If our path is invalid, we must be dormant.
    SdfPath const &path = _id->GetPath();
    if (path.IsEmpty())
        return true;

    // If our layer is invalid, we're dormant.  Otherwise we're dormant if the
    // layer has no spec at this path.
    SdfLayerHandle const &layer = _id->GetLayer();
    return ARCH_UNLIKELY(!layer) || !layer->HasSpec(path);
}

bool
SdfSpec::PermissionToEdit() const
{
    return _id ? GetLayer()->PermissionToEdit() : false;
}

std::vector<TfToken>
SdfSpec::ListFields() const
{
    if (!_id) {
        return std::vector<TfToken>();
    }
    const SdfPath & path = _id->GetPath();
    return GetLayer()->ListFields(path);
}

bool 
SdfSpec::_HasField(const TfToken& name, SdfAbstractDataValue* value) const
{
    return _id ? _id->GetLayer()->HasField(_id->GetPath(), name, value) : false;
}

bool
SdfSpec::HasField(const TfToken &name) const
{
    return _id ? _id->GetLayer()->HasField(_id->GetPath(), name) : false;
}

VtValue
SdfSpec::GetField(const TfToken &name) const
{
    return _id ? _id->GetLayer()->GetField(_id->GetPath(), name) : VtValue();
}

bool
SdfSpec::SetField(const TfToken & name, const VtValue& value)
{
    if (!_id) {
	return false;
    }
    _id->GetLayer()->SetField(_id->GetPath(), name, value);
    return true;
}

bool
SdfSpec::ClearField(const TfToken & name)
{
    if (!_id) {
	return false;
    }

    _id->GetLayer()->EraseField(_id->GetPath(), name);
    return true;
}

std::vector<TfToken>
SdfSpec::GetMetaDataInfoKeys() const
{
    return GetSchema().GetMetadataFields(GetSpecType());
}

TfToken
SdfSpec::GetMetaDataDisplayGroup(TfToken const &key) const
{
    if (const SdfSchema::SpecDefinition* specDef =
        GetSchema().GetSpecDefinition(GetSpecType())) {
        return specDef->GetMetadataFieldDisplayGroup(key);
    }
    return TfToken();
}

std::vector<TfToken>
SdfSpec::ListInfoKeys() const
{
    const SdfSchemaBase& schema = GetSchema();
    if (const SdfSchema::SpecDefinition* specDef = 
        schema.GetSpecDefinition(GetSpecType())) {

        std::vector<TfToken> result;
        const std::vector<TfToken> valueFields = specDef->GetFields();
        TF_FOR_ALL(field, valueFields) {
            // Skip fields holding children.
            if (const SdfSchema::FieldDefinition* fieldDef =
                schema.GetFieldDefinition(*field)) {
                if (fieldDef->HoldsChildren())
                    continue;
            }

            if (HasInfo(*field)) {
                result.push_back(*field);
            }
        }

        return result;
    }

    return std::vector<TfToken>();
}

// Helper function that performs some common checks to determine if a given
// field is editable via the Info API.
static
bool
_CanEditInfoOnSpec(
    const TfToken& key, SdfSpecType specType,
    const SdfSchemaBase& schema,
    const SdfSchema::FieldDefinition* fieldDef,
    const char* editType)
{
    if (!fieldDef) {
        TF_CODING_ERROR("Cannot %s value for unknown field '%s'",
                        editType, key.GetText());
        return false;
    }

    if (fieldDef->IsReadOnly()) {
        TF_CODING_ERROR("Cannot %s value for read-only field '%s'",
                        editType, key.GetText());
        return false;
    }

    if (!schema.IsValidFieldForSpec(fieldDef->GetName(), specType)) {
        TF_CODING_ERROR("Field '%s' is not valid for spec type %s",
                        key.GetText(),
                        TfStringify(specType).c_str());
        return false;
    }

    return true;
}

void
SdfSpec::SetInfo( const TfToken & key, const VtValue &value )
{
    // Perform some validation on the field being modified to ensure we don't
    // author any invalid scene description. Note this function will issue
    // coding errors as needed.
    const SdfSchemaBase& schema = GetSchema();
    const SdfSchema::FieldDefinition* fieldDef = schema.GetFieldDefinition(key);
    if (!_CanEditInfoOnSpec(key, GetSpecType(), schema, fieldDef, "set")) {
        return;
    }

    // Attempt to cast the given value to the type specified for the field
    // in the schema.
    const VtValue fallback = fieldDef->GetFallbackValue();
    const VtValue castValue =
        (fallback.IsEmpty() ? value : VtValue::CastToTypeOf(value, fallback));
    if (castValue.IsEmpty()) {
        TF_CODING_ERROR("Cannot set field '%s' of type '%s' to provided value "
                        "'%s' because the value is an incompatible type '%s', "
                        "on spec <%s>",
                        key.GetText(),
                        fallback.GetTypeName().c_str(),
                        TfStringify(value).c_str(),
                        value.GetTypeName().c_str(),
                        GetPath().GetString().c_str());
        return;
    }

    // XXX:
    // There is a hole here that could lead to problems. For fields whose
    // value types are container-ish (e.g., SdfListOp, std::vector, std::map)
    // Sd tries to detect if the container is empty and clears the field
    // if it is. We don't (yet) have a great way to detect this situation
    // here.
    //
    // The reason this can lead to problems is because HasInfo(...) relies
    // solely on whether a field has been set. So, we might run into cases
    // like this:
    //
    // prim.SetInfo('nameChildrenOrder', [])
    // prim.HasInfo('nameChildrenOrder')
    //     --> True, even though this ought to return False because there aren't
    //         any name children specified.
    //
    // If this becomes a problem, the interim workaround is to use ClearInfo
    // to clear a field instead of setting it to an empty value.

    SetField(key, castValue);
}

void
SdfSpec::SetInfoDictionaryValue(const TfToken &dictionaryKey,
                               const TfToken &entryKey, const VtValue &value)
{
    // XXX: Instead of copying, modifying, then re-setting the dictionary,
    //      could this use the proxy to edit the dictionary directly?
    VtDictionary dict = SdfDictionaryProxy(SdfCreateHandle(this), dictionaryKey);
    if (value.IsEmpty()) {
        dict.erase(entryKey);
    } else {
        dict[entryKey] = value;
    }
    SetInfo(dictionaryKey, VtValue(dict));
}

bool
SdfSpec::HasInfo( const TfToken & key ) const
{
    // It's not an error to call this method with a key that isn't registered
    // with the schema. The file writer needs to be able to query for the
    // presence of metadata fields registered via plugins, such as wizardData.
    // It might be the case that no plugins are registered when the file writer
    // is called, so Plug won't know about the plugin that defines wizardData
    // and wizardData will not be in the schema. We still want to be able to
    // write out files in this case.
    return HasField(key);
}

void
SdfSpec::ClearInfo( const TfToken & key )
{
    // Perform some validation to ensure we allow the clearing of this field
    // via the Info API. Note this function will issue coding errors as needed.
    const SdfSchemaBase& schema = GetSchema();
    const SdfSchema::FieldDefinition* fieldDef = schema.GetFieldDefinition(key);
    if (!_CanEditInfoOnSpec(key, GetSpecType(), schema, fieldDef, "clear")) {
        return;
    }

    SdfChangeBlock block;

    ClearField(key);

    // In case this spec is made inert when the info is removed, schedule it to
    // be cleaned up (if the caller has enabled cleanup tracking)
    Sdf_CleanupTracker::GetInstance().AddSpecIfTracking(*this);
}

TfType
SdfSpec::GetTypeForInfo( const TfToken & key ) const
{
    return GetSchema().GetFallback(key).GetType();
}

const VtValue&
SdfSpec::GetFallbackForInfo( const TfToken & key ) const
{
    static VtValue empty;

    const SdfSchemaBase& schema = GetSchema();
    const SdfSchema::FieldDefinition* def = schema.GetFieldDefinition(key);
    if (!def) {
        TF_CODING_ERROR("Unknown field '%s'", key.GetText());
        return empty;
    }

    const SdfSpecType objType = GetSpecType();
    const SdfSchema::SpecDefinition* specDef =
        schema.GetSpecDefinition(objType);
    if (!specDef || !specDef->IsMetadataField(key)) {
        TF_CODING_ERROR("Non-metadata key '%s' for type %s",
            key.GetText(), TfStringify(objType).c_str());
        return empty;
    }

    return def->GetFallbackValue();
}

bool
SdfSpec::WriteToStream(std::ostream& out, size_t indent) const
{
    return GetLayer()->GetFileFormat()->WriteToStream(*this, out, indent);
}

bool
SdfSpec::IsInert(bool ignoreChildren) const
{
    return _id ? GetLayer()->_IsInert(_id->GetPath(), ignoreChildren)
        : false;
}

VtValue
SdfSpec::GetInfo(const TfToken& key) const
{
    const SdfSchema::FieldDefinition* def = GetSchema().GetFieldDefinition(key);
    if (!def) {
        TF_CODING_ERROR("Invalid info key: %s",key.GetText());
        return VtValue();
    }

    VtValue value = GetField(key);
    return !value.IsEmpty() ? value : def->GetFallbackValue();
}

bool
SdfSpec::operator==(const SdfSpec& rhs) const
{
    return (_id == rhs._id);
}

bool
SdfSpec::operator<(const SdfSpec& rhs) const
{
    return (_id < rhs._id);
}

bool
SdfSpec::_MoveSpec(const SdfPath &oldPath, const SdfPath &newPath) const
{
    return GetLayer()->_MoveSpec(oldPath, newPath);
}

bool
SdfSpec::_DeleteSpec(const SdfPath &path)
{
    return GetLayer()->_DeleteSpec(path);
}
