//
// Copyright 2024 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
