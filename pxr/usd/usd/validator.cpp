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

#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usd/validationError.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdValidator::UsdValidator(const UsdValidatorMetadata& metadata) :
    _metadata(metadata)
{
}

UsdValidator::UsdValidator(const UsdValidatorMetadata& metadata,
                           const UsdValidateLayerTaskFn& validateLayerTaskFn) :
    _metadata(metadata), _validateTaskFn(validateLayerTaskFn)
{
}

UsdValidator::UsdValidator(const UsdValidatorMetadata& metadata,
                           const UsdValidateStageTaskFn& validateStageTaskFn) :
    _metadata(metadata), _validateTaskFn(validateStageTaskFn)
{
}

UsdValidator::UsdValidator(const UsdValidatorMetadata& metadata,
                           const UsdValidatePrimTaskFn& validatePrimTaskFn) :
    _metadata(metadata), _validateTaskFn(validatePrimTaskFn)
{
}

const UsdValidateLayerTaskFn*
UsdValidator::_GetValidateLayerTask() const
{
    return std::get_if<UsdValidateLayerTaskFn>(&_validateTaskFn);
}

const UsdValidateStageTaskFn*
UsdValidator::_GetValidateStageTask() const
{
    return std::get_if<UsdValidateStageTaskFn>(&_validateTaskFn);
}

const UsdValidatePrimTaskFn*
UsdValidator::_GetValidatePrimTask() const
{
    return std::get_if<UsdValidatePrimTaskFn>(&_validateTaskFn);
}

const UsdValidationErrorVector
UsdValidator::Validate(const SdfLayerHandle &layer) const
{
    const UsdValidateLayerTaskFn *layerTaskFn = _GetValidateLayerTask();
    if (layerTaskFn) {
        return (*layerTaskFn)(layer);
    }
    return {};
}

const UsdValidationErrorVector
UsdValidator::Validate(const UsdStagePtr &usdStage) const
{
    const UsdValidateStageTaskFn *stageTaskFn = _GetValidateStageTask();
    if (stageTaskFn) {
        return (*stageTaskFn)(usdStage);
    }
    return {};
}

const UsdValidationErrorVector
UsdValidator::Validate(const UsdPrim &usdPrim) const
{
    const UsdValidatePrimTaskFn *primTaskFn = _GetValidatePrimTask();
    if (primTaskFn) {
        return (*primTaskFn)(usdPrim);
    }
    return {};
}

UsdValidatorSuite::UsdValidatorSuite(const UsdValidatorMetadata& metadata,
                         const std::vector<const UsdValidator*>& validators) :
    _metadata(metadata), _containedValidators(validators)
{
}

PXR_NAMESPACE_CLOSE_SCOPE

