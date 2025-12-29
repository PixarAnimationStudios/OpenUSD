//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"
#include "pxr/usdValidation/usdValidation/validator.h"
#include "pxr/usdValidation/usdValidation/fixer.h"

#include "pxr/usd/usd/attribute.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    // Register test plugin validators here
    // Test validators simply just return errors, we need to make sure various
    // UsdValidationContext APIs work and get the expected errors back, when
    // Validate is called in various scenarios on a validation context instance.
    {
        const TfToken validatorName(
            "testUsdValidationContextValidatorsPlugin:Test1");
        const TfToken validationErrorId("Test1Error");
        const UsdValidateStageTaskFn stageTaskFn
            = [validationErrorId](const UsdStagePtr &usdStage, 
                 const UsdValidationTimeRange &/*timeRange*/) {
                  return UsdValidationErrorVector { UsdValidationError(
                      validationErrorId, UsdValidationErrorType::Error,
                      { UsdValidationErrorSite(usdStage,
                                               SdfPath::AbsoluteRootPath()) },
                      "A stage validator error",
                      VtValue("ErrorMetadata")) };
              };

        TfErrorMark m;
        std::vector<UsdValidationFixer> fixers;
        fixers.emplace_back(
            TfToken("TestFixer1"), 
            "A test fixer which caters to all errors",
            [](const UsdValidationError &/*error*/,
               const UsdEditTarget &/*editTarget*/,
               const UsdTimeCode &/*timeCode*/) {
                return true;
            } /*fixerImplFn*/,
            [](const UsdValidationError &error,
               const UsdEditTarget &/*editTarget*/,
               const UsdTimeCode &/*timeCode*/) {
                TF_AXIOM(error.GetData() == VtValue("ErrorMetadata"));
                return true;
            } /*fixerCanApplyFixFn*/,
            TfTokenVector{},
            TfToken());
        fixers.emplace_back(
            TfToken("TestFixer2"), 
            "A test fixer which caters to error named Test1Error",
            [](const UsdValidationError &/*error*/,
               const UsdEditTarget &/*editTarget*/,
               const UsdTimeCode &/*timeCode*/) {
                return true;
            } /*fixerImplFn*/,
            [](const UsdValidationError &error,
               const UsdEditTarget &/*editTarget*/,
               const UsdTimeCode &/*timeCode*/) {
                TF_AXIOM(error.GetData() == VtValue("ErrorMetadata"));
                return true;
            } /*fixerCanApplyFixFn*/,
            TfTokenVector{},
            validationErrorId);
        fixers.emplace_back(
            TfToken("TestFixer3"), 
            "A test fixer which caters to error named SomeOtherError",
            [](const UsdValidationError &/*error*/,
               const UsdEditTarget &/*editTarget*/,
               const UsdTimeCode &/*timeCode*/) {
                return true;
            } /*fixerImplFn*/,
            [](const UsdValidationError &error,
               const UsdEditTarget &/*editTarget*/,
               const UsdTimeCode &/*timeCode*/) {
                TF_AXIOM(error.GetData() == VtValue("ErrorMetadata"));
                return true;
            } /*fixerCanApplyFixFn*/,
            TfTokenVector{},
            TfToken("SomeOtherError"));

        registry.RegisterPluginValidator(validatorName, stageTaskFn, fixers);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName(
            "testUsdValidationContextValidatorsPlugin:Test2");
        const UsdValidateLayerTaskFn layerTaskFn
            = [](const SdfLayerHandle &layer) {
                  const TfToken validationErrorId("Test2Error");
                  return UsdValidationErrorVector { UsdValidationError(
                      validationErrorId, UsdValidationErrorType::Error,
                      { UsdValidationErrorSite(layer,
                                               SdfPath::AbsoluteRootPath()) },
                      "A layer validator error") };
              };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, layerTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName(
            "testUsdValidationContextValidatorsPlugin:Test3");
        const UsdValidatePrimTaskFn primTaskFn = [](
            const UsdPrim &prim, 
            const UsdValidationTimeRange &timeRange) {

            const TfToken validationErrorId("Test3Error");

            UsdValidationErrorVector errors;
            if (timeRange.IncludesTimeCodeDefault()) {
                std::string errStr = "default time checker within a time "
                    "dependent validator";
                errors.push_back(UsdValidationError(
                    validationErrorId, UsdValidationErrorType::Error,
                    { UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
                    errStr));
            }
            for (const UsdAttribute &attr : prim.GetAttributes()) {
                std::vector<double> timeSamples;
                attr.GetTimeSamplesInInterval(
                    timeRange.GetInterval(), &timeSamples);
                for (const double timeCode : timeSamples) {
                    std::string errStr = "A prim validator error at timeCode: "
                        + std::to_string(timeCode);
                    errors.push_back(
                        UsdValidationError(
                            validationErrorId, UsdValidationErrorType::Error,
                            { UsdValidationErrorSite(prim.GetStage(), 
                                                     prim.GetPath()) },
                            errStr));
                }
            }
            return errors;
        };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName(
            "testUsdValidationContextValidatorsPlugin:Test4");
        const UsdValidatePrimTaskFn primTaskFn = [](
            const UsdPrim &prim, 
            const UsdValidationTimeRange &timeRange) {
            const TfToken validationErrorId("Test4Error");
            return UsdValidationErrorVector { UsdValidationError(
                validationErrorId, UsdValidationErrorType::Error,
                { UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
                "A testBaseType prim type validator error") };
        };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName(
            "testUsdValidationContextValidatorsPlugin:Test5");
        const UsdValidatePrimTaskFn primTaskFn = [](
            const UsdPrim &prim, 
            const UsdValidationTimeRange &/*timeRange*/) {
            const TfToken validationErrorId("Test5Error");
            return UsdValidationErrorVector { UsdValidationError(
                validationErrorId, UsdValidationErrorType::Error,
                { UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
                "A testDerivedType prim type validator error") };
        };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName(
            "testUsdValidationContextValidatorsPlugin:Test6");
        const UsdValidatePrimTaskFn primTaskFn = [](
            const UsdPrim &prim, 
            const UsdValidationTimeRange &/*timeRange*/) {
            const TfToken validationErrorId("Test6Error");
            return UsdValidationErrorVector { UsdValidationError(
                validationErrorId, UsdValidationErrorType::Error,
                { UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
                "A testNestedDerivedType prim type validator error") };
        };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName(
            "testUsdValidationContextValidatorsPlugin:Test7");
        const UsdValidatePrimTaskFn primTaskFn = [](
            const UsdPrim &prim, 
            const UsdValidationTimeRange &/*timeRange*/) {
            const TfToken validationErrorId("Test7Error");
            return UsdValidationErrorVector { UsdValidationError(
                validationErrorId, UsdValidationErrorType::Error,
                { UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
                "A testAPISchema prim type validator error") };
        };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken suiteName(
            "testUsdValidationContextValidatorsPlugin:TestSuite");
        const std::vector<const UsdValidationValidator *> containedValidators
            = registry.GetOrLoadValidatorsByName(
                { TfToken("testUsdValidationContextValidatorsPlugin:Test1"),
                  TfToken("testUsdValidationContextValidatorsPlugin:Test2"),
                  TfToken("testUsdValidationContextValidatorsPlugin:Test3") });

        TfErrorMark m;
        registry.RegisterPluginValidatorSuite(suiteName, containedValidators);
        TF_AXIOM(m.IsClean());
    }
}
