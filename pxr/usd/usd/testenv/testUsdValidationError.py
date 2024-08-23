#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Plug, Sdf, Usd


class TestUsdValidationError(unittest.TestCase):
    def test_CreateDefaultErrorSite(self):
        errorSite = Usd.ValidationErrorSite()
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

    def _VerifyErrorSiteWithLayer(self, errorSite: Usd.ValidationErrorSite, 
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
        errorSite = Usd.ValidationErrorSite(stage.GetRootLayer(), 
                                            testPrimPath)
        self._VerifyErrorSiteWithLayer(errorSite, stage.GetRootLayer(), 
                                           testPrimPath)

    def test_CreateErrorSiteWithLayerAndPropertySpec(self):
        stage = Usd.Stage.CreateInMemory()
        testPrimPath = Sdf.Path("/test")
        testPrim = stage.DefinePrim(testPrimPath, "Xform")
        testAttr = testPrim.CreateAttribute("attr", Sdf.ValueTypeNames.Int)
        testAttrPath = testAttr.GetPath()
        errorSite = Usd.ValidationErrorSite(stage.GetRootLayer(), 
                                            testAttrPath)
        self._VerifyErrorSiteWithLayer(errorSite, stage.GetRootLayer(), 
                                           testAttrPath)

    def _VerifyErrorSiteWithStage(self, errorSite: Usd.ValidationErrorSite, 
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
            self, errorSite: Usd.ValidationErrorSite, stage: Usd.Stage, 
            layer: Sdf.Layer, objectPath: Sdf.Path):
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
        errorSite = Usd.ValidationErrorSite(stage, testPrimPath)
        self._VerifyErrorSiteWithStage(errorSite, stage, testPrimPath)

        # With layer also
        errorSite = Usd.ValidationErrorSite(stage, testPrimPath, 
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
        errorSite = Usd.ValidationErrorSite(stage, testAttrPath)
        self._VerifyErrorSiteWithStage(errorSite, stage, testAttrPath)

        # With layer also
        errorSite = Usd.ValidationErrorSite(stage, testAttrPath, 
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
                    Usd.ValidationErrorSite(**args)

    def _VerifyValidationError(self, error, 
                                 errorType=Usd.ValidationErrorType.None_, 
                                 errorSites=[], errorMessage=""):
        self.assertEqual(error.GetType(), errorType)
        self.assertEqual(error.GetSites(), errorSites)
        self.assertEqual(error.GetMessage(), errorMessage)
        if errorType != Usd.ValidationErrorType.None_:
            self.assertTrue(error.GetErrorAsString())
            self.assertFalse(error.HasNoError())
        else:
            self.assertFalse(error.GetErrorAsString())
            self.assertTrue(error.HasNoError())

    def test_CreateDefaultValidationError(self):
        validationError = Usd.ValidationError()
        self._VerifyValidationError(validationError)

    def test_CreateValidationErrorWithKeywordArgs(self):
        errors = [
            {
                "errorType": Usd.ValidationErrorType.None_,
                "errorSites": [],
                "errorMessage": ""
            },
            {
                "errorType": Usd.ValidationErrorType.Error,
                "errorSites": [Usd.ValidationErrorSite()],
                "errorMessage": "This is an error."
            },
            {
                "errorType": Usd.ValidationErrorType.Warn,
                "errorSites": [Usd.ValidationErrorSite()],
                "errorMessage": "This is a warning."
            },
            {
                "errorType": Usd.ValidationErrorType.Info,
                "errorSites": [Usd.ValidationErrorSite(), 
                               Usd.ValidationErrorSite()],
                "errorMessage": "This is an info."
            },
        ]

        for error in errors:
            with self.subTest(errorType=error["errorType"]):
                validationError = Usd.ValidationError(**error)
                self._VerifyValidationError(validationError, **error)

    def test_CreateValidationErrorWithInvalidArgs(self):
        errors = {
            "Wrong Error Type": {
                "errorType": "wrongType",
                "errorSites": [],
                "errorMessage": ""
            },
            "Wrong Sites Type": {
                "errorType": Usd.ValidationErrorType.None_,
                "errorSites": "wrongType",
                "errorMessage": ""
            },
            "Wrong Message Type": {
                "errorType": Usd.ValidationErrorType.None_,
                "errorSites": [],
                "errorMessage": 123
            },
        }

        for errorCategory, error in errors.items():
            with self.subTest(errorType=errorCategory):
                with self.assertRaises(Exception):
                    Usd.ValidationError(**error)


if __name__ == "__main__":
    unittest.main()
