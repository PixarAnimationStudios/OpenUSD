//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdValidation/usdValidation/fixer.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/editTarget.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdValidationFixer::UsdValidationFixer(
    const TfToken &name, const std::string &description, 
    const FixerImplFn &fixerImplFn, const FixerCanApplyFn &canApplyFn,
    const TfTokenVector &keywords,
    const TfToken &errorName) : 
    _name(name), _description(description), _fixerImplFn(fixerImplFn),
    _canApplyFn(canApplyFn), _keywords(keywords), 
    _errorName(errorName)
{
}

bool
UsdValidationFixer::IsAssociatedWithErrorName(const TfToken &errorName) const
{
    return _errorName.IsEmpty() || _errorName == errorName;
}

bool
UsdValidationFixer::HasKeyword(const TfToken &keyword) const
{
    return std::find(_keywords.begin(), _keywords.end(), keyword) !=
        _keywords.end();
}

bool
UsdValidationFixer::CanApplyFix(
    const UsdValidationError& error, const UsdEditTarget &editTarget,
    const UsdTimeCode &timeCode) const
{
    if (!IsAssociatedWithErrorName(error.GetName())) {
        return false;
    }

    if (!_canApplyFn) {
        return false;
    }

    if (!editTarget.IsValid()) {
        return false;
    }

    return _canApplyFn(error, editTarget, timeCode);
}

bool
UsdValidationFixer::ApplyFix(
    const UsdValidationError& error, const UsdEditTarget &editTarget,
    const UsdTimeCode &timeCode) const
{
    if (!IsAssociatedWithErrorName(error.GetName())) {
        return false;
    }

    if (!_fixerImplFn) {
        return false;
    }

    if (!editTarget.IsValid()) {
        return false;
    }

    if (!_fixerImplFn(error, editTarget, timeCode)) {
        return false;
    }

    const SdfLayerHandle &layer = editTarget.GetLayer();
    if (!TF_VERIFY(layer, "Fixer '%s' released its edit target's layer",
                   _name.GetText())) {
        return false;
    }

    // Anonymous layers have no backing asset to save to, so the fix is
    // complete once it has been authored.
    if (layer->IsAnonymous()) {
        return true;
    }

    return layer->Save();
}

PXR_NAMESPACE_CLOSE_SCOPE
