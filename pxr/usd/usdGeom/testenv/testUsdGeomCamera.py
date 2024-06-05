#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

    def _GetSchemaClippingRange(self, schema, time):
        val = schema.GetClippingRangeAttr().Get(time)
        return Gf.Range1f(val[0], val[1])

    def _GetSchemaClippingPlanes(self, schema, time):
        val = schema.GetClippingPlanesAttr().Get(time)
        return [Gf.Vec4f(float(i[0]),
                         float(i[1]),
                         float(i[2]),
                         float(i[3])) for i in val]

    def _CheckValues(self, camera, schema, time):
        self.assertEqual(camera.transform, schema.GetLocalTransformation(Usd.TimeCode(time)))
        self.assertEqual(camera.projection, self._GetSchemaProjection(schema, time))
        self.assertEqual(camera.horizontalAperture, schema.GetHorizontalApertureAttr().Get(time))
        self.assertEqual(camera.verticalAperture, schema.GetVerticalApertureAttr().Get(time))
        self.assertEqual(camera.horizontalApertureOffset,
                         schema.GetHorizontalApertureOffsetAttr().Get(time))
        self.assertEqual(camera.verticalApertureOffset,
                         schema.GetVerticalApertureOffsetAttr().Get(time))
        self.assertEqual(camera.focalLength, schema.GetFocalLengthAttr().Get(time))
        self.assertEqual(camera.clippingRange, self._GetSchemaClippingRange(schema, time))
        self.assertEqual(camera.clippingPlanes, self._GetSchemaClippingPlanes(schema, time))
        self.assertTrue(Gf.IsClose(camera.fStop, schema.GetFStopAttr().Get(time), 1e-6))
        self.assertEqual(camera.focusDistance, schema.GetFocusDistanceAttr().Get(time))

    def test_GetCamera(self):
        usdStage = Usd.Stage.CreateInMemory()
        usdCamera = UsdGeom.Camera.Define(usdStage, '/camera')

        # test fall-back values
        self._CheckValues(Gf.Camera(), usdCamera, 1.0)

        usdCamera.MakeMatrixXform().Set(Gf.Matrix4d(3.0))
        usdCamera.GetProjectionAttr().Set(UsdGeom.Tokens.orthographic)
        usdCamera.GetHorizontalApertureAttr().Set(5.1)
        usdCamera.GetVerticalApertureAttr().Set(2.0)
        usdCamera.GetHorizontalApertureOffsetAttr().Set(-0.11)
        usdCamera.GetVerticalApertureOffsetAttr().Set(0.12)
        usdCamera.GetFocalLengthAttr().Set(28)
        usdCamera.GetClippingRangeAttr().Set(Gf.Vec2f(5, 15))
        usdCamera.GetClippingPlanesAttr().Set([(1,2,3,4), (8,7,6,5)])
        usdCamera.GetFStopAttr().Set(1.2)
        usdCamera.GetFocusDistanceAttr().Set(300)

        camera = usdCamera.GetCamera(1.0)

        # test assigned values
        self._CheckValues(camera, usdCamera, 1.0)

    def test_SetFromCamera(self):
        camera = Gf.Camera()

        usdStage = Usd.Stage.CreateInMemory()
        usdCamera = UsdGeom.Camera.Define(usdStage, '/camera')
        usdCameraProj = usdCamera.GetProjectionAttr()

        # test fall-back values
        self._CheckValues(camera, usdCamera, 1.0)
        self.assertEqual(usdCameraProj.GetResolveInfo().GetSource(),
                         Usd.ResolveInfoSourceFallback)

        camera.transform = (
            Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1.0,2.0,3.0),10.0)) *
            Gf.Matrix4d().SetTranslate(Gf.Vec3d(4.0,5.0,6.0)))
        camera.projection = Gf.Camera.Orthographic
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

        # test assigned values
        self._CheckValues(camera, usdCamera, 1.0)
        self.assertEqual(usdCameraProj.GetResolveInfo().GetSource(),
                         Usd.ResolveInfoSourceTimeSamples)

        usdCamera.SetFromCamera(camera, 1.0)
            
        actual = usdCamera.GetLocalTransformation(Usd.TimeCode(1.0))
        expected  = Gf.Matrix4d( 0.9858929135, 0.14139860385,-0.089563373740, 0.0,
                                -0.1370579618, 0.98914839500, 0.052920390613, 0.0,
                                 0.0960743367,-0.03989846462, 0.994574197504, 0.0,
                                 4.0         , 5.0          , 6.0           , 1.0)

        for a, e in zip(actual, expected):
            self.assertTrue(Gf.IsClose(a, e, 1e-2))

    def test_SetFromCameraWithComposition(self):
        stage = Usd.Stage.Open("layers_a_b.usda")
        layerA = Sdf.Layer.FindOrOpen("a.usda")
        layerB = Sdf.Layer.FindOrOpen("b.usda")
        stage.SetEditTarget(layerB)

        usdCamera = UsdGeom.Camera.Define(stage, '/camera')

        camera = Gf.Camera()
        newXform = Gf.Matrix4d().SetTranslate(Gf.Vec3d(100, 200, 300))
        camera.transform = newXform
        camera.horizontalAperture = 500.0

        # Verify that trying to SetFromCamera from a weaker edit target does not crash,
        # and does not modify any existing camera attributes.
        usdCamera.SetFromCamera(camera, 1.0)
        self.assertEqual(usdCamera.GetHorizontalApertureAttr().Get(1.0), 1.0)

        # Now use the stronger layer
        stage.SetEditTarget(layerA)

        # This should succeed
        usdCamera.SetFromCamera(camera, 1.0)

        self.assertEqual(usdCamera.GetHorizontalApertureAttr().Get(1.0), 500.0)
        self.assertEqual(usdCamera.ComputeLocalToWorldTransform(1.0), newXform)


if __name__ == '__main__':
    unittest.main()
