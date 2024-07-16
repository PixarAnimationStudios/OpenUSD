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
    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous();
    UsdStageRefPtr usdStage = UsdStage::Open(rootLayer);

    UsdValidationErrorVector errors = validator->Validate(usdStage);

    // Verify both metersPerUnit and upAxis errors are present
    TF_AXIOM(errors.size() == 2);
    auto rootLayerIdentifier = rootLayer->GetIdentifier().c_str();
    std::vector<std::string> expectedErrorMessages = {
            TfStringPrintf("Stage with root layer <%s> does not specify its linear scale in metersPerUnit.", rootLayerIdentifier),
            TfStringPrintf("Stage with root layer <%s> does not specify an upAxis.", rootLayerIdentifier)
    };

    for(int i = 0; i < errors.size(); ++i)
    {
        TF_AXIOM(errors[i].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[i].GetSites().size() == 1);
        TF_AXIOM(errors[i].GetSites()[0].IsValid());
        TF_AXIOM(errors[i].GetMessage() == expectedErrorMessages[i]);
    }

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