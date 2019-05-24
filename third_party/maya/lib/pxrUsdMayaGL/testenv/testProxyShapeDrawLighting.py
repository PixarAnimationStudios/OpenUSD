#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

from maya import cmds

import os
import sys
import unittest


class testProxyShapeDrawLighting(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        cls._testDir = os.path.abspath('.')

        cls._testRootNodeName = 'ProxyShapeDrawLightingTest'
        cls._testSceneName = '%s.ma' % cls._testRootNodeName
        cls._testSceneFullPath = os.path.abspath(cls._testSceneName)

        cls._nativeNodePathName = '|%s|Native' % cls._testRootNodeName
        cls._nativeTorusPathName = '%s|Torus' % cls._nativeNodePathName
        cls._nativePlanePathName = '%s|Plane' % cls._nativeNodePathName

        cls._hydraNodePathName = '|%s|Hydra' % cls._testRootNodeName
        cls._hydraTorusPathName = '%s|Torus' % cls._hydraNodePathName
        cls._hydraPlanePathName = '%s|Plane' % cls._hydraNodePathName

        cls._spotLightNameFormat = 'SpotLight_%d'
        cls._directionalLightNameFormat = 'DirectionalLight_%d'
        cls._numberOfLights = 4

        cls._cameraName = 'MainCamera'

    @classmethod
    def _HideNativeGeometry(cls):
        cmds.hide(cls._nativeTorusPathName)
        cmds.hide(cls._nativePlanePathName)

    @classmethod
    def _HideHydraGeometry(cls):
        cmds.hide(cls._hydraTorusPathName)
        cmds.hide(cls._hydraPlanePathName)

    @classmethod
    def _HideAllLights(cls):
        for i in range(1, cls._numberOfLights + 1):
            cmds.hide(cls._spotLightNameFormat % i)
            cmds.hide(cls._directionalLightNameFormat % i)

    @classmethod
    def _ShowSpotLight(cls, lightIndex):
        cmds.showHidden(cls._spotLightNameFormat % lightIndex)

    @classmethod
    def _ShowDirectionalLight(cls, lightIndex):
        cmds.showHidden(cls._directionalLightNameFormat % lightIndex)

    @classmethod
    def _SetShadowsEnabled(cls, enabled):
        for i in range(1, cls._numberOfLights + 1):
            cmds.setAttr(
                '%sShape.useDepthMapShadows' % cls._spotLightNameFormat % i,
                enabled)
            cmds.setAttr(
                '%sShape.useDepthMapShadows' % cls._directionalLightNameFormat % i,
                enabled)

    def setUp(self):
        # To control where the rendered images are written, we force Maya to
        # use the test directory as the workspace.
        cmds.workspace(self._testDir, o=True)

        cmds.file(self._testSceneFullPath, open=True, force=True)

    def _WriteViewportImage(self, outputImageName, suffix):
        # Make sure the hardware renderer is available
        MAYA_RENDERER_NAME = 'mayaHardware2'
        mayaRenderers = cmds.renderer(query=True, namesOfAvailableRenderers=True)
        self.assertIn(MAYA_RENDERER_NAME, mayaRenderers)

        # Make it the current renderer.
        cmds.setAttr('defaultRenderGlobals.currentRenderer', MAYA_RENDERER_NAME,
            type='string')
        # Set the image format to PNG.
        cmds.setAttr('defaultRenderGlobals.imageFormat', 32)
        # Set the render mode to shaded and textured.
        cmds.setAttr('hardwareRenderingGlobals.renderMode', 4)
        # Specify the output image prefix. The path to it is built from the
        # workspace directory.
        cmds.setAttr('defaultRenderGlobals.imageFilePrefix',
            '%s_%s' % (outputImageName, suffix),
            type='string')
        # Apply the viewer's color transform to the rendered image, otherwise
        # it comes out too dark.
        cmds.setAttr('defaultColorMgtGlobals.outputTransformEnabled', 1)

        # Do the render.
        cmds.ogsRender(camera=self._cameraName, currentFrame=True, width=960,
            height=540)

    def _RenderEachLight(self, enableShadows=False):
        self._SetShadowsEnabled(enableShadows)

        for i in range(1, self._numberOfLights + 1):
            self._HideAllLights()
            self._ShowSpotLight(i)
            self._WriteViewportImage(self._testName, 'Spot_%d' % i)

            self._HideAllLights()
            self._ShowDirectionalLight(i)
            self._WriteViewportImage(self._testName, 'Directional_%d' % i)

    def testMayaNativeTorusAndPlane(self):
        """
        Tests performing a hardware render of Maya native geometry.

        An image is generated for each light in isolation.
        """
        self._testName = 'MayaNativeTorusAndPlane'

        self._HideHydraGeometry()

        self._RenderEachLight(enableShadows=False)

    def testMayaNativeTorusAndPlaneWithShadows(self):
        """
        Tests performing a hardware render of Maya native geometry with shadows.

        An image is generated for each light in isolation.
        """
        self._testName = 'MayaNativeTorusAndPlaneWithShadows'

        self._HideHydraGeometry()

        self._RenderEachLight(enableShadows=True)

    def testHydraTorusAndPlane(self):
        """
        Tests performing a hardware render of Hydra-drawn geometry.

        Hydra draws USD proxy shapes that reference USD produced by exporting
        the Maya native geometry.

        An image is generated for each light in isolation.
        """
        self._testName = 'HydraTorusAndPlane'

        self._HideNativeGeometry()

        self._RenderEachLight(enableShadows=False)

    def testHydraTorusAndPlaneWithShadows(self):
        """
        Tests performing a hardware render of Hydra-drawn geometry with shadows.

        Hydra draws USD proxy shapes that reference USD produced by exporting
        the Maya native geometry.

        An image is generated for each light in isolation.
        """
        self._testName = 'HydraTorusAndPlaneWithShadows'

        self._HideNativeGeometry()

        self._RenderEachLight(enableShadows=True)


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(
        testProxyShapeDrawLighting)

    results = unittest.TextTestRunner(stream=sys.stdout).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1
    # maya running interactively often absorbs all the output.  comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
