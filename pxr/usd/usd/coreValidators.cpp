//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validatorTokens.h"
#include "pxr/usd/usd/validator.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    const UsdValidateStageTaskFn stageTaskFn = 
        [](const UsdStagePtr &usdStage) {
            UsdValidationErrorVector errors;
            const PcpErrorVector pcpErrors = usdStage->GetCompositionErrors();
            errors.reserve(pcpErrors.size());
            for (const PcpErrorBasePtr &pcpError : pcpErrors) {
                UsdValidationErrorSites errorSites = {
                    UsdValidationErrorSite(usdStage, pcpError->rootSite.path)};
                errors.emplace_back(UsdValidationErrorType::Error, 
                                    std::move(errorSites), 
                                    pcpError->ToString());
            }
            return errors;
        };

    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    registry.RegisterPluginValidator(
        UsdValidatorNameTokens->compositionErrorTest, stageTaskFn);
}

PXR_NAMESPACE_CLOSE_SCOPE
