//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/enum.h"
#include "pxr/usd/usd/validationError.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdValidationErrorType::None, "None");
    TF_ADD_ENUM_NAME(UsdValidationErrorType::Error, "Error");
    TF_ADD_ENUM_NAME(UsdValidationErrorType::Warn, "Warn");
    TF_ADD_ENUM_NAME(UsdValidationErrorType::Info, "Info");
}

UsdValidationErrorSite::UsdValidationErrorSite(
    const SdfLayerHandle &layer, const SdfPath &objectPath) :
        _layer(layer), _objectPath(objectPath)
{
}

UsdValidationErrorSite::UsdValidationErrorSite(const UsdStagePtr &usdStage, 
                     const SdfPath &objectPath, 
                     const SdfLayerHandle &layer) :
    _usdStage(usdStage), _layer(layer), _objectPath(objectPath)
{
}

UsdValidationError::UsdValidationError() : 
    _errorType(UsdValidationErrorType::None)
{
    _validator = nullptr;
}

UsdValidationError::UsdValidationError(const UsdValidationErrorType &type,
                                   const UsdValidationErrorSites &errorSites,
                                   const std::string &errorMsg) :
    _errorType(type), _errorSites(errorSites), _errorMsg(errorMsg)
{
    _validator = nullptr;
}

std::string
UsdValidationError::GetErrorAsString() const
{
    return _errorType == UsdValidationErrorType::None ? _errorMsg : TfStringPrintf(
        "%s: %s", TfEnum::GetDisplayName(_errorType).c_str(), _errorMsg.c_str());
}

void
UsdValidationError::_SetValidator(const UsdValidator *validator)
{
    _validator = validator;
}

PXR_NAMESPACE_CLOSE_SCOPE

