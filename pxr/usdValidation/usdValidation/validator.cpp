//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdValidation/usdValidation/validator.h"

#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdValidationValidator::UsdValidationValidator(
    const UsdValidationValidatorMetadata &metadata)
    : _metadata(metadata)
{
}

UsdValidationValidator::UsdValidationValidator(
    const UsdValidationValidatorMetadata &metadata,
    const UsdValidateLayerTaskFn &validateLayerTaskFn)
    : _metadata(metadata)
    , _validateTaskFn(validateLayerTaskFn)
{
}

UsdValidationValidator::UsdValidationValidator(
    const UsdValidationValidatorMetadata &metadata,
    const UsdValidateStageTaskFn &validateStageTaskFn)
    : _metadata(metadata)
    , _validateTaskFn(validateStageTaskFn)
{
}

UsdValidationValidator::UsdValidationValidator(
    const UsdValidationValidatorMetadata &metadata,
    const UsdValidatePrimTaskFn &validatePrimTaskFn)
    : _metadata(metadata)
    , _validateTaskFn(validatePrimTaskFn)
{
}

const UsdValidateLayerTaskFn *
UsdValidationValidator::_GetValidateLayerTask() const
{
    return std::get_if<UsdValidateLayerTaskFn>(&_validateTaskFn);
}

const UsdValidateStageTaskFn *
UsdValidationValidator::_GetValidateStageTask() const
{
    return std::get_if<UsdValidateStageTaskFn>(&_validateTaskFn);
}

const UsdValidatePrimTaskFn *
UsdValidationValidator::_GetValidatePrimTask() const
{
    return std::get_if<UsdValidatePrimTaskFn>(&_validateTaskFn);
}

UsdValidationErrorVector
UsdValidationValidator::Validate(const SdfLayerHandle &layer) const
{
    const UsdValidateLayerTaskFn *layerTaskFn = _GetValidateLayerTask();
    if (layerTaskFn) {
        UsdValidationErrorVector errors = (*layerTaskFn)(layer);
        for (UsdValidationError &error : errors) {
            error._SetValidator(this);
        }
        return errors;
    }
    return {};
}

UsdValidationErrorVector
UsdValidationValidator::Validate(
    const UsdStagePtr &usdStage,
    const UsdValidationTimeRange &timeRange) const
{
    const UsdValidateStageTaskFn *stageTaskFn = _GetValidateStageTask();
    if (stageTaskFn) {
        UsdValidationErrorVector errors = 
            (*stageTaskFn)(usdStage, timeRange);
        for (UsdValidationError &error : errors) {
            error._SetValidator(this);
        }
        return errors;
    }
    return {};
}

UsdValidationErrorVector
UsdValidationValidator::Validate(
    const UsdPrim &usdPrim, 
    const UsdValidationTimeRange &timeRange) const
{
    const UsdValidatePrimTaskFn *primTaskFn = _GetValidatePrimTask();
    if (primTaskFn) {
        UsdValidationErrorVector errors = 
            (*primTaskFn)(usdPrim, timeRange);
        for (UsdValidationError &error : errors) {
            error._SetValidator(this);
        }
        return errors;
    }
    return {};
}

UsdValidationValidatorSuite::UsdValidationValidatorSuite(
    const UsdValidationValidatorMetadata &metadata,
    const std::vector<const UsdValidationValidator *> &validators)
    : _metadata(metadata)
    , _containedValidators(validators)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
