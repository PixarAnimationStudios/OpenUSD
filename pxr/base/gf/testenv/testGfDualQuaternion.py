#!/pxrpythonsubst
#
# Copyright 2021 Pixar
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

# Tuples of dual quaternion type, vec3 type, and closeVal
testClasses = (
    (Gf.DualQuatd, Gf.Quatd, Gf.Vec3d, 0.00001),
    (Gf.DualQuatf, Gf.Quatf, Gf.Vec3f, 0.0001),
    (Gf.DualQuath, Gf.Quath, Gf.Vec3h, 0.002),
)

class TestGfDualQuaternion(unittest.TestCase):

    def test_Constructors(self):
        for dualQuatType, quatType, vec3Type, closeVal in testClasses:
            self.assertIsInstance(dualQuatType(), dualQuatType)
            self.assertIsInstance(dualQuatType(0), dualQuatType)
            self.assertIsInstance(dualQuatType(quatType(1, vec3Type(0))),
                                  dualQuatType)
            self.assertIsInstance(dualQuatType(quatType(1, vec3Type(0)),
                                               quatType(1, vec3Type(1, 1, 1))),
                                  dualQuatType)
            self.assertIsInstance(dualQuatType(quatType(1, vec3Type(0)),
                                               vec3Type(1, 1, 1)),
                                  dualQuatType)
            self.assertIsInstance(dualQuatType.GetIdentity(), dualQuatType)
            self.assertIsInstance(dualQuatType(dualQuatType()), dualQuatType)

            dq = dualQuatType(2)
            self.assertEqual(dq.real, quatType(2))
            self.assertEqual(dq.dual, quatType(0))

            dq = dualQuatType(quatType(1, 2, 3, 4))
            self.assertEqual(dq.real, quatType(1, 2, 3, 4))
            self.assertEqual(dq.dual, quatType(0))

            dq = dualQuatType(quatType(1, 2, 3, 4), quatType(5, 6, 7, 8))
            self.assertEqual(dq.real, quatType(1, 2, 3, 4))
            self.assertEqual(dq.dual, quatType(5, 6, 7, 8))

            dq = dualQuatType(quatType(1), vec3Type(1, 2, 3))
            self.assertEqual(dq.real, quatType(1))
            self.assertEqual(dq.dual.real, 0)
            self.assertTrue(Gf.IsClose(dq.dual.imaginary, vec3Type(0.5, 1.0, 1.5), closeVal))
            self.assertTrue(Gf.IsClose(dq.GetTranslation(), vec3Type(1, 2, 3), closeVal))

        # Testing conversions between DualQuat[h,f,d]
        self.assertIsInstance(Gf.DualQuath(Gf.DualQuatf()), Gf.DualQuath)
        self.assertIsInstance(Gf.DualQuath(Gf.DualQuatd()), Gf.DualQuath)
        self.assertIsInstance(Gf.DualQuatf(Gf.DualQuath()), Gf.DualQuatf)
        self.assertIsInstance(Gf.DualQuatf(Gf.DualQuatd()), Gf.DualQuatf)
        self.assertIsInstance(Gf.DualQuatd(Gf.DualQuath()), Gf.DualQuatd)
        self.assertIsInstance(Gf.DualQuatd(Gf.DualQuatf()), Gf.DualQuatd)

    def test_Properties(self):
        for dualQuatType, quatType, vec3Type, closeVal in testClasses:
            dq = dualQuatType()
            dq.real = quatType(1, vec3Type(0))
            self.assertEqual(dq.real, quatType(1, vec3Type(0)))
            dq.dual = quatType(2, vec3Type(0))
            self.assertEqual(dq.dual, quatType(2, vec3Type(0)))

    def test_Methods(self):
        for dualQuatType, quatType, vec3Type, closeVal in testClasses:
            dq = dualQuatType()
            self.assertEqual(dualQuatType.GetZero(), dualQuatType(0))
            self.assertEqual(dualQuatType.GetIdentity(), dualQuatType(1))

            # Can not convert std::pair<pxr_half::half, pxr_half::half> to python. Skip it for now.
            if dualQuatType is not Gf.DualQuath:
                self.assertTrue(dualQuatType.GetZero().GetLength() == (0, 0) and
                                dualQuatType.GetIdentity().GetLength() == (1, 0) and
                                Gf.IsClose(dualQuatType(quatType(1,vec3Type(2,3,4))).GetLength(),
                                           (5.4772255750516612, 0), closeVal))

            dq = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8))).GetNormalized()
            (re, du) = (dq.real, dq.dual)
            self.assertTrue(Gf.IsClose(re.real, 0.182574, closeVal) and
                            Gf.IsClose(re.imaginary, vec3Type(0.365148, 0.547723, 0.730297), closeVal) and
                            Gf.IsClose(du.real, 0.486864, closeVal) and
                            Gf.IsClose(du.imaginary, vec3Type(0.243432, 4.440892e-16, -0.243432), closeVal))

            dq = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8))).GetNormalized(10)
            self.assertEqual(dq, dualQuatType.GetIdentity())

            dq = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8))).Normalize()
            (re, du) = (dq.real, dq.dual)
            self.assertTrue(Gf.IsClose(re.real, 0.182574, closeVal) and
                            Gf.IsClose(re.imaginary, vec3Type(0.365148, 0.547723, 0.730297), closeVal) and
                            Gf.IsClose(du.real, 0.486864, closeVal) and
                            Gf.IsClose(du.imaginary, vec3Type(0.243432, 4.440892e-16, -0.243432), closeVal))

            dq = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8))).Normalize(10)
            self.assertEqual(dq, dualQuatType.GetIdentity())

            dq = dualQuatType.GetIdentity()
            self.assertEqual(dq, dq.GetInverse())
            dq = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8))).GetInverse()
            (re, du) = (dq.real, dq.dual)
            self.assertTrue(Gf.IsClose(re.real, 0.033333, closeVal) and
                            Gf.IsClose(re.imaginary, vec3Type(-0.066667, -0.1, -0.133333), closeVal) and
                            Gf.IsClose(du.real, 0.011111, closeVal) and
                            Gf.IsClose(du.imaginary, vec3Type(0.111111, 0.233333, 0.355556), closeVal))

            dq = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8))).GetNormalized()
            self.assertTrue(Gf.IsClose(dq.GetTranslation(), vec3Type(-0.533333, 0, -1.066666), closeVal))
            dq.SetTranslation(vec3Type(1,1,1))
            self.assertTrue(Gf.IsClose(dq.GetTranslation(), vec3Type(1,1,1), closeVal))

    def test_Operators(self):
        for dualQuatType, quatType, vec3Type, closeVal in testClasses:
            dq1 = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8)))
            dq2 = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8)))
            self.assertEqual(dq1, dq2)
            self.assertFalse(dq1 != dq2)
            dq2.real = quatType(2,vec3Type(2,3,4))
            self.assertTrue(dq1 != dq2)

            dq = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8)))
            dq += dq
            self.assertEqual(dq, dualQuatType(quatType(2,vec3Type(4,6,8)), quatType(10,vec3Type(12,14,16))))

            dq -= dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8)))
            self.assertEqual(dq, dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8))))

            dq *= dualQuatType.GetIdentity()
            self.assertEqual(dq, dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8))))

            dq *= dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8)))
            self.assertEqual(dq, dualQuatType(quatType(-28,vec3Type(4,6,8)), quatType(-120,vec3Type(32,44,56))))

            dq = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8)))
            dq *= 10
            self.assertEqual(dq, dualQuatType(quatType(10,vec3Type(20,30,40)), quatType(50,vec3Type(60,70,80))))

            dq /= 10
            self.assertEqual(dq, dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8))))

            dq = dq + dq
            self.assertEqual(dq, dualQuatType(quatType(2,vec3Type(4,6,8)), quatType(10,vec3Type(12,14,16))))

            dq = dq - dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8)))
            self.assertEqual(dq, dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8))))

            dq = dq * dq
            self.assertEqual(dq, dualQuatType(quatType(-28,vec3Type(4,6,8)), quatType(-120,vec3Type(32,44,56))))

            dq = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8)))
            dq = dq * 10
            self.assertEqual(dq, dualQuatType(quatType(10,vec3Type(20,30,40)), quatType(50,vec3Type(60,70,80))))

            dq = 0.5 * dq
            self.assertEqual(dq, dualQuatType(quatType(5,vec3Type(10,15,20)), quatType(25,vec3Type(30,35,40))))

            dq = dq / 5
            self.assertEqual(dq, dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8))))

            # code coverage goodness
            self.assertEqual(dq, eval(repr(dq)))

            self.assertTrue(len(str(dualQuatType())))

            dq1 = dualQuatType(quatType(1, [2,3,4]), quatType(1, [2,3,4]))
            dq2 = dualQuatType(quatType(2, [3,4,5]), quatType(2, [3,4,5]))
            self.assertTrue(Gf.IsClose(Gf.Dot(dq1, dq2), 80, closeVal))

            dq = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8)))
            self.assertEqual(dq, dq.GetConjugate().GetConjugate())
            (re, du) = (dq.real, dq.dual)
            (cre, cdu) = (dq.GetConjugate().real, dq.GetConjugate().dual)
            self.assertTrue(Gf.IsClose(cre.real, re.GetConjugate().real, closeVal) and
                            Gf.IsClose(cre.imaginary, re.GetConjugate().imaginary, closeVal) and
                            Gf.IsClose(cdu.real, du.GetConjugate().real, closeVal) and
                            Gf.IsClose(cdu.imaginary, du.GetConjugate().imaginary, closeVal))

            dq = dualQuatType(quatType(1,vec3Type(2,3,4))).GetNormalized()
            self.assertEqual(dq.Transform(vec3Type(0)), vec3Type(0))
            dq = dualQuatType(quatType(1,vec3Type(2,3,4)), quatType(5,vec3Type(6,7,8))).GetNormalized()
            self.assertEqual(dq.Transform(vec3Type(0)), dq.GetTranslation())
            self.assertTrue(Gf.IsClose(dq.Transform(vec3Type(1,1,1)), vec3Type(-0.333333,1,0.333333), closeVal) and
                            Gf.IsClose(dq.Transform(vec3Type(1,0,0)), vec3Type(-1.2,0.666667,-0.733333), closeVal) and
                            Gf.IsClose(dq.Transform(vec3Type(0,1,0)), vec3Type(-0.4,-0.333333,-0.133333), closeVal) and
                            Gf.IsClose(dq.Transform(vec3Type(0,0,1)), vec3Type(0.2,0.666667,-0.933333), closeVal))

    def test_Hash(self):
        for DualQuatType, QuatType, Vec3Type, _ in testClasses:
            dq = DualQuatType(
                QuatType(1.0, Vec3Type(2.0, 3.0, 4.0)),
                QuatType(2.0, Vec3Type(3.0, 4.0, 5.0))
            )
            self.assertEqual(hash(dq), hash(dq))
            self.assertEqual(hash(dq), hash(DualQuatType(dq)))

if __name__ == '__main__':
    unittest.main()
