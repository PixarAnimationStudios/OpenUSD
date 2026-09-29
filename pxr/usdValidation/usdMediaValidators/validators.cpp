//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdMedia/authorshipAPI.h"
#include "pxr/usdValidation/usdMediaValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"

PXR_NAMESPACE_OPEN_SCOPE

static UsdValidationErrorVector
_ShadowedOrDuplicateAuthorshipValidator(
    const UsdPrim &prim,
    const UsdValidationTimeRange & /*timeRange*/)
{
    UsdValidationErrorVector errors;

    for (const UsdMediaAuthorshipAPI &record :
            UsdMediaAuthorshipAPI::GetShadowed(prim)) {
        errors.emplace_back(
            UsdMediaValidationErrorNameTokens->shadowedAuthorshipApplication,
            UsdValidationErrorType::Warn,
            UsdValidationErrorSites {
                UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
            TfStringPrintf(
                "AuthorshipAPI:%s on <%s> is shadowed by a stronger, "
                "explicit apiSchemas list and is not applied on the "
                "composed prim.",
                record.GetName().GetText(), prim.GetPath().GetText()));
    }

    for (const UsdMediaAuthorshipAPI &record :
            UsdMediaAuthorshipAPI::GetDuplicates(prim)) {
        errors.emplace_back(
            UsdMediaValidationErrorNameTokens->duplicateAuthorshipApplication,
            UsdValidationErrorType::Warn,
            UsdValidationErrorSites {
                UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
            TfStringPrintf(
                "AuthorshipAPI:%s on <%s> is applied in more than one prim "
                "spec; only the strongest opinion for each field survives "
                "composition.",
                record.GetName().GetText(), prim.GetPath().GetText()));
    }

    return errors;
}

// The array length must be a multiple of the tuple-length that the schema's
// arraySizeConstraint declares for inputs.
static UsdValidationErrorVector
_AuthorshipInputsPairedValidator(
    const UsdPrim &prim,
    const UsdValidationTimeRange & /*timeRange*/)
{
    UsdValidationErrorVector errors;

    for (const UsdMediaAuthorshipAPI &record :
            UsdMediaAuthorshipAPI::GetAll(prim)) {
        const UsdAttribute attr = record.GetInputsAttr();
        if (!attr || !attr.HasAuthoredValue()) {
            continue;
        }

        const int64_t tupleLength = -attr.GetArraySizeConstraint();
        VtArray<std::string> inputs;
        if (tupleLength <= 0 || !attr.Get(&inputs) ||
            inputs.size() % static_cast<size_t>(tupleLength) == 0) {
            continue;
        }

        errors.emplace_back(
            UsdMediaValidationErrorNameTokens->unpairedAuthorshipInputs,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(prim.GetStage(), attr.GetPath()) },
            TfStringPrintf(
                "<%s> has %zu element(s), which is not a multiple of its "
                "tuple-length %lld.",
                attr.GetPath().GetText(), inputs.size(),
                static_cast<long long>(tupleLength)));
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    registry.RegisterPluginValidator(
        UsdMediaValidatorNameTokens->shadowedOrDuplicateAuthorship,
        _ShadowedOrDuplicateAuthorshipValidator);
    registry.RegisterPluginValidator(
        UsdMediaValidatorNameTokens->authorshipInputsPaired,
        _AuthorshipInputsPairedValidator);
}

PXR_NAMESPACE_CLOSE_SCOPE
