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

def err( msg ):
    return msg

class TestGfBBox3d(unittest.TestCase):

    def test_Constructors(self):
        self.assertIsInstance(Gf.Transform(), Gf.Transform), err("constructor")
        self.assertIsInstance(Gf.Transform(Gf.Vec3d(),
                                    Gf.Rotation(),
                                    Gf.Vec3d(),
                                    Gf.Vec3d(),
                                    Gf.Rotation()), Gf.Transform), err("constructor")

        # test GfMatrix4d constructor
        rotation = Gf.Rotation(Gf.Vec3d(0.5,0.6,0.7), 123)
        m = Gf.Matrix4d(1.0)
        m.SetRotate(rotation)
        t = Gf.Transform(m)
        tm = t.GetMatrix() 
        tol = 0.001
        self.assertTrue(Gf.IsClose(tm[0],m[0],tol) and \
            Gf.IsClose(tm[1],m[1],tol) and \
            Gf.IsClose(tm[2],m[2],tol) and \
            Gf.IsClose(tm[3],m[3],tol),
            err("GfTransform(GfMatrix4d) constructor"))

    def test_Properties(self):
        t = Gf.Transform()
        t.Set(Gf.Vec3d(2,3,4), Gf.Rotation(Gf.Vec3d(1,0,0), 90),
            Gf.Vec3d(1,2,3), Gf.Vec3d(), Gf.Rotation(Gf.Vec3d(1,1,1), 30))
        self.assertTrue(t.translation == Gf.Vec3d(2,3,4) and \
            t.rotation == Gf.Rotation(Gf.Vec3d(1,0,0), 90) and \
            t.scale == Gf.Vec3d(1,2,3) and \
            t.pivotPosition == Gf.Vec3d() and \
            t.pivotOrientation == Gf.Rotation(Gf.Vec3d(1,1,1), 30), err("Set"))
        self.assertEqual(eval(repr(t)), t)

        t = Gf.Transform()
        self.assertEqual(eval(repr(t)), t)
        t.SetMatrix(Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1), 30)))
        self.assertTrue(Gf.IsClose(t.rotation.axis, Gf.Vec3d(1,1,1).GetNormalized(), 0.0001) and \
            Gf.IsClose(t.rotation.angle, 30, 0.0001), err("SetMatrix"))
        self.assertEqual(eval(repr(t)), t)

        self.assertEqual(t.SetMatrix(Gf.Matrix4d(1)).GetMatrix(), Gf.Matrix4d(1), err("Get/SetMatrix"))

        t = Gf.Transform()
        t.Set(Gf.Vec3d(4,5,6), Gf.Rotation(Gf.Vec3d(1,0,0), 90), Gf.Vec3d(1,2,3),
            Gf.Vec3d(2,3,4), Gf.Rotation(Gf.Vec3d(1,1,1), 30))
        self.assertEqual(eval(repr(t)), t)
        m = t.GetMatrix()

        t = Gf.Transform()
        t.SetIdentity()
        self.assertEqual(eval(repr(t)), t)
        self.assertEqual(t.GetMatrix(), Gf.Matrix4d(1), err("Get/SetIdentity"))

        t.scale = Gf.Vec3d(1,2,3)
        self.assertEqual(eval(repr(t)), t)
        self.assertEqual(t.scale, Gf.Vec3d(1,2,3), err("scale"))

        t.pivotOrientation = Gf.Rotation(Gf.Vec3d.XAxis(), 30)
        self.assertEqual(eval(repr(t)), t)
        self.assertEqual(t.pivotOrientation, Gf.Rotation(Gf.Vec3d.XAxis(), 30), err("scaleOrientation"))

        t.pivotPosition = Gf.Vec3d(3,2,1)
        self.assertEqual(eval(repr(t)), t)
        self.assertEqual(t.pivotPosition, Gf.Vec3d(3,2,1), err("center"))

        t.translation = Gf.Vec3d(3,4,5)
        self.assertEqual(eval(repr(t)), t)
        self.assertEqual(t.translation, Gf.Vec3d(3,4,5), err("translation"))

        t.rotation = Gf.Rotation(Gf.Vec3d.YAxis(), 60)
        self.assertEqual(eval(repr(t)), t)
        self.assertEqual(t.rotation, Gf.Rotation(Gf.Vec3d.YAxis(), 60), err("rotation"))

        self.assertTrue(len(str(Gf.Transform())), err("str"))

    def test_Methods(self):
        t1 = Gf.Transform()
        t2 = Gf.Transform()
        t1.SetMatrix(Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1), 60)))
        t2.SetMatrix(Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1), 60)))
        self.assertEqual(eval(repr(t1)), t1)
        self.assertEqual(t1, t2, err("equality"))

        t2.SetMatrix(Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d.ZAxis(), 60)))
        self.assertNotEqual(t1, t2, err("inequality"))

        t1 = Gf.Transform()
        t2 = Gf.Transform()
        t1.SetMatrix(Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1), 60)))
        t2.rotation = Gf.Rotation(Gf.Vec3d.YAxis(), 60)
        t1 *= t2
        self.assertEqual(eval(repr(t1)), t1)
        self.assertTrue(Gf.IsClose(t1.rotation.axis, Gf.Vec3d(0.495572, 0.858356, 0.132788), 0.0001) and
            Gf.IsClose(t1.rotation.angle, 105.447, 0.0001), err("*="))

        t1 = Gf.Transform()
        t2 = Gf.Transform()
        t1.SetMatrix(Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1), 60)))
        t2.rotation = Gf.Rotation(Gf.Vec3d.YAxis(), 60)
        t3 = t1 * t2
        self.assertEqual(eval(repr(t3)), t3)
        self.assertTrue(Gf.IsClose(t3.rotation.axis, Gf.Vec3d(0.495572, 0.858356, 0.132788), 0.0001) and \
            Gf.IsClose(t3.rotation.angle, 105.447, 0.0001), err("*="))

        t1 = Gf.Transform()
        m = Gf.Matrix4d().SetScale(Gf.Vec3d(0,1,1))
        t1.SetMatrix(m)
        self.assertTrue(Gf.IsClose(t1.scale, Gf.Vec3d(0.0, 1, 1), 1e-4), err("SetMatrix"))

if __name__ == '__main__':
    unittest.main()
