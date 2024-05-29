//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdUtils/registeredVariantSet.h"
#include "pxr/pxr.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    // lowerCamelCase of enums
    (never)
    (ifAuthored)
    (always)
);

// static
bool
UsdUtilsRegisteredVariantSet::GetSelectionExportPolicyFromString(
    const std::string& selectionExportPolicyStr,
    SelectionExportPolicy* outSelectionExportPolicy)
{
    SelectionExportPolicy selectionExportPolicy;
    if (selectionExportPolicyStr == _tokens->never) {
        selectionExportPolicy
            = UsdUtilsRegisteredVariantSet::SelectionExportPolicy::Never;
    }
    else if (selectionExportPolicyStr == _tokens->ifAuthored) {
        selectionExportPolicy
            = UsdUtilsRegisteredVariantSet::SelectionExportPolicy::IfAuthored;
    }
    else if (selectionExportPolicyStr == _tokens->always) {
        selectionExportPolicy
            = UsdUtilsRegisteredVariantSet::SelectionExportPolicy::Always;
    }
    else {
        return false;
    }

    if (outSelectionExportPolicy) {
        *outSelectionExportPolicy = selectionExportPolicy;
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
