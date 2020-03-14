#!/pxrpythonsubst
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

from pxr import Sdf, Tf, UsdUtils, Vt, Gf
import unittest

class TestUsdUtilsStitchClips(unittest.TestCase):
    def setUp(self):
        if not hasattr(self, 'setupComplete'):
            self.layerFileNames = ['src/Particles_Splash.101.usd',
                                   'src/Particles_Splash.102.usd',
                                   'src/Particles_Splash.103.usd',
                                   'src/Particles_Splash.104.usd',
                                   'src/Particles_Splash.105.usd',
                                   'src/Particles_Splash.106.usd',
                                   'src/Particles_Splash.107.usd',
                                   'src/Particles_Splash.108.usd',
                                   'src/PTS_dev.1.usd',
                                   'src/PTS_dev.2.usd',
                                   'src/PTS_dev.3.usd',
                                   'src/PTS_dev.4.usd',
                                   'src/PTS_dev.5.usd',
                                   'src/PTS_dev.6.usd',
                                   'src/PTS_dev.7.usd',
                                   'src/PTS_dev.8.usd',
                                   'src/PTS_dev.9.usd',
                                   'src/PTS_dev.10.usd',
                                   'src/PTS_dev.11.usd',
                                   'src/PTS_dev.12.usd',
                                   'src/PTS_dev.13.usd',
                                   'src/PTS_dev.14.usd',
                                   'src/PTS_dev.15.usd',
                                   'src/PTS_dev.16.usd',
                                   'src/PTS_dev.17.usd',
                                   'src/PTS_dev.18.usd',
                                   'src/PTS_dev.19.usd',
                                   'src/PTS_dev.20.usd',
                                   'src/PTS_dev.21.usd',
                                   'src/PTS_dev.22.usd',
                                   'src/PTS_dev.23.usd',
                                   'src/PTS_dev.24.usd']

            self.clipPath = Sdf.Path('/World/fx/Particles_Splash')
            self.baseName = 'testModelClips.usd'
            self.startTimeCode = 101
            self.endTimeCode   = 108
            rootLayer = Sdf.Layer.FindOrOpen(self.baseName)
            self.rootLayer = rootLayer if rootLayer else Sdf.Layer.CreateNew(self.baseName)
            UsdUtils.StitchClips(self.rootLayer, self.layerFileNames[0:7], 
                                 self.clipPath, self.startTimeCode, self.endTimeCode)

        self.setupComplete = True

    def test_ValidClipMetadata(self):
        clipPrim = self.rootLayer.GetPrimAtPath(self.clipPath)
        self.assertTrue(clipPrim)
        self.assertEqual(set(clipPrim.ListInfoKeys()), 
                         set(['clips', 'specifier']))
        self.assertEqual(set(clipPrim.GetInfo('clips').keys()),
                         set(['default']))
        self.assertEqual(set(clipPrim.GetInfo('clips')['default'].keys()),
                         set(['times', 'assetPaths', 'primPath', 
                              'manifestAssetPath', 'active']))

    def test_ValidUsdLayerGeneration(self):
        self.assertTrue(self.rootLayer)

    def test_RelativeAssetPaths(self):
        import os
        rootLayerFile = 'relativePaths.usd'
        if os.path.isfile(rootLayerFile):
            os.remove(rootLayerFile)

        rootLayer = Sdf.Layer.CreateNew(rootLayerFile)
        self.assertTrue(rootLayer)

        localLayerNames = [os.path.relpath(i, os.getcwd()) 
                           for i in self.layerFileNames]

        UsdUtils.StitchClips(rootLayer, localLayerNames, self.clipPath)
        assetPaths = (
            rootLayer.GetPrimAtPath(self.clipPath).GetInfo('clipAssetPaths'))

        # ensure all paths are relative
        import itertools
        self.assertTrue(not any([os.path.isabs(i.path) for i in assetPaths]))

    # This test ensures that we are grabbing our frame data from
    # the layers directly so we incur no precision loss from string
    # parsing as we believe we were previously
    def test_NumericalPrecisionLoss(self):
        rootLayerFile = 'numericalPrecision.usd'
        clipPath = Sdf.Path("/World/fx/points")
        rootLayer = Sdf.Layer.CreateNew(rootLayerFile)
        UsdUtils.StitchClips(rootLayer, self.layerFileNames[8:], clipPath)
        self.assertTrue(rootLayer)
        self.assertEqual(rootLayer.startTimeCode, 1.00000)
        # previously, the precision error was causing
        # us to end up with an end frame of 24.0006
        self.assertEqual(rootLayer.endTimeCode, 24.000000)

    def test_FilePermissions(self):
        import os, stat
        from pxr import Tf
        rootLayerFile = 'permissions.usd'
        clipPath = Sdf.Path('/World/fx/points')
        rootLayer = Sdf.Layer.CreateNew(rootLayerFile)
        mode = stat.S_IMODE(os.stat(rootLayerFile).st_mode)
        os.chmod(rootLayerFile,
                 mode & ~(stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH))
        try:
            UsdUtils.StitchClips(rootLayer, self.layerFileNames, clipPath)
        except Tf.ErrorException as tfError:
            print "Caught expected exception %s" %tfError
        else:
            self.assertTrue(False, "Failed to raise runtime error on unwritable file." )
        finally:
            os.chmod(rootLayerFile, mode)

    def test_StitchTopologyOnly(self):
        # Generate a fresh topology
        topologyLayer = Sdf.Layer.CreateNew('topologyLayer.usd')
        UsdUtils.StitchClipsTopology(topologyLayer, self.layerFileNames[:7])
        self.assertTrue(topologyLayer)
        self.assertTrue(topologyLayer.GetPrimAtPath(self.clipPath))

        # Aggregate into an existing topology
        newClipPath = Sdf.Path('/World/fx/points')
        UsdUtils.StitchClipsTopology(topologyLayer, self.layerFileNames[8:10])
        self.assertTrue(topologyLayer)
        self.assertTrue(topologyLayer.GetPrimAtPath(newClipPath))

    def test_ExplicitEndCodes(self):
        start = 104
        end   = 105
        resultLayer = Sdf.Layer.CreateNew('explicitEndCodes.usd')
        UsdUtils.StitchClips(resultLayer, self.layerFileNames[:7],
                             self.clipPath, start, end)

        self.assertEqual(resultLayer.startTimeCode, start)
        self.assertEqual(resultLayer.endTimeCode, end)

    def test_TopologySublayerAuthoring(self):
        resultLayer = Sdf.Layer.CreateNew('sublayerTopology.usd')
        UsdUtils.StitchClips(resultLayer, self.layerFileNames[:7], self.clipPath)

        self.assertEqual(list(resultLayer.subLayerPaths), 
                         ['./sublayerTopology.topology.usd'])
        
        resultLayer = Sdf.Layer.CreateNew('foo.usd')
        topLayer = Sdf.Layer.CreateNew('foo.topology.usd')
        UsdUtils.StitchClipsTemplate(resultLayer, topLayer, self.clipPath,
                                     'asset.#.usd', 101, 120, 1)
        self.assertEqual(list(resultLayer.subLayerPaths), 
                         ['./foo.topology.usd'])
 
    def test_GenerateTopologyName(self):
        names = [("/foo/bar/baz.foo.usd", "/foo/bar/baz.foo.topology.usd"), 
                 ("foo.usda", "foo.topology.usda"), 
                 ("./mars.usd", "./mars.topology.usd")]

        for (original, expected) in names:
            self.assertEqual(UsdUtils.GenerateClipTopologyName(original), 
                             expected)

    def test_CustomSetName(self):
        """ Test authoring with a custom set name 'bob' """
        # template case
        resultLayer = Sdf.Layer.CreateNew('customSetName.usd')
        topLayer = Sdf.Layer.CreateNew('customSetName.topology.usd')
        UsdUtils.StitchClipsTemplate(resultLayer, topLayer, self.clipPath,
                                     'asset.#.usd', 101, 120, 1, 0.3, 'bob')
        self.assertEqual(list(resultLayer.subLayerPaths),
                         ['./customSetName.topology.usd'])
        self.assertEqual(resultLayer.endTimeCode, 120)
        self.assertEqual(resultLayer.startTimeCode, 101)

        prim = resultLayer.GetPrimAtPath('/World/fx/Particles_Splash')
        self.assertTrue('clips' in prim.GetMetaDataInfoKeys())
        expectedValues = {'templateStartTime': 101.0, 
                          'templateStride': 1.0, 
                          'templateActiveOffset': 0.3,
                          'primPath': '/World/fx/Particles_Splash', 
                          'templateAssetPath': 'asset.#.usd', 
                          'templateEndTime': 120.0}

        # Ensure that our custom set name applied
        actualValues = prim.GetInfo('clips')['bob']
        self.assertTrue(actualValues is not None)

        # Ensure all of our values were written out
        for k in ['templateStartTime', 'templateStride', 'templateEndTime',
                  'primPath', 'templateAssetPath']:
            self.assertEqual(expectedValues[k], actualValues[k])

        # non template case
        resultLayer = Sdf.Layer.CreateNew('customSetNameNonTemplate.usd')
        UsdUtils.StitchClips(resultLayer, self.layerFileNames[:7], 
                             self.clipPath, clipSet='bob')

        self.assertEqual(resultLayer.startTimeCode, 101)
        self.assertEqual(resultLayer.endTimeCode, 107)
        self.assertEqual(list(resultLayer.subLayerPaths),
                         ['./customSetNameNonTemplate.topology.usd'])
        
        prim = resultLayer.GetPrimAtPath('/World/fx/Particles_Splash')
        expectedValues = {'active': Vt.Vec2dArray( 
                                     [(101.0, 0.0), 
                                      (102.0, 1.0), 
                                      (103.0, 2.0), 
                                      (104.0, 3.0),
                                      (105.0, 4.0), 
                                      (106.0, 5.0), 
                                      (107.0, 6.0)]), 
                          'times': Vt.Vec2dArray( 
                                    [(101.0, 101.0), 
                                     (102.0, 102.0), 
                                     (103.0, 103.0), 
                                     (104.0, 104.0), 
                                     (105.0, 105.0), 
                                     (106.0, 106.0), 
                                     (107.0, 107.0)]), 
                          'manifestAssetPath': Sdf.AssetPath('./customSetNameNonTemplate.topology.usd'), 
                          'assetPaths': Sdf.AssetPathArray(
                                        [Sdf.AssetPath('./src/Particles_Splash.101.usd'), 
                                         Sdf.AssetPath('./src/Particles_Splash.102.usd'), 
                                         Sdf.AssetPath('./src/Particles_Splash.103.usd'), 
                                         Sdf.AssetPath('./src/Particles_Splash.104.usd'), 
                                         Sdf.AssetPath('./src/Particles_Splash.105.usd'), 
                                         Sdf.AssetPath('./src/Particles_Splash.106.usd'), 
                                         Sdf.AssetPath('./src/Particles_Splash.107.usd')]), 
                          'primPath': '/World/fx/Particles_Splash'}
        
        # Ensure that our custom set name applied
        actualValues = prim.GetInfo('clips')['bob']
        self.assertTrue(actualValues is not None)

        # Ensure all of our values were written out
        for k in ['active', 'times', 'manifestAssetPath',
                  'primPath', 'assetPaths']:
            self.assertEqual(expectedValues[k], actualValues[k])
        
if __name__ == '__main__':
    unittest.main()
