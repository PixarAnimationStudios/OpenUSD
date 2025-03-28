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

    def setUp(self):
        # Create a temporary directory containing a JSON test file, along with
        # a binary file:
        self._tempDir = tempfile.TemporaryDirectory()
        self._jsonFilePath = os.path.join(self._tempDir.name, 'text.json')
        self._binaryFilePath = os.path.join(self._tempDir.name, 'binary.bin')
        self._emptyFilePath = os.path.join(self._tempDir.name, 'empty.txt')

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
        self._jsonData = {
            'name': 'example',
            'value': 1234,
            'utf-8': 'Hello, 世界! 🌍',
            'child-object': {
                'key': 'value',
            },
        }
        with open(self._jsonFilePath, 'w') as jsonFile:
            json.dump(self._jsonData, jsonFile)

        # Write a file with no content:
        open(self._emptyFilePath, 'w').close()

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
        self._binaryData = b'\xc3\xc7\xe9\x82A0' 
        with open(self._binaryFilePath, 'wb') as binaryFile:
            binaryFile.write(self._binaryData)

    def tearDown(self):
        # Cleanup the temporary directory:
        self._tempDir.cleanup()

    def _getResolvedTextFilepath(self):
        """
        Return the resolved path of the Attribute referencing the JSON file,
        located in the in-memory test Stage.
        """
        return Ar.ResolvedPath(self._jsonFilePath)
    
    def _getResolvedEmptyFilepath(self):
        """
        Return the resolved path of the Attribute referencing an empty file.
        """
        return Ar.ResolvedPath(self._emptyFilePath)
    
    def _getNonExistingTextFilepath(self):
        """Return the resolved path of an non-existing asset."""
        non_existing_json_file = os.path.join(self._tempDir.name, 
                                              'non-existing-asset.json')
        return Ar.ResolvedPath(non_existing_json_file)

    def _getResolvedBinaryFilepath(self):
        """
        Return the resolved path of the Attribute referencing the JSON file,
        located in the in-memory test Stage.
        """
        return Ar.ResolvedPath(self._binaryFilePath)
    
    def test_readingNonExistingAssetResultsInInvalidAsset(self):
        """
        Validate that attempting to read a non-existing asset returns None
        """
        print("test_readingNonExistingAssetResultsInInvalidAsset")
        nonExistingJsonFile = self._getNonExistingTextFilepath()
        self.assertIsNone(
            Ar.GetResolver().OpenAsset(resolvedPath=nonExistingJsonFile))

    def test_arAssetHasExpectedContentSize(self):
        """
        Validate that the referenced `pxr.Ar.Asset` has a size comparable
        with the the content of the sample JSON data serialized to the
        temporary directory.
        """
        jsonFile = self._getResolvedTextFilepath()
        expectedJsonContentSize = os.path.getsize(str(jsonFile))
        with Ar.GetResolver().OpenAsset(resolvedPath=jsonFile) as jsonAsset:
            self.assertEqual(jsonAsset.GetSize(), expectedJsonContentSize)

    def test_arAssetBufferRead(self):
        """
        Validate that the referenced `pxr.Ar.Asset` content matches the
        actual JSON file in the temporary test directory, by reading it into a
        valid buffer.
        """
        jsonFile = self._getResolvedTextFilepath()
        with Ar.GetResolver().OpenAsset(resolvedPath=jsonFile) as jsonAsset:
            assetSize = jsonAsset.GetSize()

            buffer = jsonAsset.Read(assetSize, 0)

            self.assertEqual(buffer.decode(), json.dumps(self._jsonData))
            self.assertEqual(len(buffer), assetSize)

    def test_ArAssetPreventReadingBeyondSourceAsset(self):
        """
        Validate that if a large amount of data is requested, the resulting
        buffer will only return the amount of data actually available in the 
        asset.
        """

        jsonFile = self._getResolvedTextFilepath()
        jsonContentSize = os.path.getsize(str(jsonFile))
        offset = 10

        with Ar.GetResolver().OpenAsset(resolvedPath=jsonFile) as jsonAsset:
            buffer = jsonAsset.Read(999999, offset)
            self.assertEqual(len(buffer), jsonContentSize - offset)

    def test_ArAssetInvalidReadOffsetThrowsError(self):
        """
        Validate that reading with an invalid offset will throw an error.
        """

        jsonFile = self._getResolvedTextFilepath()
        with Ar.GetResolver().OpenAsset(resolvedPath=jsonFile) as jsonAsset:
             with self.assertRaisesRegex(ValueError, 'Invalid read offset'):
                 jsonAsset.Read(10, 100000)


    def test_arAssetReadWithSpecifiedLength(self):
        """
        Validate that the referenced `pxr.Ar.Asset` content matches the
        actual JSON file in the temporary test directory, by reading it into a
        valid buffer and reading it up to a given length.
        """
        jsonFile = self._getResolvedTextFilepath()
        with Ar.GetResolver().OpenAsset(resolvedPath=jsonFile) as jsonAsset:
            assetSize = jsonAsset.GetSize()

            # Read the first half of the file content:
            bufferSize = assetSize // 2
            buffer = jsonAsset.Read(bufferSize, 0)

            self.assertEqual(buffer.decode(),
                            json.dumps(self._jsonData)[:bufferSize])
            self.assertEqual(len(buffer), bufferSize)

            # Read the second half of the file content:
            bufferSize = assetSize - bufferSize
            buffer = jsonAsset.Read(bufferSize, assetSize // 2)

            self.assertEqual(buffer.decode(),
                            json.dumps(self._jsonData)[bufferSize:])
            self.assertEqual(len(buffer), bufferSize)

    def test_readingAssetWithNoContentReturnsValidContext(self):
        """
        Validate that attempting to read a file with no content returns a valid
        operation.
        """
        emptyFile = self._getResolvedEmptyFilepath()
        with Ar.GetResolver().OpenAsset(resolvedPath=emptyFile) as emptyAsset:
            self.assertTrue(emptyAsset, 'Expected asset to be valid')

    def test_arAssetBufferCanBeAccessedForTextContent(self):
        """
        Validate that the referenced asset content matches the actual JSON
        file in the temporary test directory.
        """
        jsonFile = self._getResolvedTextFilepath()
        with Ar.GetResolver().OpenAsset(resolvedPath=jsonFile) as jsonAsset:
            self.assertTrue(jsonAsset, 'Expected asset to be valid')
            jsonContent = jsonAsset.GetBuffer()

        self.assertEqual(jsonContent.decode(), json.dumps(self._jsonData))
        self.assertIn(b'Hello, \\u4e16\\u754c! \\ud83c\\udf0d', jsonContent)

    def test_arAssetBufferCanBeAccessedForBinaryContent(self):
        """
        Validate that the referenced asset content matches the actual binary
        file in the temporary test directory.
        """
        binaryFile = self._getResolvedBinaryFilepath()
        with Ar.GetResolver().OpenAsset(resolvedPath=binaryFile) as binaryAsset:
            self.assertTrue(binaryAsset, 'Expected asset to be valid')
            binaryContent = binaryAsset.GetBuffer()

            self.assertEqual(binaryContent, self._binaryData)

    def test_arAssetContextManagerReleasesResourceUponExiting(self):
        """
        Validate that asset resources are freed upon exiting their initial
        context manager scope.
        """
        jsonFile = self._getResolvedTextFilepath()
        asset = Ar.GetResolver().OpenAsset(resolvedPath=jsonFile)

        # Ensure the asset is deemed valid when consuming its resources within
        # an initial scoped context:
        with asset:
            self.assertTrue(
                asset,
                'Expected asset to be valid before release of context manager')

        # Ensure the asset is deemed invalid after exiting its initial scoped
        # context:
        with self.assertRaisesRegex(RuntimeError, 
                                    'Unable to access invalid asset'):
            with asset:
                pass


if __name__ == '__main__':
    unittest.main()
