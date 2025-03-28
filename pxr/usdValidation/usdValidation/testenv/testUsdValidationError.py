#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Plug, Sdf, Usd, Tf, UsdValidation


class TestUsdValidationError(unittest.TestCase):
    def test_CreateDefaultErrorSite(self):
        errorSite = UsdValidation.ValidationErrorSite()
        self.assertFalse(errorSite.IsValid())
        self.assertFalse(errorSite.IsValidSpecInLayer())
        self.assertFalse(errorSite.IsPrim())
        self.assertFalse(errorSite.IsProperty())
        self.assertFalse(errorSite.GetPropertySpec())
        self.assertFalse(errorSite.GetPrimSpec())
        self.assertFalse(errorSite.GetProperty())
        self.assertFalse(errorSite.GetPrim())
        self.assertFalse(errorSite.GetLayer())
        self.assertFalse(errorSite.GetStage())

    def _VerifyErrorSiteWithLayer(self, 
                                  errorSite: UsdValidation.ValidationErrorSite, 
                                  layer: Sdf.Layer, objectPath: Sdf.Path):
        self.assertTrue(errorSite.IsValid())
        self.assertTrue(errorSite.IsValidSpecInLayer())
        self.assertFalse(errorSite.IsPrim())
        self.assertFalse(errorSite.IsProperty())
        expectedPropertySpec = layer.GetPropertyAtPath(
            objectPath) if objectPath.IsPropertyPath() else None
        self.assertEqual(errorSite.GetPropertySpec(), expectedPropertySpec)
        expectedPrimSpec = layer.GetPrimAtPath(
            objectPath) if objectPath.IsPrimPath() else None
        self.assertEqual(errorSite.GetPrimSpec(), expectedPrimSpec)
        self.assertFalse(errorSite.GetProperty())
        self.assertFalse(errorSite.GetPrim())
        self.assertEqual(errorSite.GetLayer(), layer)
        self.assertFalse(errorSite.GetStage())

    def test_CreateErrorSiteWithLayerAndPrimSpec(self):
        stage = Usd.Stage.CreateInMemory()
        testPrimPath = Sdf.Path("/test")
        stage.DefinePrim(testPrimPath, "Xform")
        errorSite = UsdValidation.ValidationErrorSite(stage.GetRootLayer(), 
                                            testPrimPath)
        self._VerifyErrorSiteWithLayer(errorSite, stage.GetRootLayer(), 
                                           testPrimPath)

    def test_CreateErrorSiteWithLayerAndPropertySpec(self):
        stage = Usd.Stage.CreateInMemory()
        testPrimPath = Sdf.Path("/test")
        testPrim = stage.DefinePrim(testPrimPath, "Xform")
        testAttr = testPrim.CreateAttribute("attr", Sdf.ValueTypeNames.Int)
        testAttrPath = testAttr.GetPath()
        errorSite = UsdValidation.ValidationErrorSite(stage.GetRootLayer(), 
                                            testAttrPath)
        self._VerifyErrorSiteWithLayer(errorSite, stage.GetRootLayer(), 
                                           testAttrPath)

    def _VerifyErrorSiteWithStage(self, 
                                  errorSite: UsdValidation.ValidationErrorSite, 
                                  stage: Usd.Stage, objectPath: Sdf.Path):
        self.assertTrue(errorSite.IsValid())
        self.assertFalse(errorSite.IsValidSpecInLayer())
        self.assertEqual(errorSite.IsPrim(), objectPath.IsPrimPath())
        self.assertEqual(errorSite.IsProperty(), objectPath.IsPropertyPath())
        self.assertFalse(errorSite.GetPropertySpec())
        self.assertFalse(errorSite.GetPrimSpec())
        expectedProperty= stage.GetPropertyAtPath(
            objectPath) if objectPath.IsPropertyPath() else Usd.Property()
        self.assertEqual(errorSite.GetProperty(), expectedProperty)
        expectedPrim = stage.GetPrimAtPath(
            objectPath) if objectPath.IsPrimPath() else Usd.Prim()
        self.assertEqual(errorSite.GetPrim(), expectedPrim)
        self.assertFalse(errorSite.GetLayer())
        self.assertEqual(errorSite.GetStage(), stage)

    def _VerifyErrorSiteWithStageAndLayer(
            self, errorSite: UsdValidation.ValidationErrorSite, 
            stage: Usd.Stage, layer: Sdf.Layer, objectPath: Sdf.Path):
        self.assertTrue(errorSite.IsValid())
        self.assertTrue(errorSite.IsValidSpecInLayer())
        self.assertEqual(errorSite.IsPrim(), objectPath.IsPrimPath())
        self.assertEqual(errorSite.IsProperty(), objectPath.IsPropertyPath())

        expectedPropertySpec = layer.GetPropertyAtPath(
            objectPath) if objectPath.IsPropertyPath() else None
        self.assertEqual(expectedPropertySpec, errorSite.GetPropertySpec())
        expectedPrimSpec = layer.GetPrimAtPath(
            objectPath) if objectPath.IsPrimPath() else None
        self.assertEqual(expectedPrimSpec, errorSite.GetPrimSpec())
        expectedProperty = stage.GetPropertyAtPath(
            objectPath) if objectPath.IsPropertyPath() else Usd.Property()
        self.assertEqual(expectedProperty, errorSite.GetProperty())
        expectedPrim = stage.GetPrimAtPath(
            objectPath) if objectPath.IsPrimPath() else Usd.Prim()
        self.assertEqual(expectedPrim, errorSite.GetPrim())

        self.assertEqual(errorSite.GetLayer(), layer)
        self.assertEqual(errorSite.GetStage(), stage)

    def test_CreateErrorSiteWithStageAndPrim(self):
        stage = Usd.Stage.CreateInMemory()
        testPrimPath = Sdf.Path("/test")
        stage.DefinePrim(testPrimPath, "Xform")
        errorSite = UsdValidation.ValidationErrorSite(stage, testPrimPath)
        self._VerifyErrorSiteWithStage(errorSite, stage, testPrimPath)

        # With layer also
        errorSite = UsdValidation.ValidationErrorSite(stage, testPrimPath, 
                                            stage.GetRootLayer())
        self._VerifyErrorSiteWithStageAndLayer(errorSite, stage, 
                                                     stage.GetRootLayer(), 
                                                     testPrimPath)

    def test_CreateErrorSiteWithStageAndProperty(self):
        stage = Usd.Stage.CreateInMemory()
        testPrimPath= Sdf.Path("/test")
        testPrim = stage.DefinePrim(testPrimPath, "Xform")
        testAttr = testPrim.CreateAttribute("attr", Sdf.ValueTypeNames.Int)
        testAttrPath = testAttr.GetPath()
        errorSite = UsdValidation.ValidationErrorSite(stage, testAttrPath)
        self._VerifyErrorSiteWithStage(errorSite, stage, testAttrPath)

        # With layer also
        errorSite = UsdValidation.ValidationErrorSite(stage, testAttrPath, 
                                            stage.GetRootLayer())
        self._VerifyErrorSiteWithStageAndLayer(errorSite, stage, 
                                                     stage.GetRootLayer(), 
                                                     testAttrPath)

    def test_CreateErrorSiteWithInvalidArgs(self):
        stage = Usd.Stage.CreateInMemory()
        testPrimPath= Sdf.Path("/test")
        stage.DefinePrim(testPrimPath, "Xform")
        errors = {
            "Wrong Stage Type": {
                "stage": "wrong stage",
                "layer": stage.GetRootLayer(),
                "objectPath": testPrimPath
            },
            "Wrong Layer Type": {
                "stage": stage,
                "layer": "wrong layer",
                "objectPath": testPrimPath
            },
            "Wrong Path Type": {
                "stage": stage,
                "layer": stage.GetRootLayer(),
                "objectPath": 123
            },
        }

        for errorCategory, args in errors.items():
            with self.subTest(errorType=errorCategory):
                with self.assertRaises(Exception):
                    UsdValidation.ValidationErrorSite(**args)

    def _VerifyValidationError(self, error, name="",
                               errorType=UsdValidation.ValidationErrorType.None_, 
                               errorSites=None, errorMessage=""):
        if not errorSites:
            errorSites = []
        self.assertEqual(error.GetName(), name)
        self.assertEqual(error.GetType(), errorType)
        self.assertEqual(error.GetSites(), errorSites)
        self.assertEqual(error.GetMessage(), errorMessage)
        if errorType != UsdValidation.ValidationErrorType.None_:
            self.assertTrue(error.GetErrorAsString())
            self.assertFalse(error.HasNoError())
        else:
            self.assertFalse(error.GetErrorAsString())
            self.assertTrue(error.HasNoError())
        # check if coding error is raised for these errors when trying to get
        # its identifier
        try:
            error.GetIdentifier();
        except Tf.ErrorException as e:
            expectedErrorStr = \
            "Validator not set on ValidationError. Possibly this validation " \
            "error was not created via a call to " \
            "UsdValidationValidator::Validate(), which is responsible to set " \
            "the validator on the error."
            self.assertTrue(expectedErrorStr in str(e))

    def test_CreateDefaultValidationError(self):
        validationError = UsdValidation.ValidationError()
        self._VerifyValidationError(validationError)

    def test_CreateValidationErrorWithKeywordArgs(self):
        errors = [
            {
                "name": "error1",
                "errorType": UsdValidation.ValidationErrorType.None_,
                "errorSites": [],
                "errorMessage": ""
            },
            {
                "name": "error2",
                "errorType": UsdValidation.ValidationErrorType.Error,
                "errorSites": [UsdValidation.ValidationErrorSite()],
                "errorMessage": "This is an error."
            },
            {
                "name": "error3",
                "errorType": UsdValidation.ValidationErrorType.Warn,
                "errorSites": [UsdValidation.ValidationErrorSite()],
                "errorMessage": "This is a warning."
            },
            {
                "name": "error4",
                "errorType": UsdValidation.ValidationErrorType.Info,
                "errorSites": [UsdValidation.ValidationErrorSite(), 
                               UsdValidation.ValidationErrorSite()],
                "errorMessage": "This is an info."
            },
        ]

        for error in errors:
            with self.subTest(errorType=error["errorType"]):
                validationError = UsdValidation.ValidationError(**error)
                self._VerifyValidationError(validationError, **error)

    def test_CreateValidationErrorWithInvalidArgs(self):
        errors = {
            "Wrong Error Type": {
                "name": "error1",
                "errorType": "wrongType",
                "errorSites": [],
                "errorMessage": ""
            },
            "Wrong Sites Type": {
                "name": "error2",
                "errorType": UsdValidation.ValidationErrorType.None_,
                "errorSites": "wrongType",
                "errorMessage": ""
            },
            "Wrong Message Type": {
                "name": "error3",
                "errorType": UsdValidation.ValidationErrorType.None_,
                "errorSites": [],
                "errorMessage": 123
            },
        }

        for errorCategory, error in errors.items():
            with self.subTest(errorType=errorCategory):
                with self.assertRaises(Exception):
                    UsdValidation.ValidationError(**error)


if __name__ == "__main__":
    unittest.main()
