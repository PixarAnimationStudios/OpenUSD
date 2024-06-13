//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdGeom/validatorTokens.h"

#include <algorithm>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

static
UsdValidationErrorVector
_SubsetFamilies(const UsdPrim& usdPrim)
{
    if (!(usdPrim && usdPrim.IsInFamily<UsdGeomImageable>(
            UsdSchemaRegistry::VersionPolicy::All))) {
        return {};
    }

    const UsdGeomImageable imageable(usdPrim);
    if (!imageable) {
        return {};
    }

    const TfToken::Set subsetFamilyNames =
        UsdGeomSubset::GetAllGeomSubsetFamilyNames(imageable);

    // Sort the family names so that they are in dictionary order, making
    // the order in which they are validated more predictable.
    TfTokenVector subsetFamilyNamesVec(
        subsetFamilyNames.begin(), subsetFamilyNames.end());
    std::sort(
        subsetFamilyNamesVec.begin(), subsetFamilyNamesVec.end(),
        TfDictionaryLessThan());

    UsdValidationErrorVector errors;

    for (const TfToken& subsetFamilyName : subsetFamilyNamesVec) {
        const std::vector<UsdGeomSubset> familySubsets =
            UsdGeomSubset::GetGeomSubsets(
                imageable,
                /* elementType = */ TfToken(),
                /* familyName = */ subsetFamilyName);

        // Determine the element type of the family by looking at the
        // first subset.
        TfToken elementType;
        familySubsets[0u].GetElementTypeAttr().Get(&elementType);

        std::string reason;
        if (!UsdGeomSubset::ValidateFamily(
                imageable, elementType, subsetFamilyName, &reason)) {
            const UsdValidationErrorSites primErrorSites = {
                UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath())
            };

            errors.emplace_back(
                UsdValidationErrorType::Error,
                primErrorSites,
                TfStringPrintf(
                    "Imageable prim <%s> has invalid subset family '%s': %s",
                    usdPrim.GetPath().GetText(),
                    subsetFamilyName.GetText(),
                    reason.c_str())
            );
        }
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();

    registry.RegisterPluginValidator(
        UsdGeomValidatorNameTokens->subsetFamilies,
        _SubsetFamilies);
}

PXR_NAMESPACE_CLOSE_SCOPE
