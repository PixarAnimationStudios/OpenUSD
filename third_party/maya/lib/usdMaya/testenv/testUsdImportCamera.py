#!/bedrock_subst/bin/pypixb

# Initialize maya.
import pixrun
pixrun.setupEnv()
from pixar import pxMayaVersion
pxMayaVersion.setup(version='mayaversion_subst')
import maya.standalone
maya.standalone.initialize('pypix')

from maya import cmds
from maya import OpenMaya as OM
from maya import OpenMayaAnim as OMA

from pxr import Gf
from pxr import Usd

import pixar.mayaUtil.api

from Mentor.Runtime import Assert
from Mentor.Runtime import AssertEqual
from Mentor.Runtime import AssertFalse
from Mentor.Runtime import AssertTrue
from Mentor.Runtime import FindDataFile
from Mentor.Runtime import Fixture
from Mentor.Runtime import Runner

import math


class testUsdImportCamera(Fixture):

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

    def ClassSetup(self):
        cmds.file(new=True, force=True)

        # Import the USD file.
        usdFilePath = FindDataFile('UsdImportCameraTest.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdImport(file=usdFilePath, readAnimData=True)

        self.stage = Usd.Stage.Open(usdFilePath)
        Assert(self.stage)

        AssertEqual(self.stage.GetStartTimeCode(), self.START_TIMECODE)
        AssertEqual(self.stage.GetEndTimeCode(), self.END_TIMECODE)

    def _GetMayaCameraAndTransform(self, cameraName):
        cameraTransformFullName = '|UsdImportCameraTest|Cameras|%s' % cameraName
        cameraShapeFullName = '%s|%sShape' % (cameraTransformFullName, cameraName)
        cameraObj = pixar.mayaUtil.api.NameToMObject(cameraShapeFullName)

        cameraFn = OM.MFnCamera(cameraObj)

        parentObj = cameraFn.parent(0)
        AssertTrue(parentObj.hasFn(OM.MFn.kTransform))

        transformFn = OM.MFnTransform(parentObj)

        return (cameraFn, transformFn)

    def TestImportStaticOrthographicCamera(self):
        (cameraFn, transformFn) = self._GetMayaCameraAndTransform('OrthoCamStatic')

        AssertTrue(Gf.IsClose(cameraFn.nearClippingPlane(), 1.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.farClippingPlane(), 100.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.focalLength(), 50.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.focusDistance(), 5.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.fStop(), 8.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.horizontalFilmAperture(), 1.377953, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.horizontalFilmOffset(), 0.0, 1e-6))
        AssertTrue(cameraFn.isOrtho())
        AssertTrue(Gf.IsClose(cameraFn.verticalFilmAperture(), 1.377953, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.verticalFilmOffset(), 0.0, 1e-6))

        translation = transformFn.getTranslation(OM.MSpace.kTransform)
        transVec = Gf.Vec3d(translation.x, translation.y, translation.z)
        AssertTrue(Gf.IsClose(transVec, Gf.Vec3d(0.0, 0.0, 5.0), 1e-6))

        # There should be no animation on either the camera shape or the transform.
        AssertFalse(OMA.MAnimUtil.isAnimated(cameraFn.object()))
        AssertFalse(OMA.MAnimUtil.isAnimated(transformFn.object()))

    def TestImportStaticPerspectiveCamera(self):
        (cameraFn, transformFn) = self._GetMayaCameraAndTransform('PerspCamStatic')

        AssertTrue(Gf.IsClose(cameraFn.nearClippingPlane(), 0.1, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.farClippingPlane(), 10000.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.focalLength(), 35.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.focusDistance(), 5.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.fStop(), 11.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.horizontalFilmAperture(), 1.417322, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.horizontalFilmOffset(), 1.0, 1e-6))
        AssertFalse(cameraFn.isOrtho())
        AssertTrue(Gf.IsClose(cameraFn.verticalFilmAperture(), 0.944882, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.verticalFilmOffset(), 2.0, 1e-6))

        rotation = OM.MEulerRotation()
        transformFn.getRotation(rotation)
        rotVec = Gf.Vec3d(self._ConvertRadiansToDegrees(rotation.x),
                          self._ConvertRadiansToDegrees(rotation.y),
                          self._ConvertRadiansToDegrees(rotation.z))
        AssertTrue(Gf.IsClose(rotVec, Gf.Vec3d(45.0, 0.0, 0.0), 1e-6))

        translation = transformFn.getTranslation(OM.MSpace.kTransform)
        transVec = Gf.Vec3d(translation.x, translation.y, translation.z)
        AssertTrue(Gf.IsClose(transVec, Gf.Vec3d(0.0, -5.0, 5.0), 1e-6))

        # There should be no animation on either the camera shape or the transform.
        AssertFalse(OMA.MAnimUtil.isAnimated(cameraFn.object()))
        AssertFalse(OMA.MAnimUtil.isAnimated(transformFn.object()))

    @staticmethod
    def _ValidateAnimatedPlugNames(depNodeFn, expectedPlugNames):
        animatedPlugs = OM.MPlugArray()
        OMA.MAnimUtil.findAnimatedPlugs(depNodeFn.object(), animatedPlugs)

        AssertEqual(animatedPlugs.length(), len(expectedPlugNames))

        # We can't do this with a set comprehension because it hangs Maya for
        # some reason...
        animatedPlugNames = set()
        for i in range(animatedPlugs.length()):
            plug = animatedPlugs[i]
            animatedPlugNames.add(plug.name())

        AssertEqual(expectedPlugNames, animatedPlugNames)

    @staticmethod
    def _GetAnimCurveFnForPlugName(depNodeFn, plugName):
        plug = depNodeFn.findPlug(plugName)
        animObjs = OM.MObjectArray()
        OMA.MAnimUtil.findAnimation(plug, animObjs)
        AssertEqual(animObjs.length(), 1)
        animCurveFn = OMA.MFnAnimCurve(animObjs[0])
        return animCurveFn

    @staticmethod
    def _ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues):
        for time, expectedValue in expectedTimesToValues.iteritems():
            value = animCurveFn.evaluate(OM.MTime(time))
            AssertTrue(Gf.IsClose(expectedValue, value, 1e-6))

    def TestImportPerspectiveCameraAnimatedTransform(self):
        (cameraFn, transformFn) = self._GetMayaCameraAndTransform('PerspCamAnimTransform')

        AssertTrue(Gf.IsClose(cameraFn.nearClippingPlane(), 0.1, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.farClippingPlane(), 10000.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.focalLength(), 35.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.focusDistance(), 5.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.fStop(), 5.6, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.horizontalFilmAperture(), 1.417322, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.horizontalFilmOffset(), 0.0, 1e-6))
        AssertFalse(cameraFn.isOrtho())
        AssertTrue(Gf.IsClose(cameraFn.verticalFilmAperture(), 0.944882, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.verticalFilmOffset(), 0.0, 1e-6))

        rotation = OM.MEulerRotation()
        transformFn.getRotation(rotation)
        rotVec = Gf.Vec3d(self._ConvertRadiansToDegrees(rotation.x),
                          self._ConvertRadiansToDegrees(rotation.y),
                          self._ConvertRadiansToDegrees(rotation.z))
        AssertTrue(Gf.IsClose(rotVec, Gf.Vec3d(45.0, 0.0, 0.0), 1e-6))

        translation = transformFn.getTranslation(OM.MSpace.kTransform)
        transVec = Gf.Vec3d(translation.x, translation.y, translation.z)
        AssertTrue(Gf.IsClose(transVec, Gf.Vec3d(0.0, -5.0, 5.0), 1e-6))

        # There should be no animation on the camera shape.
        AssertFalse(OMA.MAnimUtil.isAnimated(cameraFn.object()))

        # There SHOULD be animation on the transform
        AssertTrue(OMA.MAnimUtil.isAnimated(transformFn.object()))

        # Translate X and Y and rotate Z are "animated" (keyed AND varying).
        # translatorXformable will not create keys for channels that are not
        # varying.
        expectedPlugNames = set([
            'PerspCamAnimTransform.translateX',
            'PerspCamAnimTransform.translateY',
            'PerspCamAnimTransform.rotateZ'])

        self._ValidateAnimatedPlugNames(transformFn, expectedPlugNames)

        # Rotation should be keyed only at timeCodes 1, 25, 49, 73, and 96.
        animCurveFn = self._GetAnimCurveFnForPlugName(transformFn, 'rotateZ')
        AssertEqual(animCurveFn.numKeys(), 5)
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
        AssertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1: 0.0,
            25: 5.0,
            49: 0.0,
            73: -5.0,
            96: -0.025318,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        animCurveFn = self._GetAnimCurveFnForPlugName(transformFn, 'translateY')
        AssertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1: -5.0,
            25: 0.0,
            49: 5.0,
            73: 0.0,
            96: -4.983,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

    def TestImportPerspectiveCameraAnimatedShape(self):
        (cameraFn, transformFn) = self._GetMayaCameraAndTransform('PerspCamAnimShape')

        AssertTrue(Gf.IsClose(cameraFn.nearClippingPlane(), 0.1, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.farClippingPlane(), 10000.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.focalLength(), 35.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.focusDistance(), 5.0, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.fStop(), 5.6, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.horizontalFilmAperture(), 1.417322, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.horizontalFilmOffset(), 0.0, 1e-6))
        AssertFalse(cameraFn.isOrtho())
        AssertTrue(Gf.IsClose(cameraFn.verticalFilmAperture(), 0.944882, 1e-6))
        AssertTrue(Gf.IsClose(cameraFn.verticalFilmOffset(), 0.0, 1e-6))

        rotation = OM.MEulerRotation()
        transformFn.getRotation(rotation)
        rotVec = Gf.Vec3d(self._ConvertRadiansToDegrees(rotation.x),
                          self._ConvertRadiansToDegrees(rotation.y),
                          self._ConvertRadiansToDegrees(rotation.z))
        AssertTrue(Gf.IsClose(rotVec, Gf.Vec3d(45.0, 0.0, 0.0), 1e-6))

        translation = transformFn.getTranslation(OM.MSpace.kTransform)
        transVec = Gf.Vec3d(translation.x, translation.y, translation.z)
        AssertTrue(Gf.IsClose(transVec, Gf.Vec3d(0.0, -5.0, 5.0), 1e-6))

        # There SHOULD be animation on the camera shape.
        AssertTrue(OMA.MAnimUtil.isAnimated(cameraFn.object()))

        # There should be no animation on the transform
        AssertFalse(OMA.MAnimUtil.isAnimated(transformFn.object()))

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
        AssertEqual(animCurveFn.numKeys(), 3)
        expectedTimesToValues = {
            1.0: 0.1,
            49.0: 0.5,
            96.0: 1.0,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'farClipPlane')
        AssertEqual(animCurveFn.numKeys(), 3)
        expectedTimesToValues = {
            1.0: 10000.0,
            49.0: 9000.0,
            96.0: 8000.0,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Focal length is keyed over the first 25 timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'focalLength')
        AssertEqual(animCurveFn.numKeys(), 25)
        expectedTimesToValues = {
            1.0: 35.0,
            13.0: 67.5,
            25.0: 100.0,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Focus distance is keyed at ALL timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'focusDistance')
        AssertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1.0: 5.0,
            30.0: 5.0,
            60.0: 5.0,
            90.0: 5.0,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # fStop is keyed at ALL timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'fStop')
        AssertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1.0: 5.6,
            30.0: 5.6,
            60.0: 5.6,
            90.0: 5.6,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Horizontal aperture is keyed at ALL timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'horizontalFilmAperture')
        AssertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1.0: self._ConvertMMToInches(36.0),
            30.0: self._ConvertMMToInches(36.0),
            60.0: self._ConvertMMToInches(36.0),
            90.0: self._ConvertMMToInches(36.0),
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Horizontal aperture offset is keyed at ALL timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'horizontalFilmOffset')
        AssertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1.0: 0.0,
            30.0: 0.0,
            60.0: 0.0,
            90.0: 0.0,
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Vertical aperture is keyed at ALL timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'verticalFilmAperture')
        AssertEqual(animCurveFn.numKeys(), self.TIMECODE_STEPS)
        expectedTimesToValues = {
            1.0: self._ConvertMMToInches(24.0),
            30.0: self._ConvertMMToInches(24.0),
            60.0: self._ConvertMMToInches(24.0),
            90.0: self._ConvertMMToInches(24.0),
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)

        # Vertical aperture offset is keyed at three timeCodes.
        animCurveFn = self._GetAnimCurveFnForPlugName(cameraFn, 'verticalFilmOffset')
        AssertEqual(animCurveFn.numKeys(), 3)
        expectedTimesToValues = {
            1.0: 0.0,
            49.0: self._ConvertMMToInches(25.4),
            96.0: self._ConvertMMToInches(50.8),
        }
        self._ValidateAnimValuesAtTimes(animCurveFn, expectedTimesToValues)


if __name__ == '__main__':
    Runner().Main()
