#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
            self.assertEqual(extents[primpath], e)

    def test_ComputeExtentAndExtentFromPlugin(self):
        authoredExtentOnSphere = Vt.Vec3fArray(2, (Gf.Vec3f(-10.0, -10.0, -10.0),
                                            Gf.Vec3f(10.0, 10.0, 10.0)))
        explicitlyComputedExtent = Vt.Vec3fArray(2, (Gf.Vec3f(-2.0, -2.0, -2.0),
                                            Gf.Vec3f(2.0, 2.0, 2.0)))
        testFile = "test.usda"
        primPath = "/AuthoredExtentSphere"
        stage = Usd.Stage.Open(testFile)
        prim = stage.GetPrimAtPath(primPath)
        boundable = UsdGeom.Boundable(prim)
        tc = Usd.TimeCode.Default()
        extent1 = boundable.ComputeExtent(tc)
        self.assertEqual(extent1, authoredExtentOnSphere)
        extent2 = UsdGeom.Boundable.ComputeExtentFromPlugins(boundable, tc)
        self.assertEqual(extent2, explicitlyComputedExtent)

        # ComputeExtent uses geomtric properties instead of fallback extent when
        # no extent is authored
        primPath = "/sphere"
        prim = stage.GetPrimAtPath(primPath)
        boundable = UsdGeom.Boundable(prim)
        tc = Usd.TimeCode.Default()
        extent = boundable.ComputeExtent(tc)
        self.assertEqual(extent, explicitlyComputedExtent)


if __name__ == "__main__":
    unittest.main()
