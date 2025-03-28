#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os
import unittest

from pxr import Plug, Usd, UsdValidation


class TestUsdValidationRegistryPy(unittest.TestCase):
    VALIDATOR1_NAME = "TestValidator1"
    VALIDATOR2_NAME = "TestValidator2"
    VALIDATOR_SUITE_NAME = "TestValidatorSuite"
    PLUGIN_NAME = "testValidationPlugin"

    @classmethod
    def setUpClass(cls) -> None:
        # Register TestUsdValidationRegistryPy plugin. We assume the build
        # system will install it to the UsdValidationPlugins subdirectory in the
        # same location as this test.
        testPluginsDsoSearchPath = os.path.join(
            os.path.dirname(__file__),
            "UsdValidationPlugins/lib/TestUsdValidationRegistryPy*/Resources/")
        try:
            plugins = Plug.Registry().RegisterPlugins(testPluginsDsoSearchPath)
            assert len(plugins) == 1
            assert plugins[0].name == "testValidationRegistryPyPlugin"
        except RuntimeError:
            pass

        validationRegistry = UsdValidation.ValidationRegistry()
        validationRegistry.GetOrLoadAllValidators()

    def test_QueryValidatorsAndSuites(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        self.assertFalse(validationRegistry.HasValidator("invalid_validator"))

        allValidators = validationRegistry.GetOrLoadAllValidators()
        self.assertTrue(allValidators)

        validator = validationRegistry.GetOrLoadValidatorByName(
            self.VALIDATOR1_NAME
        )
        self.assertTrue(validator)
        self.assertEqual(eval(repr(validator)), validator)

        validators = validationRegistry.GetOrLoadValidatorsByName(
            [self.VALIDATOR1_NAME, self.VALIDATOR2_NAME]
        )
        self.assertEqual(len(validators), 2)

        allSuites = validationRegistry.GetOrLoadAllValidatorSuites()
        self.assertTrue(allSuites)

        validatorSuite = validationRegistry.GetOrLoadValidatorSuiteByName(
            self.VALIDATOR_SUITE_NAME
        )
        self.assertTrue(validatorSuite)
        self.assertIn(validatorSuite, allSuites)

        validatorSuites = validationRegistry.GetOrLoadValidatorSuitesByName(
            [self.VALIDATOR_SUITE_NAME]
        )
        self.assertTrue(validatorSuites)
        for suite in validatorSuites:
            self.assertIn(suite, allSuites)

    def test_QueryMetadata(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        invalidMetadata = validationRegistry.GetValidatorMetadata(
            "invalid_validator"
        )
        self.assertFalse(invalidMetadata)

        metadata: UsdValidation.ValidatorMetadata = (
            validationRegistry.GetValidatorMetadata(self.VALIDATOR1_NAME)
        )
        expectedKeywords = ["IncludedInAll", "SomeKeyword1"]
        self.assertEqual(metadata.GetKeywords(), expectedKeywords)

        allMetadatas = validationRegistry.GetAllValidatorMetadata()
        self.assertTrue(allMetadatas)

        metadatas = validationRegistry.GetValidatorMetadataForPlugin(
            "InvalidPlugin"
        )
        self.assertFalse(metadatas)

        metadatas = validationRegistry.GetValidatorMetadataForKeyword(
            "InvalidKeyword"
        )
        self.assertFalse(metadatas)
        metadatas = validationRegistry.GetValidatorMetadataForKeyword(
            "SomeKeyword1"
        )
        self.assertTrue(metadatas)
        metadatas = validationRegistry.GetValidatorMetadataForKeywords(
            expectedKeywords
        )
        self.assertTrue(metadatas)

        metadatas = validationRegistry.GetValidatorMetadataForSchemaType(
            "InvalidSchemaType"
        )
        self.assertFalse(metadatas)
        metadatas = validationRegistry.GetValidatorMetadataForSchemaType(
            "SomePrimType"
        )
        self.assertTrue(metadatas)
        metadatas = validationRegistry.GetValidatorMetadataForSchemaTypes(
            ["SomePrimType"]
        )
        self.assertTrue(metadatas)

    # We put tests for UsdValidation.Validator and UsdValidation.ValidatorSuite 
    # here as we cannot construct validator directly but only through registry.
    def test_ValidatorAndSuite(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            self.VALIDATOR1_NAME
        )
        self.assertTrue(validator)
        self.assertEqual(
            validator.GetMetadata().GetKeywords(),
            ["IncludedInAll", "SomeKeyword1"],
        )

        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim("/test")

        errors = validator.Validate(prim)
        self.assertFalse(errors)

        errors = validator.Validate(stage.GetRootLayer())
        self.assertFalse(errors)

        errors = validator.Validate(stage)
        self.assertTrue(errors)

        validatorSuite = validationRegistry.GetOrLoadValidatorSuiteByName(
            self.VALIDATOR_SUITE_NAME
        )
        self.assertEqual(
            validatorSuite.GetMetadata().GetKeywords(),
            ["IncludedInAll", "SuiteValidator"],
        )

        validators = validatorSuite.GetContainedValidators()
        self.assertEqual(len(validators), 2)
        self.assertIn(validator, validators)


if __name__ == "__main__":
    unittest.main()
