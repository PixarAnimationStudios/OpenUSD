#!/pxrpythonsubst

# Copyright, Pixar Animation Studios 2015. All rights reserved.

from pxr import Sdf, UsdUtils
from Mentor.Runtime import *


SetAssertMode(MTR_EXIT_TEST)

class TestUsdStitchBasic(Fixture):
    def ClassSetup(self):
        self.layerFileNames = [FindDataFile('src/Particles_Splash.101.usd'),
                               FindDataFile('src/Particles_Splash.102.usd'),
                               FindDataFile('src/Particles_Splash.103.usd'),
                               FindDataFile('src/Particles_Splash.104.usd'),
                               FindDataFile('src/Particles_Splash.105.usd'),
                               FindDataFile('src/Particles_Splash.106.usd'),
                               FindDataFile('src/Particles_Splash.107.usd'),
                               FindDataFile('src/Particles_Splash.108.usd'),
                               FindDataFile('src/PTS_dev.1.usd'),
                               FindDataFile('src/PTS_dev.2.usd'),
                               FindDataFile('src/PTS_dev.3.usd'),
                               FindDataFile('src/PTS_dev.4.usd'),
                               FindDataFile('src/PTS_dev.5.usd'),
                               FindDataFile('src/PTS_dev.6.usd'),
                               FindDataFile('src/PTS_dev.7.usd'),
                               FindDataFile('src/PTS_dev.8.usd'),
                               FindDataFile('src/PTS_dev.9.usd'),
                               FindDataFile('src/PTS_dev.10.usd'),
                               FindDataFile('src/PTS_dev.11.usd'),
                               FindDataFile('src/PTS_dev.12.usd'),
                               FindDataFile('src/PTS_dev.13.usd'),
                               FindDataFile('src/PTS_dev.14.usd'),
                               FindDataFile('src/PTS_dev.15.usd'),
                               FindDataFile('src/PTS_dev.16.usd'),
                               FindDataFile('src/PTS_dev.17.usd'),
                               FindDataFile('src/PTS_dev.18.usd'),
                               FindDataFile('src/PTS_dev.19.usd'),
                               FindDataFile('src/PTS_dev.20.usd'),
                               FindDataFile('src/PTS_dev.21.usd'),
                               FindDataFile('src/PTS_dev.22.usd'),
                               FindDataFile('src/PTS_dev.23.usd'),
                               FindDataFile('src/PTS_dev.24.usd')]

        self.clipPath = Sdf.Path('/World/fx/Particles_Splash')
        self.baseName = 'testModelClips.usd'
        self.startTimeCode = 101
        self.rootLayer = Sdf.Layer.CreateNew(self.baseName)
        UsdUtils.StitchClips(self.rootLayer, self.layerFileNames[0:7], 
                             self.clipPath, self.startTimeCode)

    def TestValidClipMetadata(self):
        clipPrim = self.rootLayer.GetPrimAtPath(self.clipPath)
        assert clipPrim
        assert set(clipPrim.ListInfoKeys()) == set(['clipTimes',
            'clipAssetPaths', 'clipPrimPath', 'clipManifestAssetPath',
            'clipActive', 'specifier'])

    def TestValidUsdLayerGeneration(self):
        assert self.rootLayer

    def TestRelativeAssetPaths(self):
        import os
        rootLayerFile = 'relativePaths.usd'
        if os.path.isfile(rootLayerFile):
            os.remove(rootLayerFile)

        rootLayer = Sdf.Layer.CreateNew(rootLayerFile)
        assert rootLayer

        localLayerNames = [os.path.relpath(i, os.getcwd()) 
                           for i in self.layerFileNames]

        UsdUtils.StitchClips(rootLayer, localLayerNames, self.clipPath)
        assetPaths = (
            rootLayer.GetPrimAtPath(self.clipPath).GetInfo('clipAssetPaths'))

        # ensure all paths are relative
        import itertools
        assert not any([os.path.isabs(i.path) for i in assetPaths])

    def TestMultipleIterations(self):
        import os
        rootLayerFile = 'multipleIterations.usd'
        if os.path.isfile(rootLayerFile):
            os.remove(rootLayerFile)

        rootLayer = Sdf.Layer.CreateNew(rootLayerFile)
        UsdUtils.StitchClips(rootLayer, self.layerFileNames[0:2], self.clipPath)
        UsdUtils.StitchClips(rootLayer, self.layerFileNames[2:5], self.clipPath)
        UsdUtils.StitchClips(rootLayer, self.layerFileNames[5:7], self.clipPath)

        assert rootLayer
        # ensure all assets were added
        assetPaths = (
            rootLayer.GetPrimAtPath(self.clipPath).GetInfo('clipAssetPaths'))
        assert len(assetPaths) == 7 

        # ensure that order was respected
        particleSplashIter = 101 
        for assetPath in assetPaths:
            assert str(particleSplashIter) in str(assetPath)
            particleSplashIter = particleSplashIter + 1

        # ensure that frame data is sensible
        assert rootLayer.startTimeCode == 101 
        assert rootLayer.endTimeCode == 107 

    def TestDestructiveTopology(self):
        import os
        rootLayerFile = 'testDestructiveTopology.usd'
        clipPath = "/World/fx"
        otherClipPath = '/World/AnAlternateUniverse'
        topologyLayerFile = 'testDestructiveTopology.topology.usd'
        inputLayer = FindDataFile('src/topology.usd')
        otherInputLayer = FindDataFile('src/alt_topology.usd')

        if os.path.isfile(rootLayerFile):
            os.remove(rootLayerFile)
            
        if os.path.isfile(topologyLayerFile):
            os.remove(topologyLayerFile)

        # when we do want topology to persist
        rootLayer = Sdf.Layer.CreateNew(rootLayerFile)
        UsdUtils.StitchClips(rootLayer, [inputLayer], clipPath,
                             reuseExistingTopology=False)
        UsdUtils.StitchClips(rootLayer, [otherInputLayer], otherClipPath,
                             reuseExistingTopology=True)
        topologyLayer = Sdf.Layer.FindOrOpen(topologyLayerFile)
        assert topologyLayer.GetPrimAtPath(otherClipPath)
        assert topologyLayer.GetPrimAtPath(clipPath)

        # when we don't want topology to persist
        UsdUtils.StitchClips(rootLayer, [inputLayer], clipPath,
                             reuseExistingTopology=False)
        UsdUtils.StitchClips(rootLayer, [otherInputLayer], otherClipPath,
                             reuseExistingTopology=False)
        topologyLayer = Sdf.Layer.FindOrOpen(topologyLayerFile)
        assert not topologyLayer.GetPrimAtPath(clipPath)
        assert topologyLayer.GetPrimAtPath(otherClipPath)

    # This test ensures that we are grabbing our frame data from
    # the layers directly so we incur no precision loss from string
    # parsing as we believe we were previously
    def TestNumericalPrecisionLoss(self):
        rootLayerFile = 'numericalPrecision.usd'
        clipPath = Sdf.Path("/World/fx/points")
        rootLayer = Sdf.Layer.CreateNew(rootLayerFile)
        UsdUtils.StitchClips(rootLayer, self.layerFileNames[8:], clipPath)
        assert rootLayer
        assert rootLayer.startTimeCode == 1.00000
        # previously, the precision error was causing
        # us to end up with an end frame of 24.0006
        assert rootLayer.endTimeCode == 24.000000

    def TestFilePermissions(self):
        import os
        from pxr import Tf
        rootLayerFile = 'permissions.usd'
        clipPath = Sdf.Path('/World/fx/points')
        rootLayer = Sdf.Layer.CreateNew(rootLayerFile)
        os.system('chmod -w ' + rootLayerFile)
        try:
            UsdUtils.StitchClips(rootLayer, self.layerFileNames, clipPath)
        except Tf.ErrorException as tfError:
            print "Caught expected exception %s" %tfError
        else:
            assert False, "Failed to raise runtime error on unwritable file." 
        finally:
            os.system('chmod +w ' + rootLayerFile)

if __name__ == '__main__':
    Runner().Main()
