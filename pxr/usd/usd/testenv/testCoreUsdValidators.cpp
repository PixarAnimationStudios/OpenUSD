//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validatorTokens.h"
#include "pxr/usd/usd/validationRegistry.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static
void TestCoreUsdStageMetadataDefined()
{

    // Get stageMetadataChecker
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
            UsdValidatorNameTokens->stageMetadataChecker);
    TF_AXIOM(validator);

    // Create an empty stage
    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();

    // Validate knowing there is no default prim
    UsdValidationErrorVector errors = validator->Validate(usdStage);

    // Verify the correct error is returned
    TF_AXIOM(errors.size() == 1);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    const std::string expectedErrorMsg = "Stage has missing or invalid defaultPrim.";
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
}

int
main()
{
    TestCoreUsdStageMetadataDefined();

    std::cout << "OK\n";
}
