#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

from pxr import Gf, Vt, Usd, UsdGeom
import unittest

class TestUsdGeomExtentFromPlugins(unittest.TestCase):
    primpaths = [ '/capsule', '/cone', '/cube', '/cylinder', '/sphere' ]

    def test_Default(self):
        extents = {
            '/capsule' :  Vt.Vec3fArray(2, (Gf.Vec3f(-2.0, -2.0, -3.0),
                                            Gf.Vec3f(2.0, 2.0, 3.0))),
            '/cone' :     Vt.Vec3fArray(2, (Gf.Vec3f(-2.0, -2.0, -2.0),
                                            Gf.Vec3f(2.0, 2.0, 2.0))),
            '/cube' :     Vt.Vec3fArray(2, (Gf.Vec3f(-2.0, -2.0, -2.0),
                                            Gf.Vec3f(2.0, 2.0, 2.0))),
            '/cylinder' : Vt.Vec3fArray(2, (Gf.Vec3f(-2.0, -2.0, -2.0),
                                            Gf.Vec3f(2.0, 2.0, 2.0))),
            '/sphere' :   Vt.Vec3fArray(2, (Gf.Vec3f(-2.0, -2.0, -2.0),
                                            Gf.Vec3f(2.0, 2.0, 2.0)))
        }
        testFile = "test.usda"
        s = Usd.Stage.Open(testFile)
        for primpath in self.primpaths:
            p = s.GetPrimAtPath(primpath)
            b = UsdGeom.Boundable(p)
            tc = Usd.TimeCode.Default()
            e = UsdGeom.Boundable.ComputeExtentFromPlugins(b, tc)
            print primpath, e
            self.assertEqual(extents[primpath], e)

    def test_TimeSampled(self):
        extents = {
            '/capsule' :  Vt.Vec3fArray(2, (Gf.Vec3f(-4.0, -4.0, -6.0),
                                            Gf.Vec3f(4.0, 4.0, 6.0))),
            '/cone' :     Vt.Vec3fArray(2, (Gf.Vec3f(-4.0, -4.0, -3.0),
                                            Gf.Vec3f(4.0, 4.0, 3.0))),
            '/cube' :     Vt.Vec3fArray(2, (Gf.Vec3f(-3.0, -3.0, -3.0),
                                            Gf.Vec3f(3.0, 3.0, 3.0))),
            '/cylinder' : Vt.Vec3fArray(2, (Gf.Vec3f(-4.0, -4.0, -3.0),
                                            Gf.Vec3f(4.0, 4.0, 3.0))),
            '/sphere' :   Vt.Vec3fArray(2, (Gf.Vec3f(-4.0, -4.0, -4.0),
                                            Gf.Vec3f(4.0, 4.0, 4.0)))
        }
        testFile = "test.usda"
        s = Usd.Stage.Open(testFile)
        for primpath in self.primpaths:
            p = s.GetPrimAtPath(primpath)
            b = UsdGeom.Boundable(p)
            tc = Usd.TimeCode(2.0)
            e = UsdGeom.Boundable.ComputeExtentFromPlugins(b, tc)
            print primpath, e
            self.assertEqual(extents[primpath], e)

if __name__ == "__main__":
    unittest.main()
