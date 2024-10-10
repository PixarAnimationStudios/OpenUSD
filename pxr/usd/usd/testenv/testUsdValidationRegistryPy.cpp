//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validator.h"

#include <vector>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    {
        const UsdValidateStageTaskFn stageTaskFn =
            [](const UsdStagePtr &usdStage)
        {
            return UsdValidationErrorVector{UsdValidationError(
                TfToken("TestValidator1"),
                UsdValidationErrorType::Error,
                {UsdValidationErrorSite(usdStage, SdfPath("/"))},
                "This is an error on the stage")};
        };

        std::cout<<"Registering TestValidator1\n";
        // Register the validator
        TfErrorMark m;
        UsdValidatorMetadata metadata{
            TfToken("TestValidator1"),
            nullptr,
            {TfToken("IncludedInAll"), TfToken("SomeKeyword1")},
            "TestValidator1 for keywords metadata parsing",
            {},
            false};

        registry.RegisterValidator(metadata, stageTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const UsdValidatePrimTaskFn primTaskFn = [](const UsdPrim & /*prim*/)
        { return UsdValidationErrorVector{}; };

        // Register the validator
        TfErrorMark m;
        UsdValidatorMetadata metadata{
            TfToken("TestValidator2"),
            nullptr,
            {TfToken("IncludedInAll")},
            "TestValidator2 for schemaType metadata parsing",
            {TfToken("SomePrimType"), TfToken("SomeAPISchema")},
            false};
        registry.RegisterValidator(metadata, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const std::vector<const UsdValidator *> containedValidators =
            registry.GetOrLoadValidatorsByName(
                {TfToken("TestValidator1"),
                 TfToken("TestValidator2")});
        TfErrorMark m;
        UsdValidatorMetadata metadata{
            TfToken("TestValidatorSuite"),
            nullptr,
            {TfToken("IncludedInAll"), TfToken("SuiteValidator")},
            "Suite of TestValidator1 and TestValidator2",
            {},
            true};
        registry.RegisterValidatorSuite(metadata, containedValidators);
        TF_AXIOM(m.IsClean());
    }
}
