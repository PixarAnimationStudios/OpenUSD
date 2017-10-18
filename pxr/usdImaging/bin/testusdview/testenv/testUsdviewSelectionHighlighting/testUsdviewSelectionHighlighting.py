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

from pxr import Sdf, UsdImagingGL

# Remove any unwanted visuals from the view.
def _modifySettings(mainWindow):
    mainWindow.showBBoxes = False
    mainWindow.showHUD = False

# Make a single selection.
def _testSingleSelection(mainWindow):
    mainWindow.selectNodeByPath("/backSphere", UsdImagingGL.GL.ALL_INSTANCES, "replace")
    mainWindow._itemSelectionChanged()

    viewportShot = mainWindow.GrabViewportShot()
    viewportShot.save("sel_highlight.png", "PNG")

# Make a single selection with selection highlighting disabled.
def _testNoHighlightSelection(mainWindow):
    mainWindow._ui.actionNever.setChecked(True)
    mainWindow._changeSelHighlightMode(mainWindow._ui.actionNever)

    mainWindow.selectNodeByPath("/backSphere", UsdImagingGL.GL.ALL_INSTANCES, "replace")
    mainWindow._itemSelectionChanged()

    viewportShot = mainWindow.GrabViewportShot()
    viewportShot.save("sel_highlight_none.png", "PNG")

    mainWindow._ui.actionOnly_when_paused.setChecked(True)
    mainWindow._changeSelHighlightMode(mainWindow._ui.actionOnly_when_paused)

# Make two selections.
def _testDoubleSelection(mainWindow):
    mainWindow.selectNodeByPath("/backSphere", UsdImagingGL.GL.ALL_INSTANCES, "replace")
    mainWindow._itemSelectionChanged()
    mainWindow.selectNodeByPath("/frontSphere", UsdImagingGL.GL.ALL_INSTANCES, "add")
    mainWindow._itemSelectionChanged()

    viewportShot = mainWindow.GrabViewportShot()
    viewportShot.save("sel_highlight_double.png", "PNG")

# Make a single selection with a non-default selection color.
def _testColorSelection(mainWindow):
    mainWindow._ui.actionSelCyan.setChecked(True)
    mainWindow._changeHighlightColor(mainWindow._ui.actionSelCyan)

    mainWindow.selectNodeByPath("/backSphere", UsdImagingGL.GL.ALL_INSTANCES, "replace")
    mainWindow._itemSelectionChanged()

    viewportShot = mainWindow.GrabViewportShot()
    viewportShot.save("sel_highlight_color.png", "PNG")

    mainWindow._ui.actionSelYellow.setChecked(True)
    mainWindow._changeHighlightColor(mainWindow._ui.actionSelYellow)

# Test that selection highlighting works properly in usdview
def testUsdviewInputFunction(mainWindow):
    _modifySettings(mainWindow)
    _testSingleSelection(mainWindow)
    _testNoHighlightSelection(mainWindow)
    _testDoubleSelection(mainWindow)
    _testColorSelection(mainWindow)

