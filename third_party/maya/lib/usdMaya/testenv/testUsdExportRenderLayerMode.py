#!/pxrpythonsubst

# Initialize maya.
import pixrun
pixrun.setupEnv()
from pixar import pxMayaVersion
pxMayaVersion.setup(version='mayaversion_subst')
import maya.standalone
maya.standalone.initialize('pypix')

import os

from pxr import Usd

from Mentor.Runtime import Assert
from Mentor.Runtime import AssertEqual
from Mentor.Runtime import AssertFalse
from Mentor.Runtime import AssertTrue
from Mentor.Runtime import FindDataFile
from Mentor.Runtime import Fixture
from Mentor.Runtime import Runner


class testUsdExportRenderLayerMode(Fixture):

    def _GetDefaultRenderLayerName(self):
        from maya import OpenMayaRender as OMR
        defaultRenderLayerObj = OMR.MFnRenderLayer.defaultRenderLayer()
        defaultRenderLayer = OMR.MFnRenderLayer(defaultRenderLayerObj)
        return defaultRenderLayer.name()

    def _GetCurrentRenderLayerName(self):
        from maya import OpenMayaRender as OMR
        currentRenderLayerObj = OMR.MFnRenderLayer.currentLayer()
        currentRenderLayer = OMR.MFnRenderLayer(currentRenderLayerObj)
        return currentRenderLayer.name()

    def _GetExportedStage(self, activeRenderLayerName, renderLayerMode='defaultLayer'):
        from maya import cmds

        cmds.file(FindDataFile('UsdExportRenderLayerModeTest.ma'), open=True, force=True)

        usdFilePathFormat = 'UsdExportRenderLayerModeTest_activeRenderLayer-%s_renderLayerMode-%s.usda'
        usdFilePath = usdFilePathFormat % (activeRenderLayerName, renderLayerMode)
        usdFilePath = os.path.abspath(usdFilePath)

        cmds.editRenderLayerGlobals(currentRenderLayer=activeRenderLayerName)
        AssertEqual(self._GetCurrentRenderLayerName(), activeRenderLayerName)

        # Export to USD.
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none',
            renderLayerMode=renderLayerMode)

        stage = Usd.Stage.Open(usdFilePath)
        Assert(stage)

        # Make sure the current render layer is the same as what it was pre-export.
        AssertEqual(self._GetCurrentRenderLayerName(), activeRenderLayerName)

        return stage

    def _AssertDisplayColorEqual(self, prim, expectedColor):
        from pxr import UsdGeom
        from pxr import Vt

        # expectedColor should be given as a list, so turn it into the type we'd
        # expect to get out of the displayColor attribute, namely a VtVec3fArray.
        expectedColor = Vt.Vec3fArray(1, expectedColor)

        gprim = UsdGeom.Gprim(prim)
        Assert(gprim)

        displayColorPrimvar = gprim.GetDisplayColorPrimvar()
        Assert(displayColorPrimvar)
        AssertEqual(displayColorPrimvar.ComputeFlattened(), expectedColor)

    def _VerifyModelingVariantMode(self, stage):
        modelPrim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest')
        Assert(modelPrim)

        variantSets = modelPrim.GetVariantSets()
        Assert(variantSets)
        AssertEqual(variantSets.GetNames(), ['modelingVariant'])

        modelingVariant = variantSets.GetVariantSet('modelingVariant')
        Assert(modelingVariant)

        expectedVariants = [
            'RenderLayerOne',
            'RenderLayerTwo',
            'RenderLayerThree',
            self._GetDefaultRenderLayerName()
        ]

        AssertEqual(set(modelingVariant.GetVariantNames()), set(expectedVariants))

        # The default modeling variant name should have the same name as the
        # default render layer.
        variantSelection = modelingVariant.GetVariantSelection()
        AssertEqual(variantSelection, self._GetDefaultRenderLayerName())

        # All cubes should be active in the default variant.
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeOne')
        AssertTrue(prim.IsActive())
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeTwo')
        AssertTrue(prim.IsActive())
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeThree')
        AssertTrue(prim.IsActive())

        # Only one cube should be active in each of the render layer variants.
        modelingVariant.SetVariantSelection('RenderLayerOne')
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeOne')
        AssertTrue(prim.IsActive())
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeTwo')
        AssertFalse(prim.IsActive())
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeThree')
        AssertFalse(prim.IsActive())

        modelingVariant.SetVariantSelection('RenderLayerTwo')
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeOne')
        AssertFalse(prim.IsActive())
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeTwo')
        AssertTrue(prim.IsActive())
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeThree')
        AssertFalse(prim.IsActive())

        modelingVariant.SetVariantSelection('RenderLayerThree')
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeOne')
        AssertFalse(prim.IsActive())
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeTwo')
        AssertFalse(prim.IsActive())
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeThree')
        AssertTrue(prim.IsActive())

    def TestDefaultLayerModeWithDefaultLayerActive(self):
        stage = self._GetExportedStage(self._GetDefaultRenderLayerName(),
            renderLayerMode='defaultLayer')

        # CubeTwo should be red in the default layer.
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeTwo')
        Assert(prim)
        self._AssertDisplayColorEqual(prim, [1.0, 0.0, 0.0])

    def TestCurrentLayerModeWithDefaultLayerActive(self):
        stage = self._GetExportedStage(self._GetDefaultRenderLayerName(),
            renderLayerMode='currentLayer')

        # The default layer IS the current layer, so CubeTwo should still be red.
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeTwo')
        Assert(prim)
        self._AssertDisplayColorEqual(prim, [1.0, 0.0, 0.0])

    def TestModelingVariantModeWithDefaultLayerActive(self):
        stage = self._GetExportedStage(self._GetDefaultRenderLayerName(),
            renderLayerMode='modelingVariant')

        self._VerifyModelingVariantMode(stage)

    def TestDefaultLayerModeWithNonDefaultLayerActive(self):
        stage = self._GetExportedStage('RenderLayerTwo',
            renderLayerMode='defaultLayer')

        # The render layer should have been switched to the default layer where
        # CubeTwo should be red.
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeTwo')
        Assert(prim)
        self._AssertDisplayColorEqual(prim, [1.0, 0.0, 0.0])

    def TestCurrentLayerModeWithNonDefaultLayerActive(self):
        stage = self._GetExportedStage('RenderLayerTwo',
            renderLayerMode='currentLayer')

        # CubeTwo should be blue in the layer 'RenderLayerTwo'.
        prim = stage.GetPrimAtPath('/UsdExportRenderLayerModeTest/Geom/CubeTwo')
        self._AssertDisplayColorEqual(prim, [0.0, 0.0, 1.0])

    def TestModelingVariantModeWithNonDefaultLayerActive(self):
        stage = self._GetExportedStage('RenderLayerTwo',
            renderLayerMode='modelingVariant')

        # 'modelingVariant' renderLayerMode switches to the default render layer
        # first, so the results here should be the same as when the default
        # layer is active.
        self._VerifyModelingVariantMode(stage)


if __name__ == '__main__':
    Runner().Main()
