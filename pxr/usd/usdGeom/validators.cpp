//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usdGeom/validatorTokens.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usd/validator.h"

PXR_NAMESPACE_OPEN_SCOPE

    static
    UsdValidationErrorVector
    _GetStageMetadataErrors(const UsdStagePtr &usdStage)
    {
        UsdValidationErrorVector errors;
        if (!usdStage->HasAuthoredMetadata(
                UsdGeomTokens->metersPerUnit)) {
            errors.emplace_back(UsdValidationErrorType::Error,
                                UsdValidationErrorSites{UsdValidationErrorSite(usdStage, SdfPath("/"))},
                                "Stage does not specify its linear scale in "
                                "metersPerUnit.");
        }
        if (!usdStage->HasAuthoredMetadata(
                UsdGeomTokens->upAxis)) {
            errors.emplace_back(UsdValidationErrorType::Error,
                                UsdValidationErrorSites{UsdValidationErrorSite(usdStage, SdfPath("/"))},
                                "Stage does not specify an upAxis.");
        }

        return errors;
    }

    TF_REGISTRY_FUNCTION(UsdValidationRegistry)
    {
        UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
        registry.RegisterPluginValidator(
                UsdGeomValidatorNameTokens->stageMetadataChecker, _GetStageMetadataErrors);
    }

PXR_NAMESPACE_CLOSE_SCOPE
