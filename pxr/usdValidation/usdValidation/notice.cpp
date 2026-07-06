//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdValidation/usdValidation/notice.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define< UsdValidationNotice::DidRegisterValidator,
                    TfType::Bases<TfNotice> >();
    TfType::Define< UsdValidationNotice::DidRegisterValidatorSuite,
                    TfType::Bases<TfNotice> >();
}

UsdValidationNotice::DidRegisterValidator::DidRegisterValidator(
    const UsdValidationValidator *registeredValidator)
    : _registeredValidator(registeredValidator)
{
}

UsdValidationNotice::DidRegisterValidator::~DidRegisterValidator()
{
}

UsdValidationNotice::DidRegisterValidatorSuite::DidRegisterValidatorSuite(
    const UsdValidationValidatorSuite *registeredSuite)
    : _registeredSuite(registeredSuite)
{
}

UsdValidationNotice::DidRegisterValidatorSuite::~DidRegisterValidatorSuite()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

