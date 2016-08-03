#!/pxrpythonsubst

from pxr import Gf, Usd, UsdGeom

from Mentor.Runtime import (Runner,
                            Fixture,
                            Assert,
                            AssertTrue,
                            AssertFalse,
                            AssertClose,
                            AssertEqual,
                            AssertException)

class TestUsdGeomCamera(Fixture):

    def GetSchemaProjection(self, schema, time):
        val = schema.GetProjectionAttr().Get(time)
        if val == UsdGeom.Tokens.perspective:
            return Gf.Camera.Perspective
        if val == UsdGeom.Tokens.orthographic:
            return Gf.Camera.Orthographic
        return None

    def GetSchemaClippingRange(self, schema, time):
        val = schema.GetClippingRangeAttr().Get(time)
        return Gf.Range1f(val[0], val[1])

    def GetSchemaClippingPlanes(self, schema, time):
        val = schema.GetClippingPlanesAttr().Get(time)
        return [Gf.Vec4f(float(i[0]),
                         float(i[1]),
                         float(i[2]),
                         float(i[3])) for i in val]

    def AssertEqual(self, camera, schema, time, skipTransformAndClippingPlanes = False):
        if not skipTransformAndClippingPlanes:
            AssertEqual(camera.transform, schema.GetLocalTransformation(Usd.TimeCode(time)))
        AssertEqual(camera.projection, self.GetSchemaProjection(schema, time))
        AssertEqual(camera.horizontalAperture, schema.GetHorizontalApertureAttr().Get(time))
        AssertEqual(camera.verticalAperture, schema.GetVerticalApertureAttr().Get(time))
        AssertEqual(camera.horizontalApertureOffset,
                    schema.GetHorizontalApertureOffsetAttr().Get(time))
        AssertEqual(camera.verticalApertureOffset,
                    schema.GetVerticalApertureOffsetAttr().Get(time))
        AssertEqual(camera.focalLength, schema.GetFocalLengthAttr().Get(time))
        AssertEqual(camera.clippingRange, self.GetSchemaClippingRange(schema, time))
        if not skipTransformAndClippingPlanes:
            AssertEqual(camera.clippingPlanes, self.GetSchemaClippingPlanes(schema, time))
        AssertClose(camera.fStop, schema.GetFStopAttr().Get(time))
        AssertEqual(camera.focusDistance, schema.GetFocusDistanceAttr().Get(time))

    def TestGetCamera(self):
        usdStage = Usd.Stage.CreateInMemory()
        usdCamera = UsdGeom.Camera.Define(usdStage, '/camera')

        # test fall-back values
        self.AssertEqual(Gf.Camera(), usdCamera, 1.0)

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
        self.AssertEqual(camera, usdCamera, 1.0)

        camera = usdCamera.GetCamera(1.0, Gf.Camera.ZUp)
        self.AssertEqual(camera, usdCamera, 1.0, skipTransformAndClippingPlanes = True)

        AssertEqual(camera.transform, Gf.Matrix4d( 3, 0, 0, 0,
                                                   0, 0, 3, 0,
                                                   0,-3, 0, 0,
                                                   0, 0, 0, 3))
        AssertEqual(camera.clippingPlanes, [Gf.Vec4f(1.0, 3.0, -2.0, 4.0),
                                            Gf.Vec4f(8.0, 6.0, -7.0, 5.0)])

    def TestSetFromCamera(self):
        camera = Gf.Camera()

        usdStage = Usd.Stage.CreateInMemory()
        usdCamera = UsdGeom.Camera.Define(usdStage, '/camera')

        # test fall-back values
        self.AssertEqual(camera, usdCamera, 1.0)

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
        self.AssertEqual(camera, usdCamera, 1.0)

        usdCamera.SetFromCamera(camera, 1.0)
        AssertClose(
            usdCamera.GetLocalTransformation(Usd.TimeCode(1.0)),
            Gf.Matrix4d( 0.9858929135, 0.14139860385,-0.089563373740, 0.0,
                        -0.1370579618, 0.98914839500, 0.052920390613, 0.0,
                         0.0960743367,-0.03989846462, 0.994574197504, 0.0,
                         4.0         , 5.0          , 6.0           , 1.0),
            epsilon = 1e-6)

if __name__ == '__main__':
    Runner().Main()
