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
}

UsdValidationError::UsdValidationError(const UsdValidationErrorType &type,
                                   const UsdValidationErrorSites &errorSites,
                                   const std::string &errorMsg) :
    _errorType(type), _errorSites(errorSites), _errorMsg(errorMsg)
{
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

PXR_NAMESPACE_CLOSE_SCOPE

