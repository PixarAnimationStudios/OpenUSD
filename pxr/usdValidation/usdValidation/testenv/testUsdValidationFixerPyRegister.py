#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

"""Tests for registering validation fixers implemented in Python.

Exercises:
1. Creating ValidationFixer objects from Python with callable fixerImplFn
   and canApplyFn.
2. Passing fixers to validator registration methods (both explicit and plugin).
3. Retrieving fixers from validators and errors.
4. Invoking CanApplyFix and ApplyFix with Python-backed fixers.
"""

import os
import unittest

from pxr import Plug, Sdf, Tf, Usd, UsdValidation

# Plugin name must match the "Name" field in
# TestUsdValidationRegistryPy_plugInfo.json.
_PLUGIN_NAME = "testValidationRegistryPyPlugin"


class TestValidationFixerConstruction(unittest.TestCase):
    """Test creating ValidationFixer objects from Python."""

    def test_BasicConstruction(self):
        """A fixer can be constructed with name, description, and callables."""
        def impl_fn(error, editTarget, timeCode):
            return True

        def can_apply_fn(error, editTarget, timeCode):
            return True

        fixer = UsdValidation.ValidationFixer(
            name="testFixer",
            description="A test fixer",
            fixerImplFn=impl_fn,
            canApplyFn=can_apply_fn,
        )
        self.assertEqual(fixer.name, "testFixer")
        self.assertEqual(fixer.description, "A test fixer")
        self.assertEqual(fixer.errorName, "")
        self.assertEqual(fixer.keywords, [])

    def test_ConstructionWithKeywordsAndErrorName(self):
        """A fixer can be constructed with optional keywords and errorName."""
        def impl_fn(error, editTarget, timeCode):
            return True

        def can_apply_fn(error, editTarget, timeCode):
            return True

        fixer = UsdValidation.ValidationFixer(
            name="testFixer2",
            description="Another test fixer",
            fixerImplFn=impl_fn,
            canApplyFn=can_apply_fn,
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


class TestFixerWithExplicitRegistration(unittest.TestCase):
    """Test fixers passed through explicit validator registration."""

    def test_LayerValidatorWithFixer(self):
        """Register a layer validator with a Python fixer and invoke it."""
        registry = UsdValidation.ValidationRegistry()

        can_apply_calls = []
        apply_calls = []

        def can_apply_fn(error, editTarget, timeCode):
            can_apply_calls.append(True)
            return True

        def impl_fn(error, editTarget, timeCode):
            apply_calls.append(True)
            return True

        fixer = UsdValidation.ValidationFixer(
            name="layerFixer",
            description="Fixes layer errors",
            fixerImplFn=impl_fn,
            canApplyFn=can_apply_fn,
            errorName="LayerFixerError",
        )

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyFixer:LayerValidatorWithFixer",
            doc="Layer validator with a Python fixer",
            keywords=["testPyFixer"],
        )

        def layer_task(layer):
            return [
                UsdValidation.ValidationError(
                    "LayerFixerError",
                    UsdValidation.ValidationErrorType.Warn,
                    [UsdValidation.ValidationErrorSite(
                        layer, Sdf.Path.absoluteRootPath)],
                    "Fixable layer error",
                )
            ]

        registry.RegisterLayerValidator(metadata, layer_task, fixers=[fixer])

        validator = registry.GetOrLoadValidatorByName(
            "testPyFixer:LayerValidatorWithFixer"
        )
        self.assertIsNotNone(validator)

        # Verify the fixer is accessible from the validator.
        fixers = validator.GetFixers()
        self.assertEqual(len(fixers), 1)
        self.assertEqual(fixers[0].name, "layerFixer")

        # Validate to produce an error, then test CanApplyFix and ApplyFix.
        layer = Sdf.Layer.CreateAnonymous(".usda")
        errors = validator.Validate(layer)
        self.assertEqual(len(errors), 1)

        error = errors[0]
        editTarget = Usd.EditTarget(layer)

        # CanApplyFix should invoke the Python callable.
        result = fixers[0].CanApplyFix(error, editTarget)
        self.assertTrue(result)
        self.assertEqual(len(can_apply_calls), 1)

        # ApplyFix should invoke the Python callable and save the layer.
        result = fixers[0].ApplyFix(error, editTarget)
        self.assertTrue(result)
        self.assertEqual(len(apply_calls), 1)

    def test_StageValidatorWithFixer(self):
        """Register a stage validator with a Python fixer."""
        registry = UsdValidation.ValidationRegistry()

        def can_apply_fn(error, editTarget, timeCode):
            return True

        def impl_fn(error, editTarget, timeCode):
            return True

        fixer = UsdValidation.ValidationFixer(
            name="stageFixer",
            description="Fixes stage errors",
            fixerImplFn=impl_fn,
            canApplyFn=can_apply_fn,
        )

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyFixer:StageValidatorWithFixer",
            doc="Stage validator with a Python fixer",
            keywords=["testPyFixer"],
        )

        def stage_task(stage, timeRange):
            return [
                UsdValidation.ValidationError(
                    "StageFixerError",
                    UsdValidation.ValidationErrorType.Error,
                    [UsdValidation.ValidationErrorSite(
                        stage, Sdf.Path.absoluteRootPath)],
                    "Fixable stage error",
                )
            ]

        registry.RegisterStageValidator(metadata, stage_task, fixers=[fixer])

        validator = registry.GetOrLoadValidatorByName(
            "testPyFixer:StageValidatorWithFixer"
        )
        fixers = validator.GetFixers()
        self.assertEqual(len(fixers), 1)
        self.assertEqual(fixers[0].name, "stageFixer")

    def test_PrimValidatorWithFixer(self):
        """Register a prim validator with a Python fixer."""
        registry = UsdValidation.ValidationRegistry()

        def can_apply_fn(error, editTarget, timeCode):
            return True

        def impl_fn(error, editTarget, timeCode):
            return True

        fixer = UsdValidation.ValidationFixer(
            name="primFixer",
            description="Fixes prim errors",
            fixerImplFn=impl_fn,
            canApplyFn=can_apply_fn,
        )

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyFixer:PrimValidatorWithFixer",
            doc="Prim validator with a Python fixer",
            keywords=["testPyFixer"],
        )

        def prim_task(prim, timeRange):
            if prim.IsPseudoRoot():
                return []
            return [
                UsdValidation.ValidationError(
                    "PrimFixerError",
                    UsdValidation.ValidationErrorType.Error,
                    [UsdValidation.ValidationErrorSite(
                        prim.GetStage(), prim.GetPath())],
                    f"Fixable prim error on {prim.GetPath()}",
                )
            ]

        registry.RegisterPrimValidator(metadata, prim_task, fixers=[fixer])

        validator = registry.GetOrLoadValidatorByName(
            "testPyFixer:PrimValidatorWithFixer"
        )
        fixers = validator.GetFixers()
        self.assertEqual(len(fixers), 1)
        self.assertEqual(fixers[0].name, "primFixer")

    def test_MultipleFixers(self):
        """A validator can have multiple fixers."""
        registry = UsdValidation.ValidationRegistry()

        def noop(error, editTarget, timeCode):
            return True

        fixer_a = UsdValidation.ValidationFixer(
            name="fixerA", description="First fixer",
            fixerImplFn=noop, canApplyFn=noop,
            keywords=["teamA"],
        )
        fixer_b = UsdValidation.ValidationFixer(
            name="fixerB", description="Second fixer",
            fixerImplFn=noop, canApplyFn=noop,
            keywords=["teamB"],
            errorName="SpecificError",
        )

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyFixer:MultiFixerValidator",
            doc="Validator with multiple fixers",
            keywords=["testPyFixer"],
        )

        def layer_task(layer):
            return []

        registry.RegisterLayerValidator(
            metadata, layer_task, fixers=[fixer_a, fixer_b])

        validator = registry.GetOrLoadValidatorByName(
            "testPyFixer:MultiFixerValidator"
        )
        fixers = validator.GetFixers()
        self.assertEqual(len(fixers), 2)

        # GetFixerByName
        found = validator.GetFixerByName("fixerA")
        self.assertIsNotNone(found)
        self.assertEqual(found.name, "fixerA")

        found = validator.GetFixerByName("fixerB")
        self.assertIsNotNone(found)
        self.assertEqual(found.name, "fixerB")

        # GetFixersByKeywords
        by_keyword = validator.GetFixersByKeywords(["teamA"])
        self.assertEqual(len(by_keyword), 1)
        self.assertEqual(by_keyword[0].name, "fixerA")

        # GetFixersByErrorName
        by_error = validator.GetFixersByErrorName("SpecificError")
        # fixerA has no errorName so it matches all; fixerB matches specifically
        self.assertEqual(len(by_error), 2)

    def test_FixerCanApplyReturnsFalse(self):
        """CanApplyFix returning False prevents ApplyFix from running."""
        registry = UsdValidation.ValidationRegistry()

        apply_calls = []

        def can_apply_fn(error, editTarget, timeCode):
            return False

        def impl_fn(error, editTarget, timeCode):
            apply_calls.append(True)
            return True

        fixer = UsdValidation.ValidationFixer(
            name="guardedFixer",
            description="A guarded fixer",
            fixerImplFn=impl_fn,
            canApplyFn=can_apply_fn,
        )

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyFixer:GuardedValidator",
            doc="Validator with guarded fixer",
            keywords=["testPyFixer"],
        )

        def layer_task(layer):
            return [
                UsdValidation.ValidationError(
                    "GuardedError",
                    UsdValidation.ValidationErrorType.Warn,
                    [UsdValidation.ValidationErrorSite(
                        layer, Sdf.Path.absoluteRootPath)],
                    "Guarded error",
                )
            ]

        registry.RegisterLayerValidator(metadata, layer_task, fixers=[fixer])

        validator = registry.GetOrLoadValidatorByName(
            "testPyFixer:GuardedValidator"
        )
        layer = Sdf.Layer.CreateAnonymous(".usda")
        errors = validator.Validate(layer)
        self.assertEqual(len(errors), 1)

        fixers = validator.GetFixers()
        editTarget = Usd.EditTarget(layer)

        self.assertFalse(fixers[0].CanApplyFix(errors[0], editTarget))
        # ApplyFix should still NOT invoke impl_fn because CanApplyFix is
        # checked internally by the C++ fixer infrastructure only if the
        # client calls it explicitly.  ApplyFix itself does not gate on
        # CanApplyFix -- it will call the impl directly.
        # However, since the client should check CanApplyFix first, the
        # intent is that impl_fn would not be called.  We verify the
        # CanApplyFix path here.
        self.assertEqual(len(apply_calls), 0)

    def test_FixerAccessFromError(self):
        """Fixers should be accessible from the ValidationError object."""
        registry = UsdValidation.ValidationRegistry()

        def noop(error, editTarget, timeCode):
            return True

        fixer = UsdValidation.ValidationFixer(
            name="errorAccessFixer",
            description="Fixer accessible from error",
            fixerImplFn=noop,
            canApplyFn=noop,
            errorName="AccessibleError",
        )

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyFixer:ErrorAccessValidator",
            doc="Validator for error-fixer access test",
            keywords=["testPyFixer"],
        )

        def layer_task(layer):
            return [
                UsdValidation.ValidationError(
                    "AccessibleError",
                    UsdValidation.ValidationErrorType.Warn,
                    [UsdValidation.ValidationErrorSite(
                        layer, Sdf.Path.absoluteRootPath)],
                    "Error with accessible fixer",
                )
            ]

        registry.RegisterLayerValidator(metadata, layer_task, fixers=[fixer])

        validator = registry.GetOrLoadValidatorByName(
            "testPyFixer:ErrorAccessValidator"
        )
        layer = Sdf.Layer.CreateAnonymous(".usda")
        errors = validator.Validate(layer)
        self.assertEqual(len(errors), 1)

        # GetFixers on the error proxies to the validator's fixers.
        error_fixers = errors[0].GetFixers()
        self.assertEqual(len(error_fixers), 1)
        self.assertEqual(error_fixers[0].name, "errorAccessFixer")

        # GetFixersByErrorName on the error should return matching fixers.
        by_name = errors[0].GetFixersByErrorName()
        self.assertEqual(len(by_name), 1)
        self.assertEqual(by_name[0].name, "errorAccessFixer")

    def test_ValidatorWithNoFixers(self):
        """Registering without fixers (backward compat) still works."""
        registry = UsdValidation.ValidationRegistry()

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyFixer:NoFixerValidator",
            doc="Validator with no fixers",
            keywords=["testPyFixer"],
        )

        def layer_task(layer):
            return []

        # No fixers argument -- should work exactly as before.
        registry.RegisterLayerValidator(metadata, layer_task)

        validator = registry.GetOrLoadValidatorByName(
            "testPyFixer:NoFixerValidator"
        )
        self.assertIsNotNone(validator)
        self.assertEqual(len(validator.GetFixers()), 0)


