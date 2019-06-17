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


class testBatchRendererIsolateSelection(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        # To control where the rendered images are written, we force Maya to
        # use the test directory as the workspace.
        cmds.workspace(os.path.abspath('.'), o=True)

    def _MakeModelPanel(self):
        window = cmds.window(widthHeight=(400, 400))
        cmds.paneLayout()
        panel = cmds.modelPanel()
        cmds.modelPanel(panel, edit=True, camera='top')
        cmds.modelEditor(cmds.modelPanel(panel, q=True, modelEditor=True),
                edit=True, displayAppearance='smoothShaded', rnm='vp2Renderer')
        cmds.showWindow(window)
        return panel

    def _HitTest(self, panel, cube1, cube2, cube3):
        width = cmds.modelEditor(panel, q=True, width=True)
        height = cmds.modelEditor(panel, q=True, height=True)

        # The scene is set up so it looks like this from the "top" camera:
        #                                           #
        #-----------+                   +-----------#
        #           |     +-------+     |           #
        #           |     |       |     |           #
        # cube2     |     | cube1 |     |     cube3 #
        #           |     |       |     |           #
        #           |     +-------+     |           #
        #-----------+                   +-----------#
        #                                           #
        cmds.refresh(force=True)
        test1 = bool(cmds.hitTest(panel, width / 2, height / 2))
        self.assertEqual(test1, cube1)

        cmds.refresh(force=True)
        test2 = bool(cmds.hitTest(panel, 2, height / 2))
        self.assertEqual(test2, cube2)

        cmds.refresh(force=True)
        test3 = bool(cmds.hitTest(panel, width - 2, height / 2))
        self.assertEqual(test3, cube3)

    def testIsolateSelection(self):
        cmds.file(os.path.abspath('IsolateSelectionTest.ma'),
                open=True, force=True)

        # Isolate selection seems to only apply to the interactive viewports,
        # and not to ogsRenders. However, it's difficult for us to render
        # viewports with exact sizes to do baseline comparisons. Since isolate
        # selection needs to work with selection hit-testing as well, we use
        # that as a proxy for checking that isolate selection draws properly.
        panel1 = self._MakeModelPanel()
        panel2 = self._MakeModelPanel()

        # Load all assemblies.
        UsdMaya.LoadReferenceAssemblies()
        self._HitTest(panel1, True, True, True)
        self._HitTest(panel2, True, True, True)

        # Isolate CubeModel1.
        cmds.select("CubeModel1")
        cmds.isolateSelect(panel1, state=True)
        self._HitTest(panel1, True, False, False)
        self._HitTest(panel2, True, True, True)

        # Isolate CubeModel2 and CubeModel3.
        cmds.select("CubeModel2", "CubeModel3")
        cmds.isolateSelect(panel1, loadSelected=True)
        self._HitTest(panel1, False, True, True)

        # Get out of isolate selection.
        cmds.isolateSelect(panel1, state=False)
        self._HitTest(panel1, True, True, True)

        # Enter isolate selection on panel2 this time.
        cmds.select("CubeModel2", "CubeModel3")
        cmds.isolateSelect(panel2, state=True)
        self._HitTest(panel1, True, True, True)
        self._HitTest(panel2, False, True, True)

        # Then add an isolate selection to panel1 again.
        cmds.select("CubeModel1", "CubeModel2")
        cmds.isolateSelect(panel1, state=True)
        self._HitTest(panel1, True, True, False)
        self._HitTest(panel2, False, True, True)

        # Unload assemblies.
        UsdMaya.UnloadReferenceAssemblies()
        self._HitTest(panel1, False, False, False)
        self._HitTest(panel2, False, False, False)

        # Reload assemblies.
        UsdMaya.LoadReferenceAssemblies()
        self._HitTest(panel1, True, True, False)
        self._HitTest(panel2, False, True, True)


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(
            testBatchRendererIsolateSelection)

    results = unittest.TextTestRunner(stream=sys.stdout).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1
    # maya running interactively often absorbs all the output.  comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
