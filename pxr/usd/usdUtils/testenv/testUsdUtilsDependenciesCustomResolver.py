#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from pxr import Ar, Plug, Sdf, Usd, UsdUtils

import os, shutil, unittest

def _ResolvedPath(relativePath):
    return "testresolved:" + os.path.abspath(relativePath)

class TestUsdUtilsDependenciesCustomResolver(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Register test resolver plugins
        # Test plugins are installed relative to this script
        testRoot = os.path.join(
            os.path.dirname(os.path.abspath(__file__)), 'UsdUtilsPlugins')

        pr = Plug.Registry()

        testURIResolverPath = os.path.join(
            testRoot, 'lib/TestUsdUtilsDependenciesCustomResolver*/Resources/')
        pr.RegisterPlugins(testURIResolverPath)

    def test_ComputeAllDependencies(self):
        """Tests that ComputeAllDependencies correctly sets identifier and """
        """resolved paths when using a resolver with multiple uri schemes """

        mainIdentifier = "test:basic/main.usda"
        expectedMainResolvedPath = "testresolved:basic/main.usda"
        dependencyIdentifier = "test:basic/dependency.usda"
        expectedDependencyResolvedPath = _ResolvedPath("basic/dependency.usda")

        layers, _, _ = UsdUtils.ComputeAllDependencies(mainIdentifier)

        self.assertEqual(len(layers), 2)
        layer0 = layers[0]
        self.assertEqual(layer0.identifier, mainIdentifier)
        self.assertEqual(layer0.resolvedPath, expectedMainResolvedPath)

        layer1 = layers[1]
        self.assertEqual(layer1.identifier, dependencyIdentifier)
        self.assertPathsEqual(layer1.resolvedPath, expectedDependencyResolvedPath)

    def test_FindOrOpenLayer(self):
        mainIdentifier = "test:basic/main.usda"
        expectedMainResolvedPath = _ResolvedPath("basic/main.usda")
        dependencyIdentifier = "test:basic/dependency.usda"
        expectedDependencyResolvedPath =_ResolvedPath("basic/dependency.usda")

        UsdUtils.ComputeAllDependencies(mainIdentifier)

        layer = Sdf.Layer.FindOrOpen(mainIdentifier)
        self.assertEqual(layer.identifier, mainIdentifier)
        self.assertPathsEqual(layer.resolvedPath, expectedMainResolvedPath)

        layer = Sdf.Layer.FindOrOpen(dependencyIdentifier)
        self.assertEqual(layer.identifier, dependencyIdentifier)
        self.assertPathsEqual(layer.resolvedPath, expectedDependencyResolvedPath)
        
    def test_LocalizeAsset(self):
        """Tests that asset localization works on assets that do not 
        use filesystem paths"""

        assetPathStr = "test:localize/root.usd"
        localizationDir = "root_localized"
        localizedAsset = os.path.join(localizationDir, "root.usd")

        if os.path.exists(localizationDir):
            shutil.rmtree(localizationDir)
        os.mkdir(localizationDir)

        self.assertTrue(UsdUtils.LocalizeAsset(assetPathStr, localizationDir))

        # Validate that the localized file can be opened on a stage.
        stage = Usd.Stage.Open(localizedAsset)
        self.assertIsNotNone(stage)

        localizedFiles = self._GetFileList(localizationDir)

        expectedFiles = ['0/asset.txt', '0/clip1.usda', '0/clip2.usda', 
                         '0/ref_a.usd', '0/ref_b.usd', 'root.usd']
        
        self.assertEqual(localizedFiles, expectedFiles)

    def _GetFileList(self, localizedAssetDir):
        rootFolderPathStr = localizedAssetDir + os.sep
        contents = []
        for path, directories, files in os.walk(localizedAssetDir):
            for file in files:
                localizedPath = os.path.join(path,file)
                localizedPath = localizedPath.replace(rootFolderPathStr, '')
                contents.append(localizedPath.replace('\\', '/'))

        contents.sort()
        return contents

    def test_ComputeAllDependencies(self):
        """Tests that ComputeAllDependencies works with the custom resolver"""
        assetPathStr = "test:localize/root.usd"

        layers, assets, unresolved = \
            UsdUtils.ComputeAllDependencies(assetPathStr)
        
        self.assertEqual(layers, [Sdf.Layer.Find(l) for l in [
            "test:localize/root.usd", 
            "test:localize/clip1.usda", 
            "test:localize/clip2.usda", 
            "test:localize/ref_a.usd", 
            "test:localize/ref_b.usd"]])

        self.assertEqual(len(assets), 1)
        self.assertPathsEqual(assets[0], _ResolvedPath("localize/asset.txt"))

        self.assertEqual(unresolved, [])

    def assertPathsEqual(self, path1, path2):
        # Flip backslashes to forward slashes and make sure path case doesn't
        # cause test failures to accommodate platform differences. We don't use
        # os.path.normpath since that might fix up other differences we'd want
        # to catch in these tests.
        self.assertEqual(
            os.path.normcase(str(path1)), os.path.normcase(str(path2)))


if __name__=="__main__":
    unittest.main()
