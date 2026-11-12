//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnosticTrap.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usdValidation/usdValidation/context.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include <iostream>
#include <set>
#include <string>
#include <utility>

PXR_NAMESPACE_USING_DIRECTIVE

static std::string
_LayerContents()
{
    static const std::string layerContents =
        R"usda(#usda 1.0
        def "World"
        {
            def BaseTypeTest "baseType"
            {
            }
            def DerivedTypeTest "derivedType"
            {
            }
            def NestedDerivedTypeTest "nestedDerivedType"
            {
            }
            def "somePrim" (
                prepend apiSchemas = ["APISchemaTestAPI"]
            )
            {
            }
        }
    )usda";
    return layerContents;
}

static SdfLayerRefPtr
_CreateTestLayer()
{
    SdfLayerRefPtr testLayer = SdfLayer::CreateAnonymous(".usda");
    testLayer->ImportFromString(_LayerContents());
    return testLayer;
}

static void
_TestError1(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test1"));
    TF_AXIOM(error.GetSites().size() == 1);
    TF_AXIOM(error.GetSites()[0].IsPrim());
    TF_AXIOM(error.GetSites()[0].GetPrim().GetPath()
             == SdfPath::AbsoluteRootPath());
}

static void
_TestError2(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test2"));
    TF_AXIOM(error.GetSites().size() == 1);
    TF_AXIOM(error.GetSites()[0].IsValidSpecInLayer());
}

static void
_TestError3(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test3"));
    const std::set<SdfPath> expectedPrimPaths
        = { SdfPath("/World"), SdfPath("/World/baseType"),
            SdfPath("/World/derivedType"), SdfPath("/World/nestedDerivedType"),
            SdfPath("/World/somePrim") };
    TF_AXIOM(expectedPrimPaths.find(error.GetSites()[0].GetPrim().GetPath())
             != expectedPrimPaths.end());
}

static void
_TestError4(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test4"));
    const std::set<SdfPath> expectedPrimPaths
        = { SdfPath("/World/baseType"), SdfPath("/World/derivedType"),
            SdfPath("/World/nestedDerivedType") };
    TF_AXIOM(expectedPrimPaths.find(error.GetSites()[0].GetPrim().GetPath())
             != expectedPrimPaths.end());
}

static void
_TestError5(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test5"));
    const std::set<SdfPath> expectedPrimPaths
        = { SdfPath("/World/derivedType"),
            SdfPath("/World/nestedDerivedType") };
    TF_AXIOM(expectedPrimPaths.find(error.GetSites()[0].GetPrim().GetPath())
             != expectedPrimPaths.end());
}

static void
_TestError6(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test6"));
    TF_AXIOM(error.GetSites().size() == 1);
    TF_AXIOM(error.GetSites()[0].IsPrim());
    TF_AXIOM(error.GetSites()[0].GetPrim().GetName()
             == TfToken("nestedDerivedType"));
}

static void
_TestError7(const UsdValidationError &error)
{
    TF_AXIOM(error.GetName() == TfToken("Test7Error"));
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test7"));
    TF_AXIOM(error.GetSites().size() == 1);
    TF_AXIOM(error.GetSites()[0].IsPrim());
    TF_AXIOM(error.GetSites()[0].GetPrim().GetName() == TfToken("somePrim"));
}

static void
_TestNonPluginError(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("nonPluginValidator"));

    const SdfPath expectedPrimPaths = SdfPath("/");
    TF_AXIOM(error.GetSites()[0].GetPrim().GetPath() == expectedPrimPaths);
}

