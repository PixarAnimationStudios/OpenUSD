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

from pxr import UsdMaya

from maya import cmds

import os
import sys
import unittest


class testProxyShapeDrawUsdChangeProcessing(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls._testDir = os.path.abspath('.')

        cls._cameraName = 'MainCamera'

    def setUp(self):
        cmds.file(new=True, force=True)

        # To control where the rendered images are written, we force Maya to
        # use the test directory as the workspace.
        cmds.workspace(self._testDir, o=True)

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

    def _RunTest(self, dagPathName):
        rootPrim = UsdMaya.GetPrim(dagPathName)
        self.assertTrue(rootPrim)

        prim = rootPrim.GetChild('Geom').GetChild('Primitive')
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), 'Cube')
        self._WriteViewportImage(self._testName, 'initial')

        prim.SetTypeName('Sphere')
        self.assertEqual(prim.GetTypeName(), 'Sphere')
        self._WriteViewportImage(self._testName, 'Sphere')

        prim.SetTypeName('Cube')
        self.assertEqual(prim.GetTypeName(), 'Cube')
        self._WriteViewportImage(self._testName, 'Cube')

    def testUsdChangeProcessingAssembly(self):
        """
        Tests that authoring on a USD stage that is referenced by an assembly
        node (in "Collapsed" representation with a proxy shape underneath)
        invokes drawing refreshes in Maya correctly.
        """
        self._testName = 'UsdChangeProcessingTest_Assembly'

        mayaSceneFile = '%s.ma' % self._testName
        mayaSceneFullPath = os.path.abspath(mayaSceneFile)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        UsdMaya.LoadReferenceAssemblies()

        assemblyDagPathName = '|UsdChangeProcessingTest|Primitive'
        self._RunTest(assemblyDagPathName)

    def testUsdChangeProcessingProxy(self):
        """
        Tests that authoring on a USD stage that is referenced by a proxy shape
        invokes drawing refreshes in Maya correctly.
        """
        self._testName = 'UsdChangeProcessingTest_Proxy'

        mayaSceneFile = '%s.ma' % self._testName
        mayaSceneFullPath = os.path.abspath(mayaSceneFile)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        UsdMaya.LoadReferenceAssemblies()

        proxyShapeDagPathName = '|UsdChangeProcessingTest|Primitive|PrimitiveShape'
        self._RunTest(proxyShapeDagPathName)


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(
        testProxyShapeDrawUsdChangeProcessing)

    results = unittest.TextTestRunner(stream=sys.stdout).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1
    # maya running interactively often absorbs all the output.  comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
