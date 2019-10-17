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

class testProxyShapeDrawColors(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        cmds.loadPlugin('pxrUsd')

        cls._testDir = os.path.abspath('.')

    def setUp(self):
        mayaFilePath = os.path.abspath('stage.ma')
        cmds.file(mayaFilePath, open=True, force=True)

        # To control where the rendered images are written, we force Maya to
        # use the test directory as the workspace.
        cmds.workspace(self._testDir, o=True)

    def testMeshNextToAssemblyAndImported(self):
        """
        Tests the colors between a mesh with (0.55, 0.55, 0.55)
        exporting that file and then re-importing it, and also referencing
        it back into the same scene.

        While this is a bit more than just "GL" testing, it's a useful place to
        centralize all this.  If we don't like that this is testing usdImport
        functionality, we can remove

        This will render as follows:


        blank    |  usdImport
                 |
        ---------+-----------
        modeled  |  ref'd
                 |

        """
        x = self._PlaneWithColor((0.55, 0.55, 0.55))
        cmds.loadPlugin('pxrUsd')
        cwd = os.path.abspath('.')
        usdFile = os.path.join(cwd, 'plane.usd')
        cmds.select(x)
        cmds.usdExport(file=usdFile, selection=True, shadingMode='displayColor')
        assembly = cmds.assembly(name='ref', type='pxrUsdReferenceAssembly')
        cmds.xform(assembly, translation=(30.48, 0, 0))
        cmds.setAttr('%s.filePath' % assembly, usdFile, type='string')
        cmds.setAttr('%s.primPath' % assembly, '/pPlane1', type='string')
        cmds.assembly(assembly, edit=True, active='Collapsed')

        x = cmds.usdImport(file=usdFile)
        cmds.xform(x, translation=(30.48, 30.48, 0))

        cmds.setAttr("hardwareRenderingGlobals.floatingPointRTEnable", 0)
        cmds.setAttr('defaultColorMgtGlobals.outputTransformEnabled', 0)
        self._Snapshot('default')
        cmds.setAttr("hardwareRenderingGlobals.floatingPointRTEnable", 1)
        cmds.setAttr('defaultColorMgtGlobals.outputTransformEnabled', 1)
        self._Snapshot('colorMgt')

    def _PlaneWithColor(self, c):
        ret = cmds.polyPlane(width=30.48, height=30.48, sx=4, sy=4, ax=(0, 0, 1))
        cmds.setAttr('lambert1.color', *c)
        return ret

    def _Snapshot(self, outName):
        cmds.select([])
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

        cmds.setAttr('defaultRenderGlobals.imageFilePrefix', outName,
                type='string')

        # Do the render.
        cmds.ogsRender(camera='top', currentFrame=True, width=400,
            height=400)

if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(testProxyShapeDrawColors)

    results = unittest.TextTestRunner(stream=sys.stdout).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1
    # maya running interactively often absorbs all the output.  comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
