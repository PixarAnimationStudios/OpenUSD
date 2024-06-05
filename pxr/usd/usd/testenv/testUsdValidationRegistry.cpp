//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validator.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();

    {
        const TfToken validatorName("testValidationPlugin:TestValidator1");
        const UsdValidateStageTaskFn stageTaskFn = [](
            const UsdStagePtr & usdStage)
        {
            return UsdValidationErrorVector{
                    UsdValidationError(UsdValidationErrorType::Error, 
                                      {UsdValidationErrorSite(usdStage, 
                                                              SdfPath("/"))},
                                      "This is an error on the stage")};
        };

        // Register the validator
        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, stageTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName("testValidationPlugin:TestValidator2");
        const UsdValidatePrimTaskFn primTaskFn = [](const UsdPrim & /*prim*/)
        {
            return UsdValidationErrorVector{};
        };

        // Register the validator
        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName("testValidationPlugin:TestValidator3");
        const UsdValidatePrimTaskFn primTaskFn = [](const UsdPrim & /*prim*/)
            {
                return UsdValidationErrorVector{};
            };

        // Register the validator
        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken suiteName("testValidationPlugin:TestValidatorSuite");
        const std::vector<const UsdValidator*> containedValidators =
            registry.GetOrLoadValidatorsByName(
                {TfToken("testValidationPlugin:TestValidator1"),
                 TfToken("testValidationPlugin:TestValidator2")});
        TfErrorMark m;
        registry.RegisterPluginValidatorSuite(suiteName, containedValidators);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName("testValidationPlugin:FailedValidator");
        const UsdValidateStageTaskFn stageTaskFn = [](
            const UsdStagePtr & /*stage*/)
        {
            return UsdValidationErrorVector{};
        };

        // Register the validator
        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, stageTaskFn);
        TF_AXIOM(!m.IsClean());
    }
    {
        const TfToken suiteName("testValidationPlugin:FailedValidatorSuite");
        const std::vector<const UsdValidator*> containedValidators =
            registry.GetOrLoadValidatorsByName(
                {TfToken("testValidationPlugin:TestValidator2"),
                 TfToken("testValidationPlugin:TestValidator1")});
        TfErrorMark m;
        registry.RegisterPluginValidatorSuite(suiteName, containedValidators);
        TF_AXIOM(!m.IsClean());
    }
    {
        const TfToken suiteName("testValidationPlugin:FailedValidatorSuite2");
        const std::vector<const UsdValidator*> containedValidators =
            registry.GetOrLoadValidatorsByName(
                {TfToken("testValidationPlugin:TestValidator2")});
        TfErrorMark m;
        registry.RegisterPluginValidatorSuite(suiteName, containedValidators);
        TF_AXIOM(!m.IsClean());
    }
}

