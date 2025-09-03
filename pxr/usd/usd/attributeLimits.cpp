//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usd/attributeLimits.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdLimitsKeys, USD_LIMITS_KEYS);

UsdAttributeLimits::UsdAttributeLimits(
    const UsdAttribute& attr,
    const TfToken& subDictKey)
    : _attr(attr),
      _subDictKey(subDictKey)
{
}

bool
UsdAttributeLimits::IsValid() const
{
    return _attr && !_subDictKey.IsEmpty();
}

UsdAttribute
UsdAttributeLimits::GetAttribute() const
{
    return _attr;
}

TfToken
UsdAttributeLimits::GetSubDictKey() const
{
    return _subDictKey;
}

bool
UsdAttributeLimits::HasAuthored() const
{
    if (!IsValid()) {
        return false;
    }

    return _attr.HasAuthoredMetadataDictKey(
        SdfFieldKeys->Limits, _subDictKey);
}

bool
UsdAttributeLimits::Clear()
{
    if (!IsValid()) {
        TF_CODING_ERROR("Invalid limits");
        return false;
    }

    return _attr.ClearMetadataByDictKey(
        SdfFieldKeys->Limits, _subDictKey);
}

bool
UsdAttributeLimits::HasAuthored(const TfToken& key) const
{
    if (!IsValid() || key.IsEmpty()) {
        return false;
    }

    return _attr.HasAuthoredMetadataDictKey(
        SdfFieldKeys->Limits,
        _MakeKeyPath(_subDictKey, key));
}

bool
UsdAttributeLimits::Clear(const TfToken& key)
{
    if (!IsValid()) {
        TF_CODING_ERROR("Invalid limits");
        return false;
    }

    if (key.IsEmpty()) {
        TF_CODING_ERROR("Empty key");
        return false;
    }

    return _attr.ClearMetadataByDictKey(
        SdfFieldKeys->Limits,
        _MakeKeyPath(_subDictKey, key));
}

bool
UsdAttributeLimits::HasAuthoredMinimum() const
{
    return HasAuthored(UsdLimitsKeys->Minimum);
}

bool
UsdAttributeLimits::ClearMinimum()
{
    if (!IsValid()) {
        TF_CODING_ERROR("Invalid limits");
        return false;
    }

    return Clear(UsdLimitsKeys->Minimum);
}

bool
UsdAttributeLimits::HasAuthoredMaximum() const
{
    return HasAuthored(UsdLimitsKeys->Maximum);
}

bool
UsdAttributeLimits::ClearMaximum()
{
    if (!IsValid()) {
        TF_CODING_ERROR("Invalid limits");
        return false;
    }

    return Clear(UsdLimitsKeys->Maximum);
}

bool
UsdAttributeLimits::Validate(
    const VtDictionary& subDict,
    ValidationResult* result) const
{
    if (!IsValid()) {
        return false;
    }

    // Verify that min/max entries hold our value type, or can be casted to it
    const VtValue typeDefault = _attr.GetTypeName().GetDefaultValue();
    VtDictionary invalidValues;
    VtDictionary conformedSubDict = subDict;

    for (const auto& it : subDict) {
        const std::string& key = it.first;
        const VtValue& value = it.second;

        // Only validate min/max entries
        if (key != UsdLimitsKeys->Minimum &&
            key != UsdLimitsKeys->Maximum) {
            continue;
        }

        const VtValue cast = VtValue::CastToTypeOf(value, typeDefault);

        if (cast.IsEmpty()) {
            invalidValues[key] = value;
        }
        else {
            conformedSubDict[key] = cast;
        }
    }

    const bool success = invalidValues.empty();

    if (result) {
        *result = ValidationResult(
            success,
            invalidValues,
            success ? conformedSubDict : VtDictionary(),
            _attr.GetPath(),
            _attr.GetTypeName().GetType().GetTypeName());
    }

    return success;
}

bool
UsdAttributeLimits::Set(const VtDictionary& subDict)
{
    if (!IsValid()) {
        TF_CODING_ERROR("Invalid limits");
        return false;
    }

    ValidationResult result;
    if (!Validate(subDict, &result)) {
        TF_CODING_ERROR(result.GetErrorString());
        return false;
    }

    return _attr.SetMetadataByDictKey(
        SdfFieldKeys->Limits, _subDictKey,
        VtValue(result.GetConformedSubDict()));
}

VtValue
UsdAttributeLimits::Get(const TfToken& key) const
{
    if (!IsValid() || key.IsEmpty()) {
        return {};
    }

    VtValue value;
    if (_attr.GetMetadataByDictKey(
          SdfFieldKeys->Limits,
          _MakeKeyPath(_subDictKey, key),
          &value)) {
        return value;
    }
    return {};
}

bool
UsdAttributeLimits::Set(
    const TfToken& key,
    const VtValue& value)
{
    if (!IsValid()) {
        TF_CODING_ERROR("Invalid limits");
        return false;
    }

    if (key.IsEmpty()) {
        TF_CODING_ERROR("Empty key");
        return false;
    }

    if ((key == UsdLimitsKeys->Minimum || key == UsdLimitsKeys->Maximum) &&
         value.GetType() != _attr.GetTypeName().GetType()) {
        TF_CODING_ERROR(
            "Unexpected limits value type (%s) for attribute '%s' "
            "(expected %s)",
            value.GetTypeName().c_str(),
            _attr.GetPath().GetText(),
            _attr.GetTypeName().GetType().GetTypeName().c_str());
        return false;
    }

    return _attr.SetMetadataByDictKey(
        SdfFieldKeys->Limits,
        _MakeKeyPath(_subDictKey, key),
        value);
}

VtValue
UsdAttributeLimits::GetMinimum() const
{
    return Get(UsdLimitsKeys->Minimum);
}

bool
UsdAttributeLimits::SetMinimum(const VtValue& value)
{
    return Set(UsdLimitsKeys->Minimum, value);
}

VtValue
UsdAttributeLimits::GetMaximum() const
{
    return Get(UsdLimitsKeys->Maximum);
}

bool
UsdAttributeLimits::SetMaximum(const VtValue& value)
{
    return Set(UsdLimitsKeys->Maximum, value);
}

/* static */
TfToken
UsdAttributeLimits::_MakeKeyPath(
    const TfToken& key1,
    const TfToken& key2)
{
    if (key1.IsEmpty()) {
        return key2;
    }

    if (key2.IsEmpty()) {
        return key1;
    }

    return TfToken(
        TfStringPrintf("%s%c%s",
                       key1.GetText(),
                       UsdObject::GetNamespaceDelimiter(),
                       key2.GetText()));
}

std::string
UsdAttributeLimits::ValidationResult::GetErrorString() const
{
    if (_invalidValuesDict.empty()) {
        return {};
    }

    std::string keysAndTypes;
    for (const auto& it : _invalidValuesDict) {
        if (!keysAndTypes.empty()) {
            keysAndTypes += ", ";
        }
        keysAndTypes +=
            TfStringPrintf("%s (%s)",
                it.first.c_str(),
                it.second.GetTypeName().c_str());
    }

    return TfStringPrintf(
        "%zu limits value key(s) have an unexpected type for attribute <%s> "
        "(expected %s): %s",
        _invalidValuesDict.size(),
        _attrPath.GetText(),
        _attrTypeName.c_str(),
        keysAndTypes.c_str());
}

PXR_NAMESPACE_CLOSE_SCOPE
