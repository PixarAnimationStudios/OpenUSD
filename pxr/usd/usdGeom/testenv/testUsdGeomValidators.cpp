//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usdGeom/validatorTokens.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static
void TestUsdStageMetadata()
{
    // Get stageMetadataChecker
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
            UsdGeomValidatorNameTokens->stageMetadataChecker);
    TF_AXIOM(validator);

    // Create an empty stage
    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();

    UsdValidationErrorVector errors = validator->Validate(usdStage);

    // Verify both metersPerUnit and upAxis errors are present
    TF_AXIOM(errors.size() == 2);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    const std::string expectedMetersPerUnitErrorMsg = "Stage does not specify its linear scale in metersPerUnit.";
    TF_AXIOM(errors[0].GetMessage() == expectedMetersPerUnitErrorMsg);
    TF_AXIOM(errors[1].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[1].GetSites().size() == 1);
    TF_AXIOM(errors[1].GetSites()[0].IsValid());
    const std::string expectedUpAxisErrorMsg = "Stage does not specify an upAxis.";
    TF_AXIOM(errors[1].GetMessage() == expectedUpAxisErrorMsg);

    // Fix the errors
    UsdGeomSetStageMetersPerUnit(usdStage, 0.01);
    UsdGeomSetStageUpAxis(usdStage, UsdGeomTokens->y);

    errors = validator->Validate(usdStage);

    // Verify the errors are fixed
    TF_AXIOM(errors.size() == 0);
}

int
main()
{
    TestUsdStageMetadata();

    std::cout << "OK\n";
}
