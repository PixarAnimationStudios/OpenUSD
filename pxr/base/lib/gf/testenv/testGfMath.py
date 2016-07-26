#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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
#

import sys
import math
import unittest
from pxr.Gf import *

def err( msg ):
    return "ERROR: " + msg + " failed"

class TestGfMath(unittest.TestCase):

    def _AssertListIsClose(self, first, second, delta=1e-6):
        self.assertTrue(len(first) == len(second))
        for (f,s) in zip(first, second):
            self.assertAlmostEqual(f, s, delta=delta)

    def test_HalfRoundTrip(self):
        from pxr.Gf import _HalfRoundTrip
        self.assertEqual(1.0, _HalfRoundTrip(1.0))
        self.assertEqual(1.0, _HalfRoundTrip(1))
        self.assertEqual(2.0, _HalfRoundTrip(2))
        self.assertEqual(3.0, _HalfRoundTrip(3L))

        with self.assertRaises(TypeError):
            _HalfRoundTrip([])

    def test_RadiansDegrees(self):
        self.assertEqual(0, RadiansToDegrees(0))
        self.assertEqual(180, RadiansToDegrees(math.pi))
        self.assertEqual(720, RadiansToDegrees(4 * math.pi))

        self.assertEqual(0, DegreesToRadians(0))
        self.assertEqual(math.pi, DegreesToRadians(180))
        self.assertEqual(8 * math.pi, DegreesToRadians(4 * 360))

    def test_Sqr(self):
        self.assertEqual(9, Sqr(3))
        self.assertAlmostEqual(Sqr(math.sqrt(2)), 2, delta=1e-7)
        self.assertEqual(5, Sqr(Vec2i(1,2)))
        self.assertEqual(14, Sqr(Vec3i(1,2,3)))
        self.assertEqual(5, Sqr(Vec2d(1,2)))
        self.assertEqual(14, Sqr(Vec3d(1,2,3)))
        self.assertEqual(30, Sqr(Vec4d(1,2,3,4)))
        self.assertEqual(5, Sqr(Vec2f(1,2)))
        self.assertEqual(14, Sqr(Vec3f(1,2,3)))
        self.assertEqual(30, Sqr(Vec4f(1,2,3,4)))

    def test_Sgn(self):
        self.assertEqual(-1, Sgn(-3))
        self.assertEqual(1, Sgn(3))
        self.assertEqual(0, Sgn(0))
        self.assertEqual(-1, Sgn(-3.3))
        self.assertEqual(1, Sgn(3.3))
        self.assertEqual(0, Sgn(0.0))

    def test_Sqrt(self):
        self.assertAlmostEqual( Sqrt(2), math.sqrt(2), delta=1e-5 )
        self.assertAlmostEqual( Sqrtf(2), math.sqrt(2), delta=1e-5 )
        
    def test_Exp(self):
        self.assertAlmostEqual( Sqr(math.e), Exp(2), delta=1e-5 )
        self.assertAlmostEqual( Sqr(math.e), Expf(2), delta=1e-5 )

    def test_Log(self):
        self.assertEqual(1, Log(math.e))
        self.assertAlmostEqual(Log(Exp(math.e)), math.e, delta=1e-5)
        self.assertAlmostEqual(Logf(math.e), 1, delta=1e-5)
        self.assertAlmostEqual(Logf(Expf(math.e)), math.e, delta=1e-5)

    def test_Floor(self):
        self.assertEqual(3, Floor(3.141))
        self.assertEqual(-4, Floor(-3.141))
        self.assertEqual(3, Floorf(3.141))
        self.assertEqual(-4, Floorf(-3.141))

    def test_Ceil(self):
        self.assertEqual(4, Ceil(3.141))
        self.assertEqual(-3, Ceil(-3.141))
        self.assertEqual(4, Ceilf(3.141))
        self.assertEqual(-3, Ceilf(-3.141))

    def test_Abs(self):
        self.assertAlmostEqual(Abs(-3.141), 3.141, delta=1e-6)
        self.assertAlmostEqual(Abs(3.141), 3.141, delta=1e-6)
        self.assertAlmostEqual(Absf(-3.141), 3.141, delta=1e-6)
        self.assertAlmostEqual(Absf(3.141), 3.141, delta=1e-6)

    def test_Round(self):
        self.assertEqual(3, Round(3.1))
        self.assertEqual(4, Round(3.5))
        self.assertEqual(-3, Round(-3.1))
        self.assertEqual(-4, Round(-3.6))
        self.assertEqual(3, Roundf(3.1))
        self.assertEqual(4, Roundf(3.5))
        self.assertEqual(-3, Roundf(-3.1))
        self.assertEqual(-4, Roundf(-3.6))

    def test_Pow(self):
        self.assertEqual(16, Pow(2, 4))
        self.assertEqual(16, Powf(2, 4))
           
    def test_Clamp(self):
        self.assertAlmostEqual(Clamp(3.141, 3.1, 3.2), 3.141, delta=1e-5)
        self.assertAlmostEqual(Clamp(2.141, 3.1, 3.2), 3.1, delta=1e-5)
        self.assertAlmostEqual(Clamp(4.141, 3.1, 3.2), 3.2, delta=1e-5)
        self.assertAlmostEqual(Clampf(3.141, 3.1, 3.2), 3.141, delta=1e-5)
        self.assertAlmostEqual(Clampf(2.141, 3.1, 3.2), 3.1, delta=1e-5)
        self.assertAlmostEqual(Clampf(4.141, 3.1, 3.2), 3.2, delta=1e-5)

    def test_Mod(self):
        self.assertEqual(2, Mod(5, 3))
        self.assertEqual(1, Mod(-5, 3))
        self.assertEqual(2, Modf(5, 3))
        self.assertEqual(1, Modf(-5, 3))

if __name__ == '__main__':
    unittest.main()
