#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from pxr import Tf, Ar, Sdf, UsdUtils, Plug, Usd
import os, shutil
import unittest

def _RemoveExistingDir(dir):
    if os.path.exists(dir):
        print("Removing existing localization directory: {}".format(dir))
        shutil.rmtree(dir)

class TestUsdUtilsAssetLocalization(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Test plugins are installed relative to this script
        testRoot = os.path.join(
            os.path.dirname(os.path.abspath(__file__)), "UsdUtilsPlugins")

        pr = Plug.Registry()

        # Register test file format plugin
        testFileFormatPath = os.path.join(testRoot,
            "lib/TestUsdUtilsLocalizeExternalAssetDependencies*/Resources/")
        pr.RegisterPlugins(testFileFormatPath)

        # Register custom filesystem path resolver
        testCustomFilesystemResolverPath = os.path.join(
            testRoot, 'lib/TestUsdUtilsCustomFilesystemResolver*/Resources/')
        pr.RegisterPlugins(testCustomFilesystemResolverPath)

        # This is used to test custom filesystem paths paths below ('///).  This
        # particular resolver will defer to the default resolver in all other 
        # cases.
        Ar.SetPreferredResolver("CustomFilesystemResolver")

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
        
    def test_ExternalAssetsAppearInUsdzPackage(self):
        assetPath = "externalAssets/root.usda"
        archivePath = "externalAssets.usdz"
        context = Ar.GetResolver().CreateDefaultContextForAsset(assetPath)
        with Ar.ResolverContextBinder(context):
            self.assertTrue(UsdUtils.CreateNewUsdzPackage(
                assetPath, archivePath, editLayersInPlace=True))

        zf = Sdf.ZipFile.Open(archivePath)
        expectedAssets = [
            "root.usda", 
            "asset/asset",
            "asset/file1.external", 
            "asset/subdir/file2.external", 
            "asset/asset.testformat"
        ]
        self.assertEqual(expectedAssets, zf.GetFileNames())

    def test_ExternalAssetsAppearInUsdzPackageContextDep(self):
        assetPath = "externalAssets/rootContextDep.usda"
        archivePath = "externalAssetsContextDep.usdz"
        context = Ar.GetResolver().CreateDefaultContextForAsset(assetPath)
        with Ar.ResolverContextBinder(context):
            self.assertTrue(UsdUtils.CreateNewUsdzPackage(
                assetPath, archivePath, editLayersInPlace=True))

        zf = Sdf.ZipFile.Open(archivePath)
        expectedAssets = [
            "rootContextDep.usda", 
            "0/asset",
            "0/file1.external", 
            "0/subdir/file2.external", 
            "0/asset.testformat"
        ]

        self.assertEqual(expectedAssets, zf.GetFileNames())

    def test_ExternalAssetsAppearInLocalizedPackage(self):
        assetPath = "externalAssets/root.usda"
        localizationDir = "externalAssets_localized"
        _RemoveExistingDir(localizationDir)

        self.assertTrue(UsdUtils.LocalizeAsset(assetPath, localizationDir))

        expectedAssets = [
            "root.usda", 
            "asset/asset", 
            "asset/file1.external", 
            "asset/subdir/file2.external", 
            "asset/asset.testformat"
        ]

        for expectedAsset in expectedAssets:
            self.assertTrue(os.path.exists(
                os.path.join(localizationDir, expectedAsset)))
            
    def test_ExternalAssetsAppearInLocalizedPackageContextDep(self):
        assetPath = "externalAssets/rootContextDep.usda"
        localizationDir = "externalAssetsContextDep_localized"
        _RemoveExistingDir(localizationDir)

        self.assertTrue(UsdUtils.LocalizeAsset(assetPath, localizationDir))

        expectedAssets = [
            "rootContextDep.usda", 
            "0/asset", 
            "0/file1.external", 
            "0/subdir/file2.external", 
            "0/asset.testformat"
        ]

        for expectedAsset in expectedAssets:
            self.assertTrue(os.path.exists(
                os.path.join(localizationDir, expectedAsset)))

            
    def test_ExternalAssetsAboveRootTriggerWarningInUsdz(self):
        assetPath = "externalAssetOutsideRoot/root.usda"
        archivePath = "externalAssetOutsideRoot.usdz"
        context = Ar.GetResolver().CreateDefaultContextForAsset(assetPath)

        with Tf.DiagnosticTrap() as trap:
            with Ar.ResolverContextBinder(context):
                self.assertTrue(UsdUtils.CreateNewUsdzPackage(
                    assetPath, archivePath, editLayersInPlace=True))
            
            self.assertTrue(trap.HasWarnings())
            commentaryStr = "Could not determine package relative path."
            self.assertTrue(
                any(commentaryStr in s.commentary for s in trap.GetWarnings()))

        zf = Sdf.ZipFile.Open(archivePath)
        expectedAssets = [
            "root.usda", 
            "asset/file1.external", 
            "asset/asset.testformat"
        ]
        self.assertEqual(expectedAssets, zf.GetFileNames())

    def test_ExternalAssetsAboveRootTriggerWarningInLocalizedPackage(self):
        assetPath = "externalAssetOutsideRoot/root.usda"
        localizationDir = "externalAssetOutsideRoot_localized"
        _RemoveExistingDir(localizationDir)

        with Tf.DiagnosticTrap() as trap:
            self.assertTrue(UsdUtils.LocalizeAsset(assetPath, localizationDir))

            self.assertTrue(trap.HasWarnings())
            commentaryStr = "Could not determine package relative path."
            self.assertTrue(
                any(commentaryStr in s.commentary for s in trap.GetWarnings()))

        expectedAssets = [
            "root.usda", 
            "asset/file1.external",
            "asset/asset.testformat"
        ]

        for expectedAsset in expectedAssets:
            self.assertTrue(os.path.exists(
                os.path.join(localizationDir, expectedAsset)))
            
    def test_CustomFilesystemPathsInUsdz(self):
        assetPath = "customFilesystem/root.usda"
        archivePath = "customFilesystem.usdz"
        context = Ar.GetResolver().CreateDefaultContextForAsset(assetPath)

        with Ar.ResolverContextBinder(context):
            self.assertTrue(UsdUtils.CreateNewUsdzPackage(
                assetPath, archivePath))

        zf = Sdf.ZipFile.Open(archivePath)
        expectedAssets = [
            "root.usda", 
            "0/sub.usda",
        ]
        self.assertEqual(expectedAssets, zf.GetFileNames())

    def test_CustomFilesystemPathsInInLocalizedPackage(self):
        assetPath = "customFilesystem/root.usda"
        localizationDir = "customFilesystem_localized"
        _RemoveExistingDir(localizationDir)

        self.assertTrue(UsdUtils.LocalizeAsset(assetPath, localizationDir))

        expectedAssets = [
            "root.usda", 
            "0/sub.usda"
        ]

        for expectedAsset in expectedAssets:
            self.assertTrue(os.path.exists(
                os.path.join(localizationDir, expectedAsset)), expectedAsset)
            
    def test_RemappedAssetPathsDoNotTriggerWarning(self):
        assetPath = "warnings/root_remaped.usda"
        archivePath = "warnings.usdz"
        context = Ar.GetResolver().CreateDefaultContextForAsset(assetPath)

        with Ar.ResolverContextBinder(context):
            with Tf.DiagnosticTrap() as trap:
                self.assertTrue(UsdUtils.CreateNewUsdzPackage(
                    assetPath, archivePath, editLayersInPlace=True))
                
                self.assertFalse(trap.HasWarnings())


        zf = Sdf.ZipFile.Open(archivePath)
        expectedAssets = [
            "root_remaped.usda", 
            "0/sub.usda",
        ]
        self.assertEqual(expectedAssets, zf.GetFileNames())

    def test_WarningsAreGeneratedForFilesThatCouldNotBeResolved(self):
        assetPath = "warnings/root_unresolved.usda"
        archivePath = "unresolved.usdz"
        context = Ar.GetResolver().CreateDefaultContextForAsset(assetPath)

        with Ar.ResolverContextBinder(context):
            with Tf.DiagnosticTrap() as trap:
                self.assertTrue(UsdUtils.CreateNewUsdzPackage(
                    assetPath, archivePath, editLayersInPlace=True))
                
                # Note: there is one warning for failing to enqueue the
                # unresolvable asset, and one warning for failing to open
                # the asset for writing.
                self.assertEqual(len(trap.GetWarnings()), 2)

                self.assertTrue(
                    "Failed to resolve reference @unresolvable.usda@" 
                    in trap.GetWarnings()[0].commentary)
                self.assertTrue(
                    "Skipping export of dependency @/unresolvable.usda@" 
                    in trap.GetWarnings()[1].commentary)

    def test_LocalizeExpressionVariables(self):
        assetPath = "variableExpressions/root.usda"
        archivePath = "variableExpressions.usdz"
        context = Ar.GetResolver().CreateDefaultContextForAsset(assetPath)

        with Ar.ResolverContextBinder(context):
            self.assertTrue(UsdUtils.CreateNewUsdzPackage(
                assetPath, archivePath, editLayersInPlace=True))
            
        zf = Sdf.ZipFile.Open(archivePath)
        expectedAssets = [
            "root.usda", 
            "0/attr.usda",
            "0/attr_arr.usda",
            "0/meta.usda",
            "0/meta_arr.usda",
            "0/pay.usda",
            "0/ref.usda",
            "0/sub.usda",
        ]

        self.assertEqual(expectedAssets, zf.GetFileNames())

        # ensure that the expression variable paths have been remapped and
        # saved in the asset's layers correctly.
        stage = Usd.Stage.Open(archivePath)
        self.assertIsNotNone(stage)

        # Assert that the the expression variables themselves have not been
        # altered.  This is important for preserving possible variant
        # selections.
        expectedExpressionVariables = {
            "ATTR": "attr.usda",
            "ATTR_ARR": "attr_arr.usda",
            "FILTER": True,
            "META": "meta.usda", 
            "META_ARR": "meta_arr.usda",
            "PAY": "pay.usda",
            "REF": "ref.usda",
            "SUBLAYER": "sub.usda"
        }
        self.assertEqual(
            expectedExpressionVariables, 
            stage.GetRootLayer().expressionVariables)

if __name__=="__main__":
    unittest.main()
