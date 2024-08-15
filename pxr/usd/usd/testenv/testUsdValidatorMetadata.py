#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Plug, Sdf, Usd


class TestUsdValidatorMetadata(unittest.TestCase):
    def _verify_metadata(
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

    def test_create_default_metadata(self):
        metadata = Usd.ValidatorMetadata()
        self._verify_metadata(metadata)

    def test_create_metadata_with_valid_keyword_args(self):
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
                self._verify_metadata(metadata, **args)

    def test_create_metadata_with_invalid_keyword_args(self):
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

    def test_metadata_name_getter_and_setter(self):
        metadata = Usd.ValidatorMetadata()
        for name in ["validator1", "validator2"]:
            metadata.name = name
            self.assertEqual(metadata.name, name)

        # Invalid type
        with self.assertRaises(Exception):
            metadata.name = 123

    def test_metadata_doc_getter_and_setter(self):
        metadata = Usd.ValidatorMetadata()
        for doc in ["doc1", "doc2"]:
            metadata.doc = doc
            self.assertEqual(metadata.doc, doc)

        # Invalid type
        with self.assertRaises(Exception):
            metadata.doc = 123

    def test_metadata_keywords_getter_and_setter(self):
        metadata = Usd.ValidatorMetadata()
        for keywords in [["keyword1"], ["keyword2"]]:
            metadata.keywords = keywords
            self.assertEqual(metadata.keywords, keywords)

        # Invalid type
        with self.assertRaises(Exception):
            metadata.keywords = 123
        with self.assertRaises(Exception):
            metadata.keywords = "123"

    def test_metadata_schemaTypes_getter_and_setter(self):
        metadata = Usd.ValidatorMetadata()
        for schema_types in [["PrimType1"], ["PrimType2"]]:
            metadata.schemaTypes = schema_types
            self.assertEqual(metadata.schemaTypes, schema_types)

        # Invalid type
        with self.assertRaises(Exception):
            metadata.keywords = 123
        with self.assertRaises(Exception):
            metadata.keywords = "123"

    def test_metadata_plugin_getter_and_setter(self):
        all_plugins = Plug.Registry().GetAllPlugins()
        expected_plugin = all_plugins[0] if all_plugins else None
        metadata = Usd.ValidatorMetadata()
        metadata.plugin = expected_plugin
        self.assertEqual(metadata.plugin, expected_plugin)

        # Invalid type
        with self.assertRaises(Exception):
            metadata.keywords = 123

    def test_metadata_is_suite_getter_and_setter(self):
        metadata = Usd.ValidatorMetadata()
        for suite in [True, False]:
            metadata.isSuite = suite
            self.assertEqual(metadata.isSuite, suite)

        # Invalid type
        with self.assertRaises(Exception):
            metadata.keywords = "123"


if __name__ == "__main__":
    unittest.main()
