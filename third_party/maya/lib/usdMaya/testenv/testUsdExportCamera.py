#!/bedrock_subst/bin/pypixb

# Initialize maya.
import pixrun
pixrun.setupEnv()
from pixar import pxMayaVersion
pxMayaVersion.setup(version='mayaversion_subst')
import maya.standalone
maya.standalone.initialize('pypix')

import os

from pxr import Gf
from pxr import Usd
from pxr import UsdGeom
from pxr import Vt

from Mentor.Runtime import Assert
from Mentor.Runtime import AssertEqual
from Mentor.Runtime import AssertTrue
from Mentor.Runtime import FindDataFile
from Mentor.Runtime import Fixture
from Mentor.Runtime import Runner


class testUsdExportCamera(Fixture):

    START_TIMECODE = 1.0
    END_TIMECODE = 96.0
    TIMECODE_STEPS = END_TIMECODE - START_TIMECODE + 1.0

    def ClassSetup(self):
        from maya import cmds

        cmds.file(FindDataFile('UsdExportCameraTest.ma'), open=True, force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('UsdExportCameraTest.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none',
            frameRange=(self.START_TIMECODE, self.END_TIMECODE))

        self.stage = Usd.Stage.Open(usdFilePath)
        Assert(self.stage)

        AssertEqual(self.stage.GetStartTimeCode(), self.START_TIMECODE)
        AssertEqual(self.stage.GetEndTimeCode(), self.END_TIMECODE)

    def _GetUsdCamera(self, cameraName):
        cameraPrimPath = '/UsdExportCameraTest/Cameras/%s' % cameraName
        cameraPrim = self.stage.GetPrimAtPath(cameraPrimPath)
        Assert(cameraPrim)

        usdCamera = UsdGeom.Camera(cameraPrim)
        Assert(usdCamera)

        return usdCamera

    def _ValidateUsdCamera(self, usdCamera, expectedPropertyTuples,
            isTransformAnimated, usdTime=None):
        if usdTime is None:
            usdTime = Usd.TimeCode.Default()

        schemaAttrNames = usdCamera.GetSchemaAttributeNames()

        for expectedPropertyTuple in expectedPropertyTuples:
            (propertyName, expectedValue, isAnimated) = expectedPropertyTuple

            if propertyName not in schemaAttrNames:
                raise ValueError('Invalid property name: %s' % propertyName)

            usdAttr = usdCamera.GetPrim().GetAttribute(propertyName)
            Assert(usdAttr)

            # Validate the attribute value at usdTime
            value = usdAttr.Get(usdTime)
            if (isinstance(value, float) or
                    isinstance(value, Gf.Vec2f)):
                AssertTrue(Gf.IsClose(value, expectedValue, 1e-6))
            else:
                AssertEqual(value, expectedValue)

            # Validate the number of time samples on the attribute.
            expectedAttrNumTimeSamples = 0.0
            if isAnimated:
                expectedAttrNumTimeSamples = self.TIMECODE_STEPS
            numTimeSamples = usdAttr.GetNumTimeSamples()
            AssertTrue(Gf.IsClose(numTimeSamples, expectedAttrNumTimeSamples, 1e-6))

        # Validate the number of time samples on the camera's xformOp(s).
        expectedTransformNumTimeSamples = 0.0
        if isTransformAnimated:
            expectedTransformNumTimeSamples = self.TIMECODE_STEPS

        for xformOp in usdCamera.GetOrderedXformOps():
            numTimeSamples = xformOp.GetNumTimeSamples()
            AssertTrue(Gf.IsClose(numTimeSamples, expectedTransformNumTimeSamples, 1e-6))

    def TestExportStaticOrthographicCamera(self):
        usdCamera = self._GetUsdCamera('OrthoCamStatic')

        # There should be no animation on any of the camera attributes.
        expectedPropertyTuples = [
            ('clippingRange', Gf.Vec2f(1.0, 100.0), False),
            ('focalLength', 50.0, False),
            ('focusDistance', 5.0, False),
            ('fStop', 8.0, False),
            ('horizontalAperture', 35.0, False),
            ('horizontalApertureOffset', 0.0, False),
            ('projection', UsdGeom.Tokens.orthographic, False),
            ('verticalAperture', 35.0, False),
            ('verticalApertureOffset', 0.0, False),

            ('xformOpOrder', Vt.TokenArray(['xformOp:translate']), False),
        ]

        # There should be no animation on the transform either.
        self._ValidateUsdCamera(usdCamera, expectedPropertyTuples, False)

        # Validate the camera's xformOp at the default time.
        translateOp = usdCamera.GetOrderedXformOps()[0]
        AssertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)

        AssertTrue(Gf.IsClose(translateOp.Get(), Gf.Vec3d(0.0, 0.0, 5.0), 1e-6))

    def TestExportStaticPerspectiveCamera(self):
        usdCamera = self._GetUsdCamera('PerspCamStatic')

        # There should be no animation on any of the camera attributes.
        expectedPropertyTuples = [
            ('clippingRange', Gf.Vec2f(0.1, 10000.0), False),
            ('focalLength', 35.0, False),
            ('focusDistance', 5.0, False),
            ('fStop', 11.0, False),
            ('horizontalAperture', 36.0, False),
            ('horizontalApertureOffset', 0.0, False),
            ('projection', UsdGeom.Tokens.perspective, False),
            ('verticalAperture', 24.0, False),
            ('verticalApertureOffset', 0.0, False),

            ('xformOpOrder', Vt.TokenArray(['xformOp:translate', 'xformOp:rotateX']), False),
        ]

        # There should be no animation on the transform either.
        self._ValidateUsdCamera(usdCamera, expectedPropertyTuples, False)

        # Validate the camera's xformOps at the default time.
        (translateOp, rotateOp) = usdCamera.GetOrderedXformOps()
        AssertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)
        AssertEqual(rotateOp.GetOpType(), UsdGeom.XformOp.TypeRotateX)

        AssertTrue(Gf.IsClose(translateOp.Get(), Gf.Vec3d(0.0, -5.0, 5.0), 1e-6))
        AssertTrue(Gf.IsClose(rotateOp.Get(), 45.0, 1e-6))

    def TestExportPerspectiveCameraAnimatedTransform(self):
        usdCamera = self._GetUsdCamera('PerspCamAnimTransform')

        # There should be no animation on any of the camera attributes.
        expectedPropertyTuples = [
            ('clippingRange', Gf.Vec2f(0.1, 10000.0), False),
            ('focalLength', 35.0, False),
            ('focusDistance', 5.0, False),
            ('fStop', 5.6, False),
            ('horizontalAperture', 36.0, False),
            ('horizontalApertureOffset', 0.0, False),
            ('projection', UsdGeom.Tokens.perspective, False),
            ('verticalAperture', 24.0, False),
            ('verticalApertureOffset', 0.0, False),

            ('xformOpOrder', Vt.TokenArray(['xformOp:translate', 'xformOp:rotateXYZ']), False),
        ]

        # There SHOULD be animation on the transform.
        self._ValidateUsdCamera(usdCamera, expectedPropertyTuples, True)

        # Get the camera's xformOps.
        (translateOp, rotateOp) = usdCamera.GetOrderedXformOps()
        AssertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)
        AssertEqual(rotateOp.GetOpType(), UsdGeom.XformOp.TypeRotateXYZ)

        # The xformOps should NOT have values at the default time.
        AssertEqual(translateOp.Get(), None)
        AssertEqual(rotateOp.Get(), None)

        # Validate the camera's xformOps at a few non-default timecodes.
        AssertTrue(Gf.IsClose(translateOp.Get(1), Gf.Vec3d(0.0, -5.0, 5.0), 1e-6))
        AssertTrue(Gf.IsClose(translateOp.Get(25), Gf.Vec3d(5.0, 0.0, 5.0), 1e-6))
        AssertTrue(Gf.IsClose(translateOp.Get(49), Gf.Vec3d(0.0, 5.0, 5.0), 1e-6))
        AssertTrue(Gf.IsClose(translateOp.Get(73), Gf.Vec3d(-5.0, 0.0, 5.0), 1e-6))

        AssertTrue(Gf.IsClose(rotateOp.Get(1), Gf.Vec3f(45.0, 0.0, 0.0), 1e-6))
        AssertTrue(Gf.IsClose(rotateOp.Get(25), Gf.Vec3f(45.0, 0.0, 90.0), 1e-6))
        AssertTrue(Gf.IsClose(rotateOp.Get(49), Gf.Vec3f(45.0, 0.0, 180.0), 1e-6))
        AssertTrue(Gf.IsClose(rotateOp.Get(73), Gf.Vec3f(45.0, 0.0, 270.0), 1e-6))

    def TestExportPerspectiveCameraAnimatedShape(self):
        usdCamera = self._GetUsdCamera('PerspCamAnimShape')

        # Since the focalLength on the Maya camera shape is animated, all of
        # the attributes from the camera shape will have time samples.
        # However, the values on the USD camera at the default time should 
        # not have been written and should still be the schema default values.
        expectedPropertyTuples = [
            ('clippingRange', Gf.Vec2f(1, 1000000.0), True),
            ('focalLength', 50.0, True),
            ('horizontalAperture', 20.955, True),
            ('horizontalApertureOffset', 0.0, True),
            ('projection', UsdGeom.Tokens.perspective, True),
            ('verticalAperture', 15.2908, True),
            ('verticalApertureOffset', 0.0, True),

            ('xformOpOrder', Vt.TokenArray(['xformOp:translate', 'xformOp:rotateX']), False),
        ]

        # There should be no animation on the transform.
        self._ValidateUsdCamera(usdCamera, expectedPropertyTuples, False)

        # Now validate again at a non-default time and we should see the values
        # authored in Maya.
        expectedPropertyTuples = [
            ('clippingRange', Gf.Vec2f(0.1, 10000.0), True),
            ('focalLength', 35.0, True),
            ('focusDistance', 5.0, True),
            ('fStop', 5.6, True),
            ('horizontalAperture', 36.0, True),
            ('horizontalApertureOffset', 0.0, True),
            ('projection', UsdGeom.Tokens.perspective, True),
            ('verticalAperture', 24.0, True),
            ('verticalApertureOffset', 0.0, True),

            ('xformOpOrder', Vt.TokenArray(['xformOp:translate', 'xformOp:rotateX']), False),
        ]

        # There should be no animation on the transform.
        self._ValidateUsdCamera(usdCamera, expectedPropertyTuples, False, self.START_TIMECODE)

        # The focal length should be animated on this camera, so validate the
        # value at a few timecodes.
        attr = usdCamera.GetFocalLengthAttr()
        AssertTrue(Gf.IsClose(attr.Get(self.START_TIMECODE), 35.0, 1e-6))
        AssertTrue(Gf.IsClose(attr.Get(self.END_TIMECODE), 100.0, 1e-6))


if __name__ == '__main__':
    Runner().Main()
