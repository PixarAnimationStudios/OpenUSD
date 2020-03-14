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

class TestGfRotation(unittest.TestCase):

    def test_Constructors(self):
        self.assertIsInstance(Gf.Rotation(), Gf.Rotation)
        self.assertIsInstance(Gf.Rotation(Gf.Vec3d(), 0), Gf.Rotation)
        self.assertIsInstance(Gf.Rotation(Gf.Quaternion()), Gf.Rotation)
        self.assertIsInstance(Gf.Rotation(Gf.Vec3d(), Gf.Vec3d()), Gf.Rotation)

        r = Gf.Rotation()
        r.SetAxisAngle(Gf.Vec3d(0,1,0), 30)
        self.assertEqual(eval(repr(r)), r)
        self.assertTrue(r.axis == Gf.Vec3d(0,1,0) and r.angle == 30)

        q = Gf.Quaternion(1, Gf.Vec3d(1,0,0)).GetNormalized()
        r = Gf.Rotation().SetQuaternion(q)
        self.assertEqual(eval(repr(r)), r)
        self.assertTrue(Gf.IsClose(r.axis, Gf.Vec3d(1,0,0), 0.00001) and
            Gf.IsClose(r.angle, 90, 0.00001))

        r = Gf.Rotation().SetRotateInto(Gf.Vec3d(1,0,0), Gf.Vec3d(0,1,0))
        self.assertEqual(eval(repr(r)), r)
        self.assertTrue(r.axis == Gf.Vec3d(0, 0, 1) and r.angle == 90)
        r = Gf.Rotation().SetRotateInto(Gf.Vec3d(1,0,0), Gf.Vec3d(1,0,0))
        self.assertEqual(eval(repr(r)), r)
        self.assertEqual(r, Gf.Rotation().SetIdentity())
        r = Gf.Rotation().SetRotateInto(Gf.Vec3d(1,0,0), Gf.Vec3d(-1,0,0))
        self.assertEqual(eval(repr(r)), r)

        r.SetIdentity()
        self.assertEqual(eval(repr(r)), r)
        self.assertTrue(r.axis == Gf.Vec3d(1,0,0) and r.angle == 0)

        r.axis = Gf.Vec3d(1,2,3)
        r.angle = 720
        self.assertEqual(eval(repr(r)), r)
        self.assertTrue(r.axis == Gf.Vec3d(1,2,3).GetNormalized() and r.angle == 720)

        q = Gf.Quaternion(1, Gf.Vec3d(1,0,0)).GetNormalized()
        r = Gf.Rotation().SetQuaternion(q)
        self.assertEqual(eval(repr(r)), r)
        qq = r.GetQuaternion()
        self.assertTrue(Gf.IsClose(q.real, qq.real, 0.00001) and
            Gf.IsClose(q.imaginary, qq.imaginary, 0.00001))

        r = Gf.Rotation(Gf.Vec3d(0,1,0), 720)
        self.assertEqual(eval(repr(r)), r)
        self.assertEqual(r.GetInverse(), Gf.Rotation(Gf.Vec3d(0,1,0), -720))

    def test_Decompose(self):
        r = Gf.Rotation(Gf.Vec3d(1,1,1), 30)
        self.assertEqual(eval(repr(r)), r)
        eulerAngles = r.Decompose(Gf.Vec3d.XAxis(), Gf.Vec3d.XAxis(), Gf.Vec3d.XAxis())
        eulerAngles = r.Decompose(Gf.Vec3d.XAxis(), Gf.Vec3d.YAxis(), Gf.Vec3d.ZAxis())
        self.assertTrue(Gf.IsClose(eulerAngles, Gf.Vec3d(15, 19.4712, 15), 0.0001))

        r = Gf.Rotation(Gf.Vec3d(1,0,0), 30)
        eulerAngles = r.Decompose(Gf.Vec3d.XAxis(), Gf.Vec3d.YAxis(), Gf.Vec3d.ZAxis())
        self.assertTrue(Gf.IsClose(eulerAngles, Gf.Vec3d(30, 0, 0), 0.0001))
        r = Gf.Rotation(Gf.Vec3d(1,0,0), 30)
        eulerAngles = r.Decompose(Gf.Vec3d.ZAxis(), Gf.Vec3d.XAxis(), Gf.Vec3d.YAxis())
        self.assertTrue(Gf.IsClose(eulerAngles, Gf.Vec3d(0, 30, 0), 0.0001))
        r = Gf.Rotation(Gf.Vec3d(1,0,0), 30)
        eulerAngles = r.Decompose(Gf.Vec3d.YAxis(), Gf.Vec3d.ZAxis(), Gf.Vec3d.XAxis())
        self.assertTrue(Gf.IsClose(eulerAngles, Gf.Vec3d(0, 0, 30), 0.0001))

        # Hits a numerical case in Rotation::Decompose
        v1 = Gf.Vec3d(1,0,0)
        v2 = Gf.Vec3d(0,1,0)
        v3 = Gf.Vec3d(0,0,1)
        r = Gf.Rotation(v2, 90)
        result = r.Decompose(v1, v2, v3)
        self.assertEqual(eval(repr(result)), result)
        self.assertTrue(Gf.IsClose(result, Gf.Vec3d(0, 90, -0), 0.00001))

    def test_TransformDir(self):
        r = Gf.Rotation(Gf.Vec3d(1,1,1), 30)
        dirf = Gf.Vec3f(3, 4, 5)
        dird = Gf.Vec3f(5, 4, 3)
        self.assertTrue(r.TransformDir(dirf) == Gf.Matrix4d().SetRotate(r).TransformDir(dirf) and
            r.TransformDir(dird) == Gf.Matrix4d().SetRotate(r).TransformDir(dird))

        self.assertEqual(r, eval(repr(r)))
        self.assertTrue(len(str(r)))

    def test_Operators(self):
        r1 = Gf.Rotation(Gf.Vec3d(1,1,1), 30)
        r2 = Gf.Rotation(Gf.Vec3d(1,1,1), 30)
        r3 = Gf.Rotation(Gf.Vec3d(1,0,0), 60)

        for r in [r1, r2, r3]:
            self.assertEqual(eval(repr(r)), r)

        self.assertTrue(r1 == r2 and r1 != r3)

        # coverage
        r1 = Gf.Rotation(Gf.Vec3d(0,0,0), 0)
        r1 *= r1

        r1 = Gf.Rotation(Gf.Vec3d(1,1,1), 30)
        r2 = Gf.Rotation(Gf.Vec3d(1,1,1), 60)
        r1 *= r2
        self.assertTrue(Gf.IsClose(r1.axis, Gf.Vec3d(1,1,1).GetNormalized(), 0.00001) and
            Gf.IsClose(r1.angle, 90, 0.00001))
        r1 *= 10
        self.assertTrue(Gf.IsClose(r1.axis, Gf.Vec3d(1,1,1).GetNormalized(), 0.00001) and
            Gf.IsClose(r1.angle, 900, 0.00001))
        r1 /= 10
        self.assertTrue(Gf.IsClose(r1.axis, Gf.Vec3d(1,1,1).GetNormalized(), 0.00001) and
            Gf.IsClose(r1.angle, 90, 0.00001))

        r1 = Gf.Rotation(Gf.Vec3d(1,1,1), 30)
        r2 = Gf.Rotation(Gf.Vec3d(1,1,1), 60)
        tmp = r1 * r2
        self.assertTrue(Gf.IsClose(tmp.axis, Gf.Vec3d(1,1,1).GetNormalized(), 0.00001) and
            Gf.IsClose(tmp.angle, 90, 0.00001))
        tmp = r1 * 10
        self.assertTrue(Gf.IsClose(tmp.axis, Gf.Vec3d(1,1,1).GetNormalized(), 0.00001) and \
            Gf.IsClose(tmp.angle, 300, 0.00001))
        tmp = 10 * r1
        self.assertTrue(Gf.IsClose(tmp.axis, Gf.Vec3d(1,1,1).GetNormalized(), 0.00001) and \
            Gf.IsClose(tmp.angle, 300, 0.00001))
        tmp = r1 / 10
        self.assertTrue(Gf.IsClose(tmp.axis, Gf.Vec3d(1,1,1).GetNormalized(), 0.00001) and \
            Gf.IsClose(tmp.angle, 3, 0.00001))
            
        #check for non commutativity
        r1 = Gf.Rotation(Gf.Vec3d(1,0,0), 45)
        r2 = Gf.Rotation(Gf.Vec3d(0,1,0), 45)
        r3 = r1 * r2
        r4 = r2 * r1
        self.assertEqual(r3.axis[0], r4.axis[0])
        self.assertEqual(r3.axis[1], r4.axis[1])
        self.assertEqual(r3.axis[2], -r4.axis[2])
        self.assertEqual(r3.angle, r4.angle)

        # check for associativity
        r6 = Gf.Rotation(Gf.Vec3d(0,0,1), 45)
        self.assertEqual((r1*r2)*r6, r1*(r2*r6))

        # Test that setting via a quaternion gives a valid repr.
        m = Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1), 30))
        r = m.ExtractRotation()
        self.assertEqual(eval(repr(r)), r)

    def test_RotateOntoProjected(self):
        axis = Gf.Vec3d(1, 2, 3)
        v1 = Gf.Vec3d(1, 2, 3)
        v2 = Gf.Vec3d(1, 0, 0)

        rot = Gf.Rotation.RotateOntoProjected(v1, v2, axis)
        self.assertEqual(rot.angle, 0)
        self.assertEqual(rot.axis, axis.GetNormalized())

        rot = Gf.Rotation.RotateOntoProjected(v2, v1, axis)
        self.assertEqual(rot.angle, 0)
        self.assertEqual(rot.axis, axis.GetNormalized())

        v1 = Gf.Vec3d(-2, 5, -7)
        v2 = Gf.Vec3d(1, 9, -5)
        rot = Gf.Rotation.RotateOntoProjected(v2, v1, axis)
        self.assertTrue(Gf.IsClose(rot.angle, 12.00206, 0.00001))
        self.assertEqual(rot.axis, axis.GetNormalized())

        rot = Gf.Rotation.RotateOntoProjected(v1, v2, axis)
        self.assertTrue(Gf.IsClose(rot.angle, -12.00206, 0.00001))
        self.assertEqual(rot.axis, axis.GetNormalized())

        v1 = Gf.Vec3d(3, -8, -6)
        v2 = Gf.Vec3d(10, -2, 7)
        rot = Gf.Rotation.RotateOntoProjected(v2, v1, axis)
        self.assertTrue(Gf.IsClose(rot.angle, 1.91983, 0.00001))
        self.assertEqual(rot.axis, axis.GetNormalized())

        rot = Gf.Rotation.RotateOntoProjected(v1, v2, axis)
        self.assertTrue(Gf.IsClose(rot.angle, -1.91983, 0.00001))
        self.assertEqual(rot.axis, axis.GetNormalized())

if __name__ == '__main__':
    unittest.main()
