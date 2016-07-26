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
import unittest
import math
from pxr import Gf

class TestGfHomogeneous(unittest.TestCase):

    def test_GetHomogenized(self):
        self.assertEqual(Gf.GetHomogenized(Gf.Vec4f(2, 4, 6, 2)), Gf.Vec4f(1, 2, 3, 1))
        self.assertEqual(Gf.GetHomogenized(Gf.Vec4f(1, 2, 3, 0)), Gf.Vec4f(1, 2, 3, 1))

        self.assertEqual(Gf.GetHomogenized(Gf.Vec4d(2, 4, 6, 2)), Gf.Vec4d(1, 2, 3, 1))
        self.assertEqual(Gf.GetHomogenized(Gf.Vec4d(1, 2, 3, 0)), Gf.Vec4d(1, 2, 3, 1))

    def test_GetHomogenizedCross(self):
        v1 = Gf.Vec4f(3, 1, 4, 1)
        v2 = Gf.Vec4f(5, 9, 2, 6)
        v3 = Gf.Vec3f(3, 1, 4)
        v4 = Gf.Vec3f(5./6, 9./6, 2./6)
        r = Gf.HomogeneousCross(v1, v2)
        result = Gf.Vec3f( r[0], r[1], r[2] )
        self.assertTrue(Gf.IsClose(result, v3 ^ v4, 0.00001))

        v1 = Gf.Vec4d(3, 1, 4, 1)
        v2 = Gf.Vec4d(5, 9, 2, 6)
        v3 = Gf.Vec3d(3, 1, 4)
        v4 = Gf.Vec3d(5./6, 9./6, 2./6)
        r = Gf.HomogeneousCross(v1, v2)
        result = Gf.Vec3d( r[0], r[1], r[2] )
        self.assertTrue(Gf.IsClose(result, v3 ^ v4, 0.00001))

    def test_Project(self):
        self.assertEqual(Gf.Project(Gf.Vec4d(2, 4, 6, 2)), Gf.Vec3d(1, 2, 3))
        self.assertEqual(Gf.Project(Gf.Vec4d(2, 4, 6, 0)), Gf.Vec3d(2, 4, 6))
        self.assertEqual(Gf.Project(Gf.Vec4f(2, 4, 6, 2)), Gf.Vec3f(1, 2, 3))
        self.assertEqual(Gf.Project(Gf.Vec4f(2, 4, 6, 0)), Gf.Vec3f(2, 4, 6))

if __name__ == '__main__':
    unittest.main()
