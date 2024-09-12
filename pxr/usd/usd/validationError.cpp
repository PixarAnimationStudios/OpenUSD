//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/enum.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validator.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((ValidationErrorNameDelimiter, "."))
);

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

UsdValidationErrorSite::UsdValidationErrorSite(
    const UsdStagePtr &usdStage, const SdfPath &objectPath, 
    const SdfLayerHandle &layer) :
    _usdStage(usdStage), _layer(layer), _objectPath(objectPath)
{
}

UsdValidationError::UsdValidationError() : 
    _errorType(UsdValidationErrorType::None)
{
    _validator = nullptr;
}

UsdValidationError::UsdValidationError(
    const TfToken &name, const UsdValidationErrorType &type, 
    const UsdValidationErrorSites &errorSites, const std::string &errorMsg) :
    _name(name), _errorType(type), _errorSites(errorSites), 
    _errorMsg(errorMsg)
{
    _validator = nullptr;
}

TfToken
UsdValidationError::GetIdentifier() const
{
    // A validation error is created via a call to UsdValidator::Validate(),
    // which should have set a validator on the error. But if a ValidationError
    // is created directly (not recommended), it will not have a validator set,
    // this is improper use of the API, hence we throw a coding error here.
    if (!_validator) {
        TF_CODING_ERROR("Validator not set on ValidationError. Possibly this "
                        "validation error was not created via a call to "
                        "UsdValidator::Validate(), which is responsible to set "
                        "the validator on the error.");
        return TfToken();
    }
    // If the name is empty, return the validator's name.
    if (_name.IsEmpty()) {
        return _validator->GetMetadata().name;
    }
    return TfToken(_validator->GetMetadata().name.GetString() +
                   _tokens->ValidationErrorNameDelimiter.GetString() + 
                   _name.GetString());
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

