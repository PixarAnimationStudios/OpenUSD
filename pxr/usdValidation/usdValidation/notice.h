//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_VALIDATION_USD_VALIDATION_NOTICE_H
#define PXR_USD_VALIDATION_USD_VALIDATION_NOTICE_H

#include "pxr/pxr.h"
#include "pxr/usdValidation/usdValidation/api.h"
#include "pxr/base/tf/notice.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdValidationValidator;
class UsdValidationValidatorSuite;

/// \class UsdValidationNotice
/// Notices for UsdValidation. 
///
class UsdValidationNotice
{
public:
    /// DidRegisterValidator notice is sent after a validator is dynamically
    /// registered using UsdValidationRegistry::RegisterValidator API.
    class DidRegisterValidator : public TfNotice
    {
    public:
        USDVALIDATION_API
        explicit DidRegisterValidator(
            const UsdValidationValidator *registeredValidator);

        USDVALIDATION_API
        ~DidRegisterValidator();

        const UsdValidationValidator *GetValidator() const 
        {
            return _registeredValidator;
        }
    private:
        const UsdValidationValidator *_registeredValidator;
    };

    /// DidRegisterValidatorSuite notice is sent after a validator suite is
    /// dynamically registered using
    /// UsdValidationRegistry::RegisterValidatorSuite API.
    class DidRegisterValidatorSuite : public TfNotice
    {
    public:
        USDVALIDATION_API
        explicit DidRegisterValidatorSuite(
            const UsdValidationValidatorSuite *registeredSuite);

        USDVALIDATION_API
        ~DidRegisterValidatorSuite();

        const UsdValidationValidatorSuite *GetValidatorSuite() const
        {
            return _registeredSuite;
        }
    private:
        const UsdValidationValidatorSuite *_registeredSuite;
    };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_VALIDATION_USD_VALIDATION_NOTICE_H
