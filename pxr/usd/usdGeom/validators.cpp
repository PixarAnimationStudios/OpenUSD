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
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/validatorTokens.h"
#include <pxr/usd/sdf/path.h>

#include <algorithm>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

static
UsdValidationErrorVector
_GetStageMetadataErrors(const UsdStagePtr &usdStage)
{
    UsdValidationErrorVector errors;
    if (!usdStage->HasAuthoredMetadata(
            UsdGeomTokens->metersPerUnit)) {
        errors.emplace_back(
            UsdGeomValidationErrorNameTokens->missingMetersPerUnitMetadata,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites{UsdValidationErrorSite(usdStage, 
                                                           SdfPath("/"))},
            TfStringPrintf("Stage with root layer <%s> does not specify its "
                           "linear scale in metersPerUnit.", 
                           usdStage->GetRootLayer()->GetIdentifier().c_str()));
    }
    if (!usdStage->HasAuthoredMetadata(
            UsdGeomTokens->upAxis)) {
        errors.emplace_back(
            UsdGeomValidationErrorNameTokens->missingUpAxisMetadata,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites{UsdValidationErrorSite(usdStage, 
                                                           SdfPath("/"))},
            TfStringPrintf("Stage with root layer <%s> does not specify an "
                           "upAxis.", 
                           usdStage->GetRootLayer()->GetIdentifier().c_str()));
    }

    return errors;
}

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
                UsdGeomValidationErrorNameTokens->invalidSubsetFamily,
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

static
UsdValidationErrorVector
_SubsetParentIsImageable(const UsdPrim& usdPrim)
{
    if (!(usdPrim && usdPrim.IsInFamily<UsdGeomSubset>(
            UsdSchemaRegistry::VersionPolicy::All))) {
        return {};
    }

    const UsdGeomSubset subset(usdPrim);
    if (!subset) {
        return {};
    }

    const UsdPrim parentPrim = usdPrim.GetParent();
    const UsdGeomImageable parentImageable(parentPrim);
    if (parentImageable) {
        return {};
    }

    const UsdValidationErrorSites primErrorSites = {
        UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath())
    };

    return {
        UsdValidationError(
            UsdGeomValidationErrorNameTokens->notImageableSubsetParent,
            UsdValidationErrorType::Error,
            primErrorSites,
            TfStringPrintf(
                "GeomSubset <%s> has direct parent prim <%s> that is not "
                "Imageable.",
                usdPrim.GetPath().GetText(),
                parentPrim.GetPath().GetText())
        )
    };
}

static
UsdValidationErrorVector
_GetGPrimDescendantErrors(const UsdPrim& usdPrim)
{
    UsdValidationErrorVector errors;

    if (usdPrim.IsA<UsdGeomGprim>()){

        const TfType geomSubsetType = TfType::Find<UsdGeomSubset>();
        const std::string& validGprimDescendantTypeNames = geomSubsetType.GetTypeName();

        const std::vector<TfType> validGprimDescendantTypes = {geomSubsetType};

        const auto isValidGprimDescendant = [&validGprimDescendantTypes](const UsdPrim& prim) -> bool {
            for (const TfType& validGprimDescendantType : validGprimDescendantTypes) {
                if (prim.IsA(validGprimDescendantType)) {
                    return true;
                }
            }
            return false;
        };

        const UsdPrimSubtreeRange& children = usdPrim.GetAllDescendants();
        for (UsdPrimSubtreeRange::iterator it = children.begin(); it != children.end(); ++it) {
            const UsdPrim& currentPrim = *it;

            if (isValidGprimDescendant(currentPrim)){
                continue;
            }

            errors.emplace_back(
                    UsdValidationErrorType::Error,
                    UsdValidationErrorSites{
                            UsdValidationErrorSite(usdPrim.GetStage(),
                                                   currentPrim.GetPath())
                    },
                    TfStringPrintf("Prim <%s> is a Gprim with an invalid descendant <%s> which is of type %s. Only prims of types (%s) may be descendants of Gprims.",
                                   usdPrim.GetPath().GetText(),
                                   currentPrim.GetPath().GetText(),
                                   currentPrim.GetTypeName().GetText(),
                                   validGprimDescendantTypeNames.c_str())
            );
            break;
        }
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    
    registry.RegisterPluginValidator(
        UsdGeomValidatorNameTokens->stageMetadataChecker, 
        _GetStageMetadataErrors);
    
    registry.RegisterPluginValidator(
        UsdGeomValidatorNameTokens->subsetFamilies,
        _SubsetFamilies);

    registry.RegisterPluginValidator(
            UsdGeomValidatorNameTokens->subsetParentIsImageable,
            _SubsetParentIsImageable);

    registry.RegisterPluginValidator(
            UsdGeomValidatorNameTokens->gPrimDescendantValidator,
            _GetGPrimDescendantErrors);
}

PXR_NAMESPACE_CLOSE_SCOPE
