//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validatorTokens.h"
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
        TF_AXIOM(errors[0].GetValidator() == validator);
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
    {
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
        for (size_t index = 0; index < errors.size(); ++index) {
            TF_AXIOM(errors[index].GetValidator() == compositionErrorValidator);
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
}

void
TestUsdValidators()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();

    // The following test keeps track of all the available validators within
    // UsdCoreValidators keyword, hence as new validators are added under
    // this keyword this unit test will have to be updated.
    const UsdValidatorMetadataVector coreValidatorMetadata =
            registry.GetValidatorMetadataForKeyword(
                    UsdValidatorKeywordTokens->UsdCoreValidators);
    TF_AXIOM(coreValidatorMetadata.size() == 2);

    std::set<TfToken> validatorMetadataNameSet;
    for (const UsdValidatorMetadata &metadata : coreValidatorMetadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    const std::set<TfToken> expectedValidatorNames =
            {UsdValidatorNameTokens->compositionErrorTest,
             UsdValidatorNameTokens->stageMetadataChecker};

    TF_AXIOM(std::includes(validatorMetadataNameSet.begin(),
                           validatorMetadataNameSet.end(),
                           expectedValidatorNames.begin(),
                           expectedValidatorNames.end()));
}
int 
main()
{
    // Register the test plugin
    const std::string testDir = ArchGetCwd();
    TF_AXIOM(!PlugRegistry::GetInstance().RegisterPlugins(testDir).empty());
    
    TestUsdValidationRegistry();
    TestUsdValidators();

    printf("OK\n");
}
