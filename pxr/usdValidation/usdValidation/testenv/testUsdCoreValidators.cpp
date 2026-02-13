//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/validator.h"
#include "pxr/usdValidation/usdValidation/validatorTokens.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((usdValidationPlugin, "usdValidation"))
);

static void
TestUsdValidators()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    // The following test keeps track of all the available validators within
    // UsdCoreValidators keyword, hence as new validators are added under
    // this keyword this unit test will have to be updated.
    const UsdValidationValidatorMetadataVector coreValidatorMetadata
        = registry.GetValidatorMetadataForPlugin(_tokens->usdValidationPlugin);
    TF_AXIOM(coreValidatorMetadata.size() == 3);

    std::set<TfToken> validatorMetadataNameSet;
    for (const UsdValidationValidatorMetadata &metadata :
         coreValidatorMetadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    const std::set<TfToken> expectedValidatorNames
        = { UsdValidatorNameTokens->compositionErrorTest,
            UsdValidatorNameTokens->stageMetadataChecker,
            UsdValidatorNameTokens->attributeTypeMismatch };

    TF_AXIOM(validatorMetadataNameSet == expectedValidatorNames);
}

static void
TestCoreUsdStageMetadata()
{

    // Get stageMetadataChecker
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
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
    const TfToken expectedErrorIdentifier = TfToken(
            UsdValidatorNameTokens->stageMetadataChecker.GetString() + "." +
            UsdValidationErrorNameTokens->missingDefaultPrim.GetString());
    TF_AXIOM(errors[0].GetValidator() == validator);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    const std::string expectedErrorMsg
        = TfStringPrintf("Stage with root layer <%s> has an invalid or missing "
                         "defaultPrim.",
                         rootLayer->GetIdentifier().c_str());
    const std::string error = errors[0].GetMessage();
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // Set a default prim
    usdStage->SetDefaultPrim(prim);

    errors = validator->Validate(usdStage);

    // Verify the error is gone
    TF_AXIOM(errors.empty());
}

static void
TestUsdCompositionErrorTest()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    // test to make sure CompositionErrorTest validator provided in the core
    // usd plugin works correctly by reporting all the composition errors,
    // error sites and appropriate messages pertaining to these errors.
    const UsdValidationValidator *const compositionErrorValidator
        = registry.GetOrLoadValidatorByName(
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
    const PcpErrorVector expectedPcpErrors = usdStage->GetCompositionErrors();
    TF_AXIOM(expectedPcpErrors.size() == 5);

    // Get wrapped validation errors from our compositionErrorValidator
    UsdValidationErrorVector errors
        = compositionErrorValidator->Validate(usdStage);
    TF_AXIOM(errors.size() == 5);

    // Lets make sure pcpErrors and validationErrors match
    const TfToken expectedErrorIdentifier = TfToken(
            UsdValidatorNameTokens->compositionErrorTest.GetString() + "." +
            UsdValidationErrorNameTokens->compositionError.GetString());
    for (size_t index = 0; index < errors.size(); ++index) {
        TF_AXIOM(errors[index].GetValidator() == compositionErrorValidator);
        TF_AXIOM(errors[index].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(errors[index].GetMessage()
                 == expectedPcpErrors[index]->ToString());
        TF_AXIOM(errors[index].GetSites().size() == 1);
        TF_AXIOM(errors[index].GetSites().size() == 1);
        TF_AXIOM(errors[index].GetSites()[0].IsValid());
        TF_AXIOM(errors[index].GetSites()[0].IsPrim());
        TF_AXIOM(errors[index].GetSites()[0].GetPrim().GetPath()
                 == expectedPcpErrors[index]->rootSite.path);
    }
}

static void
TestUsdAttributeTypeMismatch()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *const attributeTypeMismatchValidator
        = registry.GetOrLoadValidatorByName(
            UsdValidatorNameTokens->attributeTypeMismatch);
    TF_AXIOM(attributeTypeMismatchValidator);

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usda(#usda 1.0
        def Sphere "Sphere" {
            int radius = 1
        }
    )usda");

    SdfLayerRefPtr mainLayer = SdfLayer::CreateAnonymous(".usda");
    mainLayer->ImportFromString(
        TfStringPrintf(R"usda(#usda 1.0
            def Sphere "Sphere" (
                append references = @%s@</Sphere>
            )
            {
                float radius = 2.0
            }
        )usda" , layer->GetIdentifier().c_str()));

    const UsdStageRefPtr usdStage = UsdStage::Open(mainLayer);
    const UsdPrim spherePrim = usdStage->GetPrimAtPath(SdfPath("/Sphere"));
    TF_AXIOM(spherePrim);

    const UsdValidationErrorVector errors
        = attributeTypeMismatchValidator->Validate(spherePrim);
    TF_AXIOM(errors.size() == 2);
    const TfToken expectedErrorIdentifier = TfToken(
        UsdValidatorNameTokens->attributeTypeMismatch.GetString() + "." +
        UsdValidationErrorNameTokens->attributeTypeMismatch.GetString());
    const std::vector<std::string> expectedMessages = {
        TfStringPrintf("Type mismatch for attribute </Sphere.radius>. "
                       "Expected attribute type is 'double' but defined as "
                       "'int' in layer <%s>.",
                       layer->GetIdentifier().c_str()),
        TfStringPrintf("Type mismatch for attribute </Sphere.radius>. "
                       "Expected attribute type is 'double' but defined as "
                       "'float' in layer <%s>.",
                       mainLayer->GetIdentifier().c_str())
    };
    const std::vector<std::string> expectedLayerIdentifiers = {
        layer->GetIdentifier(),
        mainLayer->GetIdentifier()
    };
    for (const UsdValidationError &error : errors) {
        TF_AXIOM(error.GetValidator() == attributeTypeMismatchValidator);
        TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(error.GetSites().size() == 1);
        TF_AXIOM(error.GetSites()[0].IsValid());
        TF_AXIOM(error.GetSites()[0].IsValidSpecInLayer());
        const std::string layerIdentifier
            = error.GetSites()[0].GetLayer()->GetIdentifier();
        TF_AXIOM(std::find(expectedLayerIdentifiers.begin(),
                            expectedLayerIdentifiers.end(),
                            layerIdentifier) != expectedLayerIdentifiers.end());
        const std::string errorMessage = error.GetMessage();
        TF_AXIOM(std::find(expectedMessages.begin(),
                            expectedMessages.end(),
                            errorMessage) != expectedMessages.end());
    }
}   

int
main()
{
    TestUsdValidators();
    TestCoreUsdStageMetadata();
    TestUsdCompositionErrorTest();
    TestUsdAttributeTypeMismatch();

    std::cout << "OK\n";
}
