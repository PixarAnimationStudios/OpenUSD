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
import math
import unittest

from pxr import Gf
from pxr import Usd

from maya import cmds
from maya import standalone
from maya import OpenMaya
from maya import OpenMayaAnim


class testUsdImportCamera(unittest.TestCase):

    START_TIMECODE = 1.0
    END_TIMECODE = 96.0
    TIMECODE_STEPS = END_TIMECODE - START_TIMECODE + 1.0

    @staticmethod
    def _ConvertRadiansToDegrees(radians):
        return (180.0 / math.pi) * radians

    @staticmethod
    def _ConvertDegreesToRadians(degrees):
        return (math.pi / 180.0) * degrees

    @staticmethod
    def _ConvertMMToInches(mm):
        return mm / 25.4


    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.file(new=True, force=True)

        # Import the USD file.
        usdFilePath = os.path.abspath('UsdImportCameraTest.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdImport(file=usdFilePath, readAnimData=True)

        cls._stage = Usd.Stage.Open(usdFilePath)
        
    def testStageOpens(self):
        self.assertTrue(self._stage)

        self.assertEqual(self._stage.GetStartTimeCode(), self.START_TIMECODE)
        self.assertEqual(self._stage.GetEndTimeCode(), self.END_TIMECODE)

    def _GetMayaCameraAndTransform(self, cameraName):
        cameraTransformFullName = '|UsdImportCameraTest|Cameras|%s' % cameraName
        cameraShapeFullName = '%s|%sShape' % (cameraTransformFullName, cameraName)
        selectionList = OpenMaya.MSelectionList()
        selectionList.add(cameraShapeFullName)
        cameraObj = OpenMaya.MObject()
        selectionList.getDependNode(0, cameraObj)

        cameraFn = OpenMaya.MFnCamera(cameraObj)

        parentObj = cameraFn.parent(0)
        self.assertTrue(parentObj.hasFn(OpenMaya.MFn.kTransform))

        transformFn = OpenMaya.MFnTransform(parentObj)

        return (cameraFn, transformFn)

    def testImportStaticOrthographicCamera(self):
        (cameraFn, transformFn) = self._GetMayaCameraAndTransform('OrthoCamStatic')

        self.assertTrue(Gf.IsClose(cameraFn.nearClippingPlane(), 1.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.farClippingPlane(), 100.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.focalLength(), 50.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.focusDistance(), 5.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.fStop(), 8.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.horizontalFilmAperture(), 1.377953, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.horizontalFilmOffset(), 0.0, 1e-6))
        self.assertTrue(cameraFn.isOrtho())
        self.assertTrue(Gf.IsClose(cameraFn.verticalFilmAperture(), 1.377953, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.verticalFilmOffset(), 0.0, 1e-6))

        translation = transformFn.getTranslation(OpenMaya.MSpace.kTransform)
        transVec = Gf.Vec3d(translation.x, translation.y, translation.z)
        self.assertTrue(Gf.IsClose(transVec, Gf.Vec3d(0.0, 0.0, 5.0), 1e-6))

        # There should be no animation on either the camera shape or the transform.
        self.assertFalse(
            OpenMayaAnim.MAnimUtil.isAnimated(cameraFn.object()))
        self.assertFalse(
            OpenMayaAnim.MAnimUtil.isAnimated(transformFn.object()))

    def testImportStaticPerspectiveCamera(self):
        (cameraFn, transformFn) = self._GetMayaCameraAndTransform('PerspCamStatic')

        self.assertTrue(Gf.IsClose(cameraFn.nearClippingPlane(), 0.1, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.farClippingPlane(), 10000.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.focalLength(), 35.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.focusDistance(), 5.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.fStop(), 11.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.horizontalFilmAperture(), 1.417322, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.horizontalFilmOffset(), 1.0, 1e-6))
        self.assertFalse(cameraFn.isOrtho())
        self.assertTrue(Gf.IsClose(cameraFn.verticalFilmAperture(), 0.944882, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.verticalFilmOffset(), 2.0, 1e-6))

        rotation = OpenMaya.MEulerRotation()
        transformFn.getRotation(rotation)
        rotVec = Gf.Vec3d(self._ConvertRadiansToDegrees(rotation.x),
                          self._ConvertRadiansToDegrees(rotation.y),
                          self._ConvertRadiansToDegrees(rotation.z))
        self.assertTrue(Gf.IsClose(rotVec, Gf.Vec3d(45.0, 0.0, 0.0), 1e-6))

        translation = transformFn.getTranslation(OpenMaya.MSpace.kTransform)
        transVec = Gf.Vec3d(translation.x, translation.y, translation.z)
        self.assertTrue(Gf.IsClose(transVec, Gf.Vec3d(0.0, -5.0, 5.0), 1e-6))

        # There should be no animation on either the camera shape or the transform.
        self.assertFalse(OpenMayaAnim.MAnimUtil.isAnimated(cameraFn.object()))
        self.assertFalse(OpenMayaAnim.MAnimUtil.isAnimated(transformFn.object()))

    def _ValidateAnimatedPlugNames(self, depNodeFn, expectedPlugNames):
        animatedPlugs = OpenMaya.MPlugArray()
        OpenMayaAnim.MAnimUtil.findAnimatedPlugs(depNodeFn.object(), animatedPlugs)

        self.assertEqual(animatedPlugs.length(), len(expectedPlugNames))

        # We can't do this with a set comprehension because it hangs Maya for
        # some reason...
        animatedPlugNames = set()
        for i in range(animatedPlugs.length()):
            plug = animatedPlugs[i]
            animatedPlugNames.add(plug.name())

        self.assertEqual(expectedPlugNames, animatedPlugNames)

    def _GetAnimCurveFnForPlugName(self, depNodeFn, plugName):
        plug = depNodeFn.findPlug(plugName)
        animObjs = OpenMaya.MObjectArray()
        OpenMayaAnim.MAnimUtil.findAnimation(plug, animObjs)
        self.assertEqual(animObjs.length(), 1)
        animCurveFn = OpenMayaAnim.MFnAnimCurve(animObjs[0])
        return animCurveFn

    def _ValidateAnimValuesAtTimes(self, animCurveFn, expectedTimesToValues):
        for time, expectedValue in expectedTimesToValues.iteritems():
            value = animCurveFn.evaluate(OpenMaya.MTime(time))
            self.assertTrue(Gf.IsClose(expectedValue, value, 1e-6))

    def testImportPerspectiveCameraAnimatedTransform(self):
        (cameraFn, transformFn) = self._GetMayaCameraAndTransform('PerspCamAnimTransform')

        self.assertTrue(Gf.IsClose(cameraFn.nearClippingPlane(), 0.1, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.farClippingPlane(), 10000.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.focalLength(), 35.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.focusDistance(), 5.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.fStop(), 5.6, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.horizontalFilmAperture(), 1.417322, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.horizontalFilmOffset(), 0.0, 1e-6))
        self.assertFalse(cameraFn.isOrtho())
        self.assertTrue(Gf.IsClose(cameraFn.verticalFilmAperture(), 0.944882, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.verticalFilmOffset(), 0.0, 1e-6))

        rotation = OpenMaya.MEulerRotation()
        transformFn.getRotation(rotation)
        rotVec = Gf.Vec3d(self._ConvertRadiansToDegrees(rotation.x),
                          self._ConvertRadiansToDegrees(rotation.y),
                          self._ConvertRadiansToDegrees(rotation.z))
        self.assertTrue(Gf.IsClose(rotVec, Gf.Vec3d(45.0, 0.0, 0.0), 1e-6))

        translation = transformFn.getTranslation(OpenMaya.MSpace.kTransform)
        transVec = Gf.Vec3d(translation.x, translation.y, translation.z)
        self.assertTrue(Gf.IsClose(transVec, Gf.Vec3d(0.0, -5.0, 5.0), 1e-6))

        # There should be no animation on the camera shape.
        self.assertFalse(OpenMayaAnim.MAnimUtil.isAnimated(cameraFn.object()))

        # There SHOULD be animation on the transform
        self.assertTrue(OpenMayaAnim.MAnimUtil.isAnimated(transformFn.object()))

        # Translate X and Y and rotate Z are "animated" (keyed AND varying).
        # translatorXformable will not create keys for channels that are not
        # varying.
        expectedPlugNames = {'PerspCamAnimTransform.translateX',
                             'PerspCamAnimTransform.translateY',
                             'PerspCamAnimTransform.rotateZ'}

        self._ValidateAnimatedPlugNames(transformFn, expectedPlugNames)

        # Rotation should be keyed only at timeCodes 1, 25, 49, 73, and 96.
        animCurveFn = self._GetAnimCurveFnForPlugName(transformFn, 'rotateZ')
        self.assertEqual(animCurveFn.numKeys(), 5)
        expectedTimesToValues = {
            1.0: self._ConvertDegreesToRadians(0.0),
            25.0: self._ConvertDegreesToRadians(90.0),
            49.0: self._ConvertDegreesToRadians(180.0),
            73.0: self._ConvertDegreesToRadians(270.0),
            96.0: self._ConvertDegreesToRadians(359.694),
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Translation should be keyed at ALL timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(transformFn, 'translateX')
        self.assertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1: 0.0,
            25: 5.0,
            49: 0.0,
            73: -5.0,
            96: -0.025318,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        animCurveFn = self._GetAnimCurveFnForPlugName(transformFn, 'translateY')
        self.assertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1: -5.0,
            25: 0.0,
            49: 5.0,
            73: 0.0,
            96: -4.983,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

    def testImportPerspectiveCameraAnimatedShape(self):
        (cameraFn, transformFn) = self._GetMayaCameraAndTransform('PerspCamAnimShape')

        self.assertTrue(Gf.IsClose(cameraFn.nearClippingPlane(), 0.1, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.farClippingPlane(), 10000.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.focalLength(), 35.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.focusDistance(), 5.0, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.fStop(), 5.6, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.horizontalFilmAperture(), 1.417322, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.horizontalFilmOffset(), 0.0, 1e-6))
        self.assertFalse(cameraFn.isOrtho())
        self.assertTrue(Gf.IsClose(cameraFn.verticalFilmAperture(), 0.944882, 1e-6))
        self.assertTrue(Gf.IsClose(cameraFn.verticalFilmOffset(), 0.0, 1e-6))

        rotation = OpenMaya.MEulerRotation()
        transformFn.getRotation(rotation)
        rotVec = Gf.Vec3d(self._ConvertRadiansToDegrees(rotation.x),
                          self._ConvertRadiansToDegrees(rotation.y),
                          self._ConvertRadiansToDegrees(rotation.z))
        self.assertTrue(Gf.IsClose(rotVec, Gf.Vec3d(45.0, 0.0, 0.0), 1e-6))

        translation = transformFn.getTranslation(OpenMaya.MSpace.kTransform)
        transVec = Gf.Vec3d(translation.x, translation.y, translation.z)
        self.assertTrue(Gf.IsClose(transVec, Gf.Vec3d(0.0, -5.0, 5.0), 1e-6))

        # There SHOULD be animation on the camera shape.
        self.assertTrue(OpenMayaAnim.MAnimUtil.isAnimated(cameraFn.object()))

        # There should be no animation on the transform
        self.assertFalse(OpenMayaAnim.MAnimUtil.isAnimated(transformFn.object()))

        # All of the camera shape attributes (expect ortho/perspective) should
        # be animated.
        expectedPlugNames = set([
            'PerspCamAnimShapeShape.nearClipPlane',
            'PerspCamAnimShapeShape.farClipPlane',
            'PerspCamAnimShapeShape.focalLength',
            'PerspCamAnimShapeShape.focusDistance',
            'PerspCamAnimShapeShape.fStop',
            'PerspCamAnimShapeShape.horizontalFilmAperture',
            'PerspCamAnimShapeShape.horizontalFilmOffset',
            'PerspCamAnimShapeShape.verticalFilmAperture',
            'PerspCamAnimShapeShape.verticalFilmOffset'])

        self._ValidateAnimatedPlugNames(cameraFn, expectedPlugNames)

        # The clipping planes should be keyed at three timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'nearClipPlane')
        self.assertEqual(animCurveFn.numKeys(), 3)
        expectedTimesToValues = {
            1.0: 0.1,
            49.0: 0.5,
            96.0: 1.0,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'farClipPlane')
        self.assertEqual(animCurveFn.numKeys(), 3)
        expectedTimesToValues = {
            1.0: 10000.0,
            49.0: 9000.0,
            96.0: 8000.0,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Focal length is keyed over the first 25 timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'focalLength')
        self.assertEqual(animCurveFn.numKeys(), 25)
        expectedTimesToValues = {
            1.0: 35.0,
            13.0: 67.5,
            25.0: 100.0,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Focus distance is keyed at ALL timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'focusDistance')
        self.assertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1.0: 5.0,
            30.0: 5.0,
            60.0: 5.0,
            90.0: 5.0,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # fStop is keyed at ALL timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'fStop')
        self.assertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1.0: 5.6,
            30.0: 5.6,
            60.0: 5.6,
            90.0: 5.6,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Horizontal aperture is keyed at ALL timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'horizontalFilmAperture')
        self.assertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1.0: self._ConvertMMToInches(36.0),
            30.0: self._ConvertMMToInches(36.0),
            60.0: self._ConvertMMToInches(36.0),
            90.0: self._ConvertMMToInches(36.0),
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Horizontal aperture offset is keyed at ALL timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'horizontalFilmOffset')
        self.assertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1.0: 0.0,
            30.0: 0.0,
            60.0: 0.0,
            90.0: 0.0,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Vertical aperture is keyed at ALL timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'verticalFilmAperture')
        self.assertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1.0: self._ConvertMMToInches(24.0),
            30.0: self._ConvertMMToInches(24.0),
            60.0: self._ConvertMMToInches(24.0),
            90.0: self._ConvertMMToInches(24.0),
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Vertical aperture offset is keyed at three timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'verticalFilmOffset')
        self.assertEqual(animCurveFn.numKeys(), 3)
        expectedTimesToValues = {
            1.0: 0.0,
            49.0: self._ConvertMMToInches(25.4),
            96.0: self._ConvertMMToInches(50.8),
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)


if __name__ == '__main__':
    unittest.main(verbosity=2)