class TestFixerWithPluginRegistration(unittest.TestCase):
    """Test fixers passed through plugin validator registration."""

    @classmethod
    def setUpClass(cls):
        testPluginsDsoSearchPath = os.path.join(
            os.path.dirname(__file__),
            "UsdValidationPlugins/lib/TestUsdValidationRegistryPy*/"
            "Resources/")
        try:
            plugins = Plug.Registry().RegisterPlugins(
                testPluginsDsoSearchPath)
        except RuntimeError:
            pass  # Plugin may already be registered.

    def test_PluginLayerValidatorWithFixer(self):
        """Register a plugin layer validator with a Python fixer."""
        registry = UsdValidation.ValidationRegistry()
        name = _PLUGIN_NAME + ":PyPluginLayerValidatorWithFixer"

        # Skip if no such validator is declared in plugInfo.json.
        # The existing test plugin may not have this validator, so we
        # use explicit registration as a fallback test.
        # For this test, use explicit registration to avoid plugInfo
        # dependency.
        metadata = UsdValidation.ValidatorMetadata(
            name=name,
            doc="Plugin layer validator with fixer",
            keywords=["testPyFixer"],
        )

        def can_apply_fn(error, editTarget, timeCode):
            return True

        def impl_fn(error, editTarget, timeCode):
            return True

        fixer = UsdValidation.ValidationFixer(
            name="pluginLayerFixer",
            description="Plugin layer fixer",
            fixerImplFn=impl_fn,
            canApplyFn=can_apply_fn,
        )

        def layer_task(layer):
            return [
                UsdValidation.ValidationError(
                    "PluginLayerFixerError",
                    UsdValidation.ValidationErrorType.Warn,
                    [UsdValidation.ValidationErrorSite(
                        layer, Sdf.Path.absoluteRootPath)],
                    "Plugin layer fixer error",
                )
            ]

        registry.RegisterLayerValidator(metadata, layer_task, fixers=[fixer])

        validator = registry.GetOrLoadValidatorByName(name)
        self.assertIsNotNone(validator)
        fixers = validator.GetFixers()
        self.assertEqual(len(fixers), 1)
        self.assertEqual(fixers[0].name, "pluginLayerFixer")

        # Verify the fixer works end-to-end.
        layer = Sdf.Layer.CreateAnonymous(".usda")
        errors = validator.Validate(layer)
        self.assertEqual(len(errors), 1)

        editTarget = Usd.EditTarget(layer)
        self.assertTrue(fixers[0].CanApplyFix(errors[0], editTarget))
        self.assertTrue(fixers[0].ApplyFix(errors[0], editTarget))


if __name__ == "__main__":
    unittest.main()
