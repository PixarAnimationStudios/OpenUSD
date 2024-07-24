#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Usd
from pxr import UsdAbc
from pxr import UsdGeom
from pxr import Gf


class TestUsdAbcConversionBasisCurves(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        usdFile = 'original.usda'
        abcFile = 'converted.abc'

        UsdAbc._WriteAlembic(usdFile, abcFile)
        cls.stage = Usd.Stage.Open(abcFile)

    def _assertElementsAlmostEqual(self, seq1, seq2):
        self.assertTrue(all(Gf.IsClose(e1, e2, 1e-5)
                        for e1, e2 in zip(seq1, seq2)))

    def test_RoundTripLinear(self):
        time = Usd.TimeCode.EarliestTime()
        prim = self.stage.GetPrimAtPath('/Linear/Ribbons/VaryingWidth')
        schema = UsdGeom.BasisCurves(prim)
        # These attributes are uniformly time sampled
        curveType = schema.GetTypeAttr().Get()
        wrap = schema.GetWrapAttr().Get()

        self.assertEqual(curveType, UsdGeom.Tokens.linear)
        self.assertEqual(wrap, UsdGeom.Tokens.nonperiodic)

        # Interpolation metadata
        normalsInterpolation = schema.GetNormalsInterpolation()
        widthsInterpolation = schema.GetWidthsInterpolation()
        self.assertEqual(normalsInterpolation, UsdGeom.Tokens.varying)
        self.assertEqual(widthsInterpolation, UsdGeom.Tokens.varying)

        # These attributes may be varying time sampled
        curveVertexCounts = schema.GetCurveVertexCountsAttr().Get(time)
        points = schema.GetPointsAttr().Get(time)
        widths = schema.GetWidthsAttr().Get(time)
        normals = schema.GetNormalsAttr().Get(time)
       
        self._assertElementsAlmostEqual(
            points,
            [(0, 0, 0), (1, 1, 0), (1, 2, 0), (0, 3, 0), (-1, 4, 0), (-1, 5, 0), (0, 6, 0)])
        self._assertElementsAlmostEqual(
             widths,
             [0, .5, .5, .8, .5, .5, 0])
        self._assertElementsAlmostEqual(
             normals,
             [(1, 0, 0), (.98, 0, .44), (.98, 0, .44), (.707, 0, .707), (.98, 0, .44), (.98, 0, .44), (1, 0, 0)])
        self.assertEqual(list(curveVertexCounts), [7])

    def test_RoundTripCubic(self):
        time = Usd.TimeCode.EarliestTime()
        prim = self.stage.GetPrimAtPath('/Cubic/Ribbons/VertexWidth')
        schema = UsdGeom.BasisCurves(prim)

        # These attributes are uniformly time sampled
        curveType = schema.GetTypeAttr().Get()
        basis = schema.GetBasisAttr().Get()
        wrap = schema.GetWrapAttr().Get()

        self.assertEqual(curveType, UsdGeom.Tokens.cubic)
        self.assertEqual(basis, UsdGeom.Tokens.bezier)
        self.assertEqual(wrap, UsdGeom.Tokens.nonperiodic)

        # Interpolation metadata
        normalsInterpolation = schema.GetNormalsInterpolation()
        widthsInterpolation = schema.GetWidthsInterpolation()
        self.assertEqual(normalsInterpolation, UsdGeom.Tokens.varying)
        self.assertEqual(widthsInterpolation, UsdGeom.Tokens.vertex)

        # These attributes may be varying time sampled
        curveVertexCounts = schema.GetCurveVertexCountsAttr().Get(time)
        points = schema.GetPointsAttr().Get(time)
        widths = schema.GetWidthsAttr().Get(time)
        normals = schema.GetNormalsAttr().Get(time)
       
        self._assertElementsAlmostEqual(
             points,
             [(0, 0, 0), (1, 1, 0), (1, 2, 0), (0, 3, 0), (-1, 4, 0), (-1, 5, 0), (0, 6, 0)])
        self._assertElementsAlmostEqual(
             widths,
             [0, .5, .5, .8, .5, .5, 0])
        self._assertElementsAlmostEqual(
             normals,
             [(1, 0, 0), (.98, 0, .44), (.707, 0, .707)]) 
        self.assertEqual(list(curveVertexCounts), [7])


if __name__ == '__main__':
    unittest.main()
