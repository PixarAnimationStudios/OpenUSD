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

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((usdPlugin, "usd"))
);

static
void
TestUsdValidators()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();

    // The following test keeps track of all the available validators within
    // UsdCoreValidators keyword, hence as new validators are added under
    // this keyword this unit test will have to be updated.
    const UsdValidatorMetadataVector coreValidatorMetadata =
            registry.GetValidatorMetadataForPlugin(_tokens->usdPlugin);
    TF_AXIOM(coreValidatorMetadata.size() == 2);

    std::set<TfToken> validatorMetadataNameSet;
    for (const UsdValidatorMetadata &metadata : coreValidatorMetadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    const std::set<TfToken> expectedValidatorNames =
            {UsdValidatorNameTokens->compositionErrorTest,
             UsdValidatorNameTokens->stageMetadataChecker};

    TF_AXIOM(validatorMetadataNameSet == expectedValidatorNames);
}

static
void 
TestCoreUsdStageMetadata()
{

    // Get stageMetadataChecker
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
            UsdValidatorNameTokens->stageMetadataChecker);
    TF_AXIOM(validator);

    // Create an empty stage
    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous();
    UsdStageRefPtr usdStage = UsdStage::Open(rootLayer);
    UsdPrim prim = usdStage->DefinePrim(SdfPath("/test"), TfToken("Xform"));

    // Validate knowing there is no default prim
    UsdValidationErrorVector errors = validator->Validate(usdStage);

    // Verify the correct error is returned
    TF_AXIOM(errors.size() == 1);
    const TfToken expectedErrorIdentifier(
            "usd:StageMetadataChecker.MissingDefaultPrim");
    TF_AXIOM(errors[0].GetValidator() == validator);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    const std::string expectedErrorMsg = 
        TfStringPrintf("Stage with root layer <%s> has an invalid or missing "
                       "defaultPrim.", rootLayer->GetIdentifier().c_str());
    const std::string error = errors[0].GetMessage();
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // Set a default prim
    usdStage->SetDefaultPrim(prim);

    errors = validator->Validate(usdStage);

    // Verify the error is gone
    TF_AXIOM(errors.empty());
}

static
void
TestUsdCompositionErrorTest()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();

    // test to make sure CompositionErrorTest validator provided in the core
    // usd plugin works correctly by reporting all the composition errors,
    // error sites and appropriate messages pertaining to these errors.
    const UsdValidator* const compositionErrorValidator =
        registry.GetOrLoadValidatorByName(
            UsdValidatorNameTokens->compositionErrorTest);
    TF_AXIOM(compositionErrorValidator);

    static const std::string layerContents = 
        R"usda(#usda 1.0
        (
        subLayers = [
        @missingLayer.usda@
        ]
        )
        def "World"
        {
        def "Inst1" (
        instanceable = true
        prepend references = </Main>
        )
        {
        }
        def "Inst2" (
        instanceable = true
        prepend references = </Main>
        )
        {
        }
        }
        def "Main"
        {
        def "First" (
        add references = </Main/Second>
        )
        {
        }
        def "Second" (
        add references = </Main/First>
        )
        {
        }
        }
    )usda";
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(layerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);

    // Get expected list of composition errors from the stage.
    const PcpErrorVector expectedPcpErrors = 
        usdStage->GetCompositionErrors();
    TF_AXIOM(expectedPcpErrors.size() == 5);

    // Get wrapped validation errors from our compositionErrorValidator
    UsdValidationErrorVector errors = 
        compositionErrorValidator->Validate(usdStage);
    TF_AXIOM(errors.size() == 5);

    // Lets make sure pcpErrors and validationErrors match
    const TfToken expectedErrorIdentifier =
        TfToken("usd:CompositionErrorTest.CompositionError");
    for (size_t index = 0; index < errors.size(); ++index) {
        TF_AXIOM(errors[index].GetValidator() == compositionErrorValidator);
        TF_AXIOM(errors[index].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(errors[index].GetMessage() == 
                 expectedPcpErrors[index]->ToString());
        TF_AXIOM(errors[index].GetSites().size() == 1);
        TF_AXIOM(errors[index].GetSites().size() == 1);
        TF_AXIOM(errors[index].GetSites()[0].IsValid());
        TF_AXIOM(errors[index].GetSites()[0].IsPrim());
        TF_AXIOM(errors[index].GetSites()[0].GetPrim().GetPath() ==
                 expectedPcpErrors[index]->rootSite.path);
    }
}

int
main()
{
    TestUsdValidators();
    TestCoreUsdStageMetadata();
    TestUsdCompositionErrorTest();

    std::cout << "OK\n";
}
