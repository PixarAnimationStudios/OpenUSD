//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdMedia/authorshipAPI.h"
#include "pxr/usdValidation/usdMediaValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"

#include <map>
#include <set>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// Identifies authorship records unreachable via composed views, including
// applications shadowed by explicit apiSchemas list-ops or instance names
// overridden across multiple layers. See
// UsdMediaAuthorshipAPI::GetAllInPrimStacks().
static UsdValidationErrorVector
_ShadowedOrClobberedAuthorshipValidator(
    const UsdStagePtr &usdStage,
    const UsdValidationTimeRange & /*timeRange*/)
{
    UsdValidationErrorVector errors;

    using RecordId = std::pair<SdfPath, TfToken>;

    std::set<RecordId> composed;
    for (const UsdMediaAuthorshipAPI &record :
            UsdMediaAuthorshipAPI::GetAllOnStage(usdStage)) {
        composed.emplace(record.GetPrim().GetPath(), record.GetName());
    }

    std::map<RecordId, int> counts;
    for (const UsdMediaAuthorshipAPI &record :
            UsdMediaAuthorshipAPI::GetAllInPrimStacks(usdStage)) {
        ++counts[RecordId(record.GetPrim().GetPath(), record.GetName())];
    }

    for (const auto &entry : counts) {
        const SdfPath &primPath = entry.first.first;
        const TfToken &instanceName = entry.first.second;
        const int count = entry.second;

        if (composed.find(entry.first) == composed.end()) {
            errors.emplace_back(
                UsdMediaValidationErrorNameTokens
                    ->shadowedAuthorshipApplication,
                UsdValidationErrorType::Warn,
                UsdValidationErrorSites {
                    UsdValidationErrorSite(usdStage, primPath) },
                TfStringPrintf(
                    "AuthorshipAPI:%s on <%s> is shadowed by a stronger, "
                    "explicit apiSchemas list and is not applied on the "
                    "composed stage.",
                    instanceName.GetText(), primPath.GetText()));
        }

        if (count > 1) {
            errors.emplace_back(
                UsdMediaValidationErrorNameTokens
                    ->clobberedAuthorshipInstanceName,
                UsdValidationErrorType::Warn,
                UsdValidationErrorSites {
                    UsdValidationErrorSite(usdStage, primPath) },
                TfStringPrintf(
                    "AuthorshipAPI:%s on <%s> is authored in %d "
                    "contributing layers; only the strongest opinion for "
                    "each field survives composition.",
                    instanceName.GetText(), primPath.GetText(), count));
        }
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    registry.RegisterPluginValidator(
        UsdMediaValidatorNameTokens->shadowedOrClobberedAuthorship,
        _ShadowedOrClobberedAuthorshipValidator);
}

PXR_NAMESPACE_CLOSE_SCOPE
