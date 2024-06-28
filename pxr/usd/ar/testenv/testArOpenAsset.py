#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
import json
import os
import tempfile
import unittest

from pxr import Ar

class TestArOpenAsset(unittest.TestCase):

    def setUp(self) -> None:
        # Create a temporary directory containing a JSON test file, along with
        # a binary file:
        self._temp_dir = tempfile.TemporaryDirectory()
        self._json_file_path = os.path.join(self._temp_dir.name, 'text.json')
        self._empty_file_path = os.path.join(self._temp_dir.name, 'empty.txt')
        self._binary_file_path = os.path.join(self._temp_dir.name, 'binary.bin')

        # Write some sample JSON data to the test file.
        #
        # NOTE: The included UTF-8 string is represented by the following byte
        # sequence:
        #    * 'H', 'e', 'l', 'l', 'o', ',', and '!' are all 1-byte characters
        #      (ASCII).
        #    * '世' and '界' are 3-byte characters (East Asian characters).
        #    * '🌍' is a 4-byte character (emoji representing "Earth Globe
        #      Europe-Africa").
        #
        # The overall byte sequence for this UTF-8 string is: 
        #    * "Hello, \\u4e16\\u754c! \\ud83c\\udf0d"
        self._json_data = {
            'name': 'example',
            'value': 1234,
            'utf-8': 'Hello, 世界! 🌍',
            'child-object': {
                'key': 'value',
            },
        }
        with open(self._json_file_path, 'w') as json_file:
            json.dump(self._json_data, json_file)

        # Write a file with no content:
        open(self._empty_file_path, 'w').close()

        # Write some sample binary data to the test file, including characters
        # that would result in UTF-8 decoding errors such as:
        #    > UnicodeDecodeError: 'utf-8' codec can't decode byte 0xc3 in
        #    > position 0: invalid continuation byte
        #
        # Equivalent string of the following byte representation: 'ÃÇéあ',
        # where:
        #    * 'Ã' represents an 'A' with tilde in UTF-8, but can represent a
        #      different character in Latin-1.
        #    * 'Ç' represents a 'C' with cedilla in Latin-1, although would be
        #      different in UTF-8.
        #    * 'é' represents the byte 0xE9 in ISO-8859-1 (Latin-1), although
        #      in UTF-8, 0xE9 is not a valid single byte but part of a
        #      multi-byte sequence.
        #    * 'あ' is represented by the byte 0x82A0 in Shift JIS.
        self._binary_data = b'\xc3\xc7\xe9\x82A0' 
        with open(self._binary_file_path, 'wb') as binary_file:
            binary_file.write(self._binary_data)

    def tearDown(self) -> None:
        # Cleanup the temporary directory:
        self._temp_dir.cleanup()

    def _get_resolved_text_filepath(self) -> Ar.ResolvedPath:
        """
        Return the resolved path of the Attribute referencing the JSON file.
        """
        return Ar.ResolvedPath(self._json_file_path)

    def _get_resolved_empty_filepath(self) -> Ar.ResolvedPath:
        """
        Return the resolved path of the Attribute referencing an empty file.
        """
        return Ar.ResolvedPath(self._empty_file_path)

    def _get_non_existing_text_filepath(self) -> Ar.ResolvedPath:
        """Return the resolved path of an non-existing asset."""
        non_existing_json_file = os.path.join(self._temp_dir.name, 'non-existing-asset.json')
        return Ar.ResolvedPath(non_existing_json_file)

    def _get_resolved_binary_filepath(self) -> Ar.ResolvedPath:
        """
        Return the resolved path of the Attribute referencing the JSON file.
        """
        return Ar.ResolvedPath(self._binary_file_path)

    def test_reading_an_existing_asset_returns_a_valid_context(self) -> None:
        """
        Validate that reading an existing asset returns a valid context and
        does not raise an exception.
        """
        json_file = self._get_resolved_text_filepath()
        with Ar.GetResolver().OpenAsset(resolvedPath=json_file) as json_asset:
            self.assertTrue(json_asset, 'Expected asset to be valid')

    def test_reading_non_existing_asset_raises_exception(self) -> None:
        """
        Validate that attempting to read a non-existing asset raises an
        exception.
        """
        non_existing_json_file = self._get_non_existing_text_filepath()
        with self.assertRaisesRegex(ValueError, 'Failed to open asset'):
            Ar.GetResolver().OpenAsset(resolvedPath=non_existing_json_file)

    def test_reading_asset_with_no_content_returns_a_valid_context(self) -> None:
        """
        Validate that attempting to read a file with no content returns a valid
        operation.
        """
        empty_text_file = self._get_resolved_empty_filepath()
        with Ar.GetResolver().OpenAsset(resolvedPath=empty_text_file) as empty_asset:
            self.assertTrue(empty_asset, 'Expected asset to be valid')

    def test_ar_asset_buffer_can_be_accessed_for_text_content(self) -> None:
        """
        Validate that the referenced asset content matches the actual JSON
        file in the temporary test directory.
        """
        json_file = self._get_resolved_text_filepath()
        with Ar.GetResolver().OpenAsset(resolvedPath=json_file) as json_asset:
            self.assertTrue(json_asset, 'Expected asset to be valid')

            json_content = json_asset.GetBuffer()

        self.assertEqual(json_content.decode(), json.dumps(self._json_data))
        self.assertIn(b'Hello, \\u4e16\\u754c! \\ud83c\\udf0d', json_content)

    def test_ar_asset_buffer_can_be_accessed_for_binary_content(self) -> None:
        """
        Validate that the referenced asset content matches the actual binary
        file in the temporary test directory.
        """
        binary_file = self._get_resolved_binary_filepath()
        with Ar.GetResolver().OpenAsset(resolvedPath=binary_file) as binary_asset:
            self.assertTrue(binary_asset, 'Expected asset to be valid')

            binary_content = binary_asset.GetBuffer()

            self.assertEqual(binary_content, self._binary_data)

    def test_ar_asset_context_manager_releases_resource_upon_exiting(self) -> None:
        """
        Validate that asset resources are freed upon exiting their initial
        context manager scope.
        """
        json_file = self._get_resolved_text_filepath()
        asset = Ar.GetResolver().OpenAsset(resolvedPath=json_file)

        # Ensure the asset is deemed valid when consuming its resources within
        # an initial scoped context:
        with asset:
            self.assertTrue(
                asset,
                'Expected asset to be valid before release of context manager scope')

        # Ensure the asset is deemed invalid after exiting its initial scoped
        # context:
        with self.assertRaisesRegex(ValueError, 'Failed to open asset'):
            with asset:
                pass


if __name__ == '__main__':
    unittest.main()
