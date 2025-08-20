//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usdUI/attributeHints.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdUIAttributeHints::UsdUIAttributeHints() = default;

UsdUIAttributeHints::UsdUIAttributeHints(const UsdAttribute& attr)
    : UsdUIPropertyHints(attr),
      _attr(attr)
{
}

VtDictionary
UsdUIAttributeHints::GetValueLabels() const
{
    if (!_attr) {
        return {};
    }

    VtDictionary labels;
    if (_attr.GetMetadataByDictKey(
            UsdUIHintKeys->UIHints,
            UsdUIHintKeys->ValueLabels,
            &labels)) {
        return labels;
    }

    return {};
}

bool
UsdUIAttributeHints::SetValueLabels(const VtDictionary& labels)
{
    if (!_attr) {
        TF_CODING_ERROR("Invalid attribute");
        return false;
    }

    return _attr.SetMetadataByDictKey(
        UsdUIHintKeys->UIHints,
        UsdUIHintKeys->ValueLabels,
        labels);
}

VtTokenArray
UsdUIAttributeHints::GetValueLabelsOrder() const
{
    if (!_attr) {
        return {};
    }

    VtTokenArray order;
    if (_attr.GetMetadataByDictKey(
            UsdUIHintKeys->UIHints,
            UsdUIHintKeys->ValueLabelsOrder,
            &order)) {
        return order;
    }

    return {};
}

bool
UsdUIAttributeHints::SetValueLabelsOrder(const VtTokenArray& order)
{
    if (!_attr) {
        TF_CODING_ERROR("Invalid attribute");
        return false;
    }

    return _attr.SetMetadataByDictKey(
        UsdUIHintKeys->UIHints,
        UsdUIHintKeys->ValueLabelsOrder,
        order);
}

bool
UsdUIAttributeHints::ApplyValueLabel(const std::string& label)
{
    if (!_attr) {
        TF_CODING_ERROR("Invalid attribute");
        return false;
    }

    const TfToken keyPath =
        _MakeKeyPath(UsdUIHintKeys->ValueLabels, TfToken(label));
    VtValue value;

    if (_attr.GetMetadataByDictKey(
            UsdUIHintKeys->UIHints, keyPath, &value)) {
        return _attr.Set(value);
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
