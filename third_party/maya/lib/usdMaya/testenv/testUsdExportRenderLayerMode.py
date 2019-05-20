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

from pxr import Usd
from pxr import UsdGeom
from pxr import Vt

from maya import cmds
from maya import standalone
from maya import OpenMayaRender as OMR


class testUsdExportRenderLayerMode(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _GetDefaultRenderLayerName(self):
        defaultRenderLayerObj = OMR.MFnRenderLayer.defaultRenderLayer()
        defaultRenderLayer = OMR.MFnRenderLayer(defaultRenderLayerObj)
        return defaultRenderLayer.name()

    def _GetCurrentRenderLayerName(self):
        currentRenderLayerObj = OMR.MFnRenderLayer.currentLayer()
        currentRenderLayer = OMR.MFnRenderLayer(currentRenderLayerObj)
        return currentRenderLayer.name()

    def _GetExportedStage(self, activeRenderLayerName,
            renderLayerMode='defaultLayer', **kwargs):
        cmds.file(os.path.abspath('UsdExportRenderLayerModeTest.ma'),
            open=True, force=True)

        usdFilePathFormat = 'UsdExportRenderLayerModeTest_activeRenderLayer-%s_renderLayerMode-%s.usda'
        usdFilePath = usdFilePathFormat % (activeRenderLayerName, renderLayerMode)
        usdFilePath = os.path.abspath(usdFilePath)

        cmds.editRenderLayerGlobals(currentRenderLayer=activeRenderLayerName)
        self.assertEqual(self._GetCurrentRenderLayerName(), activeRenderLayerName)

        # Export to USD.
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='none', renderLayerMode=renderLayerMode, **kwargs)

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        # Make sure the current render layer is the same as what it was pre-export.
        self.assertEqual(self._GetCurrentRenderLayerName(), activeRenderLayerName)

        return stage

    def _AssertDisplayColorEqual(self, prim, expectedColor):
        # expectedColor should be given as a list, so turn it into the type we'd
        # expect to get out of the displayColor attribute, namely a VtVec3fArray.
        expectedColor = Vt.Vec3fArray(1, expectedColor)

        gprim = UsdGeom.Gprim(prim)
        self.assertTrue(gprim)

        displayColorPrimvar = gprim.GetDisplayColorPrimvar()
        self.assertTrue(displayColorPrimvar)
        self.assertEqual(displayColorPrimvar.ComputeFlattened(), expectedColor)

    def _VerifyModelingVariantMode(self, stage,
                                   variantPath='/UsdExportRenderLayerModeTest',
                                   geomPath='/UsdExportRenderLayerModeTest/Geom'):
        modelPrim = stage.GetPrimAtPath(variantPath)
        self.assertTrue(modelPrim)

        variantSets = modelPrim.GetVariantSets()
        self.assertTrue(variantSets)
        self.assertEqual(variantSets.GetNames(), ['modelingVariant'])

        modelingVariant = variantSets.GetVariantSet('modelingVariant')
        self.assertTrue(modelingVariant)

        expectedVariants = [
            'RenderLayerOne',
            'RenderLayerTwo',
            'RenderLayerThree',
            self._GetDefaultRenderLayerName()
        ]

        self.assertEqual(set(modelingVariant.GetVariantNames()), set(expectedVariants))

        # The default modeling variant name should have the same name as the
        # default render layer.
        variantSelection = modelingVariant.GetVariantSelection()
        self.assertEqual(variantSelection, self._GetDefaultRenderLayerName())

        # All cubes should be active in the default variant.
        prim = stage.GetPrimAtPath(geomPath + '/CubeOne')
        self.assertTrue(prim.IsActive())
        prim = stage.GetPrimAtPath(geomPath + '/CubeTwo')
        self.assertTrue(prim.IsActive())
        prim = stage.GetPrimAtPath(geomPath + '/CubeThree')
        self.assertTrue(prim.IsActive())

        # Only one cube should be active in each of the render layer variants.
        modelingVariant.SetVariantSelection('RenderLayerOne')
        prim = stage.GetPrimAtPath(geomPath + '/CubeOne')
        self.assertTrue(prim.IsActive())
        prim = stage.GetPrimAtPath(geomPath + '/CubeTwo')
        self.assertFalse(prim.IsActive())
        prim = stage.GetPrimAtPath(geomPath + '/CubeThree')
        self.assertFalse(prim.IsActive())

        modelingVariant.SetVariantSelection('RenderLayerTwo')
        prim = stage.GetPrimAtPath(geomPath + '/CubeOne')
        self.assertFalse(prim.IsActive())
        prim = stage.GetPrimAtPath(geomPath + '/CubeTwo')
        self.assertTrue(prim.IsActive())
        prim = stage.GetPrimAtPath(geomPath + '/CubeThree')
        self.assertFalse(prim.IsActive())

        modelingVariant.SetVariantSelection('RenderLayerThree')
        prim = stage.GetPrimAtPath(geomPath + '/CubeOne')
        self.assertFalse(prim.IsActive())
        prim = stage.GetPrimAtPath(geomPath + '/CubeTwo')
        self.assertFalse(prim.IsActive())
        prim = stage.GetPrimAtPath(geomPath + '/CubeThree')
        self.assertTrue(prim.IsActive())

    def testDefaultLayerModeWithDefaultLayerActive(self):
        stage = self._GetExportedStage(self._GetDefaultRenderLayerName(),
            renderLayerMode='defaultLayer')

        # CubeTwo should be red in the default layer.
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeTwo')
        self.assertTrue(prim)
        self._AssertDisplayColorEqual(prim, [1.0, 0.0, 0.0])

    def testCurrentLayerModeWithDefaultLayerActive(self):
        stage = self._GetExportedStage(self._GetDefaultRenderLayerName(),
            renderLayerMode='currentLayer')

        # The default layer IS the current layer, so CubeTwo should still be red.
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeTwo')
        self.assertTrue(prim)
        self._AssertDisplayColorEqual(prim, [1.0, 0.0, 0.0])

    def testModelingVariantModeWithDefaultLayerActive(self):
        stage = self._GetExportedStage(self._GetDefaultRenderLayerName(),
            renderLayerMode='modelingVariant')

        self._VerifyModelingVariantMode(stage)

    def testDefaultLayerModeWithNonDefaultLayerActive(self):
        stage = self._GetExportedStage('RenderLayerTwo',
            renderLayerMode='defaultLayer')

        # The render layer should have been switched to the default layer where
        # CubeTwo should be red.
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeTwo')
        self.assertTrue(prim)
        self._AssertDisplayColorEqual(prim, [1.0, 0.0, 0.0])

    def testCurrentLayerModeWithNonDefaultLayerActive(self):
        stage = self._GetExportedStage('RenderLayerTwo',
            renderLayerMode='currentLayer')

        # CubeTwo should be blue in the layer 'RenderLayerTwo'.
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeTwo')
        self._AssertDisplayColorEqual(prim, [0.0, 0.0, 1.0])

    def testModelingVariantModeWithNonDefaultLayerActive(self):
        stage = self._GetExportedStage('RenderLayerTwo',
            renderLayerMode='modelingVariant')

        # 'modelingVariant' renderLayerMode switches to the default render layer
        # first, so the results here should be the same as when the default
        # layer is active.
        self._VerifyModelingVariantMode(stage)

    def testModelingVariantModeWithParentScope(self):
        stage = self._GetExportedStage(self._GetDefaultRenderLayerName(),
                                       renderLayerMode='modelingVariant',
                                       parentScope='newTopLevel')

        self._VerifyModelingVariantMode(
            stage,
            variantPath='/newTopLevel',
            geomPath='/newTopLevel/UsdExportRenderLayerModeTest/Geom')


if __name__ == '__main__':
    unittest.main(verbosity=2)
