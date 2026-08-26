#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Gf, Usd, UsdGeom, Sdf, Tf
import unittest, math

class TestUsdGeomCamera(unittest.TestCase):
    def _GetSchemaProjection(self, schema, time):
        val = schema.GetProjectionAttr().Get(time)
        if val == UsdGeom.Tokens.perspective:
            return Gf.Camera.Perspective
        if val == UsdGeom.Tokens.orthographic:
            return Gf.Camera.Orthographic
        return None

    def _CheckValues(self, camera, schema, time):
        self.assertEqual(camera,                                            \
                         schema.GetLocalTransformation(Usd.TimeCode(time)))
        self.assertEqual(camera.projection,                                 \
                          self._GetSchemaProjection(schema, time))
        self.assertEqual(camera.horizontalAperture,                         \
                         schema.GetHorizontalApertureAttr().Get(time))
        self.assertEqual(camera.verticalAperture,                           \
                         schema.GetVerticalApertureAttr().Get(time))
        self.assertEqual(camera.horizontalApertureOffset,                   \
                         schema.GetHorizontalApertureOffsetAttr().Get(time))
        self.assertEqual(camera.verticalApertureOffset,                     \
                         schema.GetVerticalApertureOffsetAttr().Get(time))
        self.assertEqual(camera.focalLength,                                \
                         schema.GetFocalLengthAttr().Get(time))
        self.assertEqual(camera.clippingRange,                              \
                         self._GetSchemaClippingRange(schema, time))
        self.assertEqual(camera.clippingPlanes,                             \
                         self._GetSchemaClippingPlanes(schema, time))
        self.assertTrue(Gf.IsClose(camera.fStop,                            \
                                   schema.GetFStopAttr().Get(time), 1e-6))
        self.assertEqual(camera.focusDistance,                              \
                         schema.GetFocusDistanceAttr().Get(time))

    def _SetCamera(self, usdCamera):
        camera = Gf.Camera()
        camera.transform = (
            Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1.0,2.0,3.0),10.0)) *
            Gf.Matrix4d().SetTranslate(Gf.Vec3d(4.0,5.0,6.0)))
        camera.projection = Gf.Camera.Perspective
        camera.horizontalAperture = 5.1
        camera.verticalAperture = 2.0
        camera.horizontalApertureOffset = 0.13
        camera.verticalApertureOffset = -0.14
        camera.focalLength = 28
        camera.clippingRange = Gf.Range1f(5, 15)
        camera.clippingPlanes = [[1, 2, 3, 4], [8, 7, 6, 5]]
        camera.fStop = 1.2
        camera.focusDistance = 300

        usdCamera.SetFromCamera(camera, 1.0)
        return usdCamera, camera
    
    def _GetExpectedDimension(self, forWidth, camera):
        if forWidth:
            aperture = camera.horizontalAperture
        else:
            aperture = camera.verticalAperture
        return camera.focusDistance *(aperture) /camera.focalLength


    def test_AutoFocus(self):
        usdStage = Usd.Stage.CreateInMemory()
        usdCamera = UsdGeom.Camera.Define(usdStage, '/camera')
        self.assertTrue(UsdGeom.BackPlateAPI.CanApply(              \
            usdCamera.GetPrim(), "test:autoFocus:backPlate"))

        backPlate = UsdGeom.BackPlateAPI.Apply(                     \
            usdCamera.GetPrim(), "test:autoFocus:backPlate")

        usdCamera, camera = self._SetCamera(usdCamera)
        self.assertTrue(backPlate)
        expectedW = self._GetExpectedDimension(True, camera)
        expectedH = self._GetExpectedDimension(False, camera)

        # Test auto focus abilities
        w = backPlate.ComputeEffectiveDimension(True, 1.0)
        h = backPlate.ComputeEffectiveDimension(False, 1.0)
        scaleFactor = 2.0
        self.assertTrue(Gf.IsClose(w, expectedW, 1e-6))
        self.assertTrue(Gf.IsClose(h, expectedH, 1e-6))

        # Scaling should affect the back plate size
        backPlate.GetScaleTweakAttr().Set(Gf.Vec2f(scaleFactor, scaleFactor))
        w = backPlate.ComputeEffectiveDimension(True, 1.0)
        h = backPlate.ComputeEffectiveDimension(False, 1.0)
        self.assertTrue(Gf.IsClose(w, expectedW*scaleFactor, 1e-6))
        self.assertTrue(Gf.IsClose(h, expectedH*scaleFactor, 1e-6))

        # Translation should not affect the back plate size
        backPlate.GetTranslateTweakAttr().Set(Gf.Vec3f(1.0, 2.0, 1.0))
        w = backPlate.ComputeEffectiveDimension(True, 1.0)
        h = backPlate.ComputeEffectiveDimension(False, 1.0)
        self.assertTrue(Gf.IsClose(w, expectedW*scaleFactor, 1e-6))
        self.assertTrue(Gf.IsClose(h, expectedH*scaleFactor, 1e-6))

        # Changing the focus distance should affect the back plate size
        camera.focusDistance *= 2
        usdCamera.SetFromCamera(camera, 1.0)
        w = backPlate.ComputeEffectiveDimension(True, 1.0)
        h = backPlate.ComputeEffectiveDimension(False, 1.0)
        expectedW = self._GetExpectedDimension(True, camera)
        expectedH = self._GetExpectedDimension(False, camera)

        self.assertTrue(Gf.IsClose(w, expectedW*scaleFactor, 1e-5))
        self.assertTrue(Gf.IsClose(h, expectedH*scaleFactor, 1e-5))

        # Back plate dimensions should not increase with focus distance with
        # orthographic projections
        camera.projection = Gf.Camera.Orthographic
        usdCamera.SetFromCamera(camera, 1.0)
        w = backPlate.ComputeEffectiveDimension(True, 1.0)
        h = backPlate.ComputeEffectiveDimension(False, 1.0)
        expectedW = camera.horizontalAperture
        expectedH = camera.verticalAperture

        self.assertTrue(Gf.IsClose(w, expectedW*scaleFactor, 1e-6))
        self.assertTrue(Gf.IsClose(h, expectedH*scaleFactor, 1e-6))

    def test_SetPositionAPI(self):
        usdStage = Usd.Stage.CreateInMemory()
        usdCamera = UsdGeom.Camera.Define(usdStage, '/camera')
        self.assertTrue(UsdGeom.BackPlateAPI.CanApply(              \
            usdCamera.GetPrim(), "test:SetPositionAPI:backPlate"))
        backPlate = UsdGeom.BackPlateAPI.Apply(                     \
            usdCamera.GetPrim(), "test:SetPositionAPI:backPlate")

        usdCamera, camera = self._SetCamera(usdCamera)
        self.assertTrue(backPlate)

        self.assertTrue(Gf.IsClose(Gf.Vec3f(0.0, 0.0, 300), \
                                   backPlate.GetCameraSpacePosition(1.0), 1e-6))
        self.assertTrue(Gf.IsClose(Gf.Vec3f(4.0, 5.0, 306), \
                                   backPlate.GetWorldSpacePosition(1.0), 1e-6))
        
        # set position via world space
        backPlate.SetWorldSpacePosition(Gf.Vec3f(5.0, 7.0, 206.0), 1.0)

        self.assertTrue(Gf.IsClose(Gf.Vec3f(1.0, 2.0, -100.0), 
            backPlate.GetTranslateTweakAttr().Get(1.0), 1e-6))
        self.assertTrue(Gf.IsClose(Gf.Vec3f(1.0, 2.0, 200.0), 
            backPlate.GetCameraSpacePosition(1.0), 1e-6))
        self.assertTrue(Gf.IsClose(Gf.Vec3f(5.0, 7.0, 206.0), 
            backPlate.GetWorldSpacePosition(1.0), 1e-6))

        #set position via camera space
        backPlate.SetCameraSpacePosition(Gf.Vec3f(3.0, 4.0, 340.0), 2.0)
        self.assertTrue(Gf.IsClose(Gf.Vec3f(3.0, 4.0, 40.0), 
            backPlate.GetTranslateTweakAttr().Get(2.0), 1e-6))
        self.assertTrue(Gf.IsClose(Gf.Vec3f(3.0, 4.0, 340.0), 
            backPlate.GetCameraSpacePosition(2.0), 1e-6))
        self.assertTrue(Gf.IsClose(Gf.Vec3f(7.0, 9.0, 346.0), 
            backPlate.GetWorldSpacePosition(2.0), 1e-6))

    def test_SetAspectRatio(self):
        usdStage = Usd.Stage.CreateInMemory()
        usdCamera = UsdGeom.Camera.Define(usdStage, '/camera')
        self.assertTrue(UsdGeom.BackPlateAPI.CanApply(          \
            usdCamera.GetPrim(), "test:aspectRatio:backPlate"))
        backPlate = UsdGeom.BackPlateAPI.Apply(                 \
            usdCamera.GetPrim(), "test:aspectRatio:backPlate")

        usdCamera, camera = self._SetCamera(usdCamera)
        self.assertTrue(backPlate)

        targetW = 75.0
        targetH = 50.0
        w = backPlate.ComputeEffectiveDimension(True, 1.0)
        h = backPlate.ComputeEffectiveDimension(False, 1.0)

        backPlate.SetAspectRatio(targetW, targetH, 1.0)
        newW = backPlate.ComputeEffectiveDimension(True, 1.0)
        newH = backPlate.ComputeEffectiveDimension(False, 1.0)
        self.assertTrue(Gf.IsClose(newW, 75, 1e-6))
        self.assertTrue(Gf.IsClose(newH, 50, 1e-6))
        self.assertEqual(backPlate.GetScaleTweakAttr().Get(1.0),
                         Gf.Vec2f(targetW/w,targetH/h))

    def test_SetBackPlate(self):
        usdStage = Usd.Stage.CreateInMemory()
        usdCamera = UsdGeom.Camera.Define(usdStage, '/camera')
        self.assertTrue(UsdGeom.BackPlateAPI.CanApply(usdCamera.GetPrim(), 
                                                      "test:backPlate"))
        backPlate = UsdGeom.BackPlateAPI.Apply(usdCamera.GetPrim(), 
                                               "test:backPlate")

        usdCamera = self._SetCamera(usdCamera)
        self.assertTrue(backPlate)

        # Check default values
        self.assertEqual(                                                   \
            backPlate.GetScaleTweakAttr().Get(1.0),Gf.Vec2f(1.0,1.0))
        self.assertEqual(                                                   \
            backPlate.GetRotateXYZTweakAttr().Get(1.0),Gf.Vec3f(0.0,0.0,0.0))
        self.assertEqual(                                                   \
            backPlate.GetTranslateTweakAttr().Get(1.0),Gf.Vec3f(0.0,0.0,0.0) )
        self.assertEqual(                                                   \
            backPlate.GetLumaSlopeAttr().Get(1.0), Gf.Vec3f(1.0,1.0,1.0))
        self.assertEqual(                                                   \
            backPlate.GetLumaPowerAttr().Get(1.0), Gf.Vec3f(1.0,1.0,1.0))
        self.assertEqual(                                                   \
            backPlate.GetLumaOffsetAttr().Get(1.0), Gf.Vec3f(0.0,0.0,0.0))
        self.assertEqual(                                                   \
            backPlate.GetPlateVisibilityAttr().Get(1.0), "solo")
        self.assertEqual(backPlate.GetDepthMinOffsetAttr().Get(1.0), 0.0)
        self.assertEqual(                                                   \
            backPlate.GetDepthNormalizingFactorAttr().Get(1.0), 1.0)
        self.assertEqual(                                                   \
            backPlate.GetDepthCameraSpaceOffsetAttr().Get(1.0), 0.0)

        # Set new values
        backPlate.GetScaleTweakAttr().Set(Gf.Vec2f(2.0, 3.0))
        backPlate.GetRotateXYZTweakAttr().Set(Gf.Vec3f(2.0, 2.0, 6.0))
        backPlate.GetTranslateTweakAttr().Set(Gf.Vec3f(3.0, -2.0, -1.0))
        backPlate.GetLumaSlopeAttr().Set(Gf.Vec3f(2.0, 1.0, 0.5))
        backPlate.GetLumaPowerAttr().Set(Gf.Vec3f(3.0, 2.0, 0.5))
        backPlate.GetLumaOffsetAttr().Set(Gf.Vec3f(1.0,0.0,0.5))
        backPlate.GetDepthMinOffsetAttr().Set(2.0)
        backPlate.GetDepthNormalizingFactorAttr().Set(2.0)
        backPlate.GetDepthCameraSpaceOffsetAttr().Set(6.0)

        self.assertEqual(                                                   \
            backPlate.GetScaleTweakAttr().Get(1.0), Gf.Vec2f(2.0, 3.0))
        self.assertEqual(                                                   \
            backPlate.GetRotateXYZTweakAttr().Get(1.0), Gf.Vec3f(2.0, 2.0, 6.0))
        self.assertEqual(backPlate.GetTranslateTweakAttr().Get(1.0),        \
                         Gf.Vec3f(3.0, -2.0, -1.0))
        self.assertEqual(                                                   \
            backPlate.GetLumaSlopeAttr().Get(1.0), Gf.Vec3f(2.0, 1.0, 0.5))
        self.assertEqual(                                                   \
            backPlate.GetLumaPowerAttr().Get(1.0), Gf.Vec3f(3.0, 2.0, 0.5))
        self.assertEqual(                                                   \
            backPlate.GetLumaOffsetAttr().Get(1.0), Gf.Vec3f(1.0,0.0,0.5))
        self.assertEqual(backPlate.GetDepthMinOffsetAttr().Get(1.0), 2.0)
        self.assertEqual(                                                   \
            backPlate.GetDepthNormalizingFactorAttr().Get(1.0), 2.0)
        self.assertEqual(                                                   \
            backPlate.GetDepthCameraSpaceOffsetAttr().Get(1.0), 6.0)
        
        # PlateVisibility
        self.assertEqual(backPlate.GetPlateVisibilityAttr().Get(1.0), "solo")
        backPlate.GetPlateVisibilityAttr().Set("all")
        self.assertEqual(backPlate.GetPlateVisibilityAttr().Get(1.0), "all")
        backPlate.GetPlateVisibilityAttr().Set("mute")
        self.assertEqual(backPlate.GetPlateVisibilityAttr().Get(1.0), "mute")

if __name__ == '__main__':
    unittest.main()
