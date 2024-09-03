#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Plug, Sdf, Usd

class TestUsdValidatorMetadata(unittest.TestCase):
    def _VerifyMetadata(
        self,
        metadata: Usd.ValidatorMetadata,
        name="",
        doc="",
        keywords=[],
        schemaTypes=[],
        plugin=None,
        isSuite=False
    ):
        self.assertEqual(metadata.name, name)
        self.assertEqual(metadata.doc, doc)
        self.assertEqual(metadata.keywords, keywords)
        self.assertEqual(metadata.schemaTypes, schemaTypes)
        self.assertEqual(metadata.plugin, plugin)
        self.assertEqual(metadata.isSuite, isSuite)

    def test_CreateDefaultMetadata(self):
        metadata = Usd.ValidatorMetadata()
        self._VerifyMetadata(metadata)

    def test_CreateMetadataWithValidKeywordArgs(self):
        all_plugins = Plug.Registry().GetAllPlugins()
        expected_plugin = all_plugins[0] if all_plugins else None
        valid_metadatas = [
            {
                "name": "empty_validator"
            },
            {
                "name": "validator1",
                "doc": "This is a test validator.",
                "keywords": ["validator1", "test"],
                "schemaTypes": ["SomePrimType"],
                "plugin": None,
                "isSuite": False
            },
            {
                "name": "validator2",
                "doc": "This is another test validator.",
                "keywords": ["validator2", "test"],
                "schemaTypes": ["NewPrimType"],
                "plugin": expected_plugin,
                "isSuite": False
            }
        ]

        for args in valid_metadatas:
            with self.subTest(name=args["name"]):
                metadata = Usd.ValidatorMetadata(**args)
                self._VerifyMetadata(metadata, **args)

    def test_CreateMetadataWithInvalidKeywordArgs(self):
        invalid_metadatas = {
            "Wrong Name Type": {
                "name": 123
            },
            "Wrong Doc Type": {
                "doc": 123
            },
            "Wrong Keywords Type": {
                "keywords": 123
            },
            "Wrong Schema Types": {
                "schemaTypes": 123
            },
            "Wrong Plugin Type": {
                "plugin": 123
            },
            "Wrong IsSuite Type": {
                "isSuite": "wrong type"
            }
        }

        for error_category, args in invalid_metadatas.items():
            with self.subTest(error_type=error_category):
                with self.assertRaises(Exception):
                    Usd.ValidatorMetadata(**args)

    def test_MetadataNameImmutable(self):
        metadata = Usd.ValidatorMetadata()
        with self.assertRaises(Exception):
            metadata.name = "test"

    def test_MetadataDocImmutable(self):
        metadata = Usd.ValidatorMetadata()
        with self.assertRaises(Exception):
            metadata.doc = "doc"

    def test_MetadataKeywordsImmutable(self):
        metadata = Usd.ValidatorMetadata()
        with self.assertRaises(Exception):
            metadata.keywords = ["keywords"]

    def test_MetadataSchemaTypesImmutable(self):
        metadata = Usd.ValidatorMetadata()
        with self.assertRaises(Exception):
            metadata.schemaTypes = "PrimType1"

    def test_MetadataPluginImmutable(self):
        all_plugins = Plug.Registry().GetAllPlugins()
        expected_plugin = all_plugins[0] if all_plugins else None
        metadata = Usd.ValidatorMetadata()
        with self.assertRaises(Exception):
            metadata.plugin = expected_plugin

    def test_MetadataIsSuiteImmutable(self):
        metadata = Usd.ValidatorMetadata()
        with self.assertRaises(Exception):
            metadata.isSuite = True


if __name__ == "__main__":
    unittest.main()
