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

import unittest

from pxr import Tf
from pxr import Vt
from pxr import Usd
from pxr import UsdGeom


class TestUsdGeomHermiteCurves(unittest.TestCase):
    def testPointAndTangents(self):
        with self.assertRaises(Tf.ErrorException):
            invalid = UsdGeom.HermiteCurves.PointAndTangentArrays(
                Vt.Vec3fArray([(1, 2, 3)]),
                Vt.Vec3fArray([(1, 2, 3), (4, 5, 6)]))
        empty = UsdGeom.HermiteCurves.PointAndTangentArrays()
        self.assertEqual(list(empty.GetPoints()), [])
        self.assertEqual(list(empty.GetTangents()), [])
        self.assertTrue(empty.IsEmpty())
        self.assertFalse(empty)

        pointsAndTangents = UsdGeom.HermiteCurves.PointAndTangentArrays(
            Vt.Vec3fArray([(1, 0, 0), (-1, 0, 0)]),
            Vt.Vec3fArray([(1, 2, 3), (4, 5, 6)]))
        self.assertTrue(pointsAndTangents)
        self.assertFalse(pointsAndTangents.IsEmpty())
        self.assertEqual(
            pointsAndTangents.GetPoints(),
            Vt.Vec3fArray([(1, 0, 0), (-1, 0, 0)]))
        self.assertEqual(
            pointsAndTangents.GetTangents(), 
            Vt.Vec3fArray([(1, 2, 3), (4, 5, 6)]))

        self.assertEqual(UsdGeom.HermiteCurves.PointAndTangentArrays(),
                         UsdGeom.HermiteCurves.PointAndTangentArrays())
        self.assertEqual(UsdGeom.HermiteCurves.PointAndTangentArrays(
                                Vt.Vec3fArray([2, 0, 0]),
                                Vt.Vec3fArray([1, 0, 0])),
                         UsdGeom.HermiteCurves.PointAndTangentArrays(
                                Vt.Vec3fArray([2, 0, 0]),
                                Vt.Vec3fArray([1, 0, 0])))
        self.assertNotEqual(UsdGeom.HermiteCurves.PointAndTangentArrays(),
                            UsdGeom.HermiteCurves.PointAndTangentArrays(
                                Vt.Vec3fArray([2, 0, 0]),
                                Vt.Vec3fArray([1, 0, 0])))

    def testInterleave(self):
        interleaved = UsdGeom.HermiteCurves.PointAndTangentArrays(
            Vt.Vec3fArray([(0, 0, 0), (2, 0, 0)]),
            Vt.Vec3fArray([(1, 0, 0), (0, 1, 0)])).Interleave()
        self.assertEqual(
            interleaved,
            Vt.Vec3fArray([(0, 0, 0), (1, 0, 0), (2, 0, 0), (0, 1, 0)]))
        self.assertEqual(
            UsdGeom.HermiteCurves.PointAndTangentArrays().Interleave(),
            Vt.Vec3fArray())

        empty = UsdGeom.HermiteCurves.PointAndTangentArrays().Interleave()
        self.assertEqual(list(empty), [])

    def testSeparate(self):
        with self.assertRaises(Tf.ErrorException):
            UsdGeom.HermiteCurves.PointAndTangentArrays.Separate(
                Vt.Vec3fArray([(0, 0, 0), (1, 0, 0), (2, 0, 0)]))

        separated = UsdGeom.HermiteCurves.PointAndTangentArrays.Separate(
            Vt.Vec3fArray([(0, 0, 0), (1, 0, 0), (2, 0, 0), (0, 1, 0)]))
        self.assertEqual(
            separated,
            UsdGeom.HermiteCurves.PointAndTangentArrays(
                Vt.Vec3fArray([(0, 0, 0), (2, 0, 0)]),
                Vt.Vec3fArray([(1, 0, 0), (0, 1, 0)])))

        empty = UsdGeom.HermiteCurves.PointAndTangentArrays.Separate(
            Vt.Vec3fArray())
        self.assertTrue(empty.IsEmpty())


if __name__ == '__main__':
    unittest.main()
