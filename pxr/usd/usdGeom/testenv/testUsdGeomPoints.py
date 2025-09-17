#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, unittest
from pxr import Usd, UsdGeom

class TestUsdGeomPoints(unittest.TestCase):

    def test_ComputePointCount(self):
        stage = Usd.Stage.Open('points.usda')
        unset = UsdGeom.Points.Get(stage, '/UnsetPoints')
        blocked = UsdGeom.Points.Get(stage, '/BlockedPoints')
        empty = UsdGeom.Points.Get(stage, '/EmptyPoints')
        timeSampled = UsdGeom.Points.Get(stage, '/TimeSampledPoints')
        timeSampledAndDefault = UsdGeom.Points.Get(
            stage, '/TimeSampledAndDefaultPoints')

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
            self.assertEqual(schema.GetPointCount(timeCode), expected)
        for (schema, expected) in testDefaults:
            self.assertTrue(schema)
            self.assertEqual(schema.GetPointCount(), expected)
 
        invalid = UsdGeom.Points(Usd.Prim())
        self.assertFalse(invalid)
        with self.assertRaises(RuntimeError):
            self.assertEqual(invalid.GetPointCount(), 0)
        with self.assertRaises(RuntimeError):
            self.assertEqual(invalid.GetPointCount(
                Usd.TimeCode.EarliestTime()), 0)

if __name__ == '__main__':
    unittest.main()
