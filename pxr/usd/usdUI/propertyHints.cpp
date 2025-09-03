//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usdUI/propertyHints.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdUIPropertyHints::UsdUIPropertyHints() = default;

UsdUIPropertyHints::UsdUIPropertyHints(const UsdProperty& prop)
    : UsdUIObjectHints(prop),
      _prop(prop)
{
}

std::string
UsdUIPropertyHints::GetDisplayGroup() const
{
    if (!_prop) {
        return {};
    }

    std::string group;
    if (_prop.GetMetadataByDictKey(
            UsdUIHintKeys->UIHints,
            UsdUIHintKeys->DisplayGroup, &group)) {
        return group;
    }

    // XXX: Fall back to legacy field
    return _prop.GetDisplayGroup();
}

bool
UsdUIPropertyHints::SetDisplayGroup(const std::string& group)
{
    if (!_prop) {
        TF_CODING_ERROR("Invalid property");
        return false;
    }

    return _prop.SetMetadataByDictKey(
        UsdUIHintKeys->UIHints,
        UsdUIHintKeys->DisplayGroup,
        group);
}

std::string
UsdUIPropertyHints::GetShownIf() const
{
    if (!_prop) {
        return {};
    }

    std::string shownIf;
    if (_prop.GetMetadataByDictKey(
            UsdUIHintKeys->UIHints,
            UsdUIHintKeys->ShownIf,
            &shownIf)) {
        return shownIf;
    }

    return {};
}

bool
UsdUIPropertyHints::SetShownIf(const std::string& shownIf)
{
    if (!_prop) {
        TF_CODING_ERROR("Invalid property");
        return false;
    }

    return _prop.SetMetadataByDictKey(
        UsdUIHintKeys->UIHints,
        UsdUIHintKeys->ShownIf,
        shownIf);
}

PXR_NAMESPACE_CLOSE_SCOPE
