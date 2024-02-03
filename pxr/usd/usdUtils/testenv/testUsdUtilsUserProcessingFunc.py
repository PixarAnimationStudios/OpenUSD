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
import os, sys, shutil, tempfile, unittest

class TestUsdUtilsUserProcessFunc(unittest.TestCase):
    def test_TransientDependencies(self):
        """Tests that transient dependencies that result from invocation of the
        user processing function are included in the package"""

        def TestUserFunc(layer, depInfo):
            validPath = depInfo.assetPath.replace('test_', '')
            return UsdUtils.DependencyInfo(validPath)

        rootPath = 'transientDeps/root.usda'
        localizationDir = 'transientDeps_localized'
        localizedRoot = os.path.join(localizationDir, 'root.usda')
        self._Localize(rootPath, localizationDir, TestUserFunc)
        self.assertIsNotNone(Usd.Stage.Open(localizedRoot))

        expectedFiles = [
            'dep_metadata.usda',
            'dep_payload.usda',
            'dep_prop.usda',
            'dep_prop_metadata.usda',
            'dep_reference.usda',
            'dep_sublayer.usda',
            'dep_timesample.usda',
            'metadata.usda',
            'payload.usda',
            'prop.usda',
            'prop_metadata.usda',
            'reference.usda',
            'root.usda',
            'sublayer.usda',
            'timesample.usda'
        ]

        self._CheckLocalizedPackageContents(localizationDir, expectedFiles)

    def test_TransientDependenciesTemplateAssetPath(self):
        """Tests that transient dependencies of clips template asset paths
        that are modified by user processing functions are included"""

        def TestUserFunc(layer, depInfo):
            validPath = depInfo.assetPath.replace('test_', '')
            validDeps = [x.replace('test_', '') for x in depInfo.dependencies]
            return UsdUtils.DependencyInfo(validPath, validDeps)
        
        rootPath = 'transientDepsTemplateAssetPath/root.usda'
        localizationDir = 'transientDepsTemplateAssetPath_localized'
        localizedRoot = os.path.join(localizationDir, 'root.usda')

        self._Localize(rootPath, localizationDir, TestUserFunc)
        self.assertIsNotNone(Usd.Stage.Open(localizedRoot))

        expectedFiles = [
            'clip.001.usda',
            'dep_clip.001.usda',
            'root.usda'
        ]

        self._CheckLocalizedPackageContents(localizationDir, expectedFiles)

    def test_UserProcessingFuncRemove(self):
        """Tests that dependencies can be removed with the user processing func"""
        def TestUserFunc(layer, depInfo):
            return UsdUtils.DependencyInfo()

        rootPath = 'transientDeps/root.usda'
        localizationDir = 'transientDeps_remove_localized'
        localizedRoot = os.path.join(localizationDir, 'root.usda')

        self._Localize(rootPath, localizationDir, TestUserFunc)
        self.assertIsNotNone(Usd.Stage.Open(localizedRoot))

        expectedFiles = [
            'root.usda'
        ]

        self._CheckLocalizedPackageContents(localizationDir, expectedFiles)

    def test_FileFormatConversion(self):
        """Tests converting a layer to a crate file in a temp path and
           ensuring the resulting localized package can be loaded correctly"""
        
        with tempfile.TemporaryDirectory() as tempDir:
            def TestUserFunc(layer, depInfo):
                root, ext = os.path.splitext(depInfo.assetPath)
                if ext != '.usda':
                    return depInfo
                
                sourceDepLayer = Sdf.Layer.FindOrOpenRelativeToLayer(
                    layer, depInfo.assetPath)
                crateFileName = root + '.usdc'
                cratePath = os.path.join(tempDir, crateFileName)
                sourceDepLayer.Export(cratePath)
                return UsdUtils.DependencyInfo(cratePath)
        
            rootPath = 'fileFormatConversion/root.usda'
            localizationDir = 'fileFormatConversion_localized'
            localizedRoot = os.path.join(localizationDir, 'root.usda')

            self._Localize(rootPath, localizationDir, TestUserFunc)
            self.assertIsNotNone(Usd.Stage.Open(localizedRoot))

            expectedFiles = [
                '0/sublayer.usdc',
                'root.usda'
            ]

            self._CheckLocalizedPackageContents(localizationDir, expectedFiles)

    def test_CachedProcessingFuncValues(self):
        """Tests that the system caches processed asset path values and only
           invokes the callback once for each layer / path pair"""
        
        processedPaths = set()
        def TestUserFunc(layer, depInfo):
            self.assertTrue(depInfo.assetPath not in processedPaths)
            processedPaths.add(depInfo.assetPath)

            name, _ = os.path.splitext(os.path.basename(depInfo.assetPath))

            if name.startswith('modify'):
                return UsdUtils.DependencyInfo('./modified.usda')
            elif name.startswith('remove'):
                return UsdUtils.DependencyInfo()
            else:
                return depInfo
            
        rootPath = 'duplicatePaths/root.usda'
        localizationDir = 'duplicatePaths_localized'
        localizedRoot = os.path.join(localizationDir, 'root.usda')

        self._Localize(rootPath, localizationDir, TestUserFunc)
        self.assertIsNotNone(Usd.Stage.Open(localizedRoot))

        self.assertSetEqual(processedPaths, 
            {'./default.usda', './modify.usda', './remove.usda'})

        expectedFiles = [
            'default.usda',
            'modified.usda',
            'root.usda'
        ]

        self._CheckLocalizedPackageContents(localizationDir, expectedFiles)

    def _Localize(self, rootPath, localizationDir, userFunc):
        if os.path.isdir(localizationDir):
            shutil.rmtree(localizationDir)
        
        result = UsdUtils.LocalizeAsset(
            rootPath, localizationDir, processingFunc=userFunc)
        
        self.assertTrue(result)

    
    def _CheckLocalizedPackageContents(self, packagePath, expectedFiles):
        filesInArchive = self._GetFileList(packagePath)
        self.assertEqual(expectedFiles, filesInArchive)

    def _GetFileList(self, localizedAssetDir):
        rootFolderPathStr = localizedAssetDir + os.sep
        contents = []
        for path, directories, files in os.walk(localizedAssetDir):
            for file in files:
                localizedPath = os.path.join(path,file)
                localizedPath = localizedPath.replace(rootFolderPathStr, '')
                localizedPath = localizedPath.replace('\\', '/')
                contents.append(localizedPath)
        
        contents.sort()

        return contents


if __name__=="__main__":
    unittest.main()
