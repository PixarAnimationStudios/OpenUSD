#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os, unittest

from pxr import Plug, Sdf, Tf, Gf, Usd, UsdValidation

class TestUsdValidationContextPy(unittest.TestCase):

    @classmethod
    def setUpClass(cls) -> None:
        testTypePluginPath = os.path.abspath(
                os.path.join(os.path.curdir, "resources"))
        assert os.path.exists(testTypePluginPath)
        testValidatorPluginPath = os.path.join(
            os.path.dirname(__file__), 
            "UsdValidationPlugins/lib/TestUsdValidationContextValidators*/" \
            "Resources/")
        try:
            typePlugins = Plug.Registry().RegisterPlugins(testTypePluginPath)
            assert typePlugins
            assert len(typePlugins) == 1
            assert typePlugins[0].name == "testUsdValidationContext"
            validatorPlugins = Plug.Registry().RegisterPlugins(
                testValidatorPluginPath)
            assert validatorPlugins
            assert len(validatorPlugins) == 1
            assert validatorPlugins[0].name == \
                "testUsdValidationContextValidatorsPlugin"
        except RuntimeError:
            pass

    def _LayerContents(self):
        layerContents = """#usda 1.0
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
        """
        return layerContents

    def _CreateTestLayer(self):
        layer = Sdf.Layer.CreateAnonymous(".usda")
        layer.ImportFromString(self._LayerContents())
        return layer

    def _TestError1(self, error):
        self.assertEqual(
            error.GetValidator().GetMetadata().name,
            "testUsdValidationContextValidatorsPlugin:Test1")
        self.assertEqual(len(error.GetSites()), 1)
        self.assertTrue(error.GetSites()[0].IsPrim())
        self.assertEqual(
            error.GetSites()[0].GetPrim().GetPath(),
            Sdf.Path.absoluteRootPath)

    def _TestError2(self, error):
        self.assertEqual(
            error.GetValidator().GetMetadata().name,
            "testUsdValidationContextValidatorsPlugin:Test2")
        self.assertEqual(len(error.GetSites()), 1)
        self.assertTrue(error.GetSites()[0].IsValidSpecInLayer())

    def _TestError3(self, error):
        self.assertEqual(
            error.GetValidator().GetMetadata().name,
            "testUsdValidationContextValidatorsPlugin:Test3")
        expectedPrimPaths = {
            Sdf.Path("/World"),
            Sdf.Path("/World/baseType"),
            Sdf.Path("/World/derivedType"),
            Sdf.Path("/World/nestedDerivedType"),
            Sdf.Path("/World/somePrim")}
        self.assertTrue(
            error.GetSites()[0].GetPrim().GetPath() in expectedPrimPaths)

    def _TestError4(self, error):
        self.assertEqual(
            error.GetValidator().GetMetadata().name,
            "testUsdValidationContextValidatorsPlugin:Test4")
        expectedPrimPaths = {
            Sdf.Path("/World/baseType"),
            Sdf.Path("/World/derivedType"),
            Sdf.Path("/World/nestedDerivedType")}
        self.assertTrue(
            error.GetSites()[0].GetPrim().GetPath() in expectedPrimPaths)

    def _TestError5(self, error):
        self.assertEqual(
            error.GetValidator().GetMetadata().name,
            "testUsdValidationContextValidatorsPlugin:Test5")
        expectedPrimPaths = {
            Sdf.Path("/World/derivedType"),
            Sdf.Path("/World/nestedDerivedType")}
        self.assertTrue(
            error.GetSites()[0].GetPrim().GetPath() in expectedPrimPaths)

    def _TestError6(self, error):
        self.assertEqual(
            error.GetValidator().GetMetadata().name,
            "testUsdValidationContextValidatorsPlugin:Test6")
        self.assertEqual(len(error.GetSites()), 1)
        self.assertTrue(error.GetSites()[0].IsPrim())
        self.assertEqual(
            error.GetSites()[0].GetPrim().GetName(),
            "nestedDerivedType")

    def _TestError7(self, error):
        self.assertEqual(error.GetName(), "Test7Error")
        self.assertEqual(
            error.GetValidator().GetMetadata().name,
            "testUsdValidationContextValidatorsPlugin:Test7")
        self.assertEqual(len(error.GetSites()), 1)
        self.assertTrue(error.GetSites()[0].IsPrim())
        self.assertEqual(
            error.GetSites()[0].GetPrim().GetName(),
            "somePrim")


    def test_UsdValidationContext(self):
        # Create a ValidationContext with a suite
        suite = UsdValidation.ValidationRegistry().GetOrLoadValidatorSuiteByName(
            "testUsdValidationContextValidatorsPlugin:TestSuite")
        context = UsdValidation.ValidationContext([suite])
        testLayer = self._CreateTestLayer()
        # Run Validate(layer)
        errors = context.Validate(testLayer)
        # 1 error for Test2 validator - root layer of the stage
        self.assertEqual(len(errors), 1)
        self._TestError2(errors[0])
        # Run Validate(stage)
        stage = Usd.Stage.Open(testLayer)
        errors = context.Validate(stage)
        # 1 error for Test1 validator (stage)
        # 2 error for Test2 validator - root layer and session layer
        # 5 errors for Test3 generic prim validator which runs on all 5 prims
        self.assertEqual(len(errors), 8)
        for error in errors:
            if error.GetName() == "Test1Error":
                self._TestError1(error)
            elif error.GetName() == "Test2Error":
                self._TestError2(error)
            elif error.GetName() == "Test3Error":
                self._TestError3(error)
            else:
                self.assertFalse(True)

        # Create a ValidationContext with explicit schemaTypes
        context = UsdValidation.ValidationContext(
            [Tf.Type.FindByName("testBaseType")])
        testLayer = self._CreateTestLayer()
        # Run Validate(layer)
        errors = context.Validate(testLayer)
        # 0 errors as we do not have any layer validators selected in this
        # context.
        self.assertTrue(not errors)
        # Run Validate(stage)
        stage = Usd.Stage.Open(testLayer)
        errors = context.Validate(stage)
        # 3 errors for Test4 testBaseType prim type validator which runs on
        # the baseType, derivedType and nestedDerivedType prims
        self.assertEqual(len(errors), 3)
        for error in errors:
            self._TestError4(error)

        # Create a ValidationContext with explicit schemaType - apiSchema
        context = UsdValidation.ValidationContext(
            [Tf.Type.FindByName("testAPISchemaAPI")])
        testLayer = self._CreateTestLayer()
        # Run Validate(layer)
        errors = context.Validate(testLayer)
        # 0 errors as we do not have any layer validators selected in this
        # context.
        self.assertTrue(not errors)
        # Run Validate(stage)
        stage = Usd.Stage.Open(testLayer)
        errors = context.Validate(stage)
        # 1 error for Test7 testAPISchema prim type validator which runs on
        # the somePrim prim
        self.assertEqual(len(errors), 1)
        self._TestError7(errors[0])

        # Create a ValidationContext with keywords API and have
        # includeAllAncestors set to true
        context = UsdValidation.ValidationContext(["Keyword1"], True)
        testLayer = self._CreateTestLayer()
        # Run Validate(layer)
        errors = context.Validate(testLayer)
        # 0 errors as we do not have any layer validators selected in this
        # context.
        self.assertTrue(not errors)
        # Run Validate(stage)
        stage = Usd.Stage.Open(testLayer)
        errors = context.Validate(stage)
        # 1 error for Test1 validator
        # 5 errors for Test3 generic prim validator which runs on all 5 prims
        # 2 errors for Test5 testDerivedType prim type validator which runs on
        #   the derivedType and nestedDerivedType prims
        # 3 errors for Test4 testBaseType prim type validator which runs on
        #   the baseType, derivedType and nestedDerivedType prims (This gets
        #   includes as an ancestor type of derivedType)
        # 1 error for Test7 testAPISchema prim type validator which runs on
        #   the somePrim prim
        self.assertEqual(len(errors), 12)
        for error in errors:
            if error.GetName() == "Test1Error":
                self._TestError1(error)
            elif error.GetName() == "Test3Error":
                self._TestError3(error)
            elif error.GetName() == "Test4Error":
                self._TestError4(error)
            elif error.GetName() == "Test5Error":
                self._TestError5(error)
            elif error.GetName() == "Test7Error":
                self._TestError7(error)
            else:
                self.assertFalse(True)
        
        # Create a ValidationContext with keywords API and have
        # includeAllAncestors set to false
        context = UsdValidation.ValidationContext(["Keyword2"], False)
        testLayer = self._CreateTestLayer()
        # Run Validate(layer)
        errors = context.Validate(testLayer)
        # 1 error for Test2 validator - root layer of the stage
        self.assertEqual(len(errors), 1)
        self._TestError2(errors[0])
        # Run Validate(prims)
        stage = Usd.Stage.Open(testLayer)
        errors = context.Validate(stage.Traverse())
        # 3 errors for Test4 testBaseType prim type validator which runs on
        # the baseType, derivedType and nestedDerivedType prims
        # 1 error for Test6 testNestedDerivedType prim type validator which
        # runs on the nestedDerivedType prim
        # 5 errors for Test3 generic prim validator which runs on all 5 prims
        self.assertEqual(len(errors), 9)
        for error in errors:
            if error.GetName() == "Test3Error":
                self._TestError3(error)
            elif error.GetName() == "Test4Error":
                self._TestError4(error)
            elif error.GetName() == "Test6Error":
                self._TestError6(error)
            else:
                self.assertFalse(True)

        # Run Validate(stage)
        errors = context.Validate(stage)
        # 2 error for Test2 validator - root layer and session layer
        # 3 errors for Test4 testBaseType prim type validator which runs on
        #   the baseType, derivedType and nestedDerivedType prims
        # 1 error for Test6 testNestedDerivedType prim type validator which
        #   runs on the nestedDerivedType prim
        # Because of TestSuite:
        #   1 error for Test1 validator
        #   5 errors for Test3 generic prim validator which runs on all 5 prims
        self.assertEqual(len(errors), 12)
        for error in errors:
            if error.GetName() == "Test1Error":
                self._TestError1(error)
            elif error.GetName() == "Test2Error":
                self._TestError2(error)
            elif error.GetName() == "Test3Error":
                self._TestError3(error)
            elif error.GetName() == "Test4Error":
                self._TestError4(error)
            elif error.GetName() == "Test6Error":
                self._TestError6(error)
            else:
                self.assertFalse(True)

        # Create a ValidationContext with plugins
        context = UsdValidation.ValidationContext(
            [Plug.Registry().GetPluginWithName(
                "testUsdValidationContextValidatorsPlugin")], True)
        testLayer = self._CreateTestLayer()
        stage = Usd.Stage.Open(testLayer)
        errors = context.Validate(stage)
        # 1 error for Test1 validator
        # 2 error for Test2 validator - root layer and session layer
        # 5 errors for Test3 generic prim validator which runs on all 5 prims
        # 3 errors for Test4 testBaseType prim type validator which runs on
        #   the baseType, derivedType and nestedDerivedType prims
        # 2 error for Test5 testDerivedType prim type validator which runs on
        #   the derivedType and nestedDerivedType prims
        # 1 error for Test6 testNestedDerivedType prim type validator which
        #   runs on the nestedDerivedType prim
        # 1 error for Test7 testAPISchema prim type validator which runs on
        #   the somePrim prim
        self.assertEqual(len(errors), 15)
        for error in errors:
            if error.GetName() == "Test1Error":
                self._TestError1(error)
            elif error.GetName() == "Test2Error":
                self._TestError2(error)
            elif error.GetName() == "Test3Error":
                self._TestError3(error)
            elif error.GetName() == "Test4Error":
                self._TestError4(error)
            elif error.GetName() == "Test5Error":
                self._TestError5(error)
            elif error.GetName() == "Test6Error":
                self._TestError6(error)
            elif error.GetName() == "Test7Error":
                self._TestError7(error)
            else:
                self.assertFalse(True)

        layerWithTimeCodes = """#usda 1.0
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
        """
        # Create a ValidationContext with Test3 and Test4 validators
        validators = \
            UsdValidation.ValidationRegistry().GetOrLoadValidatorsByName(
            ["testUsdValidationContextValidatorsPlugin:Test3",
             "testUsdValidationContextValidatorsPlugin:Test4"])
        context = UsdValidation.ValidationContext(validators)
        testLayer = Sdf.Layer.CreateAnonymous(".usda")
        testLayer.ImportFromString(layerWithTimeCodes)
        stage = Usd.Stage.Open(testLayer)
        # Validate Stage with ProxyPrim traversal predicate and default
        # UsdValidationTimeRange which is full interval.
        errors = context.Validate(stage)
        # Refer the implementation for Test3 in 
        # testUsdValidationContextValidators.cpp:
        # 1 - error for Test4 testBaseType prim type validator which runs on
        #   the baseType prim. (This is not a time dependent validator).
        # 1 - error for Test3 generic prim validator which runs on the /World
        #   prim with full interval + default but since no attributes on 
        #   /World, an error is reported for that in the implementation.
        # 3 - errors for Test3 generic prim validator which runs on the 
        #   /World/baseType prim with full interval, that is 1 error for each
        #   time sample on the attr attribute.
        self.assertEqual(len(errors), 6)

        # Also Validate the same stage but with only 
        # GfInterval::GetFullInterval, which does not include 
        # UsdTimeCode::Default
        errors = context.Validate(stage, UsdValidation.ValidationTimeRange(
            Gf.Interval.GetFullInterval(), False));
        # 1 - error for Test4 testBaseType prim type validator which runs on
        #   the baseType prim. (This is not a time dependent validator).
        # 3 - errors for Test3 generic prim validator which runs on the 
        #   /World/baseType prim with full interval, that is 1 error for each
        #   time sample on the attr attribute.
        self.assertEqual(len(errors), 4)

        # Also Validate the /World/baseType prim which has attributes with
        # timeSamples but with a TimeRange which has an empty interval but
        # includes UsdTimeCode::Default.
        prims = [stage.GetPrimAtPath("/World/baseType")]
        errors = context.Validate(prims, UsdValidation.ValidationTimeRange(
            Gf.Interval(), True));
        # 1 error for Test4 testBaseType prim type validator which runs on
        #   the baseType prim. (This is not a time dependent validator).
        # 1 error for Test3 generic prim validator which runs on the
        #   /World/baseType prim with empty interval but includes 
        #   UsdTimeCode::Default. (The time dependent validator implementation 
        #   has a check for this case to emulate uniform attribute validation 
        #   on a prim which also has time dependent attributes.)
        self.assertEqual(len(errors), 2)

        # Validate specific /World/baseType prim at times 1 and 2.
        errors = context.Validate(prims, [Usd.TimeCode(1), Usd.TimeCode(2)])
        # 1 error for Test4 (non time dependent validator) testBaseType prim 
        # type validator which runs on the baseType prim.
        # 2 errors for Test3 generic prim validator which runs on the
        #  /World/baseType prim at timeCode 1 and 2, specified times.
        self.assertEqual(len(errors), 3)

if __name__ == "__main__":
    unittest.main()