void TestUsdValidationRegistry()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    {
        UsdValidatorMetadata metadata;

        TF_AXIOM(registry.GetValidatorMetadata(
            TfToken("testValidationPlugin:TestValidator1"), &metadata));
        const TfTokenVector expectedKeywords = {
            TfToken("IncludedInAll"),
            TfToken("SomeKeyword1") };
        TF_AXIOM(metadata.keywords == expectedKeywords);
        const std::string expectedDoc = "TestValidator1 for keywords metadata "
                                         "parsing";
        TF_AXIOM(metadata.doc == expectedDoc);
        TF_AXIOM(!metadata.isSuite);

        // Actually go and call validate on this and inspect the Error.
        const UsdValidator* const validator = 
            registry.GetOrLoadValidatorByName(metadata.name);
        TF_AXIOM(validator);
        UsdStageRefPtr usdStage = UsdStage::CreateInMemory();
        const UsdValidationErrorVector errors = validator->Validate(usdStage);
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(!errors[0].HasNoError());
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        const UsdValidationErrorSites &errorSites = errors[0].GetSites();
        TF_AXIOM(errorSites.size() == 1);
        TF_AXIOM(!errorSites[0].IsValidSpecInLayer());
        TF_AXIOM(errorSites[0].IsPrim());
        TF_AXIOM(!errorSites[0].IsProperty());

        // TestValidator1 has a StageTaskFn, try giving it a layer or a prim, it
        // must return a no error.
        TF_AXIOM(validator->Validate(usdStage->GetPseudoRoot()).empty());
        TF_AXIOM(validator->Validate(usdStage->GetRootLayer()).empty());
    }
    {
        UsdValidatorMetadataVector metadata =
            registry.GetValidatorMetadataForSchemaType(
                TfToken("SomePrimType"));
        TF_AXIOM(metadata.size() == 4);
        TfTokenVector expectedValue({
            TfToken("testValidationPlugin:FailedValidator"),
            TfToken("testValidationPlugin:FailedValidatorSuite"),
            TfToken("testValidationPlugin:TestValidator2"),
            TfToken("testValidationPlugin:TestValidator3")
        });
        TF_AXIOM(TfTokenVector({metadata[0].name, metadata[1].name,
            metadata[2].name, metadata[3].name}) == expectedValue);
    }
    {
        UsdValidatorMetadataVector metadata =
            registry.GetValidatorMetadataForKeyword(
                TfToken("SomeKeyword1"));
        TF_AXIOM(metadata.size() == 2);
        TfTokenVector expectedValue({
            TfToken("testValidationPlugin:TestValidator1"),
            TfToken("testValidationPlugin:TestValidator3")
        });
        TF_AXIOM(TfTokenVector({metadata[0].name, metadata[1].name}) == 
                 expectedValue);
        TF_AXIOM(!metadata[0].isSuite);
        TF_AXIOM(!metadata[1].isSuite);
    }
    {
        const UsdValidatorSuite* suiteValidator = 
            registry.GetOrLoadValidatorSuiteByName(
                TfToken("testValidationPlugin:TestValidatorSuite"));
        const UsdValidatorMetadata &metadata = suiteValidator->GetMetadata();
        TF_AXIOM(metadata.name == TfToken(
             "testValidationPlugin:TestValidatorSuite"));
        TF_AXIOM(metadata.isSuite);
        TF_AXIOM(metadata.doc == "Suite of TestValidator1 and TestValidator2");
        TF_AXIOM(metadata.keywords.size() == 2);
        TF_AXIOM(metadata.keywords == TfTokenVector({
            TfToken("IncludedInAll"), TfToken("SuiteValidator")}));
        const std::vector<const UsdValidator*> containedValidators =
            suiteValidator->GetContainedValidators();
        {
            const UsdValidatorMetadata &vm = 
                containedValidators[0]->GetMetadata();
            TF_AXIOM(vm.name == TfToken("testValidationPlugin:TestValidator1"));
            TF_AXIOM(vm.keywords.size() == 2);
            TF_AXIOM(vm.keywords == TfTokenVector({
                TfToken("IncludedInAll"), TfToken("SomeKeyword1")}));
            TF_AXIOM(vm.schemaTypes.empty());
        }
        {
            const UsdValidatorMetadata &vm = 
                containedValidators[1]->GetMetadata();
            TF_AXIOM(vm.name == TfToken("testValidationPlugin:TestValidator2"));
            TF_AXIOM(vm.keywords.size() == 1);
            TF_AXIOM(vm.keywords[0] == TfToken("IncludedInAll"));
            TF_AXIOM(vm.schemaTypes.size() == 2);
            TF_AXIOM(vm.schemaTypes == TfTokenVector({
                TfToken("SomePrimType"), TfToken("SomeAPISchema")}));
        }
    }
    {
        // Try to retrieve a failed validator (stageTask for validator which
        // provides schemaTypes
        const UsdValidator* validator = registry.GetOrLoadValidatorByName(
            TfToken("testValidationPlugin:FailedValidator"));
        TF_AXIOM(!validator);
    }
    {
        // Try to retrieve a failed validator suite (stageTask for a contained 
        // validator but suite provides schemaTypes
        const UsdValidatorSuite* suite = registry.GetOrLoadValidatorSuiteByName(
            TfToken("testValidationPlugin:FailedValidatorSuite"));
        TF_AXIOM(!suite);
    }
}

int 
main()
{
    // Register the test plugin
    const std::string testDir = ArchGetCwd();
    TF_AXIOM(!PlugRegistry::GetInstance().RegisterPlugins(testDir).empty());
    
    TestUsdValidationRegistry();

    printf("OK\n");
}
