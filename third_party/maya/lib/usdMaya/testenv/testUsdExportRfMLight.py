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
from pxr import UsdLux

from maya import cmds
from maya import standalone


class testUsdExportRfMLight(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        mayaFile = os.path.abspath('RfMLightsTest.ma')
        cmds.file(mayaFile, open=True, force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('RfMLightsTest.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='pxrRis')

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)

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
        else:
            raise NotImplementedError('Invalid light type %s' % lightTypeName)

        lightSchema = UsdLux.Light(lightPrim)
        self.assertTrue(lightSchema)

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

        if lightTypeName == 'DomeLight':
            # PxrDomeLight has no normalize attribute
            self.assertFalse(
                lightSchema.GetNormalizeAttr().HasAuthoredValueOpinion())
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

    def _ValidateUsdLuxShapingAPI(self):
        lightPrimPath = '/RfMLightsTest/Lights/DiskLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

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

        shadowAPI = UsdLux.ShadowAPI(lightPrim)
        self.assertTrue(shadowAPI)

        # Shadows are enabled by default, and we author sparsely, so there
        # should NOT be an opinion.
        self.assertFalse(
            shadowAPI.GetShadowEnableAttr().HasAuthoredValueOpinion())

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
        self._ValidateUsdLuxLight('DistantLight')
        self._ValidateUsdLuxLight('DomeLight')
        self._ValidateUsdLuxLight('MeshLight')
        self._ValidateUsdLuxLight('RectLight')
        self._ValidateUsdLuxLight('SphereLight')

        self._ValidateUsdLuxDistantLightAngle()

        self._ValidateUsdLuxRectLightTextureFile()
        self._ValidateUsdLuxDomeLightTextureFile()

        self._ValidateUsdLuxShapingAPI()

        self._ValidateUsdLuxShadowAPI()


if __name__ == '__main__':
    unittest.main(verbosity=2)
