#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

"""Tests for registering validators implemented in Python."""

import unittest

from pxr import Sdf, Usd, UsdValidation


class TestUsdValidationRegistryPyRegister(unittest.TestCase):

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


if __name__ == "__main__":
    unittest.main()
