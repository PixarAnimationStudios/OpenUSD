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

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/ts/spline.h"

#include <iostream>
#include <set>
#include <string>
#include <utility>

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
    TF_AXIOM(coreValidatorMetadata.size() == 5);

    std::set<TfToken> validatorMetadataNameSet;
    for (const UsdValidationValidatorMetadata &metadata :
         coreValidatorMetadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    const std::set<TfToken> expectedValidatorNames
        = { UsdValidatorNameTokens->compositionErrorTest,
            UsdValidatorNameTokens->stageMetadataChecker,
            UsdValidatorNameTokens->attributeTypeMismatch,
            UsdValidatorNameTokens->attributeSpecValueTypeMismatch,
            UsdValidatorNameTokens->apiSchemaApplicability };

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

        def Sphere "Sphere2" {
            double radius = 1.0
        }
    )usda");

    // Author a string default on the double spec directly, so the prim
    // validator's value check has a mismatch to report.
    layer->SetField(
        SdfPath("/Sphere2.radius"),
        SdfFieldKeys->Default,
        VtValue(std::string("CorrectTypeButWrongValue")));

    SdfLayerRefPtr mainLayer = SdfLayer::CreateAnonymous(".usda");
    mainLayer->ImportFromString(
        TfStringPrintf(R"usda(#usda 1.0
            def Sphere "RefSphere" (
                append references = @%s@</Sphere>
            ) {
                float radius = 2.0
            }

            def Sphere "RefSphere2" (
                append references = @%s@</Sphere2>
            ) {
                double radius = 2.0
            }
        )usda",
        layer->GetIdentifier().c_str(), layer->GetIdentifier().c_str()));

    const UsdStageRefPtr usdStage = UsdStage::Open(mainLayer);
    const UsdPrim spherePrim = usdStage->GetPrimAtPath(SdfPath("/RefSphere"));
    TF_AXIOM(spherePrim);

    const UsdValidationErrorVector errors
        = attributeTypeMismatchValidator->Validate(spherePrim);
    TF_AXIOM(errors.size() == 2);
    const TfToken expectedErrorIdentifier = TfToken(
        UsdValidatorNameTokens->attributeTypeMismatch.GetString() + "." +
        UsdValidationErrorNameTokens->attributeTypeMismatch.GetString());
    const std::vector<std::string> expectedMessages = {
        TfStringPrintf("Type mismatch for attribute </RefSphere.radius>: spec "
                       "</Sphere.radius> in layer <%s> is defined as 'int' "
                       "but the expected type is 'double'.",
                       layer->GetIdentifier().c_str()),
        TfStringPrintf("Type mismatch for attribute </RefSphere.radius>: spec "
                       "</RefSphere.radius> in layer <%s> is defined as 'float' "
                       "but the expected type is 'double'.",
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

    // AttributeTypeMismatch validator should also report errors for sphere2 as
    // it also checks for value type mismatches even if the attribute type is
    // correct, like in Sphere2.radius which is defined as double but has a
    // string value in one of the contributing attribute specs in the stack.
    const UsdPrim sphere2Prim = usdStage->GetPrimAtPath(SdfPath("/RefSphere2"));
    TF_AXIOM(sphere2Prim);
    const UsdValidationErrorVector errors2
        = attributeTypeMismatchValidator->Validate(sphere2Prim);
    const TfToken expectedValueTypeErrorIdentifier = TfToken(
        UsdValidatorNameTokens->attributeTypeMismatch.GetString() + "." +
        UsdValidationErrorNameTokens->attributeSpecValueTypeMismatch.GetString());
    TF_AXIOM(errors2.size() == 1);
    TF_AXIOM(errors2[0].GetValidator() == attributeTypeMismatchValidator);
    TF_AXIOM(errors2[0].GetIdentifier() == expectedValueTypeErrorIdentifier);
    TF_AXIOM(errors2[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors2[0].GetSites().size() == 1);
    TF_AXIOM(errors2[0].GetSites()[0].IsValid());
    TF_AXIOM(errors2[0].GetSites()[0].IsValidSpecInLayer());
    const std::string layerIdentifier
        = errors2[0].GetSites()[0].GetLayer()->GetIdentifier();
    TF_AXIOM(layerIdentifier == layer->GetIdentifier());
    const std::string errorMessage = errors2[0].GetMessage();
    const std::string expectedErrorMessage =
        TfStringPrintf(
            "Attribute Spec </Sphere2.radius> of composed attribute "
            "</RefSphere2.radius> is declared as 'double' (double) in layer "
            "<%s>, but its authored default value holds a value of type "
            "'string'.",
            layer->GetIdentifier().c_str());
    TF_AXIOM(errorMessage == expectedErrorMessage);

    // A relationship spec in an attribute's property stack is reported as a
    // type mismatch at the relationship spec.
    SdfLayerRefPtr relLayer = SdfLayer::CreateAnonymous("rel.usda");
    relLayer->ImportFromString(R"usda(#usda 1.0
        def "Prim" {
            rel x
        }
    )usda");
    SdfLayerRefPtr attrLayer = SdfLayer::CreateAnonymous("attr.usda");
    attrLayer->ImportFromString(
        TfStringPrintf(R"usda(#usda 1.0
            (
                subLayers = [@%s@]
            )
            def "Prim" {
                float x = 1.0
            }
        )usda",
        relLayer->GetIdentifier().c_str()));

    const UsdStageRefPtr relStage = UsdStage::Open(attrLayer);
    const UsdPrim relPrim = relStage->GetPrimAtPath(SdfPath("/Prim"));
    TF_AXIOM(relPrim);
    const UsdValidationErrorVector errors3
        = attributeTypeMismatchValidator->Validate(relPrim);
    TF_AXIOM(errors3.size() == 1);
    TF_AXIOM(errors3[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors3[0].GetSites().size() == 1);
    TF_AXIOM(errors3[0].GetSites()[0].IsValidSpecInLayer());
    TF_AXIOM(errors3[0].GetSites()[0].GetLayer() == relLayer);
    TF_AXIOM(errors3[0].GetMessage() == TfStringPrintf(
        "Type mismatch for attribute </Prim.x>: spec </Prim.x> in layer <%s> "
        "is not an attribute spec (spec type 'SdfSpecTypeRelationship').",
        relLayer->GetIdentifier().c_str()));
}

static void
TestUsdAttributeValueTypeMismatch()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *const validator
        = registry.GetOrLoadValidatorByName(
            UsdValidatorNameTokens->attributeSpecValueTypeMismatch);
    TF_AXIOM(validator);

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usda(#usda 1.0
        def "Prim" {
            token[] goodTokens = ["a", "b"]
            token[] badDefault
            string[] goodStrings = ["a", "b"]
            token[] blocked = None
            double animationBlocked = AnimationBlock
            float badSample
            int[] arrayEdit = edit [ append 1 ]
            float badSpline
        }
    )usda");

    // The text parser, SdfAttributeSpec and SdfLayer::SetTimeSample all cast
    // values to the declared type, so author mismatched values directly as
    // raw fields, the way a file format or low-level writer could.
    const SdfPath primPath("/Prim");
    layer->SetField(primPath.AppendProperty(TfToken("badDefault")),
                    SdfFieldKeys->Default,
                    VtValue(VtStringArray { "a", "b" }));
    const SdfPath badSamplePath = primPath.AppendProperty(
        TfToken("badSample"));
    layer->SetField(badSamplePath, SdfFieldKeys->TimeSamples,
                    VtValue(SdfTimeSampleMap {
                        { 1.0, VtValue(1.0f) }, { 2.0, VtValue(2.0) } }));

    // Author a double-valued spline on a float attribute directly; SetSpline 
    // rejects it.
    layer->SetField(primPath.AppendProperty(TfToken("badSpline")),
                    SdfFieldKeys->Spline,
                    VtValue(TsSpline(TfType::Find<double>())));

    const UsdValidationErrorVector errors = validator->Validate(layer);
    TF_AXIOM(errors.size() == 3);

    const TfToken expectedErrorIdentifier = TfToken(
        UsdValidatorNameTokens->attributeSpecValueTypeMismatch.GetString() + 
        "." + 
        UsdValidationErrorNameTokens->attributeSpecValueTypeMismatch.GetString());
    const std::set<std::string> expectedMessages = {
        TfStringPrintf("Attribute Spec </Prim.badDefault> is declared as "
                       "'token[]' (VtArray<TfToken>) in layer <%s>, but its "
                       "authored default value holds a value of type "
                       "'VtArray<string>'.",
                       layer->GetIdentifier().c_str()),
        TfStringPrintf("Attribute Spec </Prim.badSample> is declared as "
                       "'float' (float) in layer <%s>, but its authored "
                       "time sample at 2 holds a value of type 'double'.",
                       layer->GetIdentifier().c_str()),
        TfStringPrintf("Attribute Spec </Prim.badSpline> is declared as "
                       "'float' (float) in layer <%s>, but its authored "
                       "spline holds a value of type 'double'.",
                       layer->GetIdentifier().c_str())
    };
    std::set<std::string> messages;
    for (const UsdValidationError &error : errors) {
        TF_AXIOM(error.GetValidator() == validator);
        TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(error.GetSites().size() == 1);
        TF_AXIOM(error.GetSites()[0].IsValidSpecInLayer());
        messages.insert(error.GetMessage());
    }
    TF_AXIOM(messages == expectedMessages);
}

static void
TestUsdAPISchemaApplicability()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *const validator
        = registry.GetOrLoadValidatorByName(
            UsdValidatorNameTokens->apiSchemaApplicability);
    TF_AXIOM(validator);

    // The test schemas in resources/ cover each restriction that
    // UsdPrim::CanApplyAPI checks: ApplicabilityTestBaseOnlyAPI can only be
    // applied to an ApplicabilityTestBase prim or a derived type,
    // ApplicabilityTestRequiresTargetAPI only to a prim with
    // ApplicabilityTestTargetAPI applied, and the multiple-apply
    // ApplicabilityTestInstancesAPI only allows the instance names "left" and
    // "right".
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usda(#usda 1.0
        def ApplicabilityTestBase "Base" (
            prepend apiSchemas = ["ApplicabilityTestBaseOnlyAPI"]
        )
        {
        }
        def ApplicabilityTestDerived "Derived" (
            prepend apiSchemas = ["ApplicabilityTestBaseOnlyAPI"]
        )
        {
        }
        def ApplicabilityTestOther "Other" (
            prepend apiSchemas = ["ApplicabilityTestBaseOnlyAPI"]
        )
        {
        }
        def "Typeless" (
            prepend apiSchemas = ["ApplicabilityTestBaseOnlyAPI"]
        )
        {
        }
        def "WithTarget" (
            prepend apiSchemas = ["ApplicabilityTestTargetAPI",
                                  "ApplicabilityTestRequiresTargetAPI"]
        )
        {
        }
        def "WithoutTarget" (
            prepend apiSchemas = ["ApplicabilityTestRequiresTargetAPI"]
        )
        {
        }
        def "AllowedInstance" (
            prepend apiSchemas = ["ApplicabilityTestInstancesAPI:left"]
        )
        {
        }
        def "DisallowedInstance" (
            prepend apiSchemas = ["ApplicabilityTestInstancesAPI:up"]
        )
        {
        }
        def "Unvalidated" (
            prepend apiSchemas = ["UnregisteredAPI", "CollectionAPI:things"]
        )
        {
        }
        def "Several" (
            prepend apiSchemas = ["ApplicabilityTestBaseOnlyAPI",
                                  "ApplicabilityTestTargetAPI",
                                  "ApplicabilityTestRequiresTargetAPI",
                                  "ApplicabilityTestInstancesAPI:down"]
        )
        {
        }
        def ApplicabilityTestBase "Prototype"
        {
            def ApplicabilityTestOther "Child" (
                prepend apiSchemas = ["ApplicabilityTestBaseOnlyAPI"]
            )
            {
            }
        }
        def "Instance" (
            instanceable = true
            prepend references = </Prototype>
        )
        {
        }
    )usda");
    UsdStageRefPtr stage = UsdStage::Open(layer);

    const std::string baseOnlyWhyNot =
        "API schema 'ApplicabilityTestBaseOnlyAPI' can only be applied to "
        "prims that are of following type, or have following API schema "
        "applied: ApplicabilityTestBase.";
    const auto baseOnlyError = [&baseOnlyWhyNot](const char *primPath) {
        return std::make_pair(
            SdfPath(primPath),
            TfStringPrintf("API schema 'ApplicabilityTestBaseOnlyAPI' is "
                           "applied to prim <%s>, but cannot be applied to "
                           "it. %s", primPath, baseOnlyWhyNot.c_str()));
    };
    const auto instancesError = [](const char *primPath,
                                   const char *instanceName) {
        return std::make_pair(
            SdfPath(primPath),
            TfStringPrintf("API schema 'ApplicabilityTestInstancesAPI:%s' is "
                           "applied to prim <%s>, but cannot be applied to "
                           "it. '%s' is not an allowed instance name for "
                           "multiple apply API schema "
                           "'ApplicabilityTestInstancesAPI'.",
                           instanceName, primPath, instanceName));
    };
    const std::set<std::pair<SdfPath, std::string>> expectedErrors = {
        baseOnlyError("/Other"),
        baseOnlyError("/Typeless"),
        std::make_pair(
            SdfPath("/WithoutTarget"),
            std::string(
                "API schema 'ApplicabilityTestRequiresTargetAPI' is applied "
                "to prim </WithoutTarget>, but cannot be applied to it. API "
                "schema 'ApplicabilityTestRequiresTargetAPI' can only be "
                "applied to prims that are of following type, or have "
                "following API schema applied: ApplicabilityTestTargetAPI.")),
        instancesError("/DisallowedInstance", "up"),
        baseOnlyError("/Several"),
        instancesError("/Several", "down"),
        baseOnlyError("/Prototype/Child"),
        baseOnlyError("/Instance/Child"),
    };

    const TfToken expectedErrorIdentifier = TfToken(
        UsdValidatorNameTokens->apiSchemaApplicability.GetString() + "." +
        UsdValidationErrorNameTokens->apiSchemaNotApplicable.GetString());
    std::set<std::pair<SdfPath, std::string>> errors;
    for (const UsdPrim &prim :
            stage->Traverse(UsdTraverseInstanceProxies())) {
        for (const UsdValidationError &error : validator->Validate(prim)) {
            TF_AXIOM(error.GetValidator() == validator);
            TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
            TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
            TF_AXIOM(error.GetSites().size() == 1);
            TF_AXIOM(error.GetSites()[0].IsPrim());
            errors.emplace(error.GetSites()[0].GetPrim().GetPath(),
                           error.GetMessage());
        }
    }
    TF_AXIOM(errors == expectedErrors);
}

int
main()
{
    // The test schemas must be registered before the schema registry is
    // first used.
    const std::string testSchemaPluginPath = ArchGetCwd() + "/resources";
    TF_AXIOM(!PlugRegistry::GetInstance()
                  .RegisterPlugins(testSchemaPluginPath).empty());

    TestUsdValidators();
    TestCoreUsdStageMetadata();
    TestUsdCompositionErrorTest();
    TestUsdAttributeTypeMismatch();
    TestUsdAttributeValueTypeMismatch();
    TestUsdAPISchemaApplicability();

    std::cout << "OK\n";
}
