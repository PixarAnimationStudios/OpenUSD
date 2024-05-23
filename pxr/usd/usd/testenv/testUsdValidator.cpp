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
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usd/validationError.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static
void TestSimpleValidator()
{
    // Note that these are just to test the validator -> taskFn -> Error 
    // pipeline, validators should ideally be registered with the 
    // UsdValidationRegistry.
    //
    // Simple LayerValidator
    UsdValidateLayerTaskFn validateLayerTaskFn = 
        [](const SdfLayerHandle& layer) {
        return UsdValidationErrorVector{UsdValidationError()};
    };
    UsdValidatorMetadata metadata;
    metadata.name = TfToken("TestSimpleLayerValidator");
    metadata.docs = "This is a test.";
    UsdValidator layerValidator = UsdValidator(metadata, validateLayerTaskFn);
    SdfLayerRefPtr testLayer = SdfLayer::CreateAnonymous();
    {
        UsdValidationErrorVector errors = layerValidator.Validate(testLayer);
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].HasNoError());
        TF_AXIOM(errors[0].GetSites().empty());
    }
    // Use the LayerValidator to validate a prim!!
    // Note that this validator has no UsdValidatePrimTaskFn!
    UsdStageRefPtr usdStage2 = UsdStage::CreateInMemory();
    const UsdPrim prim = usdStage2->GetPseudoRoot();
    {
        TF_AXIOM(layerValidator.Validate(prim).empty());
    }

    // Simple StageValidator
    UsdValidateStageTaskFn validateStageTaskFn = 
        [](const UsdStageRefPtr &usdStage) {
            return UsdValidationErrorVector{
                UsdValidationError(UsdValidationErrorType::Error, 
                                   {UsdValidationErrorSite(usdStage, 
                                                           SdfPath("/"))},
                                   "This is an error on the stage")};
    };
    metadata.name = TfToken("TestSimpleStageValidator");
    metadata.docs = "This is a test.";
    UsdValidator stageValidator = UsdValidator(metadata, validateStageTaskFn);
    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();
    {
        UsdValidationErrorVector errors = stageValidator.Validate(usdStage);
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(!errors[0].HasNoError());
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        const UsdValidationErrorSites &errorSites = errors[0].GetSites();
        TF_AXIOM(errorSites.size() == 1);
        TF_AXIOM(!errorSites[0].IsValidSpecInLayer());
        TF_AXIOM(errorSites[0].IsPrim());
        TF_AXIOM(!errorSites[0].IsProperty());
    }
    // Use the StageValidator to validate a layer
    // Note that this validator has no UsdValidateLayerTaskFn
    {
        TF_AXIOM(stageValidator.Validate(testLayer).empty());
    }
    // Create a stage using the layer and validate the stage now!
    UsdStageRefPtr usdStageFromLayer = UsdStage::Open(testLayer);
    {
        UsdValidationErrorVector errors = 
            stageValidator.Validate(usdStageFromLayer);
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(!errors[0].HasNoError());
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        const UsdValidationErrorSites &errorSites = errors[0].GetSites();
        TF_AXIOM(errorSites.size() == 1);
        TF_AXIOM(!errorSites[0].IsValidSpecInLayer());
        TF_AXIOM(errorSites[0].IsPrim());
        TF_AXIOM(!errorSites[0].IsProperty());
    }

    // Simple SchemaTypeValidator
    UsdValidatePrimTaskFn validatePrimTaskFn = [](const UsdPrim &usdPrim) {
        return UsdValidationErrorVector{
            UsdValidationError(UsdValidationErrorType::Error, 
                               {UsdValidationErrorSite(usdPrim.GetStage(), 
                                                       usdPrim.GetPath())},
                               "This is an error on the stage")};
    };
    metadata.name = TfToken("TestSimplePrimValidator");
    metadata.schemaTypes = {TfToken("MadeUpPrimType")};
    metadata.docs = "This is a test.";
    UsdValidator schemaTypeValidator = UsdValidator(metadata, 
                                                    validatePrimTaskFn);
    {
        UsdValidationErrorVector errors = schemaTypeValidator.Validate(prim);
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(!errors[0].HasNoError());
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        const UsdValidationErrorSites &errorSites = errors[0].GetSites();
        TF_AXIOM(errorSites.size() == 1);
        TF_AXIOM(!errorSites[0].IsValidSpecInLayer());
        TF_AXIOM(errorSites[0].IsPrim());
        TF_AXIOM(!errorSites[0].IsProperty());
    }

    // Simple ValidatorSuite
    metadata.name = TfToken("TestValidatorSuite");
    metadata.docs = "This is a test.";
    metadata.schemaTypes = {};
    metadata.isSuite = true;
    UsdValidatorSuite validatorSuite = UsdValidatorSuite(metadata, 
                                         { 
                                            &layerValidator, 
                                            &stageValidator, 
                                            &schemaTypeValidator
                                         });
    TF_AXIOM(validatorSuite.GetContainedValidators().size() == 3);
}

int 
main(int argc, char** argv)
{
    TestSimpleValidator();

    std::cout << "OK\n";
    return EXIT_SUCCESS;
}
