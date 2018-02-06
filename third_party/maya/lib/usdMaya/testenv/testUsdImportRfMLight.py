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

from maya import OpenMaya
from maya import cmds
from maya import standalone


class testUsdImportRfMLight(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.file(new=True, force=True)

        cmds.loadPlugin('RenderMan_for_Maya', quiet=True)

        # Import from USD.
        usdFilePath = os.path.abspath('RfMLightsTest.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdImport(file=usdFilePath, shadingMode='pxrRis')

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage being imported opens successfully.
        """
        self.assertTrue(self._stage)

    def _GetMayaDependencyNode(self, objectName):
         selectionList = OpenMaya.MSelectionList()
         selectionList.add(objectName)
         mObj = OpenMaya.MObject()
         selectionList.getDependNode(0, mObj)

         depFn = OpenMaya.MFnDependencyNode(mObj)
         self.assertTrue(depFn)

         return depFn

    def _ValidateMayaLight(self, lightTypeName):
        nodePathFormat = '|RfMLightsTest|Lights|{lightTypeName}|{lightTypeName}Shape'
        nodePath = nodePathFormat.format(lightTypeName=lightTypeName)

        depFn = self._GetMayaDependencyNode(nodePath)

        testNumber = None
        if lightTypeName == 'DiskLight':
            self.assertEqual(depFn.typeName(), 'PxrDiskLight')
            testNumber = 1
        elif lightTypeName == 'DistantLight':
            self.assertEqual(depFn.typeName(), 'PxrDistantLight')
            testNumber = 2
        elif lightTypeName == 'DomeLight':
            self.assertEqual(depFn.typeName(), 'PxrDomeLight')
            testNumber = 3
        elif lightTypeName == 'MeshLight':
            self.assertEqual(depFn.typeName(), 'PxrMeshLight')
            testNumber = 4
        elif lightTypeName == 'RectLight':
            self.assertEqual(depFn.typeName(), 'PxrRectLight')
            testNumber = 5
        elif lightTypeName == 'SphereLight':
            self.assertEqual(depFn.typeName(), 'PxrSphereLight')
            testNumber = 6
        else:
            raise NotImplementedError('Invalid light type %s' % lightTypeName)

        expectedIntensity = 1.0 + (testNumber * 0.1)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath),
            expectedIntensity, 1e-6))

        expectedExposure = 0.1 * testNumber
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.exposure' % nodePath),
            expectedExposure, 1e-6))

        expectedDiffuse = 1.0 + (testNumber * 0.1)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.diffuse' % nodePath),
            expectedDiffuse, 1e-6))

        expectedSpecular = 1.0 + (testNumber * 0.1)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.specular' % nodePath),
            expectedSpecular, 1e-6))

        # PxrDomeLight has no normalize attribute
        if lightTypeName != 'DomeLight':
            self.assertTrue(cmds.getAttr('%s.areaNormalize' % nodePath))

        expectedColor = 0.1 * testNumber
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightColorR' % nodePath),
            expectedColor, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightColorG' % nodePath),
            expectedColor, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightColorB' % nodePath),
            expectedColor, 1e-6))

        self.assertTrue(cmds.getAttr('%s.enableTemperature' % nodePath))

        expectedTemperature = 6500.0 + testNumber
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.temperature' % nodePath),
            expectedTemperature, 1e-6))

    def _ValidatePxrDistantLightAngle(self):
        nodePath = '|RfMLightsTest|Lights|DistantLight|DistantLightShape'

        expectedAngle = 0.73
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.angleExtent' % nodePath),
            expectedAngle, 1e-6))

    def _ValidatePxrRectLightTextureFile(self):
        nodePath = '|RfMLightsTest|Lights|RectLight|RectLightShape'

        expectedTextureFile = './RectLight_texture.tex'
        self.assertEqual(cmds.getAttr('%s.lightColorMap' % nodePath),
            expectedTextureFile)

    def _ValidatePxrDomeLightTextureFile(self):
        nodePath = '|RfMLightsTest|Lights|DomeLight|DomeLightShape'

        expectedTextureFile = './DomeLight_texture.tex'
        self.assertEqual(cmds.getAttr('%s.lightColorMap' % nodePath),
            expectedTextureFile)

    def _ValidateMayaLightShaping(self):
        nodePath = '|RfMLightsTest|Lights|DiskLight|DiskLightShape'

        expectedFocus = 0.1
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.emissionFocus' % nodePath),
            expectedFocus, 1e-6))

        expectedFocusTint = 0.1
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.emissionFocusTintR' % nodePath),
            expectedFocusTint, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.emissionFocusTintG' % nodePath),
            expectedFocusTint, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.emissionFocusTintB' % nodePath),
            expectedFocusTint, 1e-6))

        expectedConeAngle = 91.0
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.coneAngle' % nodePath),
            expectedConeAngle, 1e-6))

        expectedConeSoftness = 0.1
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.coneSoftness' % nodePath),
            expectedConeSoftness, 1e-6))

        expectedProfilePath = './DiskLight_profile.ies'
        self.assertEqual(cmds.getAttr('%s.iesProfile' % nodePath),
            expectedProfilePath)

        expectedProfileScale = 1.1
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.iesProfileScale' % nodePath),
            expectedProfileScale, 1e-6))

    def _ValidateMayaLightShadow(self):
        nodePath = '|RfMLightsTest|Lights|RectLight|RectLightShape'

        expectedShadowsEnabled = True
        self.assertEqual(cmds.getAttr('%s.enableShadows' % nodePath),
            expectedShadowsEnabled)

        expectedShadowColor = 0.5
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.shadowColorR' % nodePath),
            expectedShadowColor, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.shadowColorG' % nodePath),
            expectedShadowColor, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.shadowColorB' % nodePath),
            expectedShadowColor, 1e-6))

        expectedShadowDistance = -0.5
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.shadowDistance' % nodePath),
            expectedShadowDistance, 1e-6))

        expectedShadowFalloff = -0.5
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.shadowFalloff' % nodePath),
            expectedShadowFalloff, 1e-6))

        expectedShadowFalloffGamma = 0.5
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.shadowFalloffGamma' % nodePath),
            expectedShadowFalloffGamma, 1e-6))

    def testImportRenderManForMayaLights(self):
        """
        Tests that UsdLux schema USD prims import into Maya as the appropriate
        RenderMan for Maya light.
        """
        self.assertTrue(cmds.pluginInfo('RenderMan_for_Maya', query=True,
            loaded=True))

        self._ValidateMayaLight('DiskLight')
        self._ValidateMayaLight('DistantLight')
        self._ValidateMayaLight('DomeLight')
        self._ValidateMayaLight('MeshLight')
        self._ValidateMayaLight('RectLight')
        self._ValidateMayaLight('SphereLight')

        self._ValidatePxrDistantLightAngle()

        self._ValidatePxrRectLightTextureFile()
        self._ValidatePxrDomeLightTextureFile()

        self._ValidateMayaLightShaping()

        self._ValidateMayaLightShadow()


if __name__ == '__main__':
    unittest.main(verbosity=2)
