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

# Maya 2017 and later use PyQt5/PySide2 while Maya 2016 and earlier use
# PyQt4/PySide. We test whether we're running in Maya 2017+ by trying to import
# PySide2, which should only be available there. If that succeeds, we import
# the rest of the modules from PySide2. Otherwise, we assume we're in 2016 or
# earlier and we import everything from PySide.
try:
    import PySide2
    usePySide2 = True
except ImportError:
    usePySide2 = False

if usePySide2:
    from PySide2 import QtCore
    from PySide2.QtTest import QTest
    from PySide2.QtWidgets import QWidget

    from shiboken2 import wrapInstance
else:
    from PySide import QtCore
    from PySide.QtTest import QTest
    from PySide.QtGui import QWidget

    from shiboken import wrapInstance

from maya import OpenMayaUI as OMUI
from maya import cmds

import os
import sys
import unittest


class testProxyShapeLiveSurface(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')
        cmds.loadPlugin('pxrUsd')

    def setUp(self):
        cmds.file(
                os.path.abspath('ProxyShapeLiveSurfaceTest.ma'),
                open=True, force=True)

    def testObjectPosition(self):
        """
        Tests that an object created interactively is positioned correctly on
        the live surface.
        """
        # Load our reference assembly.
        UsdMaya.LoadReferenceAssemblies()

        # Create a new custom viewport.
        window = cmds.window(widthHeight=(500, 400))
        cmds.paneLayout()
        panel = cmds.modelPanel()
        cmds.modelPanel(panel, edit=True, camera='persp')
        cmds.modelEditor(cmds.modelPanel(panel, q=True, modelEditor=True),
                edit=True, displayAppearance='smoothShaded', rnm='vp2Renderer')
        cmds.showWindow(window)

        # Get the viewport widget.
        view = OMUI.M3dView()
        OMUI.M3dView.getM3dViewFromModelPanel(panel, view)
        viewWidget = wrapInstance(long(view.widget()), QWidget)

        # Make our assembly live.
        cmds.makeLive('Block_1')

        # Enter interactive creation context.
        cmds.setToolTo('CreatePolyTorusCtx')

        # Click in the center of the viewport widget.
        QTest.mouseClick(viewWidget, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier,
                viewWidget.rect().center())

        # Find the torus (it should be called pTorus1).
        self.assertTrue(cmds.ls('pTorus1'))

        # Check the torus's transform.
        # The Block_1 is originally 1 unit deep and centered at the origin, so
        # its original top plane is aligned at z=0.5.
        # It is scaled 5x, which makes the top plane aligned at z=2.5.
        # Then it is translated along z by 4.0, so its top plane is finally
        # aligned at z=6.5.
        translate = cmds.xform('pTorus1', q=True, t=True)
        self.assertAlmostEqual(translate[2], 6.5, places=3)

    def testObjectNormal(self):
        """
        Tests that an object created interactively by dragging in the viewport
        has the correct orientation based on the live surface normal.
        """
        from pxr import Gf

        # Load our reference assembly.
        UsdMaya.LoadReferenceAssemblies()

        # Create a new custom viewport.
        window = cmds.window(widthHeight=(500, 400))
        cmds.paneLayout()
        panel = cmds.modelPanel()
        cmds.modelPanel(panel, edit=True, camera='persp')
        cmds.modelEditor(cmds.modelPanel(panel, q=True, modelEditor=True),
                edit=True, displayAppearance='smoothShaded', rnm='vp2Renderer')
        cmds.showWindow(window)

        # Get the viewport widget.
        view = OMUI.M3dView()
        OMUI.M3dView.getM3dViewFromModelPanel(panel, view)
        viewWidget = wrapInstance(long(view.widget()), QWidget)

        # Make our assembly live.
        cmds.makeLive('Block_2')

        # Enter interactive creation context.
        cmds.setToolTo('CreatePolyConeCtx')

        # Click in the center of the viewport widget.
        QTest.mouseClick(viewWidget, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier,
                viewWidget.rect().center())

        # Find the cone (it should be called pCone1).
        self.assertTrue(cmds.ls('pCone1'))

        # Check the cone's rotation.
        # Because our scene is Z-axis up, the cone's Z-axis should be aligned
        # with Block_2's surface normal (though it might not necessarily have
        # the same exact rotation).
        rotationAngles = cmds.xform('pCone1', q=True, ro=True)
        rotation = (Gf.Rotation(Gf.Vec3d.XAxis(), rotationAngles[0]) *
                Gf.Rotation(Gf.Vec3d.YAxis(), rotationAngles[1]) *
                Gf.Rotation(Gf.Vec3d.ZAxis(), rotationAngles[2]))
        actualZAxis = rotation.TransformDir(Gf.Vec3d.ZAxis())

        expectedRotation = (Gf.Rotation(Gf.Vec3d.XAxis(), 75.0) *
                Gf.Rotation(Gf.Vec3d.YAxis(), 90.0))
        expectedZAxis = expectedRotation.TransformDir(Gf.Vec3d.ZAxis())

        # Verify that the error angle between the two axes is less than
        # 0.1 degrees (less than ~0.0003 of a revolution, so not bad). That's
        # about as close as we're going to get.
        errorRotation = Gf.Rotation(actualZAxis, expectedZAxis)
        self.assertLess(errorRotation.GetAngle(), 0.1)

if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(
            testProxyShapeLiveSurface)

    results = unittest.TextTestRunner(stream=sys.stdout).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1
    # maya running interactively often absorbs all the output.  comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
