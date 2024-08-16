#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Plug, Sdf, Usd


class TestUsdValidationError(unittest.TestCase):
    def test_create_default_error_site(self):
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

    def _verify_error_site_with_layer(self, errorSite: Usd.ValidationErrorSite, layer: Sdf.Layer, objectPath: Sdf.Path):
        self.assertTrue(errorSite.IsValid())
        self.assertTrue(errorSite.IsValidSpecInLayer())
        self.assertFalse(errorSite.IsPrim())
        self.assertFalse(errorSite.IsProperty())
        expected_property_spec = layer.GetPropertyAtPath(objectPath) if objectPath.IsPropertyPath() else None
        self.assertEqual(errorSite.GetPropertySpec(), expected_property_spec)
        expected_prim_spec = layer.GetPrimAtPath(objectPath) if objectPath.IsPrimPath() else None
        self.assertEqual(errorSite.GetPrimSpec(), expected_prim_spec)
        self.assertFalse(errorSite.GetProperty())
        self.assertFalse(errorSite.GetPrim())
        self.assertEqual(errorSite.GetLayer(), layer)
        self.assertFalse(errorSite.GetStage())

    def test_create_error_site_with_layer_and_prim_spec(self):
        stage = Usd.Stage.CreateInMemory()
        test_prim_path = Sdf.Path("/test")
        stage.DefinePrim(test_prim_path, "Xform")
        errorSite = Usd.ValidationErrorSite(stage.GetRootLayer(), test_prim_path)
        self._verify_error_site_with_layer(errorSite, stage.GetRootLayer(), test_prim_path)

    def test_create_error_site_with_layer_and_property_spec(self):
        stage = Usd.Stage.CreateInMemory()
        test_prim_path = Sdf.Path("/test")
        test_prim = stage.DefinePrim(test_prim_path, "Xform")
        test_attr = test_prim.CreateAttribute("attr", Sdf.ValueTypeNames.Int)
        test_attr_path = test_attr.GetPath()
        errorSite = Usd.ValidationErrorSite(stage.GetRootLayer(), test_attr_path)
        self._verify_error_site_with_layer(errorSite, stage.GetRootLayer(), test_attr_path)

    def _verify_error_site_with_stage(self, errorSite: Usd.ValidationErrorSite, stage: Usd.Stage, objectPath: Sdf.Path):
        self.assertTrue(errorSite.IsValid())
        self.assertFalse(errorSite.IsValidSpecInLayer())
        self.assertEqual(errorSite.IsPrim(), objectPath.IsPrimPath())
        self.assertEqual(errorSite.IsProperty(), objectPath.IsPropertyPath())
        self.assertFalse(errorSite.GetPropertySpec())
        self.assertFalse(errorSite.GetPrimSpec())
        expected_property = stage.GetPropertyAtPath(objectPath) if objectPath.IsPropertyPath() else Usd.Property()
        self.assertEqual(errorSite.GetProperty(), expected_property)
        expected_prim = stage.GetPrimAtPath(objectPath) if objectPath.IsPrimPath() else Usd.Prim()
        self.assertEqual(errorSite.GetPrim(), expected_prim)
        self.assertFalse(errorSite.GetLayer())
        self.assertEqual(errorSite.GetStage(), stage)

    def _verify_error_site_with_stage_and_layer(self, errorSite: Usd.ValidationErrorSite, stage: Usd.Stage, layer: Sdf.Layer, objectPath: Sdf.Path):
        self.assertTrue(errorSite.IsValid())
        self.assertTrue(errorSite.IsValidSpecInLayer())
        self.assertEqual(errorSite.IsPrim(), objectPath.IsPrimPath())
        self.assertEqual(errorSite.IsProperty(), objectPath.IsPropertyPath())

        expected_property_spec = layer.GetPropertyAtPath(objectPath) if objectPath.IsPropertyPath() else None
        self.assertEqual(expected_property_spec, errorSite.GetPropertySpec())
        expected_prim_spec = layer.GetPrimAtPath(objectPath) if objectPath.IsPrimPath() else None
        self.assertEqual(expected_prim_spec, errorSite.GetPrimSpec())
        expected_property = stage.GetPropertyAtPath(objectPath) if objectPath.IsPropertyPath() else Usd.Property()
        self.assertEqual(expected_property, errorSite.GetProperty())
        expected_prim = stage.GetPrimAtPath(objectPath) if objectPath.IsPrimPath() else Usd.Prim()
        self.assertEqual(expected_prim, errorSite.GetPrim())

        self.assertEqual(errorSite.GetLayer(), layer)
        self.assertEqual(errorSite.GetStage(), stage)

    def test_create_error_site_with_stage_and_prim(self):
        stage = Usd.Stage.CreateInMemory()
        test_prim_path = Sdf.Path("/test")
        stage.DefinePrim(test_prim_path, "Xform")
        errorSite = Usd.ValidationErrorSite(stage, test_prim_path)
        self._verify_error_site_with_stage(errorSite, stage, test_prim_path)

        # With layer also
        errorSite = Usd.ValidationErrorSite(stage, test_prim_path, stage.GetRootLayer())
        self._verify_error_site_with_stage_and_layer(errorSite, stage, stage.GetRootLayer(), test_prim_path)

    def test_create_error_site_with_stage_and_property(self):
        stage = Usd.Stage.CreateInMemory()
        test_prim_path = Sdf.Path("/test")
        test_prim = stage.DefinePrim(test_prim_path, "Xform")
        test_attr = test_prim.CreateAttribute("attr", Sdf.ValueTypeNames.Int)
        test_attr_path = test_attr.GetPath()
        errorSite = Usd.ValidationErrorSite(stage, test_attr_path)
        self._verify_error_site_with_stage(errorSite, stage, test_attr_path)

        # With layer also
        errorSite = Usd.ValidationErrorSite(stage, test_attr_path, stage.GetRootLayer())
        self._verify_error_site_with_stage_and_layer(errorSite, stage, stage.GetRootLayer(), test_attr_path)

    def test_create_error_site_with_invalid_args(self):
        stage = Usd.Stage.CreateInMemory()
        test_prim_path = Sdf.Path("/test")
        stage.DefinePrim(test_prim_path, "Xform")
        errors = {
            "Wrong Stage Type": {
                "stage": "wrong stage",
                "layer": stage.GetRootLayer(),
                "objectPath": test_prim_path
            },
            "Wrong Layer Type": {
                "stage": stage,
                "layer": "wrong layer",
                "objectPath": test_prim_path
            },
            "Wrong Path Type": {
                "stage": stage,
                "layer": stage.GetRootLayer(),
                "objectPath": 123
            },
        }

        for error_category, args in errors.items():
            with self.subTest(errorType=error_category):
                with self.assertRaises(Exception):
                    Usd.ValidationErrorSite(**args)

    def _verify_validation_error(self, error, errorType=Usd.ValidationErrorType.None_, errorSites=[], errorMessage=""):
        self.assertEqual(error.GetType(), errorType)
        self.assertEqual(error.GetSites(), errorSites)
        self.assertEqual(error.GetMessage(), errorMessage)
        if errorType != Usd.ValidationErrorType.None_:
            self.assertTrue(error.GetErrorAsString())
            self.assertFalse(error.HasNoError())
        else:
            self.assertFalse(error.GetErrorAsString())
            self.assertTrue(error.HasNoError())

    def test_create_default_validation_error(self):
        validation_error = Usd.ValidationError()
        self._verify_validation_error(validation_error)

    def test_create_validation_error_with_keyword_args(self):
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
                "errorSites": [Usd.ValidationErrorSite(), Usd.ValidationErrorSite()],
                "errorMessage": "This is an info."
            },
        ]

        for error in errors:
            with self.subTest(errorType=error["errorType"]):
                validation_error = Usd.ValidationError(**error)
                self._verify_validation_error(validation_error, **error)

    def test_create_validation_error_with_invalid_args(self):
        errors = {
            "Wrong Error Type": {
                "errorType": "wrong_type",
                "errorSites": [],
                "errorMessage": ""
            },
            "Wrong Sites Type": {
                "errorType": Usd.ValidationErrorType.None_,
                "errorSites": "wong_type",
                "errorMessage": ""
            },
            "Wrong Message Type": {
                "errorType": Usd.ValidationErrorType.None_,
                "errorSites": [],
                "errorMessage": 123
            },
        }

        for error_category, error in errors.items():
            with self.subTest(errorType=error_category):
                with self.assertRaises(Exception):
                    Usd.ValidationError(**error)


if __name__ == "__main__":
    unittest.main()
