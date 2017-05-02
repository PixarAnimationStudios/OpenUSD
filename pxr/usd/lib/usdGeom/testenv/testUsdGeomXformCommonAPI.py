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

from pxr import Gf, Tf, Usd, UsdGeom, Vt
import unittest, math

class TestUsdGeomXformAPI(unittest.TestCase):
    def test_EmptyXformable(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, "/X")
        xXformAPI = UsdGeom.XformCommonAPI(x)
        self.assertTrue(xXformAPI)

        self.assertEqual(xXformAPI.GetXformVectors(Usd.TimeCode.Default()), 
                    (Gf.Vec3d(0,0,0), Gf.Vec3f(0,0,0), 
                     Gf.Vec3f(1,1,1), Gf.Vec3f(0,0,0), 
                     UsdGeom.XformCommonAPI.RotationOrderXYZ))

        self.assertTrue(xXformAPI.SetXformVectors(
            translation=Gf.Vec3d(10., 20., 30.), 
            rotation=Gf.Vec3f(30, 45, 60), 
            scale=Gf.Vec3f(1., 2., 3.), 
            pivot=Gf.Vec3f(0, 10, 0), 
            rotationOrder=
            UsdGeom.XformCommonAPI.RotationOrderYXZ,
            time=Usd.TimeCode.Default()))

        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(('xformOp:translate', 'xformOp:translate:pivot',
                           'xformOp:rotateYXZ', 'xformOp:scale',
                           '!invert!xformOp:translate:pivot')))
        self.assertTrue(xXformAPI)

        self.assertEqual(xXformAPI.GetXformVectors(Usd.TimeCode.Default()), 
                    (Gf.Vec3d(10., 20., 30.), Gf.Vec3f(30, 45, 60), 
                     Gf.Vec3f(1., 2., 3.), Gf.Vec3f(0, 10, 0), 
                     UsdGeom.XformCommonAPI.RotationOrderYXZ))

        # Call SetXformVectors with a different rotation order. This should fail 
        # and no values should get authored.
        with self.assertRaises(RuntimeError):
            xXformAPI.SetXformVectors(translation=Gf.Vec3d(100., 200., 300.),                                          
                                      rotation=Gf.Vec3f(3, 4, 6),                                          
                                      scale=Gf.Vec3f(3., 2., 1.),
                                      pivot=Gf.Vec3f(10, 0, 10), 
                                      rotationOrder= UsdGeom.XformCommonAPI.RotationOrderZYX, 
                                      time=Usd.TimeCode(10.0))

        # Verify that the second SetXformVectors did not author any values.
        self.assertEqual(xXformAPI.GetXformVectors(Usd.TimeCode(10.0)), 
                    (Gf.Vec3d(10., 20., 30.), Gf.Vec3f(30, 45, 60), 
                     Gf.Vec3f(1., 2., 3.), Gf.Vec3f(0, 10, 0), 
                     UsdGeom.XformCommonAPI.RotationOrderYXZ))

        # Adding an extra op, causes X to become incompatible.
        x.AddTranslateOp(opSuffix="extraTranslate")
        self.assertFalse(UsdGeom.XformCommonAPI(x))

    def test_SetIndividualOps(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, "/X")
        # Test the Get() method.
        xformAPI = UsdGeom.XformCommonAPI.Get(s, "/X")
        self.assertTrue(xformAPI)

        xformAPI.SetTranslate(Gf.Vec3d(2,3,4))
        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(('xformOp:translate', )))

        # The call to ClearXformOpOrder invalides the API object.
        x.ClearXformOpOrder()

        # Recreate XformCommonAPI object
        xformAPI = UsdGeom.XformCommonAPI(x)

        self.assertTrue(xformAPI)

        # Test Get with no values authored.
        self.assertEqual(xformAPI.GetXformVectors(time=Usd.TimeCode(1.)),
                    (Gf.Vec3d(0., 0., 0.), 
                     Gf.Vec3f(0., 0., 0.), 
                     Gf.Vec3f(1., 1., 1.),
                     Gf.Vec3f(0., 0., 0.),
                     UsdGeom.XformCommonAPI.RotationOrderXYZ) )

        xformAPI.SetRotate(Gf.Vec3f(30, 45, 60))

        self.assertTrue(xformAPI)

        # Call SetRotate with a different rotationOrder. This should result in a 
        # coding error being issued.
        with self.assertRaises(RuntimeError): 
            xformAPI.SetRotate(Gf.Vec3f(30, 45, 60), 
                               UsdGeom.XformCommonAPI.RotationOrderZYX)

        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(('xformOp:rotateXYZ', )))

        self.assertEqual(xformAPI.GetXformVectors(time=Usd.TimeCode(1.)),
                    (Gf.Vec3d(0., 0., 0.), 
                     Gf.Vec3f(30.0, 45.0, 60.0), 
                     Gf.Vec3f(1., 1., 1.),
                     Gf.Vec3f(0., 0., 0.),
                     UsdGeom.XformCommonAPI.RotationOrderXYZ) )

        xformAPI.SetTranslate(Gf.Vec3d(20,30,40))
        self.assertTrue(xformAPI)

        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(('xformOp:translate', 'xformOp:rotateXYZ')))

        self.assertEqual(xformAPI.GetXformVectors(time=Usd.TimeCode(1.)),
                    (Gf.Vec3d(20.0, 30.0, 40.0), 
                     Gf.Vec3f(30.0, 45.0, 60.0), 
                     Gf.Vec3f(1., 1., 1.),
                     Gf.Vec3f(0., 0., 0.),
                     UsdGeom.XformCommonAPI.RotationOrderXYZ) )

        xformAPI.SetPivot(Gf.Vec3f(100, 200, 300), Usd.TimeCode(1.))
        self.assertTrue(xformAPI)

        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(('xformOp:translate', 'xformOp:translate:pivot',
                           'xformOp:rotateXYZ', 
                           '!invert!xformOp:translate:pivot')))

        self.assertEqual(xformAPI.GetXformVectors(time=Usd.TimeCode(1.)),
                    (Gf.Vec3d(20.0, 30.0, 40.0), 
                     Gf.Vec3f(30.0, 45.0, 60.0), 
                     Gf.Vec3f(1., 1., 1.),
                     Gf.Vec3f(100., 200., 300.),
                     UsdGeom.XformCommonAPI.RotationOrderXYZ) )

        xformAPI.SetScale(Gf.Vec3f(1.5, 2.0, 4.5), Usd.TimeCode(2.))
        self.assertTrue(xformAPI)

        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(('xformOp:translate', 'xformOp:translate:pivot',
                           'xformOp:rotateXYZ', 'xformOp:scale',
                           '!invert!xformOp:translate:pivot')))

        self.assertEqual(xformAPI.GetXformVectors(time=Usd.TimeCode(1.)),
                    (Gf.Vec3d(20.0, 30.0, 40.0), 
                     Gf.Vec3f(30.0, 45.0, 60.0), 
                     Gf.Vec3f(1.5, 2.0, 4.5),
                     Gf.Vec3f(100., 200., 300.),
                     UsdGeom.XformCommonAPI.RotationOrderXYZ) )

    def test_IncompatibleXformables(self):
        s = Usd.Stage.CreateInMemory()
        orient = UsdGeom.Xform.Define(s, "/Orient")
        orient.AddOrientOp()
        self.assertFalse(UsdGeom.XformCommonAPI(orient))

        rotX = UsdGeom.Xform.Define(s, "/RotX")
        rotX.AddRotateXOp()
        self.assertFalse(UsdGeom.XformCommonAPI(rotX))

        rotY = UsdGeom.Xform.Define(s, "/RotY")
        rotY.AddRotateYOp()
        self.assertFalse(UsdGeom.XformCommonAPI(rotY))

        rotZ = UsdGeom.Xform.Define(s, "/RotZ")
        rotZ.AddRotateZOp()
        self.assertFalse(UsdGeom.XformCommonAPI(rotZ))

        matrix = UsdGeom.Xform.Define(s, "/Matrix")
        matrix.MakeMatrixXform()
        self.assertFalse(UsdGeom.XformCommonAPI(matrix))

        badOpOrder1 = UsdGeom.Xform.Define(s, "/BadOpOrder1")
        # Add Scale before rotate
        badOpOrder1.AddScaleOp()
        badOpOrder1.AddRotateZXYOp()
        self.assertFalse(UsdGeom.XformCommonAPI(badOpOrder1))

        badOpOrder2 = UsdGeom.Xform.Define(s, "/BadOpOrder2")
        # Add Rotate before Translate
        badOpOrder2.AddRotateYZXOp()
        badOpOrder2.AddTranslateOp()
        self.assertFalse(UsdGeom.XformCommonAPI(badOpOrder2))

        badOpOrder3 = UsdGeom.Xform.Define(s, "/BadOpOrder3")
        # Add Scale before Translate
        badOpOrder3.AddScaleOp()
        badOpOrder3.AddTranslateOp()
        self.assertFalse(UsdGeom.XformCommonAPI(badOpOrder3))

        badOpOrder4 = UsdGeom.Xform.Define(s, "/BadOpOrder4")
        # Add Scale outside the (pivot, invPivot()
        UsdGeom.XformCommonAPI(badOpOrder4).SetPivot(Gf.Vec3f(10, 20, 30))
        badOpOrder4.AddScaleOp()
        self.assertFalse(UsdGeom.XformCommonAPI(badOpOrder4))

        badOpOrder5 = UsdGeom.Xform.Define(s, "/BadOpOrder5")
        # Add Rotate outside the (pivot, invPivot()
        UsdGeom.XformCommonAPI(badOpOrder5).SetPivot(Gf.Vec3f(10, 20, 30))
        badOpOrder5.AddRotateXZYOp()
        self.assertFalse(UsdGeom.XformCommonAPI(badOpOrder5))

        badOpOrder6 = UsdGeom.Xform.Define(s, "/BadOpOrder6")
        # Add translate after (pivot, invPivot()
        UsdGeom.XformCommonAPI(badOpOrder6).SetPivot(Gf.Vec3f(10, 20, 30))
        badOpOrder6.AddTranslateOp()
        self.assertFalse(UsdGeom.XformCommonAPI(badOpOrder6))

    def test_PreserveResetXformStack(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, "/World")
        api = UsdGeom.XformCommonAPI(x)

        self.assertTrue(api)

        self.assertTrue(api.SetResetXformStack(True))
        self.assertTrue(api.SetTranslate(Gf.Vec3d(10, 20, 30)))
        self.assertTrue(api.GetResetXformStack())

        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
                    Vt.TokenArray(('!resetXformStack!', 'xformOp:translate')))

        self.assertTrue(api.SetRotate(Gf.Vec3f(10, 20, 30)))
        self.assertTrue(api.GetResetXformStack())

        self.assertTrue(api.SetScale(Gf.Vec3f(10, 20, 30)))
        self.assertTrue(api.GetResetXformStack())

        self.assertTrue(api.SetPivot(Gf.Vec3f(10, 20, 30)))
        self.assertTrue(api.GetResetXformStack())

        self.assertEqual(x.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(('!resetXformStack!', 'xformOp:translate',
                           'xformOp:translate:pivot',
                           'xformOp:rotateXYZ', 'xformOp:scale',
                           '!invert!xformOp:translate:pivot')))

    def test_MatrixDecomposition(self):
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, "/X")
        xXformAPI = UsdGeom.XformCommonAPI(x)
        self.assertTrue(xXformAPI)
        self.assertTrue(xXformAPI.SetXformVectors(
            translation=Gf.Vec3d(10., 20., 30.), 
            rotation=Gf.Vec3f(30, 45, 60), 
            scale=Gf.Vec3f(1., 2., 3.), 
            pivot=Gf.Vec3f(0, 0, 0), # pivot has to be 0 for proper decomposition.
            rotationOrder=
            UsdGeom.XformCommonAPI.RotationOrderYXZ,
            time=Usd.TimeCode.Default()))

        y = UsdGeom.Xform.Define(s, "/Y")
        y.MakeMatrixXform().Set(x.GetLocalTransformation(Usd.TimeCode.Default()))
        yXformAPI = UsdGeom.XformCommonAPI(y)
        self.assertFalse(yXformAPI)

        # none of the operations will work on an incompatible xformable.
        self.assertFalse(yXformAPI.SetTranslate(Gf.Vec3d(10, 20, 30)))
        self.assertFalse(yXformAPI.SetRotate(Gf.Vec3f(10, 20, 30)))
        self.assertFalse(yXformAPI.SetScale(Gf.Vec3f(1, 2, 3)))
        self.assertFalse(yXformAPI.SetPivot(Gf.Vec3f(10, 10, 10)))

        # GetXformVectors will work on an incompatible xformable. 
        # It does a full on decomposition in this case of the composed 
        # transformation.
        # 
        # The decomposition may not yield the exact set of input vectors, but the 
        # computed local transformation must match, as verified by the last 
        # AssetClose call below.
        xformVectors = yXformAPI.GetXformVectors(Usd.TimeCode.Default())

        z = UsdGeom.Xform.Define(s, "/Z")
        zXformAPI = UsdGeom.XformCommonAPI(z)
        self.assertTrue(zXformAPI)
        self.assertTrue(zXformAPI.SetTranslate(xformVectors[0]))
        self.assertTrue(zXformAPI.SetRotate(xformVectors[1]))
        self.assertTrue(zXformAPI.SetScale(xformVectors[2]))
        self.assertTrue(zXformAPI.SetPivot(xformVectors[3]))

        # Verify that the final transform value matches, although the individual 
        # component values *may* not.
        xtrans = x.GetLocalTransformation(Usd.TimeCode.Default())
        ztrans = z.GetLocalTransformation(Usd.TimeCode.Default())
        for a, b in zip([xtrans.GetRow(i) for i in range(0,5)],
                        [ztrans.GetRow(i) for i in range(0,5)]):
            if any(map(math.isnan, a)) or any(map(math.isnan, b)):
                continue
            self.assertTrue(Gf.IsClose(a,b,1e-5))


    def test_Bug116955(self):
        """ Regression test for bug 116955, where invoking XformCommonAPI
            on an xformable containing (pre-existing) compatible xform ops 
            was crashing.
        """
        s = Usd.Stage.CreateInMemory()
        x = UsdGeom.Xform.Define(s, '/X')
        UsdGeom.XformCommonAPI(x).SetTranslate(Gf.Vec3d(1,2,3))
        UsdGeom.XformCommonAPI(x).SetTranslate(Gf.Vec3d(5,6,7))

        self.assertEqual(UsdGeom.XformCommonAPI(x).GetXformVectors(Usd.TimeCode.Default()), 
            (Gf.Vec3d(5.0, 6.0, 7.0), Gf.Vec3f(0.0, 0.0, 0.0), Gf.Vec3f(1.0, 1.0, 1.0), 
            Gf.Vec3f(0.0, 0.0, 0.0), UsdGeom.XformCommonAPI.RotationOrderXYZ))

        UsdGeom.XformCommonAPI(x).SetRotate(Gf.Vec3f(1,2,3))
        UsdGeom.XformCommonAPI(x).SetRotate(Gf.Vec3f(5,6,7))
        self.assertEqual(UsdGeom.XformCommonAPI(x).GetXformVectors(Usd.TimeCode.Default()), 
            (Gf.Vec3d(5.0, 6.0, 7.0), Gf.Vec3f(5.0, 6.0, 7.0), Gf.Vec3f(1.0, 1.0, 1.0), 
            Gf.Vec3f(0.0, 0.0, 0.0), UsdGeom.XformCommonAPI.RotationOrderXYZ))

        UsdGeom.XformCommonAPI(x).SetScale(Gf.Vec3f(2,2,2))
        UsdGeom.XformCommonAPI(x).SetScale(Gf.Vec3f(3,4,5))
        self.assertEqual(UsdGeom.XformCommonAPI(x).GetXformVectors(Usd.TimeCode.Default()), 
            (Gf.Vec3d(5.0, 6.0, 7.0), Gf.Vec3f(5.0, 6.0, 7.0), Gf.Vec3f(3.0, 4.0, 5.0), 
            Gf.Vec3f(0.0, 0.0, 0.0), UsdGeom.XformCommonAPI.RotationOrderXYZ))

        UsdGeom.XformCommonAPI(x).SetPivot(Gf.Vec3f(100,200,300))
        UsdGeom.XformCommonAPI(x).SetPivot(Gf.Vec3f(300,400,500))
        self.assertEqual(UsdGeom.XformCommonAPI(x).GetXformVectors(Usd.TimeCode.Default()), 
            (Gf.Vec3d(5.0, 6.0, 7.0), Gf.Vec3f(5.0, 6.0, 7.0), Gf.Vec3f(3.0, 4.0, 5.0), 
            Gf.Vec3f(300.0, 400.0, 500.0), UsdGeom.XformCommonAPI.RotationOrderXYZ))

        UsdGeom.XformCommonAPI(x).SetResetXformStack(True)
        UsdGeom.XformCommonAPI(x).SetResetXformStack(True)
        self.assertTrue(UsdGeom.XformCommonAPI(x).GetResetXformStack())

        UsdGeom.XformCommonAPI(x).SetResetXformStack(False)
        self.assertFalse(UsdGeom.XformCommonAPI(x).GetResetXformStack())

    def _ValidateXformVectorsByAccumulation(self, xformable,
            expectedTranslation, expectedRotation, expectedScale, expectedPivot,
            expectedRotOrder, time=None):
        if not time:
            time = Usd.TimeCode.Default()
        
        (translation, rotation, scale, pivot, rotOrder) = UsdGeom.XformCommonAPI(
            xformable).GetXformVectorsByAccumulation(time)

        self.assertTrue(Gf.IsClose(expectedTranslation, translation, 1e-5))
        self.assertTrue(Gf.IsClose(expectedRotation, rotation, 1e-5))
        self.assertTrue(Gf.IsClose(expectedScale, scale, 1e-5))
        self.assertTrue(Gf.IsClose(expectedPivot, pivot, 1e-5))
        self.assertEqual(expectedRotOrder, rotOrder)

    def test_GetXformVectorsByAccumulation(self):
        """
        Tests the GetXformVectorsByAccumulation() method of XformCommonAPI. Even
        with xformOp orders that don't strictly adhere to the xformCommonAPI, if we
        can reduce the ops by accumulation to an equivalent set of transforms that
        DOES conform, then we should still get back the common transforms,
        including pivots.
        """
        s = Usd.Stage.CreateInMemory()

        # Single axis rotation about X
        xf = UsdGeom.Xform.Define(s, "/RotX")
        xf.AddRotateXOp().Set(45.0)
        self.assertFalse(UsdGeom.XformCommonAPI(xf))
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(0.0, 0.0, 0.0),
            Gf.Vec3f(45.0, 0.0, 0.0),
            Gf.Vec3f(1.0, 1.0, 1.0),
            Gf.Vec3f(0.0, 0.0, 0.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # Single axis rotation about Y
        xf = UsdGeom.Xform.Define(s, "/RotY")
        xf.AddRotateYOp().Set(60.0)
        self.assertFalse(UsdGeom.XformCommonAPI(xf))
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(0.0, 0.0, 0.0),
            Gf.Vec3f(0.0, 60.0, 0.0),
            Gf.Vec3f(1.0, 1.0, 1.0),
            Gf.Vec3f(0.0, 0.0, 0.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # Single axis rotation about Z
        xf = UsdGeom.Xform.Define(s, "/RotZ")
        xf.AddRotateZOp().Set(115.0)
        self.assertFalse(UsdGeom.XformCommonAPI(xf))
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(0.0, 0.0, 0.0),
            Gf.Vec3f(0.0, 0.0, 115.0),
            Gf.Vec3f(1.0, 1.0, 1.0),
            Gf.Vec3f(0.0, 0.0, 0.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # Three axis rotation with a non-default (non-XYZ) rotation order.
        # Note that we have to add a pivot with a non-conforming name for
        # it to be incompatible.
        xf = UsdGeom.Xform.Define(s, "/RotZYX")
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "myRotatePivot"
            ).Set(Gf.Vec3f(-3.0, -2.0, -1.0))
        xf.AddRotateZYXOp(UsdGeom.XformOp.PrecisionFloat,
            ).Set(Gf.Vec3f(90.0, 60.0, 30.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "myRotatePivot", isInverseOp=True)
        self.assertFalse(UsdGeom.XformCommonAPI(xf))
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(0.0, 0.0, 0.0),
            Gf.Vec3f(90.0, 60.0, 30.0),
            Gf.Vec3f(1.0, 1.0, 1.0),
            Gf.Vec3f(-3.0, -2.0, -1.0),
            UsdGeom.XformCommonAPI.RotationOrderZYX)

        # Accumulation of translation ops
        xf = UsdGeom.Xform.Define(s, "/TranslationsOnly")
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionDouble, "transOne"
            ).Set(Gf.Vec3d(1.0, 2.0, 3.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionDouble, "transTwo"
            ).Set(Gf.Vec3d(9.0, 8.0, 7.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionDouble, "transThree"
            ).Set(Gf.Vec3d(10.0, 20.0, 30.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionDouble, "transFour"
            ).Set(Gf.Vec3d(90.0, 80.0, 70.0))
        self.assertFalse(UsdGeom.XformCommonAPI(xf))
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(110.0, 110.0, 110.0),
            Gf.Vec3f(0.0, 0.0, 0.0),
            Gf.Vec3f(1.0, 1.0, 1.0),
            Gf.Vec3f(0.0, 0.0, 0.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # Rotate op with a pivot
        xf = UsdGeom.Xform.Define(s, "/RotateWithPivot")
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "rotatePivot"
            ).Set(Gf.Vec3f(3.0, 6.0, 9.0))
        xf.AddRotateXYZOp(UsdGeom.XformOp.PrecisionFloat
            ).Set(Gf.Vec3f(0.0, 45.0, 0.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "rotatePivot", isInverseOp=True)
        self.assertFalse(UsdGeom.XformCommonAPI(xf))
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(0.0, 0.0, 0.0),
            Gf.Vec3f(0.0, 45.0, 0.0),
            Gf.Vec3f(1.0, 1.0, 1.0),
            Gf.Vec3f(3.0, 6.0, 9.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # Accumulation of scale ops
        xf = UsdGeom.Xform.Define(s, "/ScalesOnly")
        xf.AddScaleOp(UsdGeom.XformOp.PrecisionFloat, "scaleOne"
            ).Set(Gf.Vec3f(1.0, 2.0, 3.0))
        xf.AddScaleOp(UsdGeom.XformOp.PrecisionFloat, "scaleTwo"
            ).Set(Gf.Vec3f(2.0, 4.0, 6.0))
        xf.AddScaleOp(UsdGeom.XformOp.PrecisionFloat, "scaleThree"
            ).Set(Gf.Vec3f(10.0, 20.0, 30.0))
        self.assertFalse(UsdGeom.XformCommonAPI(xf))
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(0.0, 0.0, 0.0),
            Gf.Vec3f(0.0, 0.0, 0.0),
            Gf.Vec3f(20.0, 160.0, 540.0),
            Gf.Vec3f(0.0, 0.0, 0.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # Accumulation of scale ops with a pivot
        xf = UsdGeom.Xform.Define(s, "/ScalesWithPivot")
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "scalePivot"
            ).Set(Gf.Vec3f(15.0, 25.0, 35.0))
        xf.AddScaleOp(UsdGeom.XformOp.PrecisionFloat, "scaleOne"
            ).Set(Gf.Vec3f(10.0, 20.0, 30.0))
        xf.AddScaleOp(UsdGeom.XformOp.PrecisionFloat, "scaleTwo"
            ).Set(Gf.Vec3f(0.5, 0.5, 0.5))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "scalePivot", isInverseOp=True)
        self.assertFalse(UsdGeom.XformCommonAPI(xf))
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(0.0, 0.0, 0.0),
            Gf.Vec3f(0.0, 0.0, 0.0),
            Gf.Vec3f(5.0, 10.0, 15.0),
            Gf.Vec3f(15.0, 25.0, 35.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # Accumulation of scale ops with a pivot and translation
        xf = UsdGeom.Xform.Define(s, "/ScalesWithPivotAndTranslate")
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionDouble,
            ).Set(Gf.Vec3d(123.0, 456.0, 789.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "scalePivot"
            ).Set(Gf.Vec3f(-222.0, -444.0, -666.0))
        xf.AddScaleOp(UsdGeom.XformOp.PrecisionFloat, "scaleOne"
            ).Set(Gf.Vec3f(2.0, 2.0, 2.0))
        xf.AddScaleOp(UsdGeom.XformOp.PrecisionFloat, "scaleTwo"
            ).Set(Gf.Vec3f(2.0, 2.0, 2.0))
        xf.AddScaleOp(UsdGeom.XformOp.PrecisionFloat, "scaleThree"
            ).Set(Gf.Vec3f(2.0, 2.0, 2.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "scalePivot", isInverseOp=True)
        self.assertFalse(UsdGeom.XformCommonAPI(xf))
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(123.0, 456.0, 789.0),
            Gf.Vec3f(0.0, 0.0, 0.0),
            Gf.Vec3f(8.0, 8.0, 8.0),
            Gf.Vec3f(-222.0, -444.0, -666.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # Typical xformOp order as exported from Maya. Start out with an xformable
        # that DOES conform (xformOp naming is important).
        xf = UsdGeom.Xform.Define(s, "/MayaXformOrder")
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionDouble
            ).Set(Gf.Vec3d(1.0, 2.0, 3.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "pivot"
            ).Set(Gf.Vec3f(10.0, 20.0, 30.0))
        xf.AddRotateXYZOp(UsdGeom.XformOp.PrecisionFloat
            ).Set(Gf.Vec3f(0.0, 45.0, 0.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "pivot", isInverseOp=True)

        # Verify that it conforms.
        self.assertTrue(UsdGeom.XformCommonAPI(xf))

        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(1.0, 2.0, 3.0),
            Gf.Vec3f(0.0, 45.0, 0.0),
            Gf.Vec3f(1.0, 1.0, 1.0),
            Gf.Vec3f(10.0, 20.0, 30.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # Now add a scale pivot and a scale. The scale pivot will initially have
        # the same position as the rotate pivot.
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "scalePivot"
            ).Set(Gf.Vec3f(10.0, 20.0, 30.0))
        xf.AddScaleOp(UsdGeom.XformOp.PrecisionFloat, "mayaScale"
            ).Set(Gf.Vec3f(2.0, 4.0, 6.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "scalePivot", isInverseOp=True)

        # Verify that it no longer conforms...
        self.assertFalse(UsdGeom.XformCommonAPI(xf))

        # ... but we can still get a pivot.
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(1.0, 2.0, 3.0),
            Gf.Vec3f(0.0, 45.0, 0.0),
            Gf.Vec3f(2.0, 4.0, 6.0),
            Gf.Vec3f(10.0, 20.0, 30.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # Now change the scalePivot position so that it no longer matches the
        # rotate pivot. This means the xformOps both do not conform to common AND
        # also cannot be reduced to conform by accumulation. We should resort to
        # local transform decomposition and get a zero pivot in that case.
        xf.GetOrderedXformOps()[4].Set(Gf.Vec3f(200.0, 300.0, 400.0))

        # Verify that it does not conform...
        self.assertFalse(UsdGeom.XformCommonAPI(xf))

        # ... and that we get a zero pivot.
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(-1572.9191898578665, -898.0, -1253.9343417595162),
            Gf.Vec3f(0.0, 45.0, 0.0),
            Gf.Vec3f(2.0, 4.0, 6.0),
            Gf.Vec3f(0.0, 0.0, 0.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # An xformOp order as exported from Maya with a translate and a rotate
        # pivot specified, but no actual rotation. The xformOp order contains only
        # translations.
        xf = UsdGeom.Xform.Define(s, "/MayaXformOrderTranslateWithRotatePivotOnly")
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionDouble
            ).Set(Gf.Vec3d(20.0, 40.0, 60.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "rotatePivot"
            ).Set(Gf.Vec3f(50.0, 150.0, 250.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "rotatePivot", isInverseOp=True)
        self.assertFalse(UsdGeom.XformCommonAPI(xf))
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(20.0, 40.0, 60.0),
            Gf.Vec3f(0.0, 0.0, 0.0),
            Gf.Vec3f(1.0, 1.0, 1.0),
            Gf.Vec3f(50.0, 150.0, 250.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # An xformOp order as exported from Maya with a translate and rotate and
        # scale pivots specified, but only rotation and no scaling.
        xf = UsdGeom.Xform.Define(s, "/MayaXformOrderTranslateWithPivotsAndRotateNoScale")
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionDouble
            ).Set(Gf.Vec3d(11.0, 22.0, 33.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "rotatePivot"
            ).Set(Gf.Vec3f(111.0, 222.0, 333.0))
        xf.AddRotateZOp().Set(44.0)
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "rotatePivot", isInverseOp=True)
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "scalePivot"
            ).Set(Gf.Vec3f(111.0, 222.0, 333.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "scalePivot", isInverseOp=True)
        self.assertFalse(UsdGeom.XformCommonAPI(xf))
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(11.0, 22.0, 33.0),
            Gf.Vec3f(0.0, 0.0, 44.0),
            Gf.Vec3f(1.0, 1.0, 1.0),
            Gf.Vec3f(111.0, 222.0, 333.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

        # An xformOp order as exported from Maya with a translate and identical
        # rotate and scale pivots specified, but no actual rotation or scaling.
        # The xformOp order contains only translations.
        xf = UsdGeom.Xform.Define(s, "/MayaXformOrderTranslateWithIdenticalPivots")
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionDouble
            ).Set(Gf.Vec3d(300.0, 600.0, 900.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "rotatePivot"
            ).Set(Gf.Vec3f(-100.0, -300.0, -500.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "rotatePivot", isInverseOp=True)
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "scalePivot"
            ).Set(Gf.Vec3f(-100.0, -300.0, -500.0))
        xf.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "scalePivot", isInverseOp=True)
        self.assertFalse(UsdGeom.XformCommonAPI(xf))
        self._ValidateXformVectorsByAccumulation(xf,
            Gf.Vec3d(300.0, 600.0, 900.0),
            Gf.Vec3f(0.0, 0.0, 0.0),
            Gf.Vec3f(1.0, 1.0, 1.0),
            Gf.Vec3f(-100.0, -300.0, -500.0),
            UsdGeom.XformCommonAPI.RotationOrderXYZ)

if __name__ == "__main__":
    unittest.main()