static void
_TestUsdValidationContext()
{
    // Test the UsdValidationContext here.
    {
        // Create a ValidationContext with a suite
        const UsdValidationValidatorSuite *suite
            = UsdValidationRegistry::GetInstance()
                  .GetOrLoadValidatorSuiteByName(TfToken(
                      "testUsdValidationContextValidatorsPlugin:TestSuite"));
        UsdValidationContext context({ suite });
        SdfLayerRefPtr testLayer = _CreateTestLayer();
        // Run Validate(layer)
        UsdValidationErrorVector errors = context.Validate(testLayer);
        // 1 error for Test2 validator - root layer of the stage
        TF_AXIOM(errors.size() == 1);
        _TestError2(errors[0]);

        // Run Validate(stage)
        errors.clear();
        UsdStageRefPtr stage = UsdStage::Open(testLayer);
        errors = context.Validate(stage);
        // 1 error for Test1 validator (stage)
        // 2 error for Test2 validator - root layer and session layer
        // 5 errors for Test3 generic prim validator which runs on all 5 prims
        TF_AXIOM(errors.size() == 8);
        for (const auto &error : errors) {
            if (error.GetName() == TfToken("Test1Error")) {
                // validator corresponding to this error has fixer too, so lets
                // test that
                const std::vector<const UsdValidationFixer*> fixers = 
                    error.GetFixers();
                TF_AXIOM(fixers.size() == 3);
                TF_AXIOM(fixers[0]->GetName() == TfToken("TestFixer1"));
                TF_AXIOM(fixers[1]->GetName() == TfToken("TestFixer2"));
                TF_AXIOM(fixers[2]->GetName() == TfToken("TestFixer3"));
                TF_AXIOM(fixers[0]->IsAssociatedWithErrorName(
                    TfToken("Test1Error")) == true);
                TF_AXIOM(fixers[1]->IsAssociatedWithErrorName(
                    TfToken("Test1Error")) == true);
                TF_AXIOM(fixers[2]->IsAssociatedWithErrorName(
                    TfToken("Test1Error")) == false);
                // No edit target provided
                TF_AXIOM(fixers[0]->CanApplyFix(
                    error, UsdEditTarget(), UsdTimeCode::Default()) == false);
                TF_AXIOM(fixers[0]->CanApplyFix(
                    error, stage->GetEditTarget(), UsdTimeCode::Default()) ==
                true); 
                TF_AXIOM(fixers[1]->CanApplyFix(
                    error, stage->GetEditTarget(), UsdTimeCode::Default()) ==
                true);
                TF_AXIOM(fixers[2]->CanApplyFix(
                    error, stage->GetEditTarget(), UsdTimeCode::Default()) ==
                false);
                _TestError1(error);
            } else if (error.GetName() == TfToken("Test2Error")) {
                _TestError2(error);
            } else if (error.GetName() == TfToken("Test3Error")) {
                _TestError3(error);
            } else {
                TF_AXIOM(false);
            }
        }
    }
    {
        // Create a ValidationContext with explicit schemaTypes
        UsdValidationContext context({ TfType::FindByName("testBaseType") });
        SdfLayerRefPtr testLayer = _CreateTestLayer();
        // Run Validate(layer)
        UsdValidationErrorVector errors = context.Validate(testLayer);
        // 0 errors as we do not have any layer validators selected in this
        // context.
        TF_AXIOM(errors.empty());

        // Run Validate(stage)
        UsdStageRefPtr stage = UsdStage::Open(testLayer);
        errors = context.Validate(stage);
        // 3 errors for Test4 testBaseType prim type validator which runs on
        // the baseType, derivedType and nestedDerivedType prims
        TF_AXIOM(errors.size() == 3);

        for (const auto &error : errors) {
            _TestError4(error);
        }
    }
    {
        // Create a ValidationContext with explicit schemaType - apiSchema
        UsdValidationContext context(
            { TfType::FindByName("testAPISchemaAPI") });
        SdfLayerRefPtr testLayer = _CreateTestLayer();
        // Run Validate(layer)
        UsdValidationErrorVector errors = context.Validate(testLayer);
        // 0 errors as we do not have any layer validators selected in this
        // context.
        TF_AXIOM(errors.empty());

        // Run Validate(stage)
        UsdStageRefPtr stage = UsdStage::Open(testLayer);
        errors = context.Validate(stage);
        // 1 error for Test7 testAPISchema prim type validator which runs on
        // the somePrim prim
        TF_AXIOM(errors.size() == 1);
        _TestError7(errors[0]);
    }
    {
        // Create a ValidationContext with keywords API and have
        // includeAllAncestors set to default (true)
        UsdValidationContext context({ TfToken("Keyword1") });
        SdfLayerRefPtr testLayer = _CreateTestLayer();
        // Run Validate(layer)
        UsdValidationErrorVector errors = context.Validate(testLayer);
        // 0 errors as we do not have any layer validators selected in this
        // context.
        TF_AXIOM(errors.empty());

        // Run Validate(stage)
        UsdStageRefPtr stage = UsdStage::Open(testLayer);
        errors = context.Validate(stage);
        // 1 error for Test1 validator
        // 5 errors for Test3 generic prim validator which runs on all 5 prims
        // 2 errors for Test5 testDerivedType prim type validator which runs on
        //   the derivedType and nestedDerivedType prims
        // 3 errors for Test4 testBaseType prim type validator which runs on
        // the baseType, derivedType and nestedDerivedType prims (This gets
        // includes as an ancestor type of derivedType)
        // 1 error for Test7 testAPISchema prim type validator which runs on
        //  the somePrim prim
        TF_AXIOM(errors.size() == 12);

        for (const auto &error : errors) {
            if (error.GetName() == TfToken("Test1Error")) {
                _TestError1(error);
            } else if (error.GetName() == TfToken("Test3Error")) {
                _TestError3(error);
            } else if (error.GetName() == TfToken("Test4Error")) {
                _TestError4(error);
            } else if (error.GetName() == TfToken("Test5Error")) {
                _TestError5(error);
            } else if (error.GetName() == TfToken("Test7Error")) {
                _TestError7(error);
            } else {
                TF_AXIOM(false);
            }
        }
    }
    {
        // Create a ValidationContext with keywords API and have
        // includeAllAncestors set to false.
        UsdValidationContext context({ TfToken("Keyword2") }, false);
        SdfLayerRefPtr testLayer = _CreateTestLayer();
        // Run Validate(layer)
        UsdValidationErrorVector errors = context.Validate(testLayer);
        // 1 error for Test2 validator - root layer of the stage
        TF_AXIOM(errors.size() == 1);
        _TestError2(errors[0]);

        errors.clear();
        // Run Validate(prims)
        UsdStageRefPtr stage = UsdStage::Open(testLayer);
        errors = context.Validate(stage->Traverse());
        // 3 errors for Test4 testBaseType prim type validator which runs on
        //  the baseType, derivedType and nestedDerivedType prims
        // 1 error for Test6 testNestedDerivedType prim type validator which
        //  runs on the nestedDerivedType prim
        // 5 errors for testNonPluginValidator which runs on all prims
        // Because of TestSuite:
        // 5 errors for Test3 generic prim validator which runs on all 5 prims
        TF_AXIOM(errors.size() == 14);

        for (const auto &error : errors) {
            if (error.GetName() == TfToken("Test3Error")) {
                _TestError3(error);
            } else if (error.GetName() == TfToken("Test4Error")) {
                _TestError4(error);
            } else if (error.GetName() == TfToken("Test6Error")) {
                _TestError6(error);
            } else if (error.GetName() == TfToken("nonPluginError")) {
                _TestNonPluginError(error);
            } else {
                TF_AXIOM(false);
            }
        }

        errors.clear();
        // Run Validate(stage)
        errors = context.Validate(stage);
        // 2 error for Test2 validator - root layer and session layer
        // 3 errors for Test4 testBaseType prim type validator which runs on
        //  the baseType, derivedType and nestedDerivedType prims
        // 1 error for Test6 testNestedDerivedType prim type validator which
        //  runs on the nestedDerivedType prim
        // 5 errors for testNonPluginValidator which runs on all prims
        // Because of TestSuite:
        // 1 error for Test1 validator
        // 5 errors for Test3 generic prim validator which runs on all 5 prims
        TF_AXIOM(errors.size() == 17);

        for (const auto &error : errors) {
            if (error.GetName() == TfToken("Test1Error")) {
                _TestError1(error);
            } else if (error.GetName() == TfToken("Test2Error")) {
                _TestError2(error);
            } else if (error.GetName() == TfToken("Test3Error")) {
                _TestError3(error);
            } else if (error.GetName() == TfToken("Test4Error")) {
                _TestError4(error);
            } else if (error.GetName() == TfToken("Test6Error")) {
                _TestError6(error);
            } else if (error.GetName() == TfToken("nonPluginError")) {
                _TestNonPluginError(error);
            } else {
                TF_AXIOM(false);
            }
        }
    }
    {
        // Create a ValidationContext with plugins
        UsdValidationContext context(
            { PlugRegistry::GetInstance().GetPluginWithName(
                "testUsdValidationContextValidatorsPlugin") });
        SdfLayerRefPtr testLayer = _CreateTestLayer();
        UsdStageRefPtr stage = UsdStage::Open(testLayer);
        UsdValidationErrorVector errors = context.Validate(stage);
        // 1 error for Test1 validator
        // 2 error for Test2 validator - root layer and session layer
        // 5 errors for Test3 generic prim validator which runs on all 5 prims
        // 3 errors for Test4 testBaseType prim type validator which runs on
        //  the baseType, derivedType and nestedDerivedType prims
        // 2 error for Test5 testDerivedType prim type validator which runs on
        //   the derivedType and nestedDerivedType prims
        // 1 error for Test6 testNestedDerivedType prim type validator which
        //  runs on the nestedDerivedType prim
        // 1 error for Test7 testAPISchema prim type validator which runs on
        //  the somePrim prim
        TF_AXIOM(errors.size() == 15);

        for (const auto &error : errors) {
            if (error.GetName() == TfToken("Test1Error")) {
                _TestError1(error);
            } else if (error.GetName() == TfToken("Test2Error")) {
                _TestError2(error);
            } else if (error.GetName() == TfToken("Test3Error")) {
                _TestError3(error);
            } else if (error.GetName() == TfToken("Test4Error")) {
                _TestError4(error);
            } else if (error.GetName() == TfToken("Test5Error")) {
                _TestError5(error);
            } else if (error.GetName() == TfToken("Test6Error")) {
                _TestError6(error);
            } else if (error.GetName() == TfToken("Test7Error")) {
                _TestError7(error);
            } else {
                TF_AXIOM(false);
            }
        }
    }
    {
        static const std::string layerWithTimeCodes =
            R"usda(#usda 1.0
            def "World"
            {
                def BaseTypeTest "baseType"
                {
                    float attr = 3
                    float attr.timeSamples = {
                        1: 1,
                        2: 2,
                        3: 3
                    }
                }
            }
        )usda";

        // Create a ValidationContext with Test3 and Test4 validators
        std::vector<const UsdValidationValidator *> validators =
            UsdValidationRegistry::GetInstance().GetOrLoadValidatorsByName(
                { TfToken("testUsdValidationContextValidatorsPlugin:Test3"),
                  TfToken("testUsdValidationContextValidatorsPlugin:Test4") });
        UsdValidationContext context(validators);

        SdfLayerRefPtr testLayer = SdfLayer::CreateAnonymous(".usda");
        testLayer->ImportFromString(layerWithTimeCodes);
        UsdStageRefPtr stage = UsdStage::Open(testLayer);

        // Validate Stage with ProxyPrim traversal predicate and default
        // UsdValidationTimeRange which is full interval.
        UsdValidationErrorVector errors = context.Validate(stage);
        // Refer the implementation for Test3 in 
        // testUsdValidationContextValidators.cpp:
        // 1 - error for Test4 testBaseType prim type validator which runs on
        //   the baseType prim. (This is not a time dependent validator).
        // 1 - error for Test3 generic prim validator which runs on the /World
        //   prim with full interval + default but since no attributes on 
        //   /World, an error is reported for that in the implementation.
        // 1 - error for Test3 generic prim validator emulating handling of
        //   uniform attribute validation at default time.
        // 3 - errors for Test3 generic prim validator which runs on the 
        //   /World/baseType prim with full interval, that is 1 error for each
        //   time sample on the attr attribute.
        TF_AXIOM(errors.size() == 6);

        // Also Validate the same stage but with only 
        // GfInterval::GetFullInterval, which does not include 
        // UsdTimeCode::Default
        errors.clear();
        errors = context.Validate(
            stage, UsdValidationTimeRange(
                GfInterval::GetFullInterval(), false));
        // Refer the implementation for Test3 in 
        // testUsdValidationContextValidators.cpp:
        // 1 - error for Test4 testBaseType prim type validator which runs on
        //   the baseType prim. (This is not a time dependent validator).
        // 3 - errors for Test3 generic prim validator which runs on the 
        //   /World/baseType prim with full interval, that is 1 error for each
        //   time sample on the attr attribute.
        TF_AXIOM(errors.size() == 4);

        // Also Validate the /World/baseType prim which has attributes with
        // timeSamples but with a TimeRange which has an empty interval but
        // includes UsdTimeCode::Default.
        std::vector<UsdPrim> prims = 
            { stage->GetPrimAtPath(SdfPath("/World/baseType")) };
        errors.clear();
        errors = context.Validate(prims, 
                                  UsdValidationTimeRange(GfInterval(), true));
        // 1 error for Test4 testBaseType prim type validator which runs on
        //   the baseType prim. (This is not a time dependent validator).
        // 1 error for Test3 generic prim validator which runs on the
        //   /World/baseType prim with empty interval but includes 
        //   UsdTimeCode::Default. (The time dependent validator implementation 
        //   has a check for this case to emulate uniform attribute validation 
        //   on a prim which also has time dependent attributes.)
        TF_AXIOM(errors.size() == 2);

        // Also Validate a specific prim at a specific timeCode
        errors.clear();
        const std::vector<UsdTimeCode> timeCodes = { 1, 2 };
        // Validate specific /World/baseType prim at times 1 and 2.
        errors = context.Validate(prims, timeCodes);
        // 1 error for Test4 (non time dependent validator) testBaseType prim 
        // type validator which runs on the baseType prim.
        // 2 errors for Test3 generic prim validator which runs on the
        //  /World/baseType prim at timeCode 1 and 2, specified times.
        TF_AXIOM(errors.size() == 3);
    }
}

