#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from pxr import Ar, Sdf, UsdUtils
import os, shutil
import unittest

def _RemoveExistingDir(dir):
    if os.path.exists(dir):
        print("Removing existing localization directory: {}".format(dir))
        shutil.rmtree(dir)

class TestUsdUtilsAssetLocalization(unittest.TestCase):
    def test_RemappedReferencePathsAppearInLocalizedLayers(self):
        localizationDir = "remapRef_localized"

        _RemoveExistingDir(localizationDir)
        self.assertTrue(UsdUtils.LocalizeAsset("remapRef/root.usda",  
                                      localizationDir, editLayersInPlace=True))
        
        localizedSubPath = os.path.join(localizationDir, "0", "sub.usda")
        sub = Sdf.Layer.FindOrOpen(localizedSubPath)
        self.assertIsNotNone(sub)

        world = sub.GetPrimAtPath("/World")
        self.assertIsNotNone(world)

        self.assertListEqual(
            list(world.referenceList.GetAddedOrExplicitItems()),
            [Sdf.Reference('1/ref.usda')])
        
    def test_RemappedReferencePathsAppearInUsdzPackages(self):
        assetPath = "remapRef/root.usda"
        archivePath = "remapRef.usdz"

        context = Ar.GetResolver().CreateDefaultContextForAsset(assetPath)
        with Ar.ResolverContextBinder(context):
            self.assertTrue(UsdUtils.CreateNewUsdzPackage(
                assetPath, archivePath, editLayersInPlace=True))
        
        subPath = Ar.JoinPackageRelativePath([archivePath, "0/sub.usda"])
        sub = Sdf.Layer.FindOrOpen(subPath)
        self.assertIsNotNone(sub)

        world = sub.GetPrimAtPath("/World")
        self.assertIsNotNone(world)

        self.assertListEqual(
            list(world.referenceList.GetAddedOrExplicitItems()),
            [Sdf.Reference('1/ref.usda')])

if __name__=="__main__":
    unittest.main()
