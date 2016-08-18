#!/pxrpythonsubst

from pxr import Sdf, Usd, UsdGeom, UsdAbc, UsdUtils

# Imports for testing 
from pxr import Gf
from Mentor.Runtime import (Assert, AssertTrue, AssertClose,
                            SetIsCloseSignatureCallback,
                            SetDefaultEpsilonValue,
                            FindDataFile, Fixture, Runner)

# Setting up so that we can compare GfCamera's by frustum

SetIsCloseSignatureCallback(Gf.Frustum,
                            lambda frustum: frustum.ComputeCorners())
SetDefaultEpsilonValue(1e-5)

class TestUsdAlembicCamera(Fixture):

    @staticmethod
    def _RoundTripFileNames(baseName):
        """
        Filenames for testing a round trip usd->abc->usd.
        First file name uses FindDataFile to locate it within a test.
        """
        oldUsdFile = FindDataFile(baseName + '.usd')
        abcFile  = baseName + '_1.abc'
        newUsdFile  = baseName + '_2.usd'

        return oldUsdFile, abcFile, newUsdFile
    
    @staticmethod
    def _TestRoundTrip(baseName, camPath):
        """
        Given a base filename and a camera path, tests the round tripping
        by comparing the frustum before and after.
        """

        oldUsdFile, abcFile, newUsdFile = (
            TestUsdAlembicCamera._RoundTripFileNames(baseName))

        # USD -> ABC
        AssertTrue(UsdAbc._WriteAlembic(oldUsdFile, abcFile))

        # ABC -> USD
        layer = Sdf.Layer.FindOrOpen(abcFile)
        Assert(layer)
        AssertTrue(layer.Export(newUsdFile))
        layer = None

        # Open old and new USD file on stages
        oldStage = Usd.Stage.Open(oldUsdFile)
        Assert(oldStage)
        newStage = Usd.Stage.Open(newUsdFile)
        Assert(newStage)

        # Get old and new camera
        oldCam = oldStage.GetPrimAtPath(camPath)
        Assert(oldCam)
        newCam = newStage.GetPrimAtPath(camPath)
        Assert(newCam)

        # Iterate through frames
        for frame in range(
            int(oldStage.GetStartTimeCode()),
            int(oldStage.GetEndTimeCode()) + 1):

            # Convert to Gf camera
            oldGfCam = UsdGeom.Camera(oldCam).GetCamera(frame)
            newGfCam = UsdGeom.Camera(newCam).GetCamera(frame)

            # Compare frustums
            AssertClose(oldGfCam.frustum,
                        newGfCam.frustum)
        

    def TestRoundTrips(self):
        """
        Test Round Trip.
        """

        TestUsdAlembicCamera._TestRoundTrip(
            baseName = 'testShotFlattened',
            camPath = '/World/main_cam')

    def TestAbcToUsd(self):
        """
        Test conversion of an ABC camera to USD.
        """

        # Open ABC file on USD stage and get camera
        abcFile = FindDataFile('testAbcCamera.abc')
        Assert(abcFile)
        stage = Usd.Stage.Open(abcFile)
        Assert(stage)
        cam = stage.GetPrimAtPath('/World/main_cam')
        Assert(cam)
        
        # Convert to Gf camera to check the frustum
        gfCam = UsdGeom.Camera(cam).GetCamera(1.0)
        AssertClose(
            gfCam.frustum.ComputeCorners(),
            (Gf.Vec3d(    -4.27349785298,  -2341.70939914119,     -10.0),
             Gf.Vec3d(     4.27349785298,  -2341.70939914119,     -10.0),
             Gf.Vec3d(    -4.27349785298,  -2338.29060085880,     -10.0),
             Gf.Vec3d(     4.27349785298,  -2338.29060085880,     -10.0),
             Gf.Vec3d(-42734.97852984663, -19433.99141193865, -100000.0), 
             Gf.Vec3d( 42734.97852984663, -19433.99141193865, -100000.0),
             Gf.Vec3d(-42734.97852984663,  14753.99141193864, -100000.0),
             Gf.Vec3d( 42734.97852984663,  14753.99141193864, -100000.0)))

if __name__ == "__main__":
    Runner().Main()
