#!/pxrpythonsubst
# -*- coding: utf-8
#
# Copyright 2017 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.

import os
import subprocess
import sys
import tempfile
import unittest

from pxr import Tf, Plug, Usd

class TestUsdSchemaGen(unittest.TestCase):
    """
    Tests that schema generation works as expected with UTF-8 characters.
    """
    def test_StandardCharacterSetUsdSchemaGen(self):
        """
        Tests that the usdgenschema process exits with no errors generating a
        standard USD schema definition with traditional character sets from
        a UTF-8 encoded schema definition.

        Note: This test should generate the same behavior regardless of whether
              TF_UTF8_IDENTIFIERS is off or on.
        """
        # don't run in Python 2.7
        if not hasattr(tempfile, 'TemporaryDirectory'):
            return

        schema_file = os.path.join("unicode", "ascii_schema.usda")
        called_process_exception = None
        with tempfile.TemporaryDirectory() as temp_dir:
            usdGenSchemaCommandLine = [
                sys.executable,
                os.path.join(os.path.dirname(__file__), "..", "bin", "usdGenSchema"),
                schema_file,
                temp_dir
            ]

            try:
                # this should not throw an error, if it does, then something
                # went wrong with schema generation
                subprocess.run(usdGenSchemaCommandLine, check=True)
            except subprocess.CalledProcessError as e:
                called_process_exception = e

        # make sure nothing went wrong with schema generation
        self.assertTrue(called_process_exception is None)

    def test_UTF8CharacterSetUsdSchemaGen(self):
        """
        Tests that the usdgenschema process exists with no errors generating a
        USD schema definition with UTF-8 characters and non traditional
        identifiers from a UTF-8 encoded schema definition.

        Note: This test should only run if the TF_UTF8_IDENTIFIERS setting is on.
        """
        # don't run in Python 2.7
        if not hasattr(tempfile, 'TemporaryDirectory'):
            return

        if Tf.GetEnvSetting('TF_UTF8_IDENTIFIERS'):
            schema_file = os.path.join("unicode", "unicode_schema.usda")
            called_process_exception = None
            with tempfile.TemporaryDirectory() as temp_dir:
                usdGenSchemaCommandLine = [
                    sys.executable,
                    os.path.join(os.path.dirname(__file__), "..", "bin", "usdGenSchema"),
                    schema_file,
                    temp_dir
                ]

                try:
                    # this should not throw an error, if it does, then something
                    # went wrong with schema generation
                    subprocess.run(usdGenSchemaCommandLine, check=True)
                except subprocess.CalledProcessError as e:
                    called_process_exception = e

            # make sure nothing went wrong with schema generation
            self.assertTrue(called_process_exception is None)
        else:
            print("Skipping test_UTF8CharacterSetUsdSchemaGen because TF_UTF8_IDENTIFIERS is not set.")

    def test_FailedUTF8CharacterSetUsdSchemaGen(self):
        """
        Tests that the usdgenschema process exists with errors when encountering
        an illegal schema definition where class / attribute identifiers do not
        follow the XID_Start XID_Continue* standard.

        Note: This test should only run if the TF_UTF8_IDENTIFIERS setting is on.
        """
        # don't run in Python 2.7
        if not hasattr(tempfile, 'TemporaryDirectory'):
            return

        if Tf.GetEnvSetting('TF_UTF8_IDENTIFIERS'):
            schema_file = os.path.join("unicode", "illegal_unicode_schema.usda")
            called_process_exception = None
            with tempfile.TemporaryDirectory() as temp_dir:
                usdGenSchemaCommandLine = [
                    sys.executable,
                    os.path.join(os.path.dirname(__file__), "..", "bin", "usdGenSchema"),
                    schema_file,
                    temp_dir
                ]

                try:
                    # this should not throw an error, if it does, then something
                    # went wrong with schema generation
                    subprocess.run(usdGenSchemaCommandLine, check=True)
                except subprocess.CalledProcessError as e:
                    called_process_exception = e

            # make sure nothing went wrong with schema generation
            self.assertTrue(called_process_exception is not None)
        else:
            print("Skipping test_UTF8CharacterSetUsdSchemaGen because TF_UTF8_IDENTIFIERS is not set.")

    def test_ASCIIEncodingUsdSchemaGen(self):
        """
        Tests that a standard ASCII encoded file containing a schema definition
        succeeds as expected when running usdGenSchema with traditional 
        character sets (e.g. backward compatibility).

        Note: This test should generate the same behavior regardless of whether
              TF_UTF8_IDENTIFIERS is off or on.
        """
        # don't run in Python 2.7
        if not hasattr(tempfile, 'TemporaryDirectory'):
            return

        schema_file = os.path.join("unicode", "ascii_encoded_schema.usda")
        called_process_exception = None
        with tempfile.TemporaryDirectory() as temp_dir:
            usdGenSchemaCommandLine = [
                sys.executable,
                os.path.join(os.path.dirname(__file__), "..", "bin", "usdGenSchema"),
                schema_file,
                temp_dir
            ]

            try:
                # this should not throw an error, if it does, then something
                # went wrong with schema generation
                subprocess.run(usdGenSchemaCommandLine, check=True)
            except subprocess.CalledProcessError as e:
                called_process_exception = e

        # make sure nothing went wrong with schema generation
        self.assertTrue(called_process_exception is None)

    def test_FailedEncodingUsdSchemaGen(self):
        """
        Tests that a standard ASCII encoded file containing a schema definition
        with non-ASCII characters fails using usdGenSchema.

        Note: This test should generate the same behavior regardless of whether
              TF_UTF8_IDENTIFIERS is off or on.
        """
        # don't run in Python 2.7
        if not hasattr(tempfile, 'TemporaryDirectory'):
            return

        schema_file = os.path.join("unicode", "unicode_ascii_encoded_schema.usda")
        called_process_exception = None
        with tempfile.TemporaryDirectory() as temp_dir:
            usdGenSchemaCommandLine = [
                sys.executable,
                os.path.join(os.path.dirname(__file__), "..", "bin", "usdGenSchema"),
                schema_file,
                temp_dir
            ]

            try:
                # this should not throw an error, if it does, then something
                # went wrong with schema generation
                subprocess.run(usdGenSchemaCommandLine, check=True)
            except subprocess.CalledProcessError as e:
                called_process_exception = e

        # make sure nothing went wrong with schema generation
        self.assertTrue(called_process_exception is not None)

    def test_UTF8CodelessSchema(self):
        """
        Tests that a codeless schema with UTF8 characters in class / attribute names
        works as expected.

        Note: This test should only run if the TF_UTF8_IDENTIFIERS setting is on.
        """
        if Tf.GetEnvSetting('TF_UTF8_IDENTIFIERS'):
            # TODO: need to figure out where this is loaded from
            pr = Plug.Registry()
            codelessSchemaPlugins = pr.RegisterPlugins(os.path.abspath("unicode"))
            self.assertTrue(len(codelessSchemaPlugins) == 1)
            self.assertTrue(codelessSchemaPlugins[0].name == "testUsdSchemaGen")

            # create a prim we can apply the schema to
            stage = Usd.Stage.CreateInMemory()
            prim = stage.DefinePrim('/試験')

            # ensure it doesn't have the codeless schema applied yet
            self.assertFalse(prim.HasAPI("TestUsdSchemaGenSansCodeSchémaAPI"))

            # apply the API
            prim.ApplyAPI("TestUsdSchemaGenSansCodeSchémaAPI")

            # ensure it now has the API
            self.assertTrue(prim.HasAPI("TestUsdSchemaGenSansCodeSchémaAPI"))

            # set a value of one of the attributes
            attr = prim.GetAttribute("verstümmeln:混淆:الشعيرة")
            self.assertEqual(attr.Get(), 46)
        else:
            print("Skipping test_UTF8CharacterSetUsdSchemaGen because TF_UTF8_IDENTIFIERS is not set.")

if __name__ == "__main__":
    unittest.main()