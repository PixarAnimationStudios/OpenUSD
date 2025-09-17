#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, unittest
from pxr import Usd, UsdGeom

class TestUsdGeomBasisCurves(unittest.TestCase):
    def test_InterpolationTypes(self):
        stage = Usd.Stage.Open('basisCurves.usda')
        t = Usd.TimeCode.Default()
        c = UsdGeom.BasisCurves.Get(stage, '/BezierCubic')
        self.assertEqual(c.ComputeUniformDataSize(t), 2)
        self.assertEqual(c.ComputeVaryingDataSize(t), 4)
        self.assertEqual(c.ComputeVertexDataSize(t), 10)
        self.assertEqual(c.ComputeInterpolationForSize(1, t), UsdGeom.Tokens.constant)
        self.assertEqual(c.ComputeInterpolationForSize(2, t), UsdGeom.Tokens.uniform)
        self.assertEqual(c.ComputeInterpolationForSize(4, t), UsdGeom.Tokens.varying)
        self.assertEqual(c.ComputeInterpolationForSize(10, t), UsdGeom.Tokens.vertex)
        self.assertFalse(c.ComputeInterpolationForSize(100, t))
        self.assertFalse(c.ComputeInterpolationForSize(0, t))

    def test_ComputeCurveCount(self):
        stage = Usd.Stage.Open('basisCurves.usda')
        unset = UsdGeom.BasisCurves.Get(stage, '/UnsetVertexCounts')
        blocked = UsdGeom.BasisCurves.Get(stage, '/BlockedVertexCounts')
        empty = UsdGeom.BasisCurves.Get(stage, '/EmptyVertexCounts')
        timeSampled = UsdGeom.BasisCurves.Get(stage, '/TimeSampledVertexCounts')
        timeSampledAndDefault = UsdGeom.BasisCurves.Get(
            stage, '/TimeSampledAndDefaultVertexCounts')

        testTimeSamples = [
            (unset, Usd.TimeCode.EarliestTime(), 0),
            (blocked, Usd.TimeCode.EarliestTime(), 0),
            (empty, Usd.TimeCode.EarliestTime(), 0),
            (timeSampled, Usd.TimeCode.EarliestTime(), 3),
            (timeSampledAndDefault, Usd.TimeCode.EarliestTime(), 5)]
        testDefaults = [
            (unset, 0),
            (blocked, 0),
            (empty, 0),
            (timeSampled, 0),
            (timeSampledAndDefault, 4)]

        for (schema, timeCode, expected) in testTimeSamples:
            self.assertTrue(schema)
            self.assertEqual(schema.GetCurveCount(timeCode), expected)
        for (schema, expected) in testDefaults:
            self.assertTrue(schema)
            self.assertEqual(schema.GetCurveCount(), expected) 

        invalid = UsdGeom.BasisCurves(Usd.Prim())
        self.assertFalse(invalid)
        with self.assertRaises(RuntimeError):
            self.assertEqual(invalid.ComputeCurveCount(), 0)
        with self.assertRaises(RuntimeError):
            self.assertEqual(invalid.ComputeCurveCount(
                Usd.TimeCode.EarliestTime()), 0)

    def test_ComputeSegmentCount(self):
        stage = Usd.Stage.Open('basisCurves.usda')
        data = [
            (UsdGeom.BasisCurves.Get(stage, '/LinearNonperiodic'), [1, 2, 1, 4]),
            (UsdGeom.BasisCurves.Get(stage, '/LinearPeriodic'), [3, 7]),
            (UsdGeom.BasisCurves.Get(stage, '/CubicNonperiodicBezier'), [1, 2, 3, 1, 2]),
            (UsdGeom.BasisCurves.Get(stage, '/CubicNonperiodicBspline'), [2, 1, 3, 4]),
            (UsdGeom.BasisCurves.Get(stage, '/CubicPeriodicBezier'), [2, 3, 2]),
            (UsdGeom.BasisCurves.Get(stage, '/CubicPeriodicBspline'), [6, 9, 6]),
            (UsdGeom.BasisCurves.Get(stage, '/CubicPinnedCatmullRom'), [1, 2, 1, 4]),
            (UsdGeom.BasisCurves.Get(stage, '/CubicPinnedBezier'), [1, 2, 3, 1, 2])
        ]

        for curve, curveSegmentCounts in data:
            self.assertEqual(
                curve.ComputeSegmentCounts(Usd.TimeCode.EarliestTime()), 
                curveSegmentCounts)

if __name__ == '__main__':
    unittest.main()
