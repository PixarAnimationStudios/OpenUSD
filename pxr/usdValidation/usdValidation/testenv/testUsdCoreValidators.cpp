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
    TF_AXIOM(coreValidatorMetadata.size() == 5);

    std::set<TfToken> validatorMetadataNameSet;
    for (const UsdValidationValidatorMetadata &metadata :
         coreValidatorMetadata) {
        validatorMetadataNameSet.insert(metadata.name);
        TF_AXIOM(metadata.isTimeDependent ==
            (metadata.name == UsdValidatorNameTokens->attributeHardLimits ||
             metadata.name == UsdValidatorNameTokens->attributeAllowedTokens));
    }

    const std::set<TfToken> expectedValidatorNames
        = { UsdValidatorNameTokens->compositionErrorTest,
            UsdValidatorNameTokens->stageMetadataChecker,
            UsdValidatorNameTokens->attributeTypeMismatch,
            UsdValidatorNameTokens->attributeHardLimits,
            UsdValidatorNameTokens->attributeAllowedTokens };

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

// Validate the prim at primPath with validator, and verify that it reports
// one errorName error per expected message, in order, each sited at the
// attribute named in the message.
static void
_VerifyAttributeErrors(const UsdValidationValidator *validator,
                       const TfToken &errorName,
                       const UsdStageRefPtr &usdStage,
                       const std::string &primPath,
                       const std::vector<std::string> &expectedMessages,
                       const UsdValidationTimeRange &timeRange = {})
{
    const UsdPrim prim = usdStage->GetPrimAtPath(SdfPath(primPath));
    TF_AXIOM(prim);
    const UsdValidationErrorVector errors =
        validator->Validate(prim, timeRange);
    TF_AXIOM(errors.size() == expectedMessages.size());
    const TfToken expectedErrorIdentifier = TfToken(
        validator->GetMetadata().name.GetString() + "." +
        errorName.GetString());
    for (size_t index = 0; index < errors.size(); ++index) {
        const UsdValidationError &error = errors[index];
        TF_AXIOM(error.GetValidator() == validator);
        TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(error.GetSites().size() == 1);
        TF_AXIOM(error.GetSites()[0].IsValid());
        TF_AXIOM(error.GetSites()[0].IsProperty());
        const SdfPath attrPath = error.GetSites()[0].GetProperty().GetPath();
        TF_AXIOM(TfStringContains(expectedMessages[index],
                                  "<" + attrPath.GetString() + ">"));
        TF_AXIOM(error.GetMessage() == expectedMessages[index]);
    }
}

static void
TestUsdAttributeHardLimits()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *const validator
        = registry.GetOrLoadValidatorByName(
            UsdValidatorNameTokens->attributeHardLimits);
    TF_AXIOM(validator);
    const TfToken &errorName
        = UsdValidationErrorNameTokens->attributeOutsideHardLimits;

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usda(#usda 1.0
        def "Valid" {
            float atMinimum = 0 (
                limits = { dictionary hard = { float minimum = 0 } }
            )
            int atMaximum = 10 (
                limits = { dictionary hard = { int maximum = 10 } }
            )
            double noLimits = -1e30
            float softLimits = 2 (
                limits = { dictionary soft = { float maximum = 1 } }
            )
            float conformedFloat = 0.1 (
                limits = { dictionary hard = { double maximum = 0.1 } }
            )
            color3f vector = (0, 0.5, 1) (
                limits = { dictionary hard = {
                    color3f minimum = (0, 0, 0)
                    color3f maximum = (1, 1, 1)
                } }
            )
            float[] floatArray = [-1, 2] (
                limits = { dictionary hard = { float[] minimum = [0] } }
            )
            string stringValue = "a" (
                limits = { dictionary hard = { string minimum = "b" } }
            )
        }
        def "Invalid" {
            float belowMinimum = -0.5 (
                limits = { dictionary hard = { float minimum = 0 } }
            )
            int aboveMaximum = 11 (
                limits = { dictionary hard = { int maximum = 10 } }
            )
            double nanValue = nan (
                limits = { dictionary hard = {
                    double minimum = 0
                    double maximum = 1
                } }
            )
            float conformed = 2 (
                limits = { dictionary hard = { double maximum = 1 } }
            )
            color3f vector = (0, 1.5, 1) (
                limits = { dictionary hard = {
                    color3f minimum = (0, 0, 0)
                    color3f maximum = (1, 1, 1)
                } }
            )
        }
        def "Sampled" {
            float sampled (
                limits = { dictionary hard = { float maximum = 1 } }
            )
            float sampled.timeSamples = { 1: 0.5, 2: 1.5, 3: 2.5 }
        }
        def Sphere "ValidSphere" {
            double radius = 1 (
                limits = { dictionary hard = { double minimum = 2 } }
            )
        }
        def Sphere "InvalidSphere" {
            double radius = 0.5 (
                limits = { dictionary hard = { double minimum = 2 } }
            )
        }
    )usda");
    const UsdStageRefPtr usdStage = UsdStage::Open(layer);

    // Values within their hard limits, including limits conformed to the
    // attribute's value type, values with only soft limits, and values of
    // types that hard limits do not apply to are valid.
    _VerifyAttributeErrors(validator, errorName, usdStage, "/Valid", {});

    // Values outside their hard limits are invalid. NaN is never within the
    // hard limits, limits are conformed to the attribute's value type, and
    // vectors are compared component-wise.
    _VerifyAttributeErrors(validator, errorName, usdStage, "/Invalid", {
        "Value 11 for attribute </Invalid.aboveMaximum> is outside its hard "
        "limits [-inf, 10].",
        "Value -0.5 for attribute </Invalid.belowMinimum> is outside its "
        "hard limits [0, inf].",
        "Value 2 for attribute </Invalid.conformed> is outside its hard "
        "limits [-inf, 1].",
        "Value nan for attribute </Invalid.nanValue> is outside its hard "
        "limits [0, 1].",
        "Value (0, 1.5, 1) for attribute </Invalid.vector> is outside its "
        "hard limits [(0, 0, 0), (1, 1, 1)]."
    });

    // Time samples within the validation time range are validated, and only
    // the first value outside the hard limits is reported.
    _VerifyAttributeErrors(validator, errorName, usdStage, "/Sampled", {
        "Value 1.5 at time 2 for attribute </Sampled.sampled> is outside its "
        "hard limits [-inf, 1]."
    });
    _VerifyAttributeErrors(validator, errorName, usdStage, "/Sampled", {
        "Value 2.5 at time 3 for attribute </Sampled.sampled> is outside its "
        "hard limits [-inf, 1]."
    }, UsdValidationTimeRange(GfInterval(3, 4)));
    _VerifyAttributeErrors(validator, errorName, usdStage, "/Sampled", {},
                           UsdValidationTimeRange(UsdTimeCode(1)));

    // A value equal to the schema fallback is valid, even if it is outside
    // the hard limits.
    _VerifyAttributeErrors(validator, errorName, usdStage, "/ValidSphere", {});
    _VerifyAttributeErrors(validator, errorName, usdStage, "/InvalidSphere", {
        "Value 0.5 for attribute </InvalidSphere.radius> is outside its hard "
        "limits [2, inf]."
    });
}