// Registers a non-plugin prim validator named \p name with \p schemaTypes,
// which reports one error at each prim it runs on, and returns it.
static const UsdValidationValidator *
_RegisterSchemaTypeValidator(const std::string &name,
                             const TfTokenVector &schemaTypes)
{
    UsdValidationValidatorMetadata metadata;
    metadata.name = TfToken(name);
    metadata.doc = "Validator for schemaTypes resolution tests.";
    metadata.schemaTypes = schemaTypes;
    metadata.pluginPtr = nullptr;
    metadata.isTimeDependent = false;
    metadata.isSuite = false;

    const UsdValidatePrimTaskFn primTaskFn = [](
        const UsdPrim &prim,
        const UsdValidationTimeRange &/*timeRange*/) {
        return UsdValidationErrorVector { UsdValidationError(
            TfToken("SchemaTypeTestError"), UsdValidationErrorType::Error,
            { UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
            "A schemaTypes resolution test validator error") };
    };

    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    registry.RegisterValidator(metadata, primTaskFn);
    const UsdValidationValidator *const validator
        = registry.GetOrLoadValidatorByName(metadata.name);
    TF_AXIOM(validator);
    return validator;
}

using _ValidatorPrimPairs = std::set<std::pair<std::string, std::string>>;

// Returns the (validator name, prim path) pair for each of \p errors.
static _ValidatorPrimPairs
_GetValidatorPrimPairs(const UsdValidationErrorVector &errors)
{
    _ValidatorPrimPairs result;
    for (const UsdValidationError &error : errors) {
        TF_AXIOM(error.GetSites().size() == 1);
        result.emplace(error.GetValidator()->GetMetadata().name.GetString(),
                       error.GetSites()[0].GetPrim().GetPath().GetString());
    }
    return result;
}

// Returns the validator names in \p metadata.
static std::set<std::string>
_GetValidatorNames(const UsdValidationValidatorMetadataVector &metadata)
{
    std::set<std::string> result;
    for (const UsdValidationValidatorMetadata &m : metadata) {
        result.insert(m.name.GetString());
    }
    return result;
}

static void
_TestSchemaTypeIdentifiers()
{
    // Make sure the test schema identifiers resolve, else the tests below
    // would fail for an unrelated reason.
    TF_AXIOM(UsdSchemaRegistry::GetTypeFromName(TfToken("BaseTypeTest"))
             == TfType::FindByName("testBaseType"));
    TF_AXIOM(UsdSchemaRegistry::GetTypeFromName(TfToken("APISchemaTestAPI"))
             == TfType::FindByName("testAPISchemaAPI"));

    const UsdStageRefPtr stage = UsdStage::Open(_CreateTestLayer());

    // Typed schema, by schema identifier and by TfType name. Each runs on
    // the baseType prim and the prims derived from it.
    for (const std::string schemaType : { "BaseTypeTest", "testBaseType" }) {
        const std::string name = "schemaTypeIdentifiers:" + schemaType;
        const UsdValidationValidator *const validator
            = _RegisterSchemaTypeValidator(name, { TfToken(schemaType) });
        const UsdValidationContext context(
            std::vector<const UsdValidationValidator *> { validator });
        TF_AXIOM(_GetValidatorPrimPairs(context.Validate(stage))
                 == _ValidatorPrimPairs({
                     { name, "/World/baseType" },
                     { name, "/World/derivedType" },
                     { name, "/World/nestedDerivedType" } }));
    }

    // Applied API schema, by schema identifier and by TfType name. Each runs
    // on the somePrim prim only.
    for (const std::string schemaType :
         { "APISchemaTestAPI", "testAPISchemaAPI" }) {
        const std::string name = "schemaTypeIdentifiers:" + schemaType;
        const UsdValidationValidator *const validator
            = _RegisterSchemaTypeValidator(name, { TfToken(schemaType) });
        const UsdValidationContext context(
            std::vector<const UsdValidationValidator *> { validator });
        TF_AXIOM(_GetValidatorPrimPairs(context.Validate(stage))
                 == _ValidatorPrimPairs({ { name, "/World/somePrim" } }));
    }
}

static void
_TestSchemaTypeMultipleApply()
{
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usda(#usda 1.0
        def "World"
        {
            def "collectionPrim" (
                prepend apiSchemas = ["CollectionAPI:foo"]
            )
            {
            }
        }
    )usda");
    const UsdStageRefPtr stage = UsdStage::Open(layer);

    // A multiple-apply API schema is named without an instance name, and
    // matches any applied instance.
    for (const std::string schemaType :
         { "CollectionAPI", "UsdCollectionAPI" }) {
        const std::string name = "schemaTypeMultipleApply:" + schemaType;
        const UsdValidationValidator *const validator
            = _RegisterSchemaTypeValidator(name, { TfToken(schemaType) });
        const UsdValidationContext context(
            std::vector<const UsdValidationValidator *> { validator });
        TF_AXIOM(_GetValidatorPrimPairs(context.Validate(stage))
                 == _ValidatorPrimPairs({
                     { name, "/World/collectionPrim" } }));
    }
}

static void
_TestSchemaTypeDuplicateEntries()
{
    const UsdStageRefPtr stage = UsdStage::Open(_CreateTestLayer());

    // Both entries name the same schema, so the validator runs once per
    // matching prim, not once per entry.
    const UsdValidationValidator *const validator
        = _RegisterSchemaTypeValidator(
            "schemaTypeDuplicateEntries:validator",
            { TfToken("testBaseType"), TfToken("BaseTypeTest") });
    const UsdValidationContext context(
        std::vector<const UsdValidationValidator *> { validator });
    // 3 errors, 1 each for the baseType, derivedType and nestedDerivedType
    // prims.
    TF_AXIOM(context.Validate(stage).size() == 3);
}

static void
_TestSchemaTypeAncestors()
{
    const UsdStageRefPtr stage = UsdStage::Open(_CreateTestLayer());

    const std::string baseName = "schemaTypeAncestors:base";
    const std::string derivedName = "schemaTypeAncestors:derived";
    _RegisterSchemaTypeValidator(baseName, { TfToken("BaseTypeTest") });
    const UsdValidationValidator *const derivedValidator
        = _RegisterSchemaTypeValidator(derivedName,
                                       { TfToken("DerivedTypeTest") });

    // The context also collects every other validator registered for these
    // types, so only compare the errors from the validators above.
    const auto getAncestorTestPairs
        = [&](const UsdValidationContext &context) {
        _ValidatorPrimPairs result;
        for (const auto &pair :
             _GetValidatorPrimPairs(context.Validate(stage))) {
            if (pair.first == baseName || pair.first == derivedName) {
                result.insert(pair);
            }
        }
        return result;
    };
    const _ValidatorPrimPairs expected({
        { baseName, "/World/baseType" },
        { baseName, "/World/derivedType" },
        { baseName, "/World/nestedDerivedType" },
        { derivedName, "/World/derivedType" },
        { derivedName, "/World/nestedDerivedType" } });

    // A validator naming an ancestor type by its identifier is collected when
    // creating a context from a derived TfType.
    TF_AXIOM(getAncestorTestPairs(UsdValidationContext(
                 std::vector<TfType> {
                     TfType::FindByName("testDerivedType") }))
             == expected);

    // Same, when the derived type comes from validator metadata which names
    // it by its identifier.
    TF_AXIOM(getAncestorTestPairs(UsdValidationContext(
                 UsdValidationValidatorMetadataVector {
                     derivedValidator->GetMetadata() },
                 /* includeAllAncestors */ true))
             == expected);
}

static void
_TestSchemaTypeQueries()
{
    const std::string identifierName = "schemaTypeQueries:identifier";
    const std::string typeName = "schemaTypeQueries:typeName";
    _RegisterSchemaTypeValidator(identifierName, { TfToken("BaseTypeTest") });
    _RegisterSchemaTypeValidator(typeName, { TfToken("testBaseType") });

    // Querying by either name returns the validators registered by either
    // name. Compare as sets, as the result may contain duplicates.
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const std::set<std::string> byIdentifier = _GetValidatorNames(
        registry.GetValidatorMetadataForSchemaType(TfToken("BaseTypeTest")));
    const std::set<std::string> byTypeName = _GetValidatorNames(
        registry.GetValidatorMetadataForSchemaType(TfToken("testBaseType")));
    TF_AXIOM(byIdentifier == byTypeName);
    TF_AXIOM(byIdentifier.count(identifierName));
    TF_AXIOM(byIdentifier.count(typeName));
}

static void
_TestSchemaTypeUnknown()
{
    const std::string name = "schemaTypeUnknown:validator";
    const UsdValidationValidator *validator = nullptr;
    {
        // A schemaTypes entry which names no schema type warns on
        // registration, naming both the validator and the entry.
        TfDiagnosticTrap trap;
        validator = _RegisterSchemaTypeValidator(
            name, { TfToken("NotASchemaType") });
        const std::vector<TfWarning> &warnings = trap.GetWarnings();
        TF_AXIOM(warnings.size() == 1);
        TF_AXIOM(TfStringContains(warnings[0].GetCommentary(), name));
        TF_AXIOM(TfStringContains(warnings[0].GetCommentary(),
                                  "NotASchemaType"));
        trap.ClearWarnings();
    }

    // Such a validator never runs on any prim.
    const UsdStageRefPtr stage = UsdStage::Open(_CreateTestLayer());
    const UsdValidationContext context(
        std::vector<const UsdValidationValidator *> { validator });
    TF_AXIOM(context.Validate(stage).empty());

    // But it can still be queried by the name it was registered with.
    TF_AXIOM(_GetValidatorNames(
                 UsdValidationRegistry::GetInstance()
                     .GetValidatorMetadataForSchemaType(
                         TfToken("NotASchemaType"))).count(name));
}

int
main()
{
    // Register the test plugins
    // Plugin which provides test usd schema types
    const std::string testTypePluginPath = ArchGetCwd() + "/resources";
    TF_AXIOM(!PlugRegistry::GetInstance()
                  .RegisterPlugins(testTypePluginPath)
                  .empty());
    // Plugin which provides test validators
    const std::string testValidatorPluginPath
        = TfStringCatPaths(
              TfGetPathName(ArchGetExecutablePath()),
              "UsdValidationPlugins/lib/TestUsdValidationContextValidators*/"
              "Resources/")
        + "/";
    TF_AXIOM(!PlugRegistry::GetInstance()
                  .RegisterPlugins(testValidatorPluginPath)
                  .empty());

    // Add a non-plugin based validator here.
    {
        UsdValidationValidatorMetadata metadata;
        metadata.name = TfToken("nonPluginValidator");
        metadata.keywords = { TfToken("Keyword2") };
        metadata.pluginPtr = nullptr;
        metadata.doc = "This is a non-plugin based validator.";
        metadata.isTimeDependent = false;
        metadata.isSuite = false;

        const UsdValidatePrimTaskFn primTaskFn = [](
            const UsdPrim &prim,
            const UsdValidationTimeRange &/*timeRange*/) {
            const TfToken errorId("nonPluginError");
            return UsdValidationErrorVector { UsdValidationError(
                errorId, UsdValidationErrorType::Error,
                { UsdValidationErrorSite(prim.GetStage(),
                                         SdfPath::AbsoluteRootPath()) },
                "A non-plugin based validator error") };
        };

        // Register the validator
        TfErrorMark m;
        UsdValidationRegistry::GetInstance().RegisterValidator(metadata,
                                                               primTaskFn);
        TF_AXIOM(m.IsClean());
    }

    _TestUsdValidationContext();

    // These register further validators, so run them after
    // _TestUsdValidationContext, whose error counts depend on the set of
    // registered validators.
    _TestSchemaTypeIdentifiers();
    _TestSchemaTypeMultipleApply();
    _TestSchemaTypeDuplicateEntries();
    _TestSchemaTypeAncestors();
    _TestSchemaTypeQueries();
    _TestSchemaTypeUnknown();
}
