#!/pxrpythonsubst
#
# Copyright 2023 Pixar
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
from pxr import Ar, Sdf, Usd, UsdUtils
import os
import unittest

class TestUsdUtilsUsdzUtils(unittest.TestCase):
    def test_RelativePaths(self):
        """Test creating a .usdz file with relative asset paths"""
        
        # root.usda references @subdir/sub.usda@</Sub>, which
        # has an asset-valued attribute pointing to "../texture.jpg".
        # Create a .usdz file from this layer and verify that asset
        # is resolvable in that .usdz.
        self.assertTrue(
            UsdUtils.CreateNewUsdzPackage(
                assetPath="relativePaths/root.usda",
                usdzFilePath="relative_paths_1.usdz"))

        stage = Usd.Stage.Open('relative_paths_1.usdz')
        val = stage.GetAttributeAtPath('/Root.file').Get()
        expectedPackagePath = Ar.JoinPackageRelativePath(
            'relative_paths_1.usdz', 'texture.jpg')

        self.assertTrue(
            val.resolvedPath.endswith(expectedPackagePath),
            "'{}' does not contain expected packaged path '{}'"
            .format(val.resolvedPath, expectedPackagePath))

        # subdir/sub.usda has an asset-valued attribute pointing to 
        # "../texture.jpg". Create a .usdz file from this layer and
        # verify that asset is resolvable in that .usdz.
        self.assertTrue(
            UsdUtils.CreateNewUsdzPackage(
                assetPath="relativePaths/subdir/sub.usda",
                usdzFilePath="relative_paths_2.usdz"))

        stage = Usd.Stage.Open('relative_paths_2.usdz')
        val = stage.GetAttributeAtPath('/Sub.file').Get()
        expectedPackagePath = Ar.JoinPackageRelativePath(
            'relative_paths_2.usdz', '0/texture.jpg')

        self.assertTrue(
            val.resolvedPath.endswith(expectedPackagePath),
            "'{}' does not contain expected packaged path '{}'"
            .format(val.resolvedPath, expectedPackagePath))
        
    def test_UsdzAssetIteratorWorkingDirectory(self):
        """Ensures the working directory is correctly reset after iteration"""

        expectedWorkingDir = os.getcwd()
        with UsdUtils.UsdzAssetIterator("usdzAssetIterator/test.usdz", False) as usdAssetItr:
            for _ in usdAssetItr.UsdAssets():
                pass

        actualWorkingDir = os.getcwd()

        self.assertEqual(expectedWorkingDir, actualWorkingDir)

if __name__=="__main__":
    unittest.main()