static void
TestUsdAttributeAllowedTokens()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *const validator
        = registry.GetOrLoadValidatorByName(
            UsdValidatorNameTokens->attributeAllowedTokens);
    TF_AXIOM(validator);
    const TfToken &errorName
        = UsdValidationErrorNameTokens->attributeTokenNotAllowed;

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usda(#usda 1.0
        def Xform "Valid" {
            uniform token purpose = "render"
            token[] tokenArray = ["a", "", "b"] (allowedTokens = ["a", "b", ""])
            string stringValue = "a" (allowedTokens = ["a", "b"])
            string[] stringArray = ["a", "b"] (allowedTokens = ["a", "b"])
            token unrestricted = "anything"
            token emptyAllowedTokens = "anything" (allowedTokens = [])
        }
        def Xform "Invalid" {
            uniform token purpose = "bogus"
            token[] tokenArray = ["a", "c", "d"] (allowedTokens = ["a", "b"])
            string stringValue = "c" (allowedTokens = ["a", "b"])
            string[] stringArray = ["a", "c"] (allowedTokens = ["a", "b"])
        }
        def Xform "EmptyToken" {
            uniform token purpose = ""
        }
        def Xform "Sampled" {
            token visibility.timeSamples = {
                1: "inherited", 2: "bogus", 3: "hidden"
            }
        }
        def Xform "Fallback" {
            uniform token purpose = "default" (allowedTokens = ["render"])
        }
        def Xform "NotFallback" {
            uniform token purpose = "proxy" (allowedTokens = ["render"])
        }
    )usda");
    const UsdStageRefPtr usdStage = UsdStage::Open(layer);

    // Values that are allowed, and attributes with no or empty allowedTokens,
    // are valid.
    _VerifyAttributeErrors(validator, errorName, usdStage, "/Valid", {});

    // Values that are not allowed are invalid, whether the allowed tokens are
    // declared by the schema or authored. Only the first element of an array
    // value that is not allowed is reported.
    _VerifyAttributeErrors(validator, errorName, usdStage, "/Invalid", {
        "Value 'bogus' for attribute </Invalid.purpose> is not one of its "
        "allowed tokens ['default', 'render', 'proxy', 'guide'].",
        "Value 'c' for attribute </Invalid.stringArray> is not one of its "
        "allowed tokens ['a', 'b'].",
        "Value 'c' for attribute </Invalid.stringValue> is not one of its "
        "allowed tokens ['a', 'b'].",
        "Value 'c' for attribute </Invalid.tokenArray> is not one of its "
        "allowed tokens ['a', 'b']."
    });

    // The empty token is only valid if it is one of the allowed tokens.
    _VerifyAttributeErrors(validator, errorName, usdStage, "/EmptyToken", {
        "Value '' for attribute </EmptyToken.purpose> is not one of its "
        "allowed tokens ['default', 'render', 'proxy', 'guide']."
    });

    // Time samples within the validation time range are validated, and only
    // the first value that is not allowed is reported.
    _VerifyAttributeErrors(validator, errorName, usdStage, "/Sampled", {
        "Value 'bogus' at time 2 for attribute </Sampled.visibility> is not "
        "one of its allowed tokens ['inherited', 'invisible']."
    });
    _VerifyAttributeErrors(validator, errorName, usdStage, "/Sampled", {},
                           UsdValidationTimeRange(UsdTimeCode(1)));

    // A value equal to the schema fallback is valid, even if it is not one of
    // the allowed tokens.
    _VerifyAttributeErrors(validator, errorName, usdStage, "/Fallback", {});
    _VerifyAttributeErrors(validator, errorName, usdStage, "/NotFallback", {
        "Value 'proxy' for attribute </NotFallback.purpose> is not one of its "
        "allowed tokens ['render']."
    });
}

int
main()
{
    TestUsdValidators();
    TestCoreUsdStageMetadata();
    TestUsdCompositionErrorTest();
    TestUsdAttributeTypeMismatch();
    TestUsdAttributeHardLimits();
    TestUsdAttributeAllowedTokens();

    std::cout << "OK\n";
}
