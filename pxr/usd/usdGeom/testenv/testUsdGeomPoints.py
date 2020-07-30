#!/pxrpythonsubst
#
# Copyright 2020 Pixar
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
