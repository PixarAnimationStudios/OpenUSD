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

import sys, math
import unittest
from pxr import Gf

class TestGfQuaternion(unittest.TestCase):

    def test_Constructors(self):
        self.assertIsInstance(Gf.Quaternion(), Gf.Quaternion)
        self.assertIsInstance(Gf.Quaternion(0), Gf.Quaternion)
        self.assertIsInstance(Gf.Quaternion(1, Gf.Vec3d(1,1,1)), Gf.Quaternion)

    def test_Properties(self):
        q = Gf.Quaternion()
        q.real = 10
        self.assertEqual(q.real, 10)
        q.imaginary = Gf.Vec3d(1,2,3)
        self.assertEqual(q.imaginary, Gf.Vec3d(1,2,3))

    def test_Methods(self):
        q = Gf.Quaternion()
        self.assertEqual(Gf.Quaternion.GetIdentity(), Gf.Quaternion(1, Gf.Vec3d()))

        self.assertTrue(Gf.Quaternion.GetIdentity().GetLength() == 1 and
            Gf.IsClose(Gf.Quaternion(1,Gf.Vec3d(2,3,4)).GetLength(), 5.4772255750516612, 0.00001))
            

        q = Gf.Quaternion(1,Gf.Vec3d(2,3,4)).GetNormalized()
        self.assertTrue(Gf.IsClose(q.real, 0.182574, 0.00001) and
            Gf.IsClose(q.imaginary, Gf.Vec3d(0.365148, 0.547723, 0.730297), 0.00001))

        q = Gf.Quaternion(1,Gf.Vec3d(2,3,4)).GetNormalized(10)
        self.assertEqual(q, Gf.Quaternion.GetIdentity())

        q = Gf.Quaternion(1,Gf.Vec3d(2,3,4)).Normalize()
        self.assertTrue(Gf.IsClose(q.real, 0.182574, 0.00001) and
            Gf.IsClose(q.imaginary, Gf.Vec3d(0.365148, 0.547723, 0.730297), 0.00001))

        q = Gf.Quaternion(1,Gf.Vec3d(2,3,4)).Normalize(10)
        self.assertEqual(q, Gf.Quaternion.GetIdentity())


        q = Gf.Quaternion.GetIdentity()
        self.assertEqual(q, q.GetInverse())
        q = Gf.Quaternion(1,Gf.Vec3d(1,2,3)).Normalize()
        (re, im) = (q.real, q.imaginary)
        self.assertTrue(Gf.IsClose(q.GetInverse().real, re, 0.00001) and
            Gf.IsClose(q.GetInverse().imaginary, -im, 0.00001))

    def test_Operators(self):
        q1 = Gf.Quaternion(1,Gf.Vec3d(2,3,4))
        q2 = Gf.Quaternion(1,Gf.Vec3d(2,3,4))
        self.assertEqual(q1, q2)
        self.assertFalse(q1 != q2)
        q2.real = 2
        self.assertTrue(q1 != q2)

        q = Gf.Quaternion(1,Gf.Vec3d(2,3,4)) * Gf.Quaternion.GetIdentity()
        self.assertEqual(q, Gf.Quaternion(1,Gf.Vec3d(2,3,4)))

        q = Gf.Quaternion(1,Gf.Vec3d(2,3,4))
        q *= Gf.Quaternion.GetIdentity()
        self.assertEqual(q, Gf.Quaternion(1,Gf.Vec3d(2,3,4)))

        q *= 10
        self.assertEqual(q, Gf.Quaternion(10,Gf.Vec3d(20,30,40)))
        q = q * 10
        self.assertEqual(q, Gf.Quaternion(100,Gf.Vec3d(200,300,400)))
        q = 10 * q
        self.assertEqual(q, Gf.Quaternion(1000,Gf.Vec3d(2000,3000,4000)))
        q /= 100
        self.assertEqual(q, Gf.Quaternion(10,Gf.Vec3d(20,30,40)))
        q = q / 10
        self.assertEqual(q, Gf.Quaternion(1,Gf.Vec3d(2,3,4)))

        q += q
        self.assertEqual(q, Gf.Quaternion(2,Gf.Vec3d(4,6,8)))

        q -= Gf.Quaternion(1,Gf.Vec3d(2,3,4))
        self.assertEqual(q, Gf.Quaternion(1,Gf.Vec3d(2,3,4)))

        q = q + q
        self.assertEqual(q, Gf.Quaternion(2,Gf.Vec3d(4,6,8)))

        q = q - Gf.Quaternion(1,Gf.Vec3d(2,3,4))
        self.assertEqual(q, Gf.Quaternion(1,Gf.Vec3d(2,3,4)))

        q = q * q
        self.assertEqual(q, Gf.Quaternion(-28, Gf.Vec3d(4, 6, 8)))


        q1 = Gf.Quaternion(1,Gf.Vec3d(2,3,4)).GetNormalized()
        q2 = Gf.Quaternion(4,Gf.Vec3d(3,2,1)).GetNormalized()
        self.assertEqual(Gf.Slerp(0, q1, q2), q1)
        self.assertEqual(Gf.Slerp(1, q1, q2), q2)
        self.assertEqual(Gf.Slerp(0.5, q1, q2), Gf.Quaternion(0.5, Gf.Vec3d(0.5, 0.5, 0.5)))

        # code coverage goodness
        q1 = Gf.Quaternion(0, Gf.Vec3d(1,1,1))
        q2 = Gf.Quaternion(0, Gf.Vec3d(-1,-1,-1))
        q = Gf.Slerp(0.5, q1, q2)
        self.assertTrue(Gf.IsClose(q.real, 0, 0.0001) and
            Gf.IsClose(q.imaginary, Gf.Vec3d(1,1,1), 0.0001))

        q1 = Gf.Quaternion(0, Gf.Vec3d(1,1,1))
        q2 = Gf.Quaternion(0, Gf.Vec3d(1,1,1))
        q = Gf.Slerp(0.5, q1, q2)
        self.assertTrue(Gf.IsClose(q.real, 0, 0.0001) and 
            Gf.IsClose(q.imaginary, Gf.Vec3d(1,1,1), 0.0001))

        self.assertEqual(q, eval(repr(q)))

        self.assertTrue(len(str(Gf.Quaternion())))

if __name__ == '__main__':
    unittest.main()
