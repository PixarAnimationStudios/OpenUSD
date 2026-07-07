#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

"""Tests for validation fixers implemented in Python.

Exercises:
1. Creating ValidationFixer objects from Python with callable fixerImplFn
   and canApplyFn.
2. Passing fixers to explicit validator registration methods.
3. Retrieving fixers from validators and errors.
4. Invoking CanApplyFix and ApplyFix with Python-backed fixers.
"""

import concurrent.futures
import functools
import os
import threading
import unittest
from unittest import mock

from pxr import Plug, Sdf, Usd, UsdValidation


class TestUsdValidationFixerPy(unittest.TestCase):
    """Test Python validation fixer behavior."""

    # Must match the "Name" field in testUsdValidationFixerPy/plugInfo.json.
    PLUGIN_NAME = "testValidationFixerPyPlugin"

    @classmethod
    def setUpClass(cls):
        testPluginsDsoSearchPath = os.path.join(
            os.path.dirname(__file__), "testUsdValidationFixerPy"
        )
        try:
            plugins = Plug.Registry().RegisterPlugins(testPluginsDsoSearchPath)
            assert len(plugins) == 1
            assert plugins[0].name == cls.PLUGIN_NAME
        except RuntimeError:
            pass  # Plugin may already be registered.

    @staticmethod
    def _LayerTask(layer):
        return [
            UsdValidation.ValidationError(
                "LayerFixerError",
                UsdValidation.ValidationErrorType.Warn,
                [UsdValidation.ValidationErrorSite(layer, Sdf.Path.absoluteRootPath)],
                "Fixable layer error",
            )
        ]

    @staticmethod
    def _StageTask(stage, timeRange):
        return [
            UsdValidation.ValidationError(
                "StageFixerError",
                UsdValidation.ValidationErrorType.Error,
                [UsdValidation.ValidationErrorSite(stage, Sdf.Path.absoluteRootPath)],
                "Fixable stage error",
            )
        ]

    @staticmethod
    def _PrimTask(prim, timeRange):
        if prim.IsPseudoRoot():
            return []
        return [
            UsdValidation.ValidationError(
                "PrimFixerError",
                UsdValidation.ValidationErrorType.Error,
                [UsdValidation.ValidationErrorSite(prim.GetStage(), prim.GetPath())],
                f"Fixable prim error on {prim.GetPath()}",
            )
        ]

    @staticmethod
    def _ImplFn(error, editTarget, timeCode):
        return True

    @staticmethod
    def _CanApplyFn(error, editTarget, timeCode):
        return True

    def test_BasicConstruction(self):
        """A fixer can be constructed with name, description, and callables."""
        fixer = UsdValidation.ValidationFixer(
            name="testFixer",
            description="A test fixer",
            fixerImplFn=self._ImplFn,
            canApplyFn=self._CanApplyFn,
        )
        self.assertEqual(fixer.name, "testFixer")
        self.assertEqual(fixer.description, "A test fixer")
        self.assertEqual(fixer.errorName, "")
        self.assertEqual(fixer.keywords, [])

    def test_ConstructionWithKeywordsAndErrorName(self):
        """A fixer can be constructed with optional keywords and errorName."""
        fixer = UsdValidation.ValidationFixer(
            name="testFixer2",
            description="Another test fixer",
            fixerImplFn=self._ImplFn,
            canApplyFn=self._CanApplyFn,
            keywords=["studio", "lighting"],
            errorName="SomeError",
        )
        self.assertEqual(fixer.name, "testFixer2")
        self.assertEqual(fixer.errorName, "SomeError")
        self.assertIn("studio", fixer.keywords)
        self.assertIn("lighting", fixer.keywords)
        self.assertTrue(fixer.HasKeyword("studio"))
        self.assertFalse(fixer.HasKeyword("fx"))
        self.assertTrue(fixer.IsAssociatedWithErrorName("SomeError"))

    def test_ConstructionCallablesByValue(self):
        """A constructed fixer keeps Python callables by value."""
        calls = []

        def _CreateValidationFixer():
            def _CanApplyFn(error, editTarget, timeCode):
                calls.append("canApply")
                return True

            def _ImplFn(error, editTarget, timeCode):
                calls.append("apply")
                return False

            return UsdValidation.ValidationFixer(
                name="callableLifetimeFixer",
                description="Fixer with local Python callable refs",
                fixerImplFn=_ImplFn,
                canApplyFn=_CanApplyFn,
            )

        fixer = _CreateValidationFixer()

        layer = Sdf.Layer.CreateAnonymous(".usda")
        error = self._LayerTask(layer)[0]
        editTarget = Usd.EditTarget(layer)

        self.assertTrue(fixer.CanApplyFix(error, editTarget))
        self.assertFalse(fixer.ApplyFix(error, editTarget))

        self.assertEqual(calls, ["canApply", "apply"])

    def test_ConstructionCanApplyRaises(self):
        """Exceptions from canApplyFn are restored at the Python boundary."""
        _CanApplyFn = mock.Mock(side_effect=AttributeError)

        fixer = UsdValidation.ValidationFixer(
            name="canApplyRaisesFixer",
            description="Fixer with raising canApplyFn",
            fixerImplFn=self._ImplFn,
            canApplyFn=_CanApplyFn,
        )

        layer = Sdf.Layer.CreateAnonymous(".usda")
        error = self._LayerTask(layer)[0]
        editTarget = Usd.EditTarget(layer)

        with self.assertRaises(AttributeError):
            fixer.CanApplyFix(error, editTarget)

    def test_ConstructionFixerImplRaises(self):
        """Exceptions from fixerImplFn are restored at the Python boundary."""
        _ImplFn = mock.Mock(side_effect=AttributeError)

        fixer = UsdValidation.ValidationFixer(
            name="fixerImplRaisesFixer",
            description="Fixer with raising fixerImplFn",
            fixerImplFn=_ImplFn,
            canApplyFn=self._CanApplyFn,
        )

        layer = Sdf.Layer.CreateAnonymous(".usda")
        error = self._LayerTask(layer)[0]
        editTarget = Usd.EditTarget(layer)

        with self.assertRaises(AttributeError):
            fixer.ApplyFix(error, editTarget)

    def test_ValidationFixerByValue(self):
        """Registered validators keep copied fixer callables by value."""
        registry = UsdValidation.ValidationRegistry()
        _CanApplyFn = mock.Mock(return_value=True)
        _ImplFn = mock.Mock(return_value=False)

        fixer = UsdValidation.ValidationFixer(
            name="registeredFixer",
            description="Registered fixer",
            fixerImplFn=_ImplFn,
            canApplyFn=_CanApplyFn,
        )

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyFixer:RegisteredFixerByValue",
            doc="Validator with copied Python fixers",
            keywords=["testPyFixer"],
        )

        registry.RegisterLayerValidator(metadata, self._LayerTask, fixers=[fixer])
        del fixer

        validator = registry.GetOrLoadValidatorByName(
            "testPyFixer:RegisteredFixerByValue"
        )
        self.assertIsNotNone(validator)

        layer = Sdf.Layer.CreateAnonymous(".usda")
        errors = validator.Validate(layer)
        self.assertEqual(len(errors), 1)

        fixers = errors[0].GetFixers()
        self.assertEqual(len(fixers), 1)

        editTarget = Usd.EditTarget(layer)
        fixers[0].CanApplyFix(errors[0], editTarget)
        fixers[0].ApplyFix(errors[0], editTarget)

        _CanApplyFn.assert_called_once()
        _ImplFn.assert_called_once()

    def test_FixerLookupFromValidatorAndError(self):
        """Validator and error fixer lookup APIs return matching fixers."""
        registry = UsdValidation.ValidationRegistry()

        fixer_a = UsdValidation.ValidationFixer(
            name="fixerA",
            description="First fixer",
            fixerImplFn=self._ImplFn,
            canApplyFn=self._CanApplyFn,
            keywords=["teamA"],
        )
        fixer_b = UsdValidation.ValidationFixer(
            name="fixerB",
            description="Second fixer",
            fixerImplFn=self._ImplFn,
            canApplyFn=self._CanApplyFn,
            keywords=["teamB"],
            errorName="LayerFixerError",
        )

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyFixer:MultiFixerValidator",
            doc="Validator with multiple fixers",
            keywords=["testPyFixer"],
        )

        registry.RegisterLayerValidator(
            metadata, self._LayerTask, fixers=[fixer_a, fixer_b]
        )

        validator = registry.GetOrLoadValidatorByName("testPyFixer:MultiFixerValidator")
        fixers = validator.GetFixers()
        self.assertEqual(len(fixers), 2)

        found = validator.GetFixerByName("fixerA")
        self.assertIsNotNone(found)
        self.assertEqual(found.name, "fixerA")

        by_keyword = validator.GetFixersByKeywords(["teamA"])
        self.assertEqual(len(by_keyword), 1)
        self.assertEqual(by_keyword[0].name, "fixerA")

        by_error = validator.GetFixersByErrorName("LayerFixerError")
        self.assertEqual(len(by_error), 2)

        layer = Sdf.Layer.CreateAnonymous(".usda")
        errors = validator.Validate(layer)
        self.assertEqual(len(errors), 1)

        fixers = errors[0].GetFixersByErrorName()
        self.assertEqual(len(fixers), 2)

    def test_ThreadedFixerCallback(self):
        """A Python-backed fixer callback can be invoked from threads."""
        startEvent = threading.Event()

        def _CanApplyFn(error, editTarget, timeCode):
            return startEvent.wait(timeout=5.0)

        fixer = UsdValidation.ValidationFixer(
            name="threadedFixer",
            description="Fixer invoked from multiple threads",
            fixerImplFn=self._ImplFn,
            canApplyFn=_CanApplyFn,
        )

        layer = Sdf.Layer.CreateAnonymous(".usda")
        error = self._LayerTask(layer)[0]
        editTarget = Usd.EditTarget(layer)

        with concurrent.futures.ThreadPoolExecutor(max_workers=2) as executor:
            futures = [
                executor.submit(
                    functools.partial(fixer.CanApplyFix, error, editTarget)
                ),
                executor.submit(
                    functools.partial(fixer.CanApplyFix, error, editTarget)
                ),
            ]
            startEvent.set()

            self.assertEqual(
                [future.result(timeout=5.0) for future in futures], [True, True]
            )

    # ------------------------------------------------------------------
    # Manual registration tests
    # ------------------------------------------------------------------

    def test_LayerValidatorWithFixer(self):
        """Register a layer validator with a Python fixer."""
        registry = UsdValidation.ValidationRegistry()

        _CanApplyFn = mock.Mock(return_value=True)
        _ImplFn = mock.Mock(return_value=False)

        fixer = UsdValidation.ValidationFixer(
            name="layerFixer",
            description="Fixes layer errors",
            fixerImplFn=_ImplFn,
            canApplyFn=_CanApplyFn,
            errorName="LayerFixerError",
        )

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyFixer:LayerValidatorWithFixer",
            doc="Layer validator with a Python fixer",
            keywords=["testPyFixer"],
        )

        registry.RegisterLayerValidator(metadata, self._LayerTask, fixers=[fixer])

        validator = registry.GetOrLoadValidatorByName(
            "testPyFixer:LayerValidatorWithFixer"
        )
        self.assertIsNotNone(validator)

        fixers = validator.GetFixers()
        self.assertEqual(len(fixers), 1)
        self.assertEqual(fixers[0].name, "layerFixer")

        layer = Sdf.Layer.CreateAnonymous(".usda")
        errors = validator.Validate(layer)
        self.assertEqual(len(errors), 1)

        editTarget = Usd.EditTarget(layer)
        fixers[0].CanApplyFix(errors[0], editTarget)
        fixers[0].ApplyFix(errors[0], editTarget)

        _CanApplyFn.assert_called_once()
        _ImplFn.assert_called_once()

    def test_StageValidatorWithFixer(self):
        """Register a stage validator with a Python fixer."""
        registry = UsdValidation.ValidationRegistry()

        _CanApplyFn = mock.Mock(return_value=True)
        _ImplFn = mock.Mock(return_value=False)

        fixer = UsdValidation.ValidationFixer(
            name="stageFixer",
            description="Fixes stage errors",
            fixerImplFn=_ImplFn,
            canApplyFn=_CanApplyFn,
        )

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyFixer:StageValidatorWithFixer",
            doc="Stage validator with a Python fixer",
            keywords=["testPyFixer"],
        )

        registry.RegisterStageValidator(metadata, self._StageTask, fixers=[fixer])

        validator = registry.GetOrLoadValidatorByName(
            "testPyFixer:StageValidatorWithFixer"
        )
        fixers = validator.GetFixers()
        self.assertEqual(len(fixers), 1)
        self.assertEqual(fixers[0].name, "stageFixer")

        stage = Usd.Stage.CreateInMemory()
        errors = validator.Validate(stage)
        self.assertEqual(len(errors), 1)

        editTarget = Usd.EditTarget(stage.GetRootLayer())
        fixers[0].CanApplyFix(errors[0], editTarget)
        fixers[0].ApplyFix(errors[0], editTarget)

        _CanApplyFn.assert_called_once()
        _ImplFn.assert_called_once()

    def test_PrimValidatorWithFixer(self):
        """Register a prim validator with a Python fixer."""
        registry = UsdValidation.ValidationRegistry()

        _CanApplyFn = mock.Mock(return_value=True)
        _ImplFn = mock.Mock(return_value=False)

        fixer = UsdValidation.ValidationFixer(
            name="primFixer",
            description="Fixes prim errors",
            fixerImplFn=_ImplFn,
            canApplyFn=_CanApplyFn,
        )

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyFixer:PrimValidatorWithFixer",
            doc="Prim validator with a Python fixer",
            keywords=["testPyFixer"],
        )

        registry.RegisterPrimValidator(metadata, self._PrimTask, fixers=[fixer])

        validator = registry.GetOrLoadValidatorByName(
            "testPyFixer:PrimValidatorWithFixer"
        )
        fixers = validator.GetFixers()
        self.assertEqual(len(fixers), 1)
        self.assertEqual(fixers[0].name, "primFixer")

        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim("/TestPrim")
        errors = validator.Validate(prim)
        self.assertEqual(len(errors), 1)

        editTarget = Usd.EditTarget(stage.GetRootLayer())
        fixers[0].CanApplyFix(errors[0], editTarget)
        fixers[0].ApplyFix(errors[0], editTarget)

        _CanApplyFn.assert_called_once()
        _ImplFn.assert_called_once()

    # ------------------------------------------------------------------
    # Plugin registration tests
    # ------------------------------------------------------------------

    def test_RegisterPluginLayerValidatorWithFixer(self):
        """Register a plugin layer validator with a Python fixer."""
        registry = UsdValidation.ValidationRegistry()
        name = self.PLUGIN_NAME + ":PyPluginLayerValidatorWithFixer"

        _CanApplyFn = mock.Mock(return_value=True)
        _ImplFn = mock.Mock(return_value=False)

        fixer = UsdValidation.ValidationFixer(
            name="pluginLayerFixer",
            description="Plugin layer fixer",
            fixerImplFn=_ImplFn,
            canApplyFn=_CanApplyFn,
        )

        registry.RegisterPluginLayerValidator(name, self._LayerTask, fixers=[fixer])

        validator = registry.GetOrLoadValidatorByName(name)
        self.assertIsNotNone(validator)
        fixers = validator.GetFixers()
        self.assertEqual(len(fixers), 1)
        self.assertEqual(fixers[0].name, "pluginLayerFixer")

        layer = Sdf.Layer.CreateAnonymous(".usda")
        errors = validator.Validate(layer)
        self.assertEqual(len(errors), 1)

        editTarget = Usd.EditTarget(layer)
        fixers[0].CanApplyFix(errors[0], editTarget)
        fixers[0].ApplyFix(errors[0], editTarget)

        _CanApplyFn.assert_called_once()
        _ImplFn.assert_called_once()

    def test_RegisterPluginStageValidatorWithFixer(self):
        """Register a plugin stage validator with a Python fixer."""
        registry = UsdValidation.ValidationRegistry()
        name = self.PLUGIN_NAME + ":PyPluginStageValidatorWithFixer"

        _CanApplyFn = mock.Mock(return_value=True)
        _ImplFn = mock.Mock(return_value=False)

        fixer = UsdValidation.ValidationFixer(
            name="pluginStageFixer",
            description="Plugin stage fixer",
            fixerImplFn=_ImplFn,
            canApplyFn=_CanApplyFn,
        )

        registry.RegisterPluginStageValidator(name, self._StageTask, fixers=[fixer])

        validator = registry.GetOrLoadValidatorByName(name)
        self.assertIsNotNone(validator)
        fixers = validator.GetFixers()
        self.assertEqual(len(fixers), 1)
        self.assertEqual(fixers[0].name, "pluginStageFixer")

        stage = Usd.Stage.CreateInMemory()
        errors = validator.Validate(stage)
        self.assertEqual(len(errors), 1)

        editTarget = Usd.EditTarget(stage.GetRootLayer())
        fixers[0].CanApplyFix(errors[0], editTarget)
        fixers[0].ApplyFix(errors[0], editTarget)

        _CanApplyFn.assert_called_once()
        _ImplFn.assert_called_once()

    def test_RegisterPluginPrimValidatorWithFixer(self):
        """Register a plugin prim validator with a Python fixer."""
        registry = UsdValidation.ValidationRegistry()
        name = self.PLUGIN_NAME + ":PyPluginPrimValidatorWithFixer"

        _CanApplyFn = mock.Mock(return_value=True)
        _ImplFn = mock.Mock(return_value=False)

        fixer = UsdValidation.ValidationFixer(
            name="pluginPrimFixer",
            description="Plugin prim fixer",
            fixerImplFn=_ImplFn,
            canApplyFn=_CanApplyFn,
        )

        registry.RegisterPluginPrimValidator(name, self._PrimTask, fixers=[fixer])

        validator = registry.GetOrLoadValidatorByName(name)
        self.assertIsNotNone(validator)
        fixers = validator.GetFixers()
        self.assertEqual(len(fixers), 1)
        self.assertEqual(fixers[0].name, "pluginPrimFixer")

        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim("/PluginPrim")
        errors = validator.Validate(prim)
        self.assertEqual(len(errors), 1)

        editTarget = Usd.EditTarget(stage.GetRootLayer())
        fixers[0].CanApplyFix(errors[0], editTarget)
        fixers[0].ApplyFix(errors[0], editTarget)

        _CanApplyFn.assert_called_once()
        _ImplFn.assert_called_once()


if __name__ == "__main__":
    unittest.main()
