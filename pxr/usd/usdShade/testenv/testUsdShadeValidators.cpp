//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/shaderDefUtils.h"
#include "pxr/usd/usdShade/validatorTokens.h"

#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

void
TestUsdShadeValidators()
{
    // This should be updated with every new validators added with
    // UsdShadeValidators keyword.
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    UsdValidatorMetadataVector metadata = 
        registry.GetValidatorMetadataForKeyword(
            UsdShadeValidatorKeywordTokens->UsdShadeValidators);
    // Since other validators can be can registered with a UsdShadeValidators
    // keyword, our validators registered in usdShade are a subset of the entire
    // set.
    std::set<TfToken> validatorMetadataNameSet;
    for (const UsdValidatorMetadata &metadata : metadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    const std::set<TfToken> expectedValidatorNames = 
            {UsdShadeValidatorNameTokens->shaderSdrCompliance};

    TF_AXIOM(std::includes(validatorMetadataNameSet.begin(), 
                           validatorMetadataNameSet.end(), 
                           expectedValidatorNames.begin(), 
                           expectedValidatorNames.end()));
}

void 
TestUsdShadeShaderPropertyCompliance()
{
    // Need to setup our test shader in sdrRegistry first
    UsdStageRefPtr shaderDefStage = UsdStage::Open("./shaderDefs.usda");
    UsdShadeShader shaderDef =
        UsdShadeShader::Get(shaderDefStage, SdfPath("/TestShaderNode"));
    SdrRegistry::GetInstance().AddDiscoveryResult(
        UsdShadeShaderDefUtils::GetNodeDiscoveryResults(
            shaderDef, shaderDefStage->GetRootLayer()->GetRealPath())[0]);

    // Now lets test our ShaderProperty validator
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->shaderSdrCompliance);
    TF_AXIOM(validator);

    static const std::string layerContents =
        R"usda(#usda 1.0
               def Shader "Test"
               {
                    uniform token info:id = "TestShaderNode"
                    int inputs:inputInt = 2
                    float inputs:inputFloat = 2.0
                    float3 inputs:inputColor = (2.0, 3.0, 4.0)
                    token outputs:surface
               }
               def Shader "Bogus"
               {
                    uniform token info:id = "Bogus"
               })usda";
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(layerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/Test"));

        UsdValidationErrorVector errors = validator->Validate(usdPrim);

        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsProperty());
        TF_AXIOM(errors[0].GetSites()[0].GetProperty().GetPath() == 
                 SdfPath("/Test.inputs:inputColor"));
        const std::string expectedErrorMsg = "Incorrect type for "
            "/Test.inputs:inputColor. Expected 'color3f'; "
            "got 'float3'.";
        TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
    }

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/Bogus"));

        UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsProperty());
        TF_AXIOM(errors[0].GetSites()[0].GetProperty().GetPath() == 
                 SdfPath("/Bogus.info:id"));
        const std::string expectedErrorMsg = "shaderId 'Bogus' specified on "
            "shader prim </Bogus> not found in sdrRegistry.";
        TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
    }
}

int
main()
{
    TestUsdShadeValidators();
    TestUsdShadeShaderPropertyCompliance();
    printf("OK\n");
    return EXIT_SUCCESS;
};
