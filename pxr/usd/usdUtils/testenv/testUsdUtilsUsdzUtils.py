#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
