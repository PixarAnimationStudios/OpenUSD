#!/pxrpythonsubst

from pxr import Usd, UsdAbc, UsdGeom

from Mentor.Runtime import (Fixture,
                            SetAssertMode,
                            MTR_EXIT_TEST,
                            AssertClose,
                            AssertEqual,
                            AssertTrue,
                            Runner,
                            FindDataFile)

class TestUsdAbcConversionSubdiv(Fixture):
    
    def TestRoundTrip(self):
        
        usdFile = 'original.usda'
        abcFile = 'converted.abc'

        AssertTrue(UsdAbc._WriteAlembic(FindDataFile(usdFile), abcFile))

        origStage = Usd.Stage.Open(FindDataFile(usdFile))
        stage = Usd.Stage.Open(abcFile)

        prim = stage.GetPrimAtPath('/World/geom/CenterCross/UpLeft')
        AssertClose(prim.GetAttribute('creaseIndices').Get(),
                    [0, 1, 3, 2, 0, 4, 5, 7, 6, 4, 1, 5, 0, 4, 2, 6, 3, 7])
        AssertClose(prim.GetAttribute('creaseLengths').Get(),
                    [5, 5, 2, 2, 2, 2])
        AssertClose(prim.GetAttribute('creaseSharpnesses').Get(),
                    [1000, 1000, 1000, 1000, 1000, 1000])
        AssertClose(prim.GetAttribute('faceVertexCounts').Get(),
                    [4, 4, 4, 4, 4, 4])

        # The writer will revrse the orientation because alembic only supports
        # left handed winding order.
        AssertClose(prim.GetAttribute('faceVertexIndices').Get(),
                    [2, 6, 4, 0, 4, 5, 1, 0, 6, 7, 5, 4, 1, 5, 7, 3, 2, 3, 7, 6, 0, 1, 3, 2])

        # Check layer/stage metadata transfer
        AssertEqual(origStage.GetDefaultPrim().GetPath(),
                    stage.GetDefaultPrim().GetPath())
        AssertEqual(origStage.GetTimeCodesPerSecond(),
                    stage.GetTimeCodesPerSecond())
        AssertEqual(origStage.GetFramesPerSecond(),
                    stage.GetFramesPerSecond())
        AssertEqual(origStage.GetStartTimeCode(),
                    stage.GetStartTimeCode())
        AssertEqual(origStage.GetEndTimeCode(),
                    stage.GetEndTimeCode())
        AssertEqual(UsdGeom.GetStageUpAxis(origStage),
                    UsdGeom.GetStageUpAxis(stage))
        
#        stage.Export('converted.usda')

if __name__ == '__main__':
    Runner().Main()

