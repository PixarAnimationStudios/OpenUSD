//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usd/validationError.h"

PXR_NAMESPACE_OPEN_SCOPE

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
    std::string errorTypeAsString;
    switch(_errorType) {
        case UsdValidationErrorType::None:
            return _errorMsg;
            break;
        case UsdValidationErrorType::Error:
            errorTypeAsString = "Error";
            break;
        case UsdValidationErrorType::Warn:
            errorTypeAsString = "Warn";
            break;
        case UsdValidationErrorType::Info:
            errorTypeAsString = "Info";
            break;
    }

    const std::string separator = ": ";
    return errorTypeAsString + separator + _errorMsg;
}

void
UsdValidationError::_SetValidator(const UsdValidator *validator)
{
    _validator = validator;
}

PXR_NAMESPACE_CLOSE_SCOPE

