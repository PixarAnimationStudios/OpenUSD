//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/ostreamMethods.h"

PXR_NAMESPACE_OPEN_SCOPE


UsdStageWeakPtr
UsdObject::GetStage() const
{
    return TfCreateWeakPtr(_GetStage());
}

// ------------------------------------------------------------------------- //
// Metadata IO
// ------------------------------------------------------------------------- //

bool
UsdObject::GetMetadata(const TfToken& key, VtValue* value) const
{
    return _GetMetadataImpl(key, value);
}

bool
UsdObject::GetMetadataByDictKey(
    const TfToken& key, const TfToken &keyPath, VtValue* value) const
{
    return _GetMetadataImpl(key, value, keyPath);
}

bool
UsdObject::_GetMetadataImpl(
    const TfToken& key, VtValue* value, const TfToken &keyPath) const
{
    return _GetStage()->_GetMetadata(
        *this, key, keyPath, /*useFallbacks=*/true, value);
}

bool 
UsdObject::SetMetadata(const TfToken& key, const VtValue& value) const
{
    return _SetMetadataImpl(key, value);
}

bool
UsdObject::SetMetadataByDictKey(
        const TfToken& key, const TfToken &keyPath, const VtValue& value) const
{
    return _SetMetadataImpl(key, value, keyPath);
}

bool 
UsdObject::_SetMetadataImpl(const TfToken& key,
                            const VtValue& value,
                            const TfToken &keyPath) const
{
    return _GetStage()->_SetMetadata(*this, key, keyPath, value);
}

bool
UsdObject::ClearMetadata(const TfToken& key) const
{
    return _GetStage()->_ClearMetadata(*this, key);
}

bool
UsdObject::ClearMetadataByDictKey(
    const TfToken& key, const TfToken &keyPath) const
{
    return _GetStage()->_ClearMetadata(*this, key, keyPath);
}

bool
UsdObject::HasMetadata(const TfToken& key) const
{
    return _GetStage()->_HasMetadata(*this, key, TfToken(),
                                     /*useFallbacks*/ true);
}

bool
UsdObject::HasAuthoredMetadata(const TfToken& key) const
{
    return _GetStage()->_HasMetadata(*this, key, TfToken(),
                                     /*useFallbacks*/ false);
}

bool
UsdObject::HasMetadataDictKey(const TfToken& key, const TfToken &keyPath) const
{
    return _GetStage()->_HasMetadata(*this, key, keyPath,
                                     /*useFallbacks*/ true);
}

bool
UsdObject::HasAuthoredMetadataDictKey(
    const TfToken& key, const TfToken &keyPath) const
{
    return _GetStage()->_HasMetadata(*this, key, keyPath,
                                     /*useFallbacks*/ false);
}

UsdMetadataValueMap
UsdObject::GetAllMetadata() const
{
    UsdMetadataValueMap result;
    _GetStage()->_GetAllMetadata(*this, /*useFallbacks=*/true, &result);
    return result;
}

UsdMetadataValueMap
UsdObject::GetAllAuthoredMetadata() const
{
    UsdMetadataValueMap result;
    _GetStage()->_GetAllMetadata(*this, /*useFallbacks=*/false, &result);
    return result;
}

// ------------------------------------------------------------------------- //
// 'customData' Metadata
// ------------------------------------------------------------------------- //
VtDictionary
UsdObject::GetCustomData() const
{
    VtDictionary dict;
    GetMetadata(SdfFieldKeys->CustomData, &dict);
    return dict;
}

VtValue
UsdObject::GetCustomDataByKey(const TfToken &keyPath) const
{
    VtValue val;
    GetMetadataByDictKey(SdfFieldKeys->CustomData, keyPath, &val);
    return val;
}

void
UsdObject::SetCustomData(const VtDictionary &customData) const
{
    SetMetadata(SdfFieldKeys->CustomData, customData);
}

void
UsdObject::SetCustomDataByKey(
    const TfToken &keyPath, const VtValue &value) const
{
    SetMetadataByDictKey(SdfFieldKeys->CustomData, keyPath, value);
}

void
UsdObject::ClearCustomData() const
{
    ClearMetadata(SdfFieldKeys->CustomData);
}

void
UsdObject::ClearCustomDataByKey(const TfToken &keyPath) const
{
    ClearMetadataByDictKey(SdfFieldKeys->CustomData, keyPath);
}

bool
UsdObject::HasCustomData() const
{
    return HasMetadata(SdfFieldKeys->CustomData);
}

bool
UsdObject::HasCustomDataKey(const TfToken &keyPath) const
{
    return HasMetadataDictKey(SdfFieldKeys->CustomData, keyPath);
}

bool
UsdObject::HasAuthoredCustomData() const
{
    return HasAuthoredMetadata(SdfFieldKeys->CustomData);
}

bool
UsdObject::HasAuthoredCustomDataKey(const TfToken &keyPath) const
{
    return HasAuthoredMetadataDictKey(SdfFieldKeys->CustomData, keyPath);
}

// ------------------------------------------------------------------------- //
// 'assetInfo' Metadata
// ------------------------------------------------------------------------- //
VtDictionary
UsdObject::GetAssetInfo() const
{
    VtDictionary dict;
    GetMetadata(SdfFieldKeys->AssetInfo, &dict);
    return dict;
}

