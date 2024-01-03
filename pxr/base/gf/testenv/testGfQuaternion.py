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
from __future__ import division

import sys, math
import unittest
from pxr import Gf

# Tuples of quaternion type, vec3 type, and closeVal
testClasses = (
    (Gf.Quaternion, Gf.Vec3d, 0.00001),
    (Gf.Quatd,      Gf.Vec3d, 0.00001),
    (Gf.Quatf,      Gf.Vec3f, 0.0001),
    (Gf.Quath,      Gf.Vec3h, 0.001),
)

class TestGfQuaternion(unittest.TestCase):

    def test_Constructors(self):
        for quatType, vec3Type, closeVal in testClasses:
            self.assertIsInstance(quatType(), quatType)
            self.assertIsInstance(quatType(0), quatType)
            self.assertIsInstance(quatType(1, vec3Type(1,1,1)), quatType)
            self.assertIsInstance(quatType.GetIdentity(), quatType)

            q = quatType(2)
            self.assertEqual(q.real, 2)
            self.assertEqual(q.imaginary, vec3Type(0, 0, 0))

            if quatType is not Gf.Quaternion:
                # This constructor is not supported by Gf.Quaternion
                q = quatType(1, 2, 3, 4)
                self.assertEqual(q.real, 1)
                self.assertEqual(q.imaginary, vec3Type(2, 3, 4))

            q = quatType(1, vec3Type(2, 3, 4))
            self.assertEqual(q.real, 1)
            self.assertEqual(q.imaginary, vec3Type(2, 3, 4))

            q = quatType(1, [2, 3, 4])
            self.assertEqual(q.real, 1)
            self.assertEqual(q.imaginary, vec3Type(2, 3, 4))

        # Testing conversions between Quat[h,f,d]
        self.assertIsInstance(Gf.Quath(Gf.Quatf()), Gf.Quath)
        self.assertIsInstance(Gf.Quath(Gf.Quatd()), Gf.Quath)
        self.assertIsInstance(Gf.Quatf(Gf.Quath()), Gf.Quatf)
        self.assertIsInstance(Gf.Quatf(Gf.Quatd()), Gf.Quatf)
        self.assertIsInstance(Gf.Quatd(Gf.Quath()), Gf.Quatd)
        self.assertIsInstance(Gf.Quatd(Gf.Quatf()), Gf.Quatd)


    def test_Properties(self):
        for quatType, vec3Type, closeVal in testClasses:
            q = quatType()
            q.real = 10
            self.assertEqual(q.real, 10)
            q.imaginary = vec3Type(1,2,3)
            self.assertEqual(q.imaginary, vec3Type(1,2,3))

    def test_Methods(self):
        for quatType, vec3Type, closeVal in testClasses:
            q = quatType()
            self.assertEqual(quatType.GetZero(), quatType(0))
            self.assertEqual(quatType.GetIdentity(), quatType(1, vec3Type()))

            self.assertTrue(quatType.GetZero().GetLength() == 0 and
                            quatType.GetIdentity().GetLength() == 1)
            self.assertTrue(Gf.IsClose(quatType(1,vec3Type(2,3,4)).GetLength(),
                                       5.4772255750516612, closeVal))
            
            q = quatType(1 ,vec3Type(2,3,4)).GetNormalized()
            self.assertTrue(Gf.IsClose(q.real, 0.182574, closeVal) and
                            Gf.IsClose(q.imaginary, vec3Type(0.365148, 0.547723, 0.730297),
                                       closeVal))

            q = quatType(1, vec3Type(2,3,4)).GetNormalized(10)
            self.assertEqual(q, quatType.GetIdentity())

            # Note that in C++, Normalize returns the length before normalization
            # but in python it returns the quaternion itself.
            q = quatType(1, vec3Type(2,3,4)).Normalize()
            self.assertTrue(Gf.IsClose(q.real, 0.182574, closeVal) and
                            Gf.IsClose(q.imaginary, vec3Type(0.365148, 0.547723, 0.730297),
                                       closeVal))

            q = quatType(1, vec3Type(2,3,4)).Normalize(10)
            self.assertEqual(q, quatType.GetIdentity())


            q = quatType.GetIdentity()
            self.assertEqual(q, q.GetInverse())
            q = quatType(1, vec3Type(1,2,3)).Normalize()
            (re, im) = (q.real, q.imaginary)
            self.assertTrue(Gf.IsClose(q.GetInverse().real, re, closeVal) and
                            Gf.IsClose(q.GetInverse().imaginary, -im, closeVal))

    def test_Operators(self):
        for quatType, vec3Type, closeVal in testClasses:
            q1 = quatType(1, vec3Type(2,3,4))
            q2 = quatType(1, vec3Type(2,3,4))
            self.assertEqual(q1, q2)
            self.assertFalse(q1 != q2)
            q2.real = 2
            self.assertTrue(q1 != q2)

            q = quatType(1, vec3Type(2,3,4)) * quatType.GetZero()
            self.assertEqual(q, quatType.GetZero())

            q = quatType(1, vec3Type(2,3,4)) * quatType.GetIdentity()
            self.assertEqual(q, quatType(1, vec3Type(2,3,4)))

            q = quatType(1, vec3Type(2,3,4))
            q *= quatType.GetZero()
            self.assertEqual(q, quatType.GetZero())

            q = quatType(1, vec3Type(2,3,4))
            q_original = q
            q *= quatType.GetIdentity()
            self.assertEqual(q, quatType(1, vec3Type(2,3,4)))
            self.assertTrue(q is q_original)

            q *= 10
            self.assertEqual(q, quatType(10, vec3Type(20,30,40)))
            self.assertTrue(q is q_original)
            q = q * 10
            self.assertEqual(q, quatType(100, vec3Type(200,300,400)))
            q = 10 * q
            self.assertEqual(q, quatType(1000, vec3Type(2000,3000,4000)))
            q_original = q
            q /= 100
            self.assertEqual(q, quatType(10, vec3Type(20,30,40)))
            self.assertTrue(q is q_original)
            q = q / 10
            self.assertEqual(q, quatType(1, vec3Type(2,3,4)))

            q_original = q
            q += q
            self.assertEqual(q, quatType(2, vec3Type(4,6,8)))
            self.assertTrue(q is q_original)

            q -= quatType(1, vec3Type(2,3,4))
            self.assertEqual(q, quatType(1, vec3Type(2,3,4)))
            self.assertTrue(q is q_original)

            q = q + q
            self.assertEqual(q, quatType(2, vec3Type(4,6,8)))

            q = q - quatType(1, vec3Type(2,3,4))
            self.assertEqual(q, quatType(1, vec3Type(2,3,4)))

            q = q * q
            self.assertEqual(q, quatType(-28, vec3Type(4, 6, 8)))

            q1 = quatType(1, vec3Type(2,3,4)).GetNormalized()
            q2 = quatType(4, vec3Type(3,2,1)).GetNormalized()
            self.assertEqual(Gf.Slerp(0, q1, q2), q1)
            self.assertEqual(Gf.Slerp(1, q1, q2), q2)
            q = Gf.Slerp(0.5, q1, q2)
            self.assertTrue(Gf.IsClose(q.real, 0.5, closeVal) and
                            Gf.IsClose(q.imaginary, vec3Type(0.5, 0.5, 0.5),
                                       closeVal))

            # code coverage goodness
            q1 = quatType(0, vec3Type(1,1,1))
            q2 = quatType(0, vec3Type(-1,-1,-1))
            q = Gf.Slerp(0.5, q1, q2)
            self.assertTrue(Gf.IsClose(q.real, 0, closeVal) and
                            Gf.IsClose(q.imaginary, vec3Type(1,1,1), closeVal))

            q1 = quatType(0, vec3Type(1,1,1))
            q2 = quatType(0, vec3Type(1,1,1))
            q = Gf.Slerp(0.5, q1, q2)
            self.assertTrue(Gf.IsClose(q.real, 0, closeVal) and 
                            Gf.IsClose(q.imaginary, vec3Type(1,1,1), closeVal))

            self.assertEqual(q, eval(repr(q)))

            self.assertTrue(len(str(quatType())))

            if quatType is Gf.Quaternion:
                # The remaining tests are not for Gf.Quaternion
                continue

            q1 = quatType(1, [2,3,4])
            q2 = quatType(2, [3,4,5])

            self.assertTrue(Gf.IsClose(Gf.Dot(q1, q2), 40, closeVal))

            # GetConjugate and Transform only exist on Quatd, Quatf, and Quath
            q = quatType(1, vec3Type(2, 3, 4)).GetConjugate()
            self.assertEqual(q, quatType(1, -vec3Type(2, 3, 4)))

            # q is a 90 degree rotation around Z axis.
            theta = math.radians(90)
            cosHalfTheta = math.cos(theta/2)
            sinHalfTheta = math.sin(theta/2)
            q = quatType(cosHalfTheta, sinHalfTheta * vec3Type(0, 0, 1))
            p = vec3Type(1.0, 0.0, 0.0)  # point on the x-axis

            r1 = (q * quatType(0, p) * q.GetInverse()).imaginary
            r2 = q.Transform(p)
            self.assertTrue(Gf.IsClose(r1, vec3Type(0.0, 1.0, 0.0), closeVal) and
                            Gf.IsClose(r1, r2, closeVal))

    def test_Hash(self):
        for QuatType, Vec3Type, _ in testClasses:
            q = QuatType(1.0, Vec3Type(2.0, 3.0, 4.0))
            self.assertEqual(hash(q), hash(q))
            self.assertEqual(hash(q), hash(QuatType(q)))

if __name__ == '__main__':
    unittest.main()
