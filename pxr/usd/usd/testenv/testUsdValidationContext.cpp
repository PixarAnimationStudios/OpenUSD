//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/plug/registry.h"
#include "pxr/usd/usd/validationContext.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validator.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();

    // Register test plugin validators here
    // Test validators simply just return errors, we need to make sure various
    // UsdValidationContext APIs work and get the expected errors back, when
    // Validate is called in various scenarios on a validation context instance.
    {
        const TfToken validatorName("testUsdValidationContext:Test1");
        const UsdValidateStageTaskFn stageTaskFn = [](
            const UsdStagePtr & usdStage)
        {
            const TfToken validationErrorId("Test1Error");
            return UsdValidationErrorVector{
                UsdValidationError(
                    validationErrorId, UsdValidationErrorType::Error, 
                    {UsdValidationErrorSite(usdStage, 
                                            SdfPath::AbsoluteRootPath())},
                    "A stage validator error")};
        };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, stageTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName("testUsdValidationContext:Test2");
        const UsdValidateLayerTaskFn layerTaskFn = [](
            const SdfLayerHandle & layer)
        {
            const TfToken validationErrorId("Test2Error");
            return UsdValidationErrorVector{
                UsdValidationError(
                    validationErrorId, UsdValidationErrorType::Error, 
                    {UsdValidationErrorSite(layer, 
                                            SdfPath::AbsoluteRootPath())},
                    "A layer validator error")};
        };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, layerTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName("testUsdValidationContext:Test3");
        const UsdValidatePrimTaskFn primTaskFn = [](
            const UsdPrim & prim)
        {
            const TfToken validationErrorId("Test3Error");
            return UsdValidationErrorVector{
                UsdValidationError(
                    validationErrorId, UsdValidationErrorType::Error, 
                    {UsdValidationErrorSite(prim.GetStage(), 
                                            prim.GetPath())},
                    "A generic prim validator error")};
        };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName("testUsdValidationContext:Test4");
        const UsdValidatePrimTaskFn primTaskFn = [](
            const UsdPrim & prim)
        {
            const TfToken validationErrorId("Test4Error");
            return UsdValidationErrorVector{
                UsdValidationError(
                    validationErrorId, UsdValidationErrorType::Error, 
                    {UsdValidationErrorSite(prim.GetStage(), 
                                            prim.GetPath())},
                    "A testBaseType prim type validator error")};
        };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName("testUsdValidationContext:Test5");
        const UsdValidatePrimTaskFn primTaskFn = [](
            const UsdPrim & prim)
        {
            const TfToken validationErrorId("Test5Error");
            return UsdValidationErrorVector{
                UsdValidationError(
                    validationErrorId, UsdValidationErrorType::Error, 
                    {UsdValidationErrorSite(prim.GetStage(), 
                                            prim.GetPath())},
                    "A testDerivedType prim type validator error")};
        };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName("testUsdValidationContext:Test6");
        const UsdValidatePrimTaskFn primTaskFn = [](
            const UsdPrim & prim)
        {
            const TfToken validationErrorId("Test6Error");
            return UsdValidationErrorVector{
                UsdValidationError(
                    validationErrorId, UsdValidationErrorType::Error, 
                    {UsdValidationErrorSite(prim.GetStage(), 
                                            prim.GetPath())},
                    "A testNestedDerivedType prim type validator error")};
        };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken validatorName("testUsdValidationContext:Test7");
        const UsdValidatePrimTaskFn primTaskFn = [](
            const UsdPrim & prim)
        {
            const TfToken validationErrorId("Test7Error");
            return UsdValidationErrorVector{
                UsdValidationError(
                    validationErrorId, UsdValidationErrorType::Error, 
                    {UsdValidationErrorSite(prim.GetStage(), 
                                            prim.GetPath())},
                    "A testAPISchema prim type validator error")};
        };

        TfErrorMark m;
        registry.RegisterPluginValidator(validatorName, primTaskFn);
        TF_AXIOM(m.IsClean());
    }
    {
        const TfToken suiteName("testUsdValidationContext:TestSuite");
        const std::vector<const UsdValidator*> containedValidators =
            registry.GetOrLoadValidatorsByName(
                {TfToken("testUsdValidationContext:Test1"),
                 TfToken("testUsdValidationContext:Test2"),
                 TfToken("testUsdValidationContext:Test3")});

        TfErrorMark m;
        registry.RegisterPluginValidatorSuite(suiteName, containedValidators);
        TF_AXIOM(m.IsClean());
    }
}

static
std::string _LayerContents() 
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

static
SdfLayerRefPtr 
_CreateTestLayer() {
    SdfLayerRefPtr testLayer = SdfLayer::CreateAnonymous(".usda");
    testLayer->ImportFromString(_LayerContents());
    return testLayer;
}

static
void
_TestError1(const UsdValidationError &error) {
    TF_AXIOM(
        error.GetValidator()->GetMetadata().name == 
        TfToken("testUsdValidationContext:Test1"));
    TF_AXIOM(error.GetSites().size() == 1);
    TF_AXIOM(error.GetSites()[0].IsPrim());
    TF_AXIOM(error.GetSites()[0].GetPrim().GetPath() == 
             SdfPath::AbsoluteRootPath());
}

