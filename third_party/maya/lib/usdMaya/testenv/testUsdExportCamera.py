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

import os
import unittest

from pxr import Gf
from pxr import Usd
from pxr import UsdGeom
from pxr import Vt

from maya import cmds
from maya import standalone


class testUsdExportCamera(unittest.TestCase):

    START_TIMECODE = 1.0
    END_TIMECODE = 96.0
    TIMECODE_STEPS = END_TIMECODE - START_TIMECODE + 1.0


    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.file(os.path.abspath('UsdExportCameraTest.ma'),
                  open=True,
                  force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('UsdExportCameraTest.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none',
            frameRange=(cls.START_TIMECODE, cls.END_TIMECODE))

        cls.stage = Usd.Stage.Open(usdFilePath)


    def testStagePrerequisites(self):
        self.assertTrue(self.stage)

        self.assertEqual(self.stage.GetStartTimeCode(), self.START_TIMECODE)
        self.assertEqual(self.stage.GetEndTimeCode(), self.END_TIMECODE)

    def _GetUsdCamera(self, cameraName):
        cameraPrimPath = '/UsdExportCameraTest/Cameras/%s' % cameraName
        cameraPrim = self.stage.GetPrimAtPath(cameraPrimPath)
        self.assertTrue(cameraPrim)

        usdCamera = UsdGeom.Camera(cameraPrim)
        self.assertTrue(usdCamera)

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
            self.assertTrue(usdAttr)

            if expectedValue is None:
                self.assertFalse(usdAttr.HasAuthoredValue())
                continue

            # Validate the attribute value at usdTime
            value = usdAttr.Get(usdTime)
            if (isinstance(value, float) or
                    isinstance(value, Gf.Vec2f)):
                self.assertTrue(Gf.IsClose(value, expectedValue, 1e-6))
            else:
                self.assertEqual(value, expectedValue)

            # Validate the number of time samples on the attribute.
            expectedMaxAttrNumTimeSamples = 0.0
            if isAnimated:
                expectedMaxAttrNumTimeSamples = self.TIMECODE_STEPS
            numTimeSamples = usdAttr.GetNumTimeSamples()
            self.assertTrue(numTimeSamples <= expectedMaxAttrNumTimeSamples)

        # Validate the number of time samples on the camera's xformOp(s).
        expectedTransformNumTimeSamples = 0.0
        if isTransformAnimated:
            expectedTransformNumTimeSamples = self.TIMECODE_STEPS

        for xformOp in usdCamera.GetOrderedXformOps():
            numTimeSamples = xformOp.GetNumTimeSamples()
            self.assertTrue(Gf.IsClose(numTimeSamples, expectedTransformNumTimeSamples, 1e-6))

    def testExportStaticOrthographicCamera(self):
        usdCamera = self._GetUsdCamera('OrthoCamStatic')

        # There should be no animation on any of the camera attributes.
        expectedPropertyTuples = [
            ('clippingRange', Gf.Vec2f(1.0, 100.0), False),
            ('focalLength', 50.0, False),
            ('focusDistance', 5.0, False),
            ('fStop', 8.0, False),
            ('horizontalAperture', 119.05709, False),
            ('horizontalApertureOffset', None, False),
            ('projection', UsdGeom.Tokens.orthographic, False),
            ('verticalAperture', 119.05709, False),
            ('verticalApertureOffset', None, False),

            ('xformOpOrder', Vt.TokenArray(['xformOp:translate']), False),
        ]

        # There should be no animation on the transform either.
        self._ValidateUsdCamera(usdCamera, expectedPropertyTuples, False)

        # Validate the camera's xformOp at the default time.
        translateOp = usdCamera.GetOrderedXformOps()[0]
        self.assertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)

        self.assertTrue(Gf.IsClose(translateOp.Get(), Gf.Vec3d(0.0, 0.0, 5.0), 1e-6))

    def testExportStaticPerspectiveCamera(self):
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

            ('xformOpOrder', Vt.TokenArray(['xformOp:translate', 'xformOp:rotateXYZ']), False),
        ]

        # There should be no animation on the transform either.
        self._ValidateUsdCamera(usdCamera, expectedPropertyTuples, False)

        # Validate the camera's xformOps at the default time.
        (translateOp, rotateOp) = usdCamera.GetOrderedXformOps()
        self.assertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)
        self.assertEqual(rotateOp.GetOpType(), UsdGeom.XformOp.TypeRotateXYZ)
        self.assertTrue(UsdGeom.XformCommonAPI(usdCamera))

        self.assertTrue(Gf.IsClose(translateOp.Get(), Gf.Vec3d(0.0, -5.0, 5.0), 1e-6))
        self.assertTrue(
                Gf.IsClose(rotateOp.Get(), Gf.Vec3f(45.0, 0.0, 0.0), 1e-6))

    def testExportPerspectiveCameraAnimatedTransform(self):
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
        self.assertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)
        self.assertEqual(rotateOp.GetOpType(), UsdGeom.XformOp.TypeRotateXYZ)
        self.assertTrue(UsdGeom.XformCommonAPI(usdCamera))

        # The xformOps should NOT have values at the default time.
        self.assertEqual(translateOp.Get(), None)
        self.assertEqual(rotateOp.Get(), None)

        # Validate the camera's xformOps at a few non-default timecodes.
        self.assertTrue(Gf.IsClose(translateOp.Get(1), Gf.Vec3d(0.0, -5.0, 5.0), 1e-6))
        self.assertTrue(Gf.IsClose(translateOp.Get(25), Gf.Vec3d(5.0, 0.0, 5.0), 1e-6))
        self.assertTrue(Gf.IsClose(translateOp.Get(49), Gf.Vec3d(0.0, 5.0, 5.0), 1e-6))
        self.assertTrue(Gf.IsClose(translateOp.Get(73), Gf.Vec3d(-5.0, 0.0, 5.0), 1e-6))

        self.assertTrue(Gf.IsClose(rotateOp.Get(1), Gf.Vec3f(45.0, 0.0, 0.0), 1e-6))
        self.assertTrue(Gf.IsClose(rotateOp.Get(25), Gf.Vec3f(45.0, 0.0, 90.0), 1e-6))
        self.assertTrue(Gf.IsClose(rotateOp.Get(49), Gf.Vec3f(45.0, 0.0, 180.0), 1e-6))
        self.assertTrue(Gf.IsClose(rotateOp.Get(73), Gf.Vec3f(45.0, 0.0, 270.0), 1e-6))

    def testExportPerspectiveCameraAnimatedShape(self):
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

            ('xformOpOrder', Vt.TokenArray(['xformOp:translate', 'xformOp:rotateXYZ']), False),
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

            ('xformOpOrder', Vt.TokenArray(['xformOp:translate', 'xformOp:rotateXYZ']), False),
        ]

        # There should be no animation on the transform.
        self._ValidateUsdCamera(usdCamera, expectedPropertyTuples, False, self.START_TIMECODE)

        # The focal length should be animated on this camera, so validate the
        # value at a few timecodes.
        attr = usdCamera.GetFocalLengthAttr()
        self.assertTrue(Gf.IsClose(attr.Get(self.START_TIMECODE), 35.0, 1e-6))
        self.assertTrue(Gf.IsClose(attr.Get(self.END_TIMECODE), 100.0, 1e-6))

    def testExportOrthographicViewCheckCamera(self):
        """
        Tests exporting a specifically positioned orthographic camera. Looking
        through this camera in the Maya scene and in usdview should show a cube
        in each of the four corners of the frame.
        """
        usdCamera = self._GetUsdCamera('ViewCheck/OrthographicCamera')

        # There should be no animation on any of the camera attributes.
        expectedPropertyTuples = [
            ('clippingRange', Gf.Vec2f(0.1, 10000.0), False),
            ('focalLength', 35.0, False),
            ('focusDistance', 5.0, False),
            ('fStop', 5.6, False),
            ('horizontalAperture', 500, False),
            ('horizontalApertureOffset', None, False),
            ('projection', UsdGeom.Tokens.orthographic, False),
            ('verticalAperture', 500, False),
            ('verticalApertureOffset', None, False),

            ('xformOpOrder', Vt.TokenArray(['xformOp:translate', 'xformOp:rotateXYZ']), False),
        ]

        # There should be no animation on the transform either.
        self._ValidateUsdCamera(usdCamera, expectedPropertyTuples, False)

        # Validate the camera's xformOps at the default time.
        (translateOp, rotateOp) = usdCamera.GetOrderedXformOps()
        self.assertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)
        self.assertEqual(rotateOp.GetOpType(), UsdGeom.XformOp.TypeRotateXYZ)
        self.assertTrue(UsdGeom.XformCommonAPI(usdCamera))

        self.assertTrue(Gf.IsClose(translateOp.Get(), Gf.Vec3d(0.0, -20.0, 0.0), 1e-6))
        self.assertTrue(
                Gf.IsClose(rotateOp.Get(), Gf.Vec3f(90.0, 0.0, 0.0), 1e-6))

    def testExportPerspectiveViewCheckCamera(self):
        """
        Tests exporting a specifically positioned perspective camera. Looking
        through this camera in the Maya scene and in usdview should show a cube
        in each of the four corners of the frame.
        """
        usdCamera = self._GetUsdCamera('ViewCheck/PerspectiveCamera')

        # There should be no animation on any of the camera attributes.
        expectedPropertyTuples = [
            ('clippingRange', Gf.Vec2f(0.1, 10000.0), False),
            ('focalLength', 35.0, False),
            ('focusDistance', 5.0, False),
            ('fStop', 5.6, False),
            ('horizontalAperture', 29.6, False),
            ('horizontalApertureOffset', 0.0, False),
            ('projection', UsdGeom.Tokens.perspective, False),
            ('verticalAperture', 16.0, False),
            ('verticalApertureOffset', 0.0, False),

            ('xformOpOrder', Vt.TokenArray(['xformOp:translate', 'xformOp:rotateXYZ']), False),
        ]

        # There should be no animation on the transform either.
        self._ValidateUsdCamera(usdCamera, expectedPropertyTuples, False)

        # Validate the camera's xformOps at the default time.
        (translateOp, rotateOp) = usdCamera.GetOrderedXformOps()
        self.assertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)
        self.assertEqual(rotateOp.GetOpType(), UsdGeom.XformOp.TypeRotateXYZ)

        self.assertTrue(Gf.IsClose(translateOp.Get(), Gf.Vec3d(-25.0, 25.0, 25.0), 1e-6))
        self.assertTrue(Gf.IsClose(rotateOp.Get(), Gf.Vec3f(60.0, 0.0, -135.0), 1e-6))


if __name__ == '__main__':
    unittest.main(verbosity=2)
