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
from pxr.Usdviewq.qt import QtWidgets

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController.showBBoxes = False
    appController.showHUD = False

# Make a single selection.
def _testSingleSelection(appController):
    appController._selectionDataModel.setPrimPath("/backSphere")
    QtWidgets.QApplication.processEvents()

    viewportShot = appController.GrabViewportShot()
    viewportShot.save("sel_highlight.png", "PNG")

# Make a single selection with selection highlighting disabled.
def _testNoHighlightSelection(appController):
    appController._ui.actionNever.setChecked(True)
    appController._changeSelHighlightMode(appController._ui.actionNever)

    appController._selectionDataModel.setPrimPath("/backSphere")
    QtWidgets.QApplication.processEvents()

    viewportShot = appController.GrabViewportShot()
    viewportShot.save("sel_highlight_none.png", "PNG")

    appController._ui.actionOnly_when_paused.setChecked(True)
    appController._changeSelHighlightMode(appController._ui.actionOnly_when_paused)

# Make two selections.
def _testDoubleSelection(appController):
    with appController._selectionDataModel.batchPrimChanges:
        appController._selectionDataModel.clearPrims()
        appController._selectionDataModel.addPrimPath("/backSphere")
        appController._selectionDataModel.addPrimPath("/frontSphere")
    QtWidgets.QApplication.processEvents()

    viewportShot = appController.GrabViewportShot()
    viewportShot.save("sel_highlight_double.png", "PNG")

# Make a single selection with a non-default selection color.
def _testColorSelection(appController):
    appController._ui.actionSelCyan.setChecked(True)
    appController._changeHighlightColor(appController._ui.actionSelCyan)

    appController._selectionDataModel.setPrimPath("/backSphere")
    QtWidgets.QApplication.processEvents()

    viewportShot = appController.GrabViewportShot()
    viewportShot.save("sel_highlight_color.png", "PNG")

    appController._ui.actionSelYellow.setChecked(True)
    appController._changeHighlightColor(appController._ui.actionSelYellow)

# Test that selection highlighting works properly in usdview
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testSingleSelection(appController)
    _testNoHighlightSelection(appController)
    _testDoubleSelection(appController)
    _testColorSelection(appController)

