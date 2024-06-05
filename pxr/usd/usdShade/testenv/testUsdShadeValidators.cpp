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

#include "pxr/pxr.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validator.h"

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
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->shaderSdrCompliance);
    TF_AXIOM(validator);

    static const std::string layerContents =
        R"usda(#usda 1.0
               def Shader "Principled_BSDF"
               {
                    uniform token info:id = "UsdPreviewSurface"
                    float inputs:clearcoat = 0
                    float inputs:clearcoatRoughness = 0.03
                    float3 inputs:diffuseColor = (0.54924256, 0.13939838, 0.7999993)
                    float inputs:ior = 1.45
                    float inputs:metallic = 0
                    float inputs:opacity = 1
                    float inputs:roughness = 0.5
                    float inputs:specular = 0.5
                    token outputs:surface
               })usda";
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(layerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);
    const UsdPrim usdPrim = usdStage->GetPrimAtPath(
        SdfPath("/Principled_BSDF"));
    
    UsdValidationErrorVector errors = validator->Validate(usdPrim);
    TF_AXIOM(errors.size() == 1);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsProperty());
    TF_AXIOM(errors[0].GetSites()[0].GetProperty().GetPath() == 
             SdfPath("/Principled_BSDF.inputs:diffuseColor"));
    const std::string expectedErrorMsg = "Incorrect type for "
        "/Principled_BSDF.inputs:diffuseColor. Expected 'color3f'; "
        "got 'float3'.";
    TF_CODING_ERROR("%s", errors[0].GetMessage().c_str());
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
}

int
main()
{
    TestUsdShadeValidators();
    TestUsdShadeShaderPropertyCompliance();
    printf("OK\n");
    return EXIT_SUCCESS;
};