static
void
_TestError2(const UsdValidationError &error) {
    TF_AXIOM(
        error.GetValidator()->GetMetadata().name == 
        TfToken("testUsdValidationContext:Test2"));
    TF_AXIOM(error.GetSites().size() == 1);
    TF_AXIOM(error.GetSites()[0].IsValidSpecInLayer());
}

static
void
_TestError3(const UsdValidationError &error) {
    TF_AXIOM(
        error.GetValidator()->GetMetadata().name == 
        TfToken("testUsdValidationContext:Test3"));
    const std::set<SdfPath> expectedPrimPaths = {
        SdfPath("/World"),
        SdfPath("/World/baseType"),
        SdfPath("/World/derivedType"),
        SdfPath("/World/nestedDerivedType"),
        SdfPath("/World/somePrim")};
    TF_AXIOM(expectedPrimPaths.find(
                error.GetSites()[0].GetPrim().GetPath()) != 
             expectedPrimPaths.end());
}

static
void
_TestError4(const UsdValidationError &error) {
    TF_AXIOM(
        error.GetValidator()->GetMetadata().name == 
        TfToken("testUsdValidationContext:Test4"));
    const std::set<SdfPath> expectedPrimPaths = {
        SdfPath("/World/baseType"),
        SdfPath("/World/derivedType"),
        SdfPath("/World/nestedDerivedType")};
    TF_AXIOM(expectedPrimPaths.find(
                error.GetSites()[0].GetPrim().GetPath()) != 
             expectedPrimPaths.end());
}

static
void
_TestError5(const UsdValidationError &error) {
    TF_AXIOM(
        error.GetValidator()->GetMetadata().name == 
        TfToken("testUsdValidationContext:Test5"));
    const std::set<SdfPath> expectedPrimPaths = {
        SdfPath("/World/derivedType"),
        SdfPath("/World/nestedDerivedType")};
    TF_AXIOM(expectedPrimPaths.find(
                error.GetSites()[0].GetPrim().GetPath()) != 
             expectedPrimPaths.end());
}

static
void
_TestError6(const UsdValidationError &error) {
    TF_AXIOM(
        error.GetValidator()->GetMetadata().name == 
        TfToken("testUsdValidationContext:Test6"));
    TF_AXIOM(error.GetSites().size() == 1);
    TF_AXIOM(error.GetSites()[0].IsPrim());
    TF_AXIOM(error.GetSites()[0].GetPrim().GetName() == 
             TfToken("nestedDerivedType"));
}

static
void
_TestError7(const UsdValidationError &error) {
    TF_AXIOM(error.GetName() == TfToken("Test7Error"));
    TF_AXIOM(
        error.GetValidator()->GetMetadata().name == 
        TfToken("testUsdValidationContext:Test7"));
    TF_AXIOM(error.GetSites().size() == 1);
    TF_AXIOM(error.GetSites()[0].IsPrim());
    TF_AXIOM(error.GetSites()[0].GetPrim().GetName() == 
             TfToken("somePrim"));
}

static
void
_TestNonPluginError(const UsdValidationError &error) {
    TF_AXIOM(error.GetValidator()->GetMetadata().name == 
              TfToken("nonPluginValidator"));

    const SdfPath expectedPrimPaths = SdfPath("/");
    TF_AXIOM(error.GetSites()[0].GetPrim().GetPath() == expectedPrimPaths);
}

static
void
_TestUsdValidationContext()
{
    // Test the UsdValidationContext here.
    {
        // Create a ValidationContext with a suite
        const UsdValidatorSuite* suite = 
            UsdValidationRegistry::GetInstance().GetOrLoadValidatorSuiteByName(
                TfToken("testUsdValidationContext:TestSuite"));
        UsdValidationContext context({suite});
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
        UsdValidationContext context(
            {TfType::FindByName("testBaseType")});
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
            {TfType::FindByName("testAPISchemaAPI")});
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
        UsdValidationContext context({TfToken("Keyword1")});
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
        UsdValidationContext context({TfToken("Keyword2")}, false);
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
        UsdValidationContext context({
            PlugRegistry::GetInstance().GetPluginWithName(
                "testUsdValidationContext")});
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
}

int 
main()
{
    // Register the test plugin
    const std::string pluginPath = ArchGetCwd() + "/resources";
    TF_AXIOM(!PlugRegistry::GetInstance().RegisterPlugins(pluginPath).empty());

    // Add a non-plugin based validator here.
    {
        UsdValidatorMetadata metadata;
        metadata.name = TfToken("nonPluginValidator");
        metadata.keywords = {TfToken("Keyword2")};
        metadata.pluginPtr = nullptr;
        metadata.doc = "This is a non-plugin based validator.";
        metadata.isSuite = false;

        const UsdValidatePrimTaskFn primTaskFn = [](
            const UsdPrim &prim)
        {
            const TfToken errorId("nonPluginError");
            return UsdValidationErrorVector{
                UsdValidationError(
                    errorId, UsdValidationErrorType::Error,
                    {UsdValidationErrorSite(prim.GetStage(), 
                                            SdfPath::AbsoluteRootPath())}, 
                    "A non-plugin based validator error")};
        };

        // Register the validator
        TfErrorMark m;
        UsdValidationRegistry::GetInstance().
            RegisterValidator(metadata, primTaskFn);
        TF_AXIOM(m.IsClean());
    }

    _TestUsdValidationContext();
}
