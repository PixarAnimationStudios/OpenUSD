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
#

import os
import unittest

from pxr import Gf
from pxr import Usd
from pxr import UsdGeom
from pxr import UsdLux
from pxr import UsdRi

from maya import cmds
from maya import standalone


class testUsdExportRfMLight(unittest.TestCase):

    START_TIMECODE = 1.0
    END_TIMECODE = 5.0

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        mayaFile = os.path.abspath('RfMLightsTest.ma')
        cmds.file(mayaFile, open=True, force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('RfMLightsTest.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='pxrRis',
            frameRange=(cls.START_TIMECODE, cls.END_TIMECODE))

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)

        self.assertEqual(self._stage.GetStartTimeCode(), self.START_TIMECODE)
        self.assertEqual(self._stage.GetEndTimeCode(), self.END_TIMECODE)

    def _ValidateUsdLuxLight(self, lightTypeName):
        primPathFormat = '/RfMLightsTest/Lights/%s'

        lightPrimPath = primPathFormat % lightTypeName
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        testNumber = None
        if lightTypeName == 'DiskLight':
            self.assertTrue(lightPrim.IsA(UsdLux.DiskLight))
            testNumber = 1
        elif lightTypeName == 'DistantLight':
            self.assertTrue(lightPrim.IsA(UsdLux.DistantLight))
            testNumber = 2
        elif lightTypeName == 'DomeLight':
            self.assertTrue(lightPrim.IsA(UsdLux.DomeLight))
            testNumber = 3
        elif lightTypeName == 'MeshLight':
            self.assertTrue(lightPrim.IsA(UsdLux.GeometryLight))
            testNumber = 4
        elif lightTypeName == 'RectLight':
            self.assertTrue(lightPrim.IsA(UsdLux.RectLight))
            testNumber = 5
        elif lightTypeName == 'SphereLight':
            self.assertTrue(lightPrim.IsA(UsdLux.SphereLight))
            testNumber = 6
        elif lightTypeName == 'AovLight':
            self.assertTrue(lightPrim.IsA(UsdRi.PxrAovLight))
            testNumber = 7
        elif lightTypeName == 'EnvDayLight':
            self.assertTrue(lightPrim.IsA(UsdRi.PxrEnvDayLight))
            testNumber = 8
        else:
            raise NotImplementedError('Invalid light type %s' % lightTypeName)

        lightSchema = UsdLux.Light(lightPrim)
        self.assertTrue(lightSchema)

        if lightTypeName == 'AovLight':
            # PxrAovLight doesn't have any of the below attributes.
            return

        expectedIntensity = 1.0 + (testNumber * 0.1)
        self.assertTrue(Gf.IsClose(lightSchema.GetIntensityAttr().Get(),
            expectedIntensity, 1e-6))

        expectedExposure = 0.1 * testNumber
        self.assertTrue(Gf.IsClose(lightSchema.GetExposureAttr().Get(),
            expectedExposure, 1e-6))

        expectedDiffuse = 1.0 + (testNumber * 0.1)
        self.assertTrue(Gf.IsClose(lightSchema.GetDiffuseAttr().Get(),
            expectedDiffuse, 1e-6))

        expectedSpecular = 1.0 + (testNumber * 0.1)
        self.assertTrue(Gf.IsClose(lightSchema.GetSpecularAttr().Get(),
            expectedSpecular, 1e-6))

        if lightTypeName == 'EnvDayLight':
            # PxrEnvDayLight doesn't have any of the below attributes.
            return

        if lightTypeName == 'DomeLight':
            # PxrDomeLight has no normalize attribute
            self.assertFalse(
                lightSchema.GetNormalizeAttr().HasAuthoredValue())
        else:
            expectedNormalize = True
            self.assertEqual(lightSchema.GetNormalizeAttr().Get(),
                expectedNormalize)

        expectedColor = Gf.Vec3f(0.1 * testNumber)
        self.assertTrue(Gf.IsClose(lightSchema.GetColorAttr().Get(),
            expectedColor, 1e-6))

        expectedEnableTemperature = True
        self.assertEqual(lightSchema.GetEnableColorTemperatureAttr().Get(),
            expectedEnableTemperature)

        expectedTemperature = 6500.0 + testNumber
        self.assertTrue(Gf.IsClose(lightSchema.GetColorTemperatureAttr().Get(),
            expectedTemperature, 1e-6))

    def _ValidateDiskLightXformAnimation(self):
        lightPrimPath = '/RfMLightsTest/Lights/DiskLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        diskLight = UsdLux.DiskLight(lightPrim)
        self.assertTrue(diskLight)

        xformOps = diskLight.GetOrderedXformOps()
        self.assertEqual(len(xformOps), 1)

        translateOp = xformOps[0]

        self.assertEqual(translateOp.GetOpName(), 'xformOp:translate')
        self.assertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)

        for frame in xrange(int(self.START_TIMECODE), int(self.END_TIMECODE + 1.0)):
            expectedTranslation = Gf.Vec3d(1.0, float(frame), 1.0)
            self.assertTrue(
                Gf.IsClose(translateOp.Get(frame), expectedTranslation, 1e-6))

    def _ValidateUsdLuxDistantLightAngle(self):
        lightPrimPath = '/RfMLightsTest/Lights/DistantLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        distantLight = UsdLux.DistantLight(lightPrim)
        self.assertTrue(distantLight)

        expectedAngle = 0.73
        self.assertTrue(Gf.IsClose(distantLight.GetAngleAttr().Get(),
            expectedAngle, 1e-6))

    def _ValidateUsdLuxRectLightTextureFile(self):
        lightPrimPath = '/RfMLightsTest/Lights/RectLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        rectLight = UsdLux.RectLight(lightPrim)
        self.assertTrue(rectLight)

        expectedTextureFile = './RectLight_texture.tex'
        self.assertEqual(rectLight.GetTextureFileAttr().Get(),
            expectedTextureFile)

    def _ValidateUsdLuxDomeLightTextureFile(self):
        lightPrimPath = '/RfMLightsTest/Lights/DomeLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        domeLight = UsdLux.DomeLight(lightPrim)
        self.assertTrue(domeLight)

        expectedTextureFile = './DomeLight_texture.tex'
        self.assertEqual(domeLight.GetTextureFileAttr().Get(),
            expectedTextureFile)

    def _ValidateUsdRiPxrAovLight(self):
        lightPrimPath = '/RfMLightsTest/Lights/AovLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        aovLight = UsdRi.PxrAovLight(lightPrim)
        self.assertTrue(aovLight)

        expectedAovName = 'testAovName'
        self.assertEqual(aovLight.GetAovNameAttr().Get(), expectedAovName)

        expectedInPrimaryHit = False
        self.assertEqual(aovLight.GetInPrimaryHitAttr().Get(),
            expectedInPrimaryHit)

        expectedInReflection = True
        self.assertEqual(aovLight.GetInReflectionAttr().Get(),
            expectedInReflection)

        expectedInRefraction = True
        self.assertEqual(aovLight.GetInRefractionAttr().Get(),
            expectedInRefraction)

        expectedInvert = True
        self.assertEqual(aovLight.GetInvertAttr().Get(), expectedInvert)

        expectedOnVolumeBoundaries = False
        self.assertEqual(aovLight.GetOnVolumeBoundariesAttr().Get(),
            expectedOnVolumeBoundaries)

        expectedUseColor = True
        self.assertEqual(aovLight.GetUseColorAttr().Get(), expectedUseColor)

        expectedUseThroughput = False
        self.assertEqual(aovLight.GetUseThroughputAttr().Get(),
            expectedUseThroughput)

    def _ValidateUsdRiPxrEnvDayLight(self):
        lightPrimPath = '/RfMLightsTest/Lights/EnvDayLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        envDayLight = UsdRi.PxrEnvDayLight(lightPrim)
        self.assertTrue(envDayLight)

        expectedDay = 8
        self.assertEqual(envDayLight.GetDayAttr().Get(), expectedDay)

        expectedHaziness = 1.8
        self.assertTrue(Gf.IsClose(envDayLight.GetHazinessAttr().Get(),
            expectedHaziness, 1e-6))

        expectedHour = 8.8
        self.assertTrue(Gf.IsClose(envDayLight.GetHourAttr().Get(),
            expectedHour, 1e-6))

        expectedLatitude = 80.0
        self.assertTrue(Gf.IsClose(envDayLight.GetLatitudeAttr().Get(),
            expectedLatitude, 1e-6))

        expectedLongitude = -80.0
        self.assertTrue(Gf.IsClose(envDayLight.GetLongitudeAttr().Get(),
            expectedLongitude, 1e-6))

        expectedMonth = 8
        self.assertEqual(envDayLight.GetMonthAttr().Get(), expectedMonth)

        expectedSkyTint = Gf.Vec3f(0.8)
        self.assertTrue(Gf.IsClose(envDayLight.GetSkyTintAttr().Get(),
            expectedSkyTint, 1e-6))

        expectedSunDirection = Gf.Vec3f(0.0, 0.0, 0.8)
        self.assertTrue(Gf.IsClose(envDayLight.GetSunDirectionAttr().Get(),
            expectedSunDirection, 1e-6))

        expectedSunSize = 0.8
        self.assertTrue(Gf.IsClose(envDayLight.GetSunSizeAttr().Get(),
            expectedSunSize, 1e-6))

        expectedSunTint = Gf.Vec3f(0.8)
        self.assertTrue(Gf.IsClose(envDayLight.GetSunTintAttr().Get(),
            expectedSunTint, 1e-6))

        expectedYear = 2018
        self.assertEqual(envDayLight.GetYearAttr().Get(), expectedYear)

        expectedZone = 8.0
        self.assertTrue(Gf.IsClose(envDayLight.GetZoneAttr().Get(),
            expectedZone, 1e-6))

    def _ValidateUsdLuxShapingAPI(self):
        lightPrimPath = '/RfMLightsTest/Lights/DiskLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        self.assertTrue(lightPrim.HasAPI(UsdLux.ShapingAPI))

        shapingAPI = UsdLux.ShapingAPI(lightPrim)
        self.assertTrue(shapingAPI)

        expectedFocus = 0.1
        self.assertTrue(Gf.IsClose(shapingAPI.GetShapingFocusAttr().Get(),
            expectedFocus, 1e-6))

        expectedFocusTint = Gf.Vec3f(0.1)
        self.assertTrue(Gf.IsClose(shapingAPI.GetShapingFocusTintAttr().Get(),
            expectedFocusTint, 1e-6))

        expectedConeAngle = 91.0
        self.assertTrue(Gf.IsClose(shapingAPI.GetShapingConeAngleAttr().Get(),
            expectedConeAngle, 1e-6))

        expectedConeSoftness = 0.1
        self.assertTrue(Gf.IsClose(shapingAPI.GetShapingConeSoftnessAttr().Get(),
            expectedConeSoftness, 1e-6))

        expectedProfilePath = './DiskLight_profile.ies'
        self.assertEqual(shapingAPI.GetShapingIesFileAttr().Get(),
            expectedProfilePath)

        expectedProfileScale = 1.1
        self.assertTrue(Gf.IsClose(shapingAPI.GetShapingIesAngleScaleAttr().Get(),
            expectedProfileScale, 1e-6))

    def _ValidateUsdLuxShadowAPI(self):
        lightPrimPath = '/RfMLightsTest/Lights/RectLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        self.assertTrue(lightPrim.HasAPI(UsdLux.ShadowAPI))

        shadowAPI = UsdLux.ShadowAPI(lightPrim)
        self.assertTrue(shadowAPI)

        # Shadows are enabled by default, and we author sparsely, so there
        # should NOT be an opinion.
        self.assertFalse(
            shadowAPI.GetShadowEnableAttr().HasAuthoredValue())

        expectedShadowColor = Gf.Vec3f(0.5)
        self.assertTrue(Gf.IsClose(shadowAPI.GetShadowColorAttr().Get(),
            expectedShadowColor, 1e-6))

        expectedShadowDistance = -0.5
        self.assertTrue(Gf.IsClose(shadowAPI.GetShadowDistanceAttr().Get(),
            expectedShadowDistance, 1e-6))

        expectedShadowFalloff = -0.5
        self.assertTrue(Gf.IsClose(shadowAPI.GetShadowFalloffAttr().Get(),
            expectedShadowFalloff, 1e-6))

        expectedShadowFalloffGamma = 0.5
        self.assertTrue(Gf.IsClose(shadowAPI.GetShadowFalloffGammaAttr().Get(),
            expectedShadowFalloffGamma, 1e-6))

    def testExportRenderManForMayaLights(self):
        """
        Tests that RenderMan for Maya lights export as UsdLux schema USD prims
        correctly.
        """
        self._ValidateUsdLuxLight('DiskLight')
        self._ValidateDiskLightXformAnimation()

        self._ValidateUsdLuxLight('DistantLight')
        self._ValidateUsdLuxLight('DomeLight')
        self._ValidateUsdLuxLight('MeshLight')
        self._ValidateUsdLuxLight('RectLight')
        self._ValidateUsdLuxLight('SphereLight')
        self._ValidateUsdLuxLight('AovLight')
        self._ValidateUsdLuxLight('EnvDayLight')

        self._ValidateUsdLuxDistantLightAngle()

        self._ValidateUsdLuxRectLightTextureFile()
        self._ValidateUsdLuxDomeLightTextureFile()

        self._ValidateUsdRiPxrAovLight()
        self._ValidateUsdRiPxrEnvDayLight()

        self._ValidateUsdLuxShapingAPI()

        self._ValidateUsdLuxShadowAPI()


if __name__ == '__main__':
    unittest.main(verbosity=2)
