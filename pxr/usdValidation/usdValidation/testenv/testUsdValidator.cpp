//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestSimpleValidator()
{
    // Note that these are just to test the validator -> taskFn -> Error
    // pipeline, validators should ideally be registered with the
    // UsdValidationRegistry.
    //
    // Simple LayerValidator
    UsdValidateLayerTaskFn validateLayerTaskFn
        = [](const SdfLayerHandle &layer) {
              return UsdValidationErrorVector { UsdValidationError() };
          };
    UsdValidationValidatorMetadata metadata;
    metadata.name = TfToken("TestSimpleLayerValidator");
    metadata.doc = "This is a test.";
    metadata.isSuite = false;
    UsdValidationValidator layerValidator
        = UsdValidationValidator(metadata, validateLayerTaskFn);
    SdfLayerRefPtr testLayer = SdfLayer::CreateAnonymous();
    {
        UsdValidationErrorVector errors = layerValidator.Validate(testLayer);
        TF_AXIOM(errors.size() == 1);
        // Since no ErrorName is provided error identifier is same as the
        // validator name.
        TF_AXIOM(errors[0].GetIdentifier() == metadata.name);
        TF_AXIOM(errors[0].HasNoError());
        TF_AXIOM(errors[0].GetSites().empty());
        TF_AXIOM(errors[0].GetValidator() == &layerValidator);
    }
    // Use the LayerValidator to validate a prim!!
    // Note that this validator has no UsdValidatePrimTaskFn!
    UsdStageRefPtr usdStage2 = UsdStage::CreateInMemory();
    const UsdPrim prim = usdStage2->GetPseudoRoot();
    {
        TF_AXIOM(layerValidator.Validate(prim).empty());
    }

    const TfToken errorName("ErrorOnStage");
    const TfToken expectedErrorIdentifier(
        "TestSimpleStageValidator.ErrorOnStage");
    // Simple StageValidator
    UsdValidateStageTaskFn validateStageTaskFn
        = [errorName](const UsdStageRefPtr &usdStage,
                      const UsdValidationTimeRange &/*timeRange*/) {
              return UsdValidationErrorVector { UsdValidationError(
                  errorName, UsdValidationErrorType::Error,
                  { UsdValidationErrorSite(usdStage,
                                           SdfPath::AbsoluteRootPath()) },
                  "This is an error on the stage") };
          };
    metadata.name = TfToken("TestSimpleStageValidator");
    UsdValidationValidator stageValidator
        = UsdValidationValidator(metadata, validateStageTaskFn);
    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();
    {
        UsdValidationErrorVector errors = stageValidator.Validate(usdStage);
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(!errors[0].HasNoError());
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetValidator() == &stageValidator);
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
        UsdValidationErrorVector errors
            = stageValidator.Validate(usdStageFromLayer);
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(!errors[0].HasNoError());
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetValidator() == &stageValidator);
        const UsdValidationErrorSites &errorSites = errors[0].GetSites();
        TF_AXIOM(errorSites.size() == 1);
        TF_AXIOM(!errorSites[0].IsValidSpecInLayer());
        TF_AXIOM(errorSites[0].IsPrim());
        TF_AXIOM(!errorSites[0].IsProperty());
    }

    // Simple SchemaTypeValidator
    const TfToken errorName2("ErrorOnSchemaType");
    UsdValidatePrimTaskFn validatePrimTaskFn = 
        [errorName2](const UsdPrim &usdPrim, 
                     const UsdValidationTimeRange &/*timeRange*/) {
        return UsdValidationErrorVector { UsdValidationError(
            errorName2, UsdValidationErrorType::Error,
            { UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath()) },
            "This is an error on the stage") };
    };
    metadata.name = TfToken("TestSimplePrimValidator");
    metadata.schemaTypes = { TfToken("MadeUpPrimType") };
    const TfToken expectedErrorIdentifier2(
        "TestSimplePrimValidator.ErrorOnSchemaType");
    UsdValidationValidator schemaTypeValidator
        = UsdValidationValidator(metadata, validatePrimTaskFn);
    {
        UsdValidationErrorVector errors = schemaTypeValidator.Validate(prim);
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier2);
        TF_AXIOM(!errors[0].HasNoError());
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetValidator() == &schemaTypeValidator);
        const UsdValidationErrorSites &errorSites = errors[0].GetSites();
        TF_AXIOM(errorSites.size() == 1);
        TF_AXIOM(!errorSites[0].IsValidSpecInLayer());
        TF_AXIOM(errorSites[0].IsPrim());
        TF_AXIOM(!errorSites[0].IsProperty());
    }

    // Simple ValidatorSuite
    metadata.name = TfToken("TestValidatorSuite");
    metadata.doc = "This is a test.";
    metadata.schemaTypes = {};
    metadata.isSuite = true;
    UsdValidationValidatorSuite validatorSuite = UsdValidationValidatorSuite(
        metadata, { &layerValidator, &stageValidator, &schemaTypeValidator });
    TF_AXIOM(validatorSuite.GetContainedValidators().size() == 3);
}

int
main()
{
    TestSimpleValidator();

    std::cout << "OK\n";
}
