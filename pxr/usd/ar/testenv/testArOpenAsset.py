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
        Return the resolved path of the Attribute referencing the JSON file,
        located in the in-memory test Stage.
        """
        return Ar.ResolvedPath(self._json_file_path)

    def _get_resolved_binary_filepath(self) -> Ar.ResolvedPath:
        """
        Return the resolved path of the Attribute referencing the JSON file,
        located in the in-memory test Stage.
        """
        return Ar.ResolvedPath(self._binary_file_path)

    def test_ar_asset_can_be_opened_from_resolver(self) -> None:
        """
        Validate that the test attribute referencing a JSON file has the
        interface of a `pxr.Ar.Asset`.
        """
        json_file = self._get_resolved_text_filepath()
        json_asset = Ar.GetResolver().OpenAsset(resolvedPath=json_file)

        self.assertIsInstance(json_asset, Ar.Asset)

    def test_ar_asset_has_expected_content_size(self) -> None:
        """
        Validate that the referenced `pxr.Ar.Asset` has a size comparable
        with the the content of the sample JSON data serialized to the
        temporary directory.
        """
        json_file = self._get_resolved_text_filepath()
        json_asset = Ar.GetResolver().OpenAsset(resolvedPath=json_file)

        expected_json_content_size = len(json.dumps(self._json_data))
        self.assertEqual(json_asset.GetSize(), expected_json_content_size)

    def test_ar_asset_buffer_can_be_read_to_a_valid_buffer(self) -> None:
        """
        Validate that the referenced `pxr.Ar.Asset` content matches the
        actual JSON file in the temporary test directory, by reading it into a
        valid buffer.
        """
        json_file = self._get_resolved_text_filepath()
        json_asset = Ar.GetResolver().OpenAsset(resolvedPath=json_file)
        asset_size = json_asset.GetSize()

        buffer = bytearray(asset_size)
        size_read = json_asset.Read(buffer, asset_size, 0)

        self.assertEqual(buffer.decode(), json.dumps(self._json_data))
        self.assertEqual(size_read, asset_size)

    def test_ar_asset_buffer_can_be_read_to_a_valid_buffer_of_the_given_length(self) -> None:
        """
        Validate that the referenced `pxr.Ar.Asset` content matches the
        actual JSON file in the temporary test directory, by reading it into a
        valid buffer and reading it up to a given length.
        """
        json_file = self._get_resolved_text_filepath()
        json_asset = Ar.GetResolver().OpenAsset(resolvedPath=json_file)
        asset_size = json_asset.GetSize()

        # Read the first half of the file content:
        buffer_size = asset_size // 2
        buffer = bytearray(buffer_size)
        size_read = json_asset.Read(buffer, buffer_size, 0)

        self.assertEqual(buffer.decode(),
                         json.dumps(self._json_data)[:buffer_size])
        self.assertEqual(size_read, buffer_size)

        # Read the second half of the file content:
        buffer_size = asset_size - buffer_size
        buffer = bytearray(buffer_size)
        size_read = json_asset.Read(buffer, buffer_size, asset_size // 2)

        self.assertEqual(buffer.decode(),
                         json.dumps(self._json_data)[buffer_size:])
        self.assertEqual(size_read, buffer_size)

    def test_ar_asset_buffer_raises_an_error_when_reading_to_an_invalid_buffer(self) -> None:
        """
        Validate that an Error is raised when attempting to read the
        `pxr.Ar.Asset` into an invalid buffer.
        """
        json_file = self._get_resolved_text_filepath()
        json_asset = Ar.GetResolver().OpenAsset(resolvedPath=json_file)

        invalid_buffer = ''

        with self.assertRaises(TypeError) as e:
            json_asset.Read(invalid_buffer, json_asset.GetSize(), 0)
        self.assertEqual(str(e.exception),
                         'Object does not support buffer interface')

    def test_ar_asset_buffer_raises_an_error_if_provided_buffer_is_of_insufficient_size(self) -> None:
        """
        Validate that an Error is raised when attempting to read the
        `pxr.Ar.Asset` into a buffer of insufficient size.
        """
        json_file = self._get_resolved_text_filepath()
        json_asset = Ar.GetResolver().OpenAsset(resolvedPath=json_file)

        # Create a buffer that is 1 element short of being able to hold the
        # asset in its entirety, in order to ensure potential "off by 1" limits
        # are correctly handled:
        buffer = bytearray(json_asset.GetSize() - 1)

        with self.assertRaises(ValueError) as e:
            json_asset.Read(buffer, json_asset.GetSize(), 0)
        self.assertEqual(str(e.exception),
                         'Provided buffer is of insufficient size to hold the requested data size')


if __name__ == '__main__':
    unittest.main()
