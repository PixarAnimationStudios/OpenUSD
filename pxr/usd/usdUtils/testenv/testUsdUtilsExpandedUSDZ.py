#
# Copyright 2020 Pixar
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
#


from pxr import UsdUtils
from pxr.UsdUtils import ExpandedUSDZ


import unittest
import os
import zipfile
import tempfile
import shutil

TEST_ASSETS_ROOT = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), os.path.splitext(__file__)[0]
)


class TestExpandedUSDZ(unittest.TestCase):
    def test_FilesAreEqual(self):
        """Test that with no modifications, that the resulting
        usdz file has the same files as the source file
        """
        test_asset = os.path.join(TEST_ASSETS_ROOT, "simple.usdz")
        with zipfile.ZipFile(test_asset) as zh:
            source_namelist = zh.namelist()

        with tempfile.NamedTemporaryFile(suffix=".usdz") as tmp:
            with ExpandedUSDZ(test_asset, tmp.name):
                pass

            with zipfile.ZipFile(tmp.name) as zh:
                dest_namelist = zh.namelist()

        # Note that we don't expect ordering to be preserved outside of the
        # of the first element
        self.assertTrue(
            source_namelist[0] == dest_namelist[0],
            "The first file in the resulting usdz is different",
        )

        self.assertTrue(
            all([s in dest_namelist for s in source_namelist]),
            "The resulting usdz doesn't have the same files as the source",
        )

    def test_ChangeDefaultLayerExternal(self):
        """Tests if the context prevents setting an external default layer"""

        test_usdz = os.path.join(TEST_ASSETS_ROOT, "simple.usdz")
        new_default_layer = os.path.join(TEST_ASSETS_ROOT, "simple.usda")

        with ExpandedUSDZ(test_usdz) as usdz:
            with self.assertRaises(ValueError):
                usdz.defaultLayer = new_default_layer

    def test_ChangeDefaultLayerInternal(self):
        """Test that we can modify the """

        test_usdz = os.path.join(TEST_ASSETS_ROOT, "simple.usdz")
        new_default_layer = os.path.join(TEST_ASSETS_ROOT, "simple.usda")
        with tempfile.NamedTemporaryFile(suffix=".usdz") as tmp:
            with ExpandedUSDZ(test_usdz, tmp.name) as usdz:
                new_path = os.path.join(usdz.path, "simple.usda")
                shutil.copy(new_default_layer, new_path)
                usdz.defaultLayer = new_path

            with zipfile.ZipFile(tmp.name) as zh:
                dest_namelist = zh.namelist()
                self.assertTrue(dest_namelist[0] == "simple.usda")


if __name__ == "__main__":
    unittest.main()
