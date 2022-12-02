#!/pxrpythonsubst
#
# Copyright 2022 Pixar
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

import unittest
import tempfile

from pxr import Usd, Tf

class TestLoadUtf8Content(unittest.TestCase):
    def test_LoadUTF8TextFile(self):
        """Tests whether UTF-8 content can be appropriately loaded from a usda file."""
        ################################################################################
        if Tf.GetEnvSetting('TF_UTF8_IDENTIFIERS'):
            # loading the content should succeed under this test
            stage = Usd.Stage.Open("utf8_content.usda")
            
            # query some paths to make sure we got the content we expect
            assert stage.GetPrimAtPath('/11_Süßigkeiten')
            assert stage.GetPrimAtPath('/11_Süßigkeiten/573')
            self.assertEqual(stage.GetPrimAtPath('/11_Süßigkeiten').GetCustomData(), {'存在する': 7})
        else:
            # loading the content should fail under this test
            with self.assertRaises(Tf.ErrorException):
                stage = Usd.Stage.Open("utf8_content.usda")

    def test_LoadUTF8BinaryFile(self):
        """Tests whether UTF-8 content can be appropriately loaded from a usdc file."""
        ################################################################################
        if Tf.GetEnvSetting('TF_UTF8_IDENTIFIERS'):
            # loading the content should succeed under this test
            stage = Usd.Stage.Open("utf8_content.usda")

            # export the content to binary
            with tempfile.NamedTemporaryFile(suffix='.usdc') as f:
                f.close()
                stage.Export(f.name)
                del stage

                # reopen the binary content and ensure that the string 
                # representations have content - note they won't be the same
                # because the Export added an additional Generated from line
                text_stage = Usd.Stage.Open("utf8_content.usda")
                binary_stage = Usd.Stage.Open(f.name)

                text_layer_as_string = text_stage.ExportToString()
                self.assertTrue(len(text_layer_as_string) > 0)

                binary_layer_as_string = binary_stage.ExportToString()
                self.assertTrue(len(binary_layer_as_string) > 0)
               
                del binary_stage

if __name__ == "__main__":
    unittest.main()