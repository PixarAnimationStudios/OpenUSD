//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

UsdValidationErrorVector
UsdValidator::Validate(const SdfLayerHandle &layer) const
{
    const UsdValidateLayerTaskFn *layerTaskFn = _GetValidateLayerTask();
    if (layerTaskFn) {
        return (*layerTaskFn)(layer);
    }
    return {};
}

UsdValidationErrorVector
UsdValidator::Validate(const UsdStagePtr &usdStage) const
{
    const UsdValidateStageTaskFn *stageTaskFn = _GetValidateStageTask();
    if (stageTaskFn) {
        return (*stageTaskFn)(usdStage);
    }
    return {};
}

UsdValidationErrorVector
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

