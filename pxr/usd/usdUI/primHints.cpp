//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usdUI/primHints.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdUIPrimHints::UsdUIPrimHints() = default;

UsdUIPrimHints::UsdUIPrimHints(const UsdPrim& prim)
    : UsdUIObjectHints(prim),
      _prim(prim)
{
}

VtDictionary
UsdUIPrimHints::GetDisplayGroupsExpanded() const
{
    if (!_prim) {
        return {};
    }

    VtDictionary dict;
    if (_prim.GetMetadataByDictKey(
            UsdUIHintKeys->UIHints,
            UsdUIHintKeys->DisplayGroupsExpanded,
            &dict)) {
        return dict;
    }

    return {};
}

bool
UsdUIPrimHints::SetDisplayGroupsExpanded(const VtDictionary& expanded)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    // Verify all entires hold bool values
    for (const auto& it : expanded) {
        if (!it.second.IsHolding<bool>()) {
            TF_CODING_ERROR(
                "Unexpected value type (%s) for '%s' (bool)",
                it.second.GetTypeName().c_str(),
                UsdUIHintKeys->DisplayGroupsExpanded.GetText());
            return false;
        }
    }

    return _prim.SetMetadataByDictKey(
        UsdUIHintKeys->UIHints,
        UsdUIHintKeys->DisplayGroupsExpanded,
        expanded);
}

bool
UsdUIPrimHints::GetDisplayGroupExpanded(const std::string& group) const
{
    if (!_prim) {
        return false;
    }

    // Note we don't use GetMetadataByDictKey() here as with the other fields
    // (see further explanation in SetDisplayGroupExpanded())
    const VtDictionary expandedDict = GetDisplayGroupsExpanded();
    return VtDictionaryGet<bool>(expandedDict, group, VtDefault = false);
}

bool
UsdUIPrimHints::SetDisplayGroupExpanded(
    const std::string& group,
    bool expanded)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    // Note we don't use SetMetadataByDictKey() here as with the other fields,
    // since `group` may itself contain colon-delimited subgroups. We want
    // entries for nested groups to live at the top level rather than becoming
    // nested subdicts.
    //
    // e.g., we want:
    //
    //  dictionary displayGroupsExpanded = {
    //      bool "A" = 1
    //      bool "A:B" = 1
    //      bool "A:B:C" = 1
    //  }
    //
    // rather than:
    //
    //  dictionary displayGroupsExpanded = {
    //      dictionary A = {
    //          dictionary B = {
    //              bool C = 1
    //          }
    //      }
    //  }
    VtDictionary expandedDict = GetDisplayGroupsExpanded();
    expandedDict[group] = expanded;
    return SetDisplayGroupsExpanded(expandedDict);
}

VtDictionary
UsdUIPrimHints::GetDisplayGroupsShownIf() const
{
    if (!_prim) {
        return {};
    }

    VtDictionary dict;
    if (_prim.GetMetadataByDictKey(
            UsdUIHintKeys->UIHints,
            UsdUIHintKeys->DisplayGroupsShownIf,
            &dict)) {
        return dict;
    }

    return {};
}

bool
UsdUIPrimHints::SetDisplayGroupsShownIf(const VtDictionary& shownIf)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    // Verify all entires hold string values
    for (const auto& it : shownIf) {
        if (!it.second.IsHolding<std::string>()) {
            TF_CODING_ERROR(
                "Unexpected value type (%s) for '%s' expression: (string)",
                it.second.GetTypeName().c_str(),
                UsdUIHintKeys->DisplayGroupsShownIf.GetText());
            return false;
        }
    }

    return _prim.SetMetadataByDictKey(
        UsdUIHintKeys->UIHints,
        UsdUIHintKeys->DisplayGroupsShownIf,
        shownIf);
}

std::string
UsdUIPrimHints::GetDisplayGroupShownIf(const std::string& group) const
{
    if (!_prim) {
        return {};
    }

    std::string shownIf;
    const TfToken keyPath =
        _MakeKeyPath(UsdUIHintKeys->DisplayGroupsShownIf, TfToken(group));
    if (_prim.GetMetadataByDictKey(
            UsdUIHintKeys->UIHints, keyPath, &shownIf)) {
        return shownIf;
    }

    return {};
}

bool
UsdUIPrimHints::SetDisplayGroupShownIf(
    const std::string& group,
    const std::string& shownIf)
{
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim");
        return false;
    }

    const TfToken keyPath =
        _MakeKeyPath(UsdUIHintKeys->DisplayGroupsShownIf, TfToken(group));
    return _prim.SetMetadataByDictKey(
        UsdUIHintKeys->UIHints, keyPath, shownIf);
}

PXR_NAMESPACE_CLOSE_SCOPE
