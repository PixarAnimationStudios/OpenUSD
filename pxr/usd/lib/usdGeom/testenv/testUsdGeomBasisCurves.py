#!/pxrpythonsubst

import sys
from pxr import Usd, UsdGeom
from Mentor.Runtime import AssertEqual,\
                           AssertFalse,\
                           FindDataFile,\
                           SetAssertMode,\
                           MTR_EXIT_TEST

def Main(argv):
    TestInterpolationTypes()

def TestInterpolationTypes():
    stage = Usd.Stage.Open(FindDataFile("testUsdGeomBasisCurves/basisCurves.usda"))
    t = Usd.TimeCode.Default()
    c = UsdGeom.BasisCurves.Get(stage, '/BezierCubic')
    AssertEqual(c.ComputeUniformDataSize(t), 2)
    AssertEqual(c.ComputeVaryingDataSize(t), 4)
    AssertEqual(c.ComputeVertexDataSize(t), 10)

    AssertEqual(c.ComputeInterpolationForSize(1, t), UsdGeom.Tokens.constant)
    AssertEqual(c.ComputeInterpolationForSize(2, t), UsdGeom.Tokens.uniform)
    AssertEqual(c.ComputeInterpolationForSize(4, t), UsdGeom.Tokens.varying)
    AssertEqual(c.ComputeInterpolationForSize(10, t), UsdGeom.Tokens.vertex)
    AssertFalse(c.ComputeInterpolationForSize(100, t))
    AssertFalse(c.ComputeInterpolationForSize(0, t))

if __name__ == '__main__':
    SetAssertMode(MTR_EXIT_TEST)
    Main(sys.argv)
