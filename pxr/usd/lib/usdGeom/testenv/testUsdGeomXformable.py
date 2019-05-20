#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Gf, Tf, Sdf, Usd, UsdGeom, Vt
import unittest, math

class TestUsdGeomXformable(unittest.TestCase):
    def _AssertCloseXf(self, a, b):
        for av, bv in zip(a, b):
            self.assertTrue(Gf.IsClose(av, bv, 1e-4))

    def test_TranslateOp(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/World')
        translation = Gf.Vec3d(10., 20., 30.)
        x.AddTranslateOp().Set(translation)
        xform = x.GetLocalTransformation(Usd.TimeCode.Default())
        self._AssertCloseXf(xform, Gf.Matrix4d(1.0).SetTranslate(translation))
        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:translate', )))

    def test_ScaleOp(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/World')
        scaleVec = Gf.Vec3f(1., 2., 3.)
        x.AddScaleOp().Set(scaleVec)
        xform = x.GetLocalTransformation(Usd.TimeCode.Default())
        self._AssertCloseXf(xform, Gf.Matrix4d(1.0).SetScale(Gf.Vec3d(scaleVec)))
        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:scale', )))

    def test_ScalarRotateOps(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/X')
        x.AddRotateXOp().Set(45.)
        xformX = x.GetLocalTransformation(Usd.TimeCode.Default())
        self._AssertCloseXf(xformX, 
            Gf.Matrix4d(1.0, 0.0, 0.0, 0.0,
                        0.0, 0.7071067811865475, 0.7071067811865476, 0.0,
                        0.0, -0.7071067811865476, 0.7071067811865475, 0.0,
                        0.0, 0.0, 0.0, 1.0))
        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateX', )))

        y = UsdGeom.Xform.Define(s, '/Y')
        y.AddRotateYOp().Set(90.)
        xformY = y.GetLocalTransformation(Usd.TimeCode.Default())
        self._AssertCloseXf(xformY, 
            Gf.Matrix4d(0, 0.0, -1.0, 0.0,
                    0.0, 1.0, 0.0, 0.0,
                    1.0, 0.0, 0, 0.0,
                    0.0, 0.0, 0.0, 1.0))
        self.assertEqual(y.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateY', )))

        z = UsdGeom.Xform.Define(s, '/Z')
        z.AddRotateZOp().Set(30.)
        xformZ = z.GetLocalTransformation(Usd.TimeCode.Default())
        self._AssertCloseXf(xformZ, 
            Gf.Matrix4d(0.866025403784439, 0.5, 0, 0, 
                        -0.5, 0.866025403784439, 0, 0, 
                        0, 0, 1, 0, 
                        0, 0, 0, 1))
        self.assertEqual(z.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateZ', )))

        xy = UsdGeom.Xform.Define(s, '/XY')
        xy.AddRotateYOp().Set(90.)
        xy.AddRotateXOp().Set(45.)
        xformXY = xy.GetLocalTransformation(Usd.TimeCode.Default())
        self._AssertCloseXf(xformXY, 
            Gf.Matrix4d(0.0, 0.0, -1.0, 0.0,
                        0.7071067811865476, 0.7071067811865475, 0.0, 0.0,
                        0.7071067811865475, -0.7071067811865476, 0.0, 0.0,
                        0.0, 0.0, 0.0, 1.0))
        self.assertEqual(xy.GetXformOpOrderAttr().Get(),
                    Vt.TokenArray(('xformOp:rotateY', 'xformOp:rotateX')))

        yz = UsdGeom.Xform.Define(s, '/YZ')
        yz.AddRotateZOp().Set(30.)
        yz.AddRotateYOp().Set(90.)
        xformYZ = yz.GetLocalTransformation(Usd.TimeCode.Default())
        self._AssertCloseXf(xformYZ, 
            Gf.Matrix4d(0.0, 0.0, -1.0, 0.0,
                        -0.5, 0.8660254037844387, 0.0, 0.0,
                        0.8660254037844387, 0.5, 0.0, 0.0,
                        0.0, 0.0, 0.0, 1.0))
        self.assertEqual(yz.GetXformOpOrderAttr().Get(),
                    Vt.TokenArray(('xformOp:rotateZ', 'xformOp:rotateY')))

        zx = UsdGeom.Xform.Define(s, '/ZX')
        zx.AddRotateXOp().Set(45.)
        zx.AddRotateZOp().Set(30.)
        xformZX = zx.GetLocalTransformation(Usd.TimeCode.Default())
        self._AssertCloseXf(xformZX, 
            Gf.Matrix4d(0.8660254037844387, 0.3535533905932737, 0.35355339059327373, 0.0,
                        -0.5, 0.6123724356957945, 0.6123724356957946, 0.0,
                        0.0, -0.7071067811865476, 0.7071067811865475, 0.0,
                        0.0, 0.0, 0.0, 1.0))
        self.assertEqual(zx.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateX', 'xformOp:rotateZ')))

    def test_VectorRotateOps(self):
        s = Usd.Stage.CreateInMemory()
        rot = Gf.Vec3f(30., 45., 60.)

        # Rotation order XYZ
        xyz = UsdGeom.Xform.Define(s, '/XYZ')
        xyz.AddRotateXYZOp().Set(rot)
        xformXYZ = xyz.GetLocalTransformation(Usd.TimeCode.Default())
        self.assertEqual(xyz.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateXYZ', )))

        xyz2 = UsdGeom.Xform.Define(s, '/XYZ2')
        xyz2.AddRotateZOp().Set(rot[2])
        xyz2.AddRotateYOp().Set(rot[1])
        xyz2.AddRotateXOp().Set(rot[0])
        xformXYZ2 = xyz2.GetLocalTransformation(Usd.TimeCode.Default())
        self.assertEqual(xyz2.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateZ', 'xformOp:rotateY', 
                                   'xformOp:rotateX')))
        self._AssertCloseXf(xformXYZ, xformXYZ2)

        # Rotation order XZY
        xzy = UsdGeom.Xform.Define(s, '/XZY')
        xzy.AddRotateXZYOp().Set(rot)
        xformXZY = xzy.GetLocalTransformation(Usd.TimeCode.Default())
        self.assertEqual(xzy.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateXZY', )))

        xzy2 = UsdGeom.Xform.Define(s, '/XZY2')
        xzy2.AddRotateYOp().Set(rot[1])
        xzy2.AddRotateZOp().Set(rot[2])
        xzy2.AddRotateXOp().Set(rot[0])
        xformXZY2 = xzy2.GetLocalTransformation(Usd.TimeCode.Default())
        self.assertEqual(xzy2.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateY', 'xformOp:rotateZ', 
                                   'xformOp:rotateX')))
        self._AssertCloseXf(xformXZY, xformXZY2)

        # Rotation order YXZ
        yxz = UsdGeom.Xform.Define(s, '/YXZ')
        yxz.AddRotateYXZOp().Set(rot)
        xformYXZ = yxz.GetLocalTransformation(Usd.TimeCode.Default())
        self.assertEqual(yxz.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateYXZ', )))

        yxz2 = UsdGeom.Xform.Define(s, '/YXZ2')
        yxz2.AddRotateZOp().Set(rot[2])
        yxz2.AddRotateXOp().Set(rot[0])
        yxz2.AddRotateYOp().Set(rot[1])
        xformYXZ2 = yxz2.GetLocalTransformation(Usd.TimeCode.Default())
        self.assertEqual(yxz2.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateZ', 'xformOp:rotateX', 
                                   'xformOp:rotateY')))

        self._AssertCloseXf(xformYXZ, xformYXZ2)

        # Rotation order YZX
        yzx = UsdGeom.Xform.Define(s, '/YZX')
        yzx.AddRotateYZXOp().Set(rot)
        xformYZX = yzx.GetLocalTransformation(Usd.TimeCode.Default())
        self.assertEqual(yzx.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateYZX', )))

        yzx2 = UsdGeom.Xform.Define(s, '/YZX2')
        yzx2.AddRotateXOp().Set(rot[0])
        yzx2.AddRotateZOp().Set(rot[2])
        yzx2.AddRotateYOp().Set(rot[1])
        xformYZX2 = yzx2.GetLocalTransformation(Usd.TimeCode.Default())
        self.assertEqual(yzx2.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateX', 'xformOp:rotateZ', 
                                   'xformOp:rotateY')))

        self._AssertCloseXf(xformYZX, xformYZX2)

        # Rotation order ZXY
        zxy = UsdGeom.Xform.Define(s, '/ZXY')
        zxy.AddRotateZXYOp().Set(rot)
        xformZXY = zxy.GetLocalTransformation(Usd.TimeCode.Default())
        self.assertEqual(zxy.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateZXY', )))

        zxy2 = UsdGeom.Xform.Define(s, '/ZXY2')
        zxy2.AddRotateYOp().Set(rot[1])
        zxy2.AddRotateXOp().Set(rot[0])
        zxy2.AddRotateZOp().Set(rot[2])
        xformZXY2 = zxy2.GetLocalTransformation(Usd.TimeCode.Default())
        self.assertEqual(zxy2.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateY', 'xformOp:rotateX', 
                                   'xformOp:rotateZ')))

        self._AssertCloseXf(xformZXY, xformZXY2)

        # Rotation order ZYX
        zyx = UsdGeom.Xform.Define(s, '/ZYX')
        zyx.AddRotateZYXOp().Set(rot)
        xformZYX = zyx.GetLocalTransformation(Usd.TimeCode.Default())
        self.assertEqual(zyx.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateZYX', )))

        zyx2 = UsdGeom.Xform.Define(s, '/ZYX2')
        zyx2.AddRotateXOp().Set(rot[0])
        zyx2.AddRotateYOp().Set(rot[1])
        zyx2.AddRotateZOp().Set(rot[2])
        xformZYX2 = zyx2.GetLocalTransformation(Usd.TimeCode.Default())
        self.assertEqual(zyx2.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:rotateX', 'xformOp:rotateY', 
                                   'xformOp:rotateZ')))

        self._AssertCloseXf(xformZYX, xformZYX2)

    def test_PrestoRotatePivot(self):
        """ Test that simulates how the pivot position is taken into account in the 
            presto transformable prim with transformType=Vectors.
        """
        s = Usd.Stage.CreateInMemory()

        x = UsdGeom.Xform.Define(s, '/World')

        x.AddTranslateOp().Set(Gf.Vec3d(10., 0., 0.))

        # Use token for 'pivot'
        x.AddTranslateOp(opSuffix='pivot', isInverseOp=False).Set(Gf.Vec3d(0, 10, 0))

        x.AddRotateXYZOp().Set(Gf.Vec3f(60, 0, 30))
        
        x.AddScaleOp().Set(Gf.Vec3f(2,2,2))

        # Insert the inverse pivot. 
        inverseTranslateOp = x.AddTranslateOp(opSuffix='pivot', isInverseOp=True)

        # Calling set on an inverseOp results in a coding error. 

        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
                Vt.TokenArray(('xformOp:translate', 'xformOp:translate:pivot',
                               'xformOp:rotateXYZ', 'xformOp:scale',
                               '!invert!xformOp:translate:pivot')))

        xform = x.GetLocalTransformation(Usd.TimeCode.Default())

        self._AssertCloseXf(xform, 
                Gf.Matrix4d(1.7320508075688774, 1.0, 0.0, 0.0,
                            -0.5, 0.8660254037844389, 1.7320508075688772, 0.0,
                            0.8660254037844385, -1.5, 1.0, 0.0,
                            15.0, 1.339745962155611, -17.32050807568877, 1.0))

    def test_OrientOp(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/World')
        orientOp = x.AddOrientOp()

        orientOp.Set(Gf.Quatf(1, Gf.Vec3f(2, 3, 4)).GetNormalized())
        xform = x.GetLocalTransformation(Usd.TimeCode.Default())
        self._AssertCloseXf(xform,
                Gf.Matrix4d(-0.666666666666667,  0.66666666666667, 0.333333333333333, 0.0,
                             0.133333333333333, -0.33333333333333, 0.933333333333333, 0.0,
                             0.733333333333333,  0.66666666666666, 0.133333333333333, 0.0,
                             0.0, 0.0, 0.0, 1.0))

        # 90-degree on x-axis
        orientOp.Set(Gf.Quatf(0.7071067811865476, Gf.Vec3f(0.7071067811865475, 0, 0)))
        xform = x.GetLocalTransformation(Usd.TimeCode.Default())
        self._AssertCloseXf(xform,
                Gf.Matrix4d(1,  0,  0, 0,
                            0,  0,  1, 0,
                            0, -1,  0, 0,
                            0,  0,  0, 1))

        orientOp.Set(Gf.Quatf(0, Gf.Vec3f(0, 0, 0)))
        self._AssertCloseXf(x.GetLocalTransformation(Usd.TimeCode.Default()), Gf.Matrix4d(1.))

    def test_TransformOp(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/World')
        transformOp = x.AddTransformOp()

        xform = Gf.Matrix4d(2.0).SetTranslate(Gf.Vec3d(10, 20, 30))
        transformOp.Set(xform)
        self._AssertCloseXf(xform, x.GetLocalTransformation(Usd.TimeCode.Default()))
        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:transform', )))

        xformOp = x.MakeMatrixXform()
        self.assertEqual(xformOp.GetOpName(), "xformOp:transform")
        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('xformOp:transform', )))

        # Clear xformOpOrder
        x.ClearXformOpOrder()

        # Clearing opOrder does not remove the attribute.
        self.assertTrue(x.GetPrim().HasAttribute("xformOp:transform"))

        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray())

        self.assertTrue(x.SetXformOpOrder(orderedXformOps=[xformOp], resetXformStack=True))

        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('!resetXformStack!', 'xformOp:transform')))

    def test_ResetXformStack(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/World')
        x.AddTranslateOp().Set(Gf.Vec3d(20, 30, 40))

        x.SetResetXformStack(True)
        self.assertEqual(x.GetXformOpOrderAttr().Get(),
                    Vt.TokenArray(('!resetXformStack!', 'xformOp:translate')))
        # Calling it twice should have no effect the second time.
        x.SetResetXformStack(True)
        self.assertEqual(x.GetXformOpOrderAttr().Get(),
                    Vt.TokenArray(('!resetXformStack!', 'xformOp:translate')))

        x.SetResetXformStack(False)
        self.assertEqual(x.GetXformOpOrderAttr().Get(),
                    Vt.TokenArray(('xformOp:translate', )))

        # Again, calling this twice shouldn't make a difference.
        x.SetResetXformStack(False)
        self.assertEqual(x.GetXformOpOrderAttr().Get(),
                    Vt.TokenArray(('xformOp:translate', )))

        x.AddTransformOp().Set(Gf.Matrix4d(1.0))
        x.SetResetXformStack(True)
        self.assertEqual(x.GetXformOpOrderAttr().Get(),
                    Vt.TokenArray(('!resetXformStack!', 'xformOp:translate', 
                                   'xformOp:transform')))

        x.SetResetXformStack(False)
        self.assertEqual(x.GetXformOpOrderAttr().Get(),
                    Vt.TokenArray(('xformOp:translate', 
                                        'xformOp:transform')))

        cx = UsdGeom.Xform.Define(s, '/World/Model')
        cx.AddTranslateOp().Set(Gf.Vec3d(10, 10, 10))
        cache = UsdGeom.XformCache()
        cxCtm = cache.GetLocalToWorldTransform(cx.GetPrim())
        self._AssertCloseXf(cxCtm, Gf.Matrix4d(1.0).SetTranslate(Gf.Vec3d(30.0, 40.0, 50.0)))

        cx.SetResetXformStack(True)
        self.assertEqual(cx.GetXformOpOrderAttr().Get(),
            Vt.TokenArray(('!resetXformStack!', 'xformOp:translate')))

        # Clear the xform cache and recompute local-to-world xform.
        cache.Clear()

        newCxCtm = cache.GetLocalToWorldTransform(cx.GetPrim())
        localCxXform = cx.GetLocalTransformation(Usd.TimeCode.Default())
        self._AssertCloseXf(newCxCtm, Gf.Matrix4d(1.0).SetTranslate(Gf.Vec3d(10.0, 10.0, 10.0)))
        self._AssertCloseXf(newCxCtm, localCxXform)

        # Test resetXformStack when it's not at the beginning of xformOpOrder.
        cx.SetResetXformStack(False)
        newXformOpOrder = list(cx.GetXformOpOrderAttr().Get())
        newXformOpOrder.append(UsdGeom.XformOpTypes.resetXformStack)
        cx.GetXformOpOrderAttr().Set(newXformOpOrder)

        cx.AddTransformOp().Set(Gf.Matrix4d(2.0))
        self.assertTrue(cx.GetResetXformStack())
        
    def test_InverseOps(self):
        IDENTITY = Gf.Matrix4d(1.)

        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/World')
        x.AddTranslateOp().Set(Gf.Vec3d(20, 30, 40))
        x.AddTranslateOp(isInverseOp=True)
        self._AssertCloseXf(x.GetLocalTransformation(Usd.TimeCode.Default()), IDENTITY)

        x.AddScaleOp().Set(Gf.Vec3f(2,3,4))
        x.AddScaleOp(isInverseOp=True)
        self._AssertCloseXf(x.GetLocalTransformation(Usd.TimeCode.Default()), IDENTITY)

        x.AddRotateXOp().Set(30.)
        x.AddRotateXOp(isInverseOp=True)
        self._AssertCloseXf(x.GetLocalTransformation(Usd.TimeCode.Default()), IDENTITY)

        x.AddRotateYOp().Set(45.)
        x.AddRotateYOp(isInverseOp=True)
        self._AssertCloseXf(x.GetLocalTransformation(Usd.TimeCode.Default()), IDENTITY)

        x.AddRotateZOp().Set(60.)
        x.AddRotateZOp(isInverseOp=True)
        self._AssertCloseXf(x.GetLocalTransformation(Usd.TimeCode.Default()), IDENTITY)

        x.AddRotateXYZOp(opSuffix="firstRotate").Set(Gf.Vec3f(10, 20, 30))
        x.AddRotateXYZOp(opSuffix="firstRotate", isInverseOp=True)
        self._AssertCloseXf(x.GetLocalTransformation(Usd.TimeCode.Default()), IDENTITY)

        x.AddRotateZYXOp(opSuffix="lastRotate").Set(Gf.Vec3f(30, 60, 45))
        x.AddRotateZYXOp(opSuffix="lastRotate", isInverseOp=True)
        self._AssertCloseXf(x.GetLocalTransformation(Usd.TimeCode.Default()), IDENTITY)

        quat = Gf.Quatf(1, Gf.Vec3f(2, 3, 4))
        x.AddOrientOp().Set(quat)
        x.AddOrientOp(isInverseOp=True)
        self._AssertCloseXf(x.GetLocalTransformation(Usd.TimeCode.Default()), IDENTITY)

        rotation = Gf.Rotation(Gf.Vec3d(quat.GetImaginary()), quat.GetReal())
        x.AddTransformOp().Set(Gf.Matrix4d(rotation, Gf.Vec3d(10, 20, 30)))
        x.AddTransformOp(isInverseOp=True)
        self._AssertCloseXf(x.GetLocalTransformation(Usd.TimeCode.Default()), IDENTITY)

        # We've got tons of xform ops in x now, let's test GetOrderedXformOps API.
        orderedXformOps = x.GetOrderedXformOps()
        xformOpOrder = Vt.TokenArray(len(orderedXformOps))
        index = 0
        for op in orderedXformOps:
            xformOpOrder[index] = op.GetOpName()
            index += 1

        self.assertEqual(xformOpOrder, x.GetXformOpOrderAttr().Get())

    def test_AddExistingXformOp(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/World')
        xlateOp = x.AddTranslateOp()

        with self.assertRaises(RuntimeError):
            x.AddTranslateOp()

        # Adding an inverse op is OK, since it is considered to be separate from the 
        # original op.
        invTranslateOp = x.AddTranslateOp(isInverseOp=True)
        self.assertTrue(invTranslateOp)

        # Setting a value on an inverse op is not ok. 
        with self.assertRaises(RuntimeError):
            invTranslateOp.Set(Gf.Vec3d(1,1,1))

        scaleOp = x.AddScaleOp(precision=UsdGeom.XformOp.PrecisionDouble)
        with self.assertRaises(RuntimeError):
            invScaleOp = x.AddScaleOp(
                #precision=UsdGeom.XformOp.PrecisionFloat,  # this is the default
                isInverseOp=True)
        
    def test_SingularTransformOp(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/World')
        transformOp = x.AddTransformOp()
        singularMat = Gf.Matrix4d(32, 8, 11, 17, 
                                  8, 20, 17, 23,
                                  11, 17, 14, 26, 
                                  17, 23, 26, 2)
        transformOp.Set(singularMat, Usd.TimeCode(1.0))

        # Insert a translate op in the middle , as two consecutive inverse 
        # ops are simply skipped when computing local transform value.
        x.AddTranslateOp().Set(Gf.Vec3d(1,1,1))
        x.AddTransformOp(isInverseOp=True)

        with self.assertRaises(RuntimeError):
            xform = x.GetLocalTransformation(Usd.TimeCode(1.))

        # If the translateOp in the middle is removed from xformOpOrder, then 
        # calling GetLocalTransformation() should not result in an error as the pair
        # of consecutive inverse xformOps will get skipped.
        x.GetXformOpOrderAttr().Set(Vt.TokenArray(('xformOp:transform', 
            '!invert!xformOp:transform')))
        self.assertEqual(x.GetLocalTransformation(1.0), Gf.Matrix4d(1))

    def test_VaryingPrecisionOps(self):
        s = Usd.Stage.CreateInMemory()
        x1 = UsdGeom.Xform.Define(s, '/World')

        halfRotOp = x1.AddRotateXYZOp(precision=UsdGeom.XformOp.PrecisionHalf,
                                        opSuffix='Half')
        self.assertEqual(halfRotOp.GetPrecision(), UsdGeom.XformOp.PrecisionHalf)
        halfRotOp.Set(Gf.Vec3h(0.0, 0.0, 60.0))

        doubleRotOp = x1.AddRotateXYZOp(precision=UsdGeom.XformOp.PrecisionDouble,
                                        opSuffix='Double')
        self.assertEqual(doubleRotOp.GetPrecision(), UsdGeom.XformOp.PrecisionDouble)
        doubleRotOp.Set(Gf.Vec3d(0.0, 45.123456789, 0.0))

        floatRotOp = x1.AddRotateXYZOp(opSuffix='Float')
        self.assertEqual(floatRotOp.GetPrecision(), UsdGeom.XformOp.PrecisionFloat)
        floatRotOp.Set(Gf.Vec3f(30.0, 0.0, 0.0))

        self.assertEqual(x1.GetXformOpOrderAttr().Get(),
            Vt.TokenArray(('xformOp:rotateXYZ:Half',
                           'xformOp:rotateXYZ:Double', 
                           'xformOp:rotateXYZ:Float')))

        xform1 = x1.GetLocalTransformation(Usd.TimeCode.Default())

        x2 = UsdGeom.Xform.Define(s, '/World2')
        rotOp = x2.AddRotateXYZOp(precision=UsdGeom.XformOp.PrecisionDouble).Set(Gf.Vec3d(30.0, 45.123456789, 60.0))
        xform2 = x2.GetLocalTransformation(Usd.TimeCode.Default())

        # Everything gets converted to double internally, so the xforms computed 
        # must be equal.
        self.assertEqual(xform1, xform2)

        x3 = UsdGeom.Xform.Define(s, '/World3')
        quatd = Gf.Quatd(1., Gf.Vec3d(2., 3., 4.)).GetNormalized()
        quatf = Gf.Quatf(2., Gf.Vec3f(3., 4., 5.)).GetNormalized()
        quath = Gf.Quath(3., Gf.Vec3h(4., 5., 6.)).GetNormalized()

        defaultOrientOp = x3.AddOrientOp().Set(quatf)

        floatOrientOp = x3.AddOrientOp(precision=UsdGeom.XformOp.PrecisionFloat,
                                       opSuffix='Float')
        floatOrientOp.Set(quatf)

        doubleOrientOp = x3.AddOrientOp(precision=UsdGeom.XformOp.PrecisionDouble,
                                        opSuffix='Double')
        doubleOrientOp.Set(quatd)

        halfOrientOp = x3.AddOrientOp(precision=UsdGeom.XformOp.PrecisionHalf,
                                        opSuffix='Half')
        halfOrientOp.Set(quath)

        # Cannot set a quatd on an op that is of precision float.
        with self.assertRaises(RuntimeError):
            floatOrientOp.Set(quatd)

        xform3 = x3.GetLocalTransformation(Usd.TimeCode.Default())
        self._AssertCloseXf(xform3,
            Gf.Matrix4d(-0.27080552040616, -0.120257027227524, 0.955092988938746, 0,
                         0.95202181574436,  0.113459757051953, 0.284220593688289, 0,
                        -0.14254414216081,  0.986237867318078, 0.0837617848635551, 0,
                         0, 0, 0, 1))

    def _TestInvalidXformOp(self, attr):
        with self.assertRaises(RuntimeError):
            op = UsdGeom.XformOp(attr, isInverseOp=False)

    def test_InvalidXformOps(self):
        s = Usd.Stage.CreateInMemory()
        p = s.DefinePrim('/World', 'Xform')
        x = UsdGeom.Xformable(p)

        # Create an attribute that is not in the xformOp namespace.
        attr1 = p.CreateAttribute("myXformOp:transform", Sdf.ValueTypeNames.Matrix4d)
        self._TestInvalidXformOp(attr1)

        # Create an xform op with an invalid optype.
        attr2 = p.CreateAttribute("xformOp:translateXYZ", Sdf.ValueTypeNames.Double3)
        self._TestInvalidXformOp(attr2)

        # Create an xform op with opType=transform and typeName=Matrix4f.
        with self.assertRaises(RuntimeError):
            xformOp = x.AddTransformOp(precision=UsdGeom.XformOp.PrecisionFloat)

    def test_MightBeTimeVarying(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/World')
        translation = Gf.Vec3d(10., 20., 30.)
        xlateOp = x.AddTranslateOp()
        self.assertFalse(x.TransformMightBeTimeVarying())
        
        xlateOp.Set(translation)
        self.assertFalse(x.TransformMightBeTimeVarying())

        xlateOp.Set(translation + translation, Usd.TimeCode(1))
        self.assertFalse(x.TransformMightBeTimeVarying())
        # Test both overloads of TransformMightBeTimeVarying.
        self.assertFalse(x.TransformMightBeTimeVarying([xlateOp]))

        self.assertEqual(x.GetTimeSamples(), [1.0])
        xlateOp.Set(translation * 3, Usd.TimeCode(2))

        self.assertTrue(x.TransformMightBeTimeVarying())
        self.assertTrue(x.TransformMightBeTimeVarying([xlateOp]))
        self.assertEqual(x.GetTimeSamples(), [1.0, 2.0])

        x.GetXformOpOrderAttr().Set(Vt.TokenArray(('xformOp:translate', 
            '!resetXformStack!')))
        self.assertFalse(x.TransformMightBeTimeVarying())
        self.assertEqual(x.GetTimeSamples(), [])

        x.GetXformOpOrderAttr().Set(Vt.TokenArray(('!resetXformStack!',
            'xformOp:translate')))
        self.assertTrue(x.TransformMightBeTimeVarying())
        self.assertEqual(x.GetTimeSamples(), [1.0, 2.0])

    def test_GetTimeSamples(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/World')

        xlateOp = x.AddTranslateOp()
        scaleOp = x.AddScaleOp()

        self.assertEqual([], xlateOp.GetTimeSamples())
        self.assertEqual([], scaleOp.GetTimeSamples())

        xlate1 = Gf.Vec3d(10., 20., 30.)
        scale2 = Gf.Vec3f(1., 2., 3.)

        xlate3 = Gf.Vec3d(10., 20., 30.)
        scale4 = Gf.Vec3f(1., 2., 3.)

        self.assertEqual(x.GetTimeSamples(), [])

        xlateOp.Set(xlate1, Usd.TimeCode(1))
        self.assertEqual(x.GetTimeSamples(), [1.0])

        xlateOp.Set(xlate3, Usd.TimeCode(3))
        self.assertEqual(x.GetTimeSamples(), [1.0, 3.0])

        scaleOp.Set(scale2, Usd.TimeCode(2))
        self.assertEqual(x.GetTimeSamples(), [1.0, 2.0, 3.0])

        scaleOp.Set(scale4, Usd.TimeCode(4))
        self.assertEqual(x.GetTimeSamples(), [1.0, 2.0, 3.0, 4.0])

        self.assertEqual([1.0, 3.0], xlateOp.GetTimeSamples())
        self.assertEqual([2.0, 4.0], scaleOp.GetTimeSamples())

        self.assertEqual([3.0], 
                         xlateOp.GetTimeSamplesInInterval(Gf.Interval(2, 4)))
        self.assertEqual([2.0], 
                         scaleOp.GetTimeSamplesInInterval(Gf.Interval(0, 3)))

        self.assertEqual(x.GetTimeSamplesInInterval(Gf.Interval(1.5, 3.2)), 
                        [2.0, 3.0])

    def test_PureOvers(self):
        s = Usd.Stage.CreateInMemory()
        # Create a pure over prim and invoke various API.
        x = s.OverridePrim('/World')
        xf = UsdGeom.Xformable(x)
        xf.SetResetXformStack(True)
        xf.MakeMatrixXform().Set(Gf.Matrix4d(1.0))
        xf.SetXformOpOrder(xf.GetOrderedXformOps(), False)

    def test_Bug109853(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/World')

        # Test to make sure that having a non-existent xformOp in xformOpOrder
        # does not result in a crash when GetLocalTransformation is called.
        x.GetXformOpOrderAttr().Set(Vt.TokenArray(('xformOp:transform', )))

        # This used to crash before bug 109853 was fixed. It now results in a 
        # warning.
        x.GetLocalTransformation(Usd.TimeCode.Default())

if __name__ == "__main__":
    unittest.main()
