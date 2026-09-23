//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/propertySpec.h"
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

// The strongest spec providing a value for \p attr, or null.
static SdfPropertySpecHandle
_GetStrongestValueSpec(const UsdAttribute &attr)
{
    for (const SdfPropertySpecHandle &spec : attr.GetPropertyStack()) {
        if (spec && spec->HasDefaultValue()) {
            return spec;
        }
    }
    return SdfPropertySpecHandle();
}

static UsdValidationErrorVector
_AuthorshipInputsPairedValidator(
    const UsdPrim &prim,
    const UsdValidationTimeRange & /*timeRange*/)
{
    UsdValidationErrorVector errors;

    for (const UsdMediaAuthorshipAPI &record :
            UsdMediaAuthorshipAPI::GetAll(prim)) {
        const UsdAttribute namesAttr = record.GetInputNamesAttr();
        const UsdAttribute valuesAttr = record.GetInputValuesAttr();
        const bool hasNames = namesAttr && namesAttr.HasAuthoredValue();
        const bool hasValues = valuesAttr && valuesAttr.HasAuthoredValue();
        if (!hasNames && !hasValues) {
            continue;
        }

        const UsdAttribute &present = hasNames ? namesAttr : valuesAttr;
        const UsdValidationErrorSites sites = {
            UsdValidationErrorSite(prim.GetStage(), present.GetPath()) };

        if (hasNames != hasValues) {
            errors.emplace_back(
                UsdMediaValidationErrorNameTokens->unpairedAuthorshipInputs,
                UsdValidationErrorType::Error, sites,
                TfStringPrintf(
                    "AuthorshipAPI:%s on <%s> authors %s without %s.",
                    record.GetName().GetText(), prim.GetPath().GetText(),
                    hasNames ? "inputNames" : "inputValues",
                    hasNames ? "inputValues" : "inputNames"));
            continue;
        }

        VtArray<std::string> names, values;
        namesAttr.Get(&names);
        valuesAttr.Get(&values);
        if (names.size() != values.size()) {
            errors.emplace_back(
                UsdMediaValidationErrorNameTokens->mismatchedAuthorshipInputs,
                UsdValidationErrorType::Error, sites,
                TfStringPrintf(
                    "AuthorshipAPI:%s on <%s> has %zu inputNames but %zu "
                    "inputValues.",
                    record.GetName().GetText(), prim.GetPath().GetText(),
                    names.size(), values.size()));
            continue;
        }

        // Values resolved from different specs can be out of alignment even
        // when the lengths agree.
        const SdfPropertySpecHandle namesSpec =
            _GetStrongestValueSpec(namesAttr);
        const SdfPropertySpecHandle valuesSpec =
            _GetStrongestValueSpec(valuesAttr);
        if (namesSpec && valuesSpec &&
            (namesSpec->GetLayer() != valuesSpec->GetLayer() ||
             namesSpec->GetPath().GetPrimOrPrimVariantSelectionPath() !=
             valuesSpec->GetPath().GetPrimOrPrimVariantSelectionPath())) {
            errors.emplace_back(
                UsdMediaValidationErrorNameTokens
                    ->authorshipInputsFromDifferentSpecs,
                UsdValidationErrorType::Warn, sites,
                TfStringPrintf(
                    "AuthorshipAPI:%s on <%s> resolves inputNames from "
                    "@%s@<%s> but inputValues from @%s@<%s>.",
                    record.GetName().GetText(), prim.GetPath().GetText(),
                    namesSpec->GetLayer()->GetIdentifier().c_str(),
                    namesSpec->GetPath().GetText(),
                    valuesSpec->GetLayer()->GetIdentifier().c_str(),
                    valuesSpec->GetPath().GetText()));
        }
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
