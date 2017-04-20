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

from pxr import Gf, Usd, UsdGeom
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

    def _CheckValues(self, camera, schema, time, skipTransformAndClippingPlanes = False):
        if not skipTransformAndClippingPlanes:
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
        if not skipTransformAndClippingPlanes:
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

        camera = usdCamera.GetCamera(1.0, Gf.Camera.ZUp)
        self._CheckValues(camera, usdCamera, 1.0, skipTransformAndClippingPlanes = True)

        self.assertEqual(camera.transform, Gf.Matrix4d( 3, 0, 0, 0,
                                                   0, 0, 3, 0,
                                                   0,-3, 0, 0,
                                                   0, 0, 0, 3))
        self.assertEqual(camera.clippingPlanes, [Gf.Vec4f(1.0, 3.0, -2.0, 4.0),
                                            Gf.Vec4f(8.0, 6.0, -7.0, 5.0)])

    def test_SetFromCamera(self):
        camera = Gf.Camera()

        usdStage = Usd.Stage.CreateInMemory()
        usdCamera = UsdGeom.Camera.Define(usdStage, '/camera')
        usdCameraProj = usdCamera.GetProjectionAttr()

        # test fall-back values
        self._CheckValues(camera, usdCamera, 1.0)
        self.assertEqual(usdCameraProj.GetResolveInfo().GetSource(),
                         Usd.ResolveInfoSourceFallback);

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

if __name__ == '__main__':
    unittest.main()
