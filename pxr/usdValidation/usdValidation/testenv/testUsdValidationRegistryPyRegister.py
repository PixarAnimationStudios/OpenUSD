#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

"""Tests for registering validators implemented in Python.

Two registration paths are exercised:

1. Explicit registration (RegisterLayerValidator, etc.) -- the caller
   provides full ValidatorMetadata.

2. Plugin registration (RegisterPluginLayerValidator, etc.) -- the caller
   provides only a TfToken name; metadata comes from the plugin's
   plugInfo.json.
"""

import os
import unittest

from pxr import Plug, Sdf, Usd, UsdValidation

class TestUsdValidationRegistryPyRegister(unittest.TestCase):

    PLUGIN_NAME = "testValidationRegistryPyPlugin"

    @classmethod
    def setUpClass(cls):
        # Register the test plugin BEFORE the ValidationRegistry singleton
        # is created.  The registry constructor parses plugInfo.json from
        # all known plugins; if the plugin is not registered first, its
        # validator metadata will not be available for
        # RegisterPlugin*Validator calls.
        testPluginsDsoSearchPath = os.path.join(
            os.path.dirname(__file__),
            "UsdValidationPlugins/lib/TestUsdValidationRegistryPy*/"
            "Resources/")
        try:
            plugins = Plug.Registry().RegisterPlugins(testPluginsDsoSearchPath)
            assert len(plugins) == 1
            assert plugins[0].name == cls.PLUGIN_NAME
        except RuntimeError:
            pass  # Plugin may already be registered.

    def test_RegisterLayerValidator(self):
        # RegisterLayerValidator accepts a Python callable with the signature
        # (layer: SdfLayerHandle) -> list[ValidationError].  The callable is
        # wrapped in a C++ UsdValidateLayerTaskFn so the registry treats it
        # identically to a C++-implemented validator.
        registry = UsdValidation.ValidationRegistry()

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyRegister:LayerValidator",
            doc="A Python-implemented layer validator",
            keywords=["testPyRegister"],
        )

        # Track invocation to verify the Python callable is actually called.
        called_with = []

        def layer_task(layer):
            called_with.append(layer)
            return [
                UsdValidation.ValidationError(
                    "LayerError",
                    UsdValidation.ValidationErrorType.Warn,
                    [UsdValidation.ValidationErrorSite(
                        layer, Sdf.Path.absoluteRootPath)],
                    "Python layer validator ran",
                )
            ]

        # We register the validator explicitly here using the
        # RegisterLayerValidator API, but in practice we expect validators for a
        # python plugin to be registered via RegisterPlugin*Validator APIs, in
        # the plugin's __init__.py, and its metadata provided in plugInfo.json.
        registry.RegisterLayerValidator(metadata, layer_task)

        # The validator should be queryable by name immediately after
        # registration, before any Validate() call.
        self.assertTrue(registry.HasValidator("testPyRegister:LayerValidator"))

        validator = registry.GetOrLoadValidatorByName(
            "testPyRegister:LayerValidator"
        )
        self.assertIsNotNone(validator)

        # Invoke the validator and confirm the Python task ran, returned the
        # expected error name and type, and received the layer as its argument.
        layer = Sdf.Layer.CreateAnonymous(".usda")
        errors = validator.Validate(layer)
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "LayerError")
        self.assertEqual(
            errors[0].GetType(), UsdValidation.ValidationErrorType.Warn
        )
        # Confirm the Python callable was invoked exactly once with the layer.
        self.assertEqual(len(called_with), 1)

    def test_RegisterStageValidator(self):
        # RegisterStageValidator accepts a Python callable with the signature
        # (stage: UsdStagePtr, timeRange: UsdValidationTimeRange)
        # -> list[ValidationError].
        registry = UsdValidation.ValidationRegistry()

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyRegister:StageValidator",
            doc="A Python-implemented stage validator",
            keywords=["testPyRegister"],
        )

        def stage_task(stage, timeRange):
            return [
                UsdValidation.ValidationError(
                    "StageError",
                    UsdValidation.ValidationErrorType.Error,
                    [
                        UsdValidation.ValidationErrorSite(
                            stage, Sdf.Path.absoluteRootPath
                        )
                    ],
                    "Python stage validator ran",
                )
            ]

        registry.RegisterStageValidator(metadata, stage_task)
        self.assertTrue(registry.HasValidator("testPyRegister:StageValidator"))

        # Validate() with a UsdStage dispatches to the stage task function.
        # The default UsdValidationTimeRange is passed automatically.
        validator = registry.GetOrLoadValidatorByName(
            "testPyRegister:StageValidator"
        )
        stage = Usd.Stage.CreateInMemory()
        errors = validator.Validate(stage)
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "StageError")

    def test_RegisterPrimValidator(self):
        # RegisterPrimValidator accepts a Python callable with the signature
        # (prim: UsdPrim, timeRange: UsdValidationTimeRange)
        # -> list[ValidationError].
        registry = UsdValidation.ValidationRegistry()

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyRegister:PrimValidator",
            doc="A Python-implemented prim validator",
            keywords=["testPyRegister"],
        )

        def prim_task(prim, timeRange):
            # The registry calls this for every prim during traversal,
            # including the pseudo-root.  Skip it to avoid spurious errors.
            if prim.IsPseudoRoot():
                return []
            return [
                UsdValidation.ValidationError(
                    "PrimError",
                    UsdValidation.ValidationErrorType.Error,
                    [
                        UsdValidation.ValidationErrorSite(
                            prim.GetStage(), prim.GetPath()
                        )
                    ],
                    f"Python prim validator ran on {prim.GetPath()}",
                )
            ]

        registry.RegisterPrimValidator(metadata, prim_task)
        self.assertTrue(registry.HasValidator("testPyRegister:PrimValidator"))

        # Validate() with a UsdPrim dispatches to the prim task function.
        validator = registry.GetOrLoadValidatorByName(
            "testPyRegister:PrimValidator"
        )
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim("/TestPrim")
        errors = validator.Validate(prim)
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "PrimError")

    def test_RegisterValidatorSuite(self):
        # RegisterValidatorSuite groups existing validators under a single
        # named suite.  Suites can be retrieved and inspected for their
        # contained validators, and are usable with ValidationContext.
        registry = UsdValidation.ValidationRegistry()

        # The registry is a singleton; validators registered in earlier
        # tests persist for the lifetime of the process.  Python's unittest
        # runner executes methods alphabetically, so test_RegisterPrimValidator
        # and test_RegisterStageValidator are guaranteed to have run before
        # this method.  Retrieve the validators they registered.
        stage_validator = registry.GetOrLoadValidatorByName(
            "testPyRegister:StageValidator"
        )
        prim_validator = registry.GetOrLoadValidatorByName(
            "testPyRegister:PrimValidator"
        )
        self.assertIsNotNone(stage_validator)
        self.assertIsNotNone(prim_validator)

        # isSuite=True is required in the metadata for suite registration.
        suite_metadata = UsdValidation.ValidatorMetadata(
            name="testPyRegister:Suite",
            doc="A Python-registered validator suite",
            keywords=["testPyRegister"],
            isSuite=True,
        )

        registry.RegisterValidatorSuite(
            suite_metadata, [stage_validator, prim_validator]
        )

        # Confirm the suite is discoverable via HasValidatorSuite and
        # GetOrLoadValidatorSuiteByName.
        self.assertTrue(
            registry.HasValidatorSuite("testPyRegister:Suite")
        )

        suite = registry.GetOrLoadValidatorSuiteByName("testPyRegister:Suite")
        self.assertIsNotNone(suite)

        # Verify both validators are present in the suite's contained set.
        contained = suite.GetContainedValidators()
        self.assertEqual(len(contained), 2)
        self.assertIn(stage_validator, contained)
        self.assertIn(prim_validator, contained)

    def test_PythonValidatorInContext(self):
        """Run a Python-registered validator through ValidationContext."""
        # ValidationContext is the primary way clients run batches of
        # validators.  Verify that a Python-registered validator participates
        # correctly when driven through the context rather than called
        # directly via Validator.Validate().
        registry = UsdValidation.ValidationRegistry()

        # Register a fresh validator so this test is self-contained and does
        # not depend on test execution order.
        metadata = UsdValidation.ValidatorMetadata(
            name="testPyRegister:ContextStageValidator",
            doc="Stage validator used for context test",
            keywords=["testPyRegister"],
        )

        def context_stage_task(stage, timeRange):
            return [
                UsdValidation.ValidationError(
                    "ContextStageError",
                    UsdValidation.ValidationErrorType.Error,
                    [
                        UsdValidation.ValidationErrorSite(
                            stage, Sdf.Path.absoluteRootPath
                        )
                    ],
                    "Context stage validator ran",
                )
            ]

        registry.RegisterStageValidator(metadata, context_stage_task)
        validator = registry.GetOrLoadValidatorByName(
            "testPyRegister:ContextStageValidator"
        )
        self.assertIsNotNone(validator)

        # Construct a context with just this validator and validate a stage.
        # The context aggregates errors from all contained validators.
        context = UsdValidation.ValidationContext([validator])
        stage = Usd.Stage.CreateInMemory()
        errors = context.Validate(stage)
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "ContextStageError")

    def test_PythonValidatorReturnsEmpty(self):
        """A Python validator that returns no errors."""
        # Verify that returning an empty list from a Python task function
        # is handled correctly: no errors should be reported and no
        # exception should be raised.
        registry = UsdValidation.ValidationRegistry()

        metadata = UsdValidation.ValidatorMetadata(
            name="testPyRegister:NoOpStageValidator",
            doc="A Python stage validator that always passes",
            keywords=["testPyRegister"],
        )

        def no_op_stage_task(stage, timeRange):
            return []

        registry.RegisterStageValidator(metadata, no_op_stage_task)
        validator = registry.GetOrLoadValidatorByName(
            "testPyRegister:NoOpStageValidator"
        )
        stage = Usd.Stage.CreateInMemory()
        errors = validator.Validate(stage)
        self.assertEqual(len(errors), 0)

    # ------------------------------------------------------------------
    # Plugin registration tests
    #
    # These exercise RegisterPlugin*Validator, where the validator name
    # maps to metadata already declared in plugInfo.json.  The plugin
    # was registered in setUpClass.
    # ------------------------------------------------------------------

    def test_RegisterPluginLayerValidator(self):
        """Register a layer validator using plugin metadata."""
        registry = UsdValidation.ValidationRegistry()
        name = self.PLUGIN_NAME + ":PyPluginLayerValidator"

        called_with = []

        def layer_task(layer):
            called_with.append(layer)
            return [
                UsdValidation.ValidationError(
                    "PluginLayerError",
                    UsdValidation.ValidationErrorType.Warn,
                    [UsdValidation.ValidationErrorSite(
                        layer, Sdf.Path.absoluteRootPath)],
                    "Plugin layer validator ran",
                )
            ]

        registry.RegisterPluginLayerValidator(name, layer_task)
        self.assertTrue(registry.HasValidator(name))

        validator = registry.GetOrLoadValidatorByName(name)
        self.assertIsNotNone(validator)

        # Metadata should come from plugInfo.json, not from the caller.
        metadata = validator.GetMetadata()
        self.assertEqual(
            metadata.doc, "Layer validator registered from Python.")
        self.assertIn("testPyPlugin", metadata.GetKeywords())

        layer = Sdf.Layer.CreateAnonymous(".usda")
        errors = validator.Validate(layer)
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "PluginLayerError")
        self.assertEqual(len(called_with), 1)

    def test_RegisterPluginStageValidator(self):
        """Register a stage validator using plugin metadata."""
        registry = UsdValidation.ValidationRegistry()
        name = self.PLUGIN_NAME + ":PyPluginStageValidator"

        def stage_task(stage, timeRange):
            return [
                UsdValidation.ValidationError(
                    "PluginStageError",
                    UsdValidation.ValidationErrorType.Error,
                    [UsdValidation.ValidationErrorSite(
                        stage, Sdf.Path.absoluteRootPath)],
                    "Plugin stage validator ran",
                )
            ]

        registry.RegisterPluginStageValidator(name, stage_task)
        self.assertTrue(registry.HasValidator(name))

        validator = registry.GetOrLoadValidatorByName(name)
        stage = Usd.Stage.CreateInMemory()
        errors = validator.Validate(stage)
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "PluginStageError")

    def test_RegisterPluginPrimValidator(self):
        """Register a prim validator using plugin metadata."""
        registry = UsdValidation.ValidationRegistry()
        name = self.PLUGIN_NAME + ":PyPluginPrimValidator"

        def prim_task(prim, timeRange):
            if prim.IsPseudoRoot():
                return []
            return [
                UsdValidation.ValidationError(
                    "PluginPrimError",
                    UsdValidation.ValidationErrorType.Error,
                    [UsdValidation.ValidationErrorSite(
                        prim.GetStage(), prim.GetPath())],
                    f"Plugin prim validator ran on {prim.GetPath()}",
                )
            ]

        registry.RegisterPluginPrimValidator(name, prim_task)
        self.assertTrue(registry.HasValidator(name))

        validator = registry.GetOrLoadValidatorByName(name)
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim("/TestPrim")
        errors = validator.Validate(prim)
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "PluginPrimError")

    def test_RegisterPluginValidatorSuite(self):
        """Register a validator suite using plugin metadata."""
        # Depends on test_RegisterPluginStageValidator and
        # test_RegisterPluginPrimValidator having run first
        # (alphabetical order guarantees this).
        registry = UsdValidation.ValidationRegistry()
        suite_name = self.PLUGIN_NAME + ":PyPluginSuite"

        stage_validator = registry.GetOrLoadValidatorByName(
            self.PLUGIN_NAME + ":PyPluginStageValidator"
        )
        prim_validator = registry.GetOrLoadValidatorByName(
            self.PLUGIN_NAME + ":PyPluginPrimValidator"
        )
        self.assertIsNotNone(stage_validator)
        self.assertIsNotNone(prim_validator)

        registry.RegisterPluginValidatorSuite(
            suite_name, [stage_validator, prim_validator]
        )
        self.assertTrue(registry.HasValidatorSuite(suite_name))

        suite = registry.GetOrLoadValidatorSuiteByName(suite_name)
        self.assertIsNotNone(suite)

        contained = suite.GetContainedValidators()
        self.assertEqual(len(contained), 2)
        self.assertIn(stage_validator, contained)
        self.assertIn(prim_validator, contained)

    def test_NoticeOnRegisterValidator(self):
        from pxr import Tf

        received_validator_notices = []
        key = Tf.Notice.RegisterGlobally(
            UsdValidation.Notice.DidRegisterValidator,
            lambda notice, sender: received_validator_notices.append(
                notice.GetValidator()))

        registry = UsdValidation.ValidationRegistry()
        metadata = UsdValidation.ValidatorMetadata(
            name="testPyStageValidatorForNotice",
            doc="A validator used to test DidRegisterValidator notice"
        )
        registry.RegisterStageValidator(metadata, lambda stage, timeRange: [])

        self.assertEqual(len(received_validator_notices), 1)
        self.assertEqual(
            received_validator_notices[0].GetMetadata().name,
            "testPyStageValidatorForNotice"
        )
        key.Revoke()

    def test_NoticeOnRegisterValidatorSuite(self):
        from pxr import Tf

        received_suite_notices = []
        key = Tf.Notice.RegisterGlobally(
            UsdValidation.Notice.DidRegisterValidatorSuite,
            lambda notice, sender: received_suite_notices.append(
                notice.GetValidatorSuite()))

        registry = UsdValidation.ValidationRegistry()
        suite_metadata = UsdValidation.ValidatorMetadata(
            name="testPyValidatorSuiteForNotice",
            doc="A validator suite used to test DidRegisterValidatorSuite notice",
            isSuite=True,
        )
        registry.RegisterValidatorSuite(suite_metadata, [])

        self.assertEqual(len(received_suite_notices), 1)
        self.assertEqual(
            received_suite_notices[0].GetMetadata().name, 
            "testPyValidatorSuiteForNotice"
        )
        key.Revoke()


if __name__ == "__main__":
    unittest.main()