VtValue
UsdObject::GetAssetInfoByKey(const TfToken &keyPath) const
{
    VtValue val;
    GetMetadataByDictKey(SdfFieldKeys->AssetInfo, keyPath, &val);
    return val;
}

void
UsdObject::SetAssetInfo(const VtDictionary &customData) const
{
    SetMetadata(SdfFieldKeys->AssetInfo, customData);
}

void
UsdObject::SetAssetInfoByKey(
    const TfToken &keyPath, const VtValue &value) const
{
    SetMetadataByDictKey(SdfFieldKeys->AssetInfo, keyPath, value);
}

void
UsdObject::ClearAssetInfo() const
{
    ClearMetadata(SdfFieldKeys->AssetInfo);
}

void
UsdObject::ClearAssetInfoByKey(const TfToken &keyPath) const
{
    ClearMetadataByDictKey(SdfFieldKeys->AssetInfo, keyPath);
}

bool
UsdObject::HasAssetInfo() const
{
    return HasMetadata(SdfFieldKeys->AssetInfo);
}

bool
UsdObject::HasAssetInfoKey(const TfToken &keyPath) const
{
    return HasMetadataDictKey(SdfFieldKeys->AssetInfo, keyPath);
}

bool
UsdObject::HasAuthoredAssetInfo() const
{
    return HasAuthoredMetadata(SdfFieldKeys->AssetInfo);
}

bool
UsdObject::HasAuthoredAssetInfoKey(const TfToken &keyPath) const
{
    return HasAuthoredMetadataDictKey(SdfFieldKeys->AssetInfo, keyPath);
}


// ------------------------------------------------------------------------- //
// 'Hidden' Metadata
// ------------------------------------------------------------------------- //

bool
UsdObject::IsHidden() const
{
    bool hidden = false;
    GetMetadata(SdfFieldKeys->Hidden, &hidden);
    return hidden;
}

bool
UsdObject::SetHidden(bool hidden) const
{
    return SetMetadata(SdfFieldKeys->Hidden, hidden);
}

bool
UsdObject::ClearHidden() const
{
    return ClearMetadata(SdfFieldKeys->Hidden);
}

bool
UsdObject::HasAuthoredHidden() const
{
    return HasAuthoredMetadata(SdfFieldKeys->Hidden);
}


// ------------------------------------------------------------------------- //
// 'Documentation' Metadata
// ------------------------------------------------------------------------- //

std::string
UsdObject::GetDocumentation() const
{
    std::string documentation;
    GetMetadata(SdfFieldKeys->Documentation, &documentation);
    return documentation;
}

bool
UsdObject::SetDocumentation(const std::string& documentation) const
{
    return SetMetadata(SdfFieldKeys->Documentation, documentation);
}

bool
UsdObject::ClearDocumentation() const
{
    return ClearMetadata(SdfFieldKeys->Documentation);
}

bool
UsdObject::HasAuthoredDocumentation() const
{
    return HasAuthoredMetadata(SdfFieldKeys->Documentation);
}

// ------------------------------------------------------------------------- //
// 'DisplayName' Metadata
// ------------------------------------------------------------------------- //

std::string
UsdObject::GetDisplayName() const
{
    std::string result;
    GetMetadata(SdfFieldKeys->DisplayName, &result);
    return result;
}

bool
UsdObject::SetDisplayName(const std::string& newDisplayName) const
{
    return SetMetadata(SdfFieldKeys->DisplayName, newDisplayName);
}

bool
UsdObject::ClearDisplayName() const
{
    return ClearMetadata(SdfFieldKeys->DisplayName);
}

bool
UsdObject::HasAuthoredDisplayName() const
{
    return HasAuthoredMetadata(SdfFieldKeys->DisplayName);
}

SdfSpecType
UsdObject::_GetDefiningSpecType() const
{
    return _GetStage()->_GetDefiningSpecType(get_pointer(_Prim()), _propName);
}

std::string
UsdObject::_GetObjectDescription(const std::string &preface) const
{
    if (_type == UsdTypePrim) {
        return _Prim().GetDescription(_ProxyPrimPath());
    } else if (_type == UsdTypeAttribute) {
        return TfStringPrintf("%sattribute '%s' on ", 
                              preface.c_str(),
                              _PropName().GetText()) +
            _Prim().GetDescription(_ProxyPrimPath());
    } else if (_type == UsdTypeRelationship) {
        return TfStringPrintf("%srelationship '%s' on ",
                              preface.c_str(),
                              _PropName().GetText()) +
            _Prim().GetDescription(_ProxyPrimPath());
    } else if (_type == UsdTypeProperty) {
        return TfStringPrintf("%sproperty '%s' on ",
                              preface.c_str(),
                              _PropName().GetText()) +
            _Prim().GetDescription(_ProxyPrimPath());
    } else if (_type == UsdTypeObject) {
        return _Prim().GetDescription(_ProxyPrimPath());
    }

    return TfStringPrintf("Unknown object type %d", _type);
}

std::string 
UsdObject::GetDescription() const
{
    return _GetObjectDescription("");
}

std::string
UsdDescribe(const UsdObject &obj) {
    return obj.GetDescription();
}

PXR_NAMESPACE_CLOSE_SCOPE

