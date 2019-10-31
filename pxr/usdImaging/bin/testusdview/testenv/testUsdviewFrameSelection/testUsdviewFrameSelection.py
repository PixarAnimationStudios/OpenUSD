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

from pxr.Usdviewq.qt import QtWidgets
from pxr.Usdviewq.selectionDataModel import ALL_INSTANCES
from pxr.Usdviewq.common import SelectionHighlightModes

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.selHighlightMode = (
        SelectionHighlightModes.NEVER)

# Take a shot of the viewport and save it to a file.
def _takeShot(appController, fileName):
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

def _emitFrameAction(appController):
    appController._frameSelection()

def _emitToggleFrameAction(appController):
    appController._ui.actionToggle_Framed_View.triggered.emit() 
    QtWidgets.QApplication.processEvents()

# Test frame selection and toggling between cached frame views.
def _testFrameSelection(appController):

    # Frame the front sphere.

    appController._dataModel.selection.setPrimPath("/frontSphere", ALL_INSTANCES)
    _emitFrameAction(appController)

    _takeShot(appController, "framed.png")

    # Toggle back to the start camera.

    _emitToggleFrameAction(appController)
    _takeShot(appController, "toggleToStart.png")

    # Rotate the start camera.

    appController._dataModel.viewSettings.freeCamera.rotTheta = 90
    appController._stageView.updateGL()

    _takeShot(appController, "rotatedStart.png")

    # Toggle back to the framed camera, and make sure it did not rotate as well.

    _emitToggleFrameAction(appController)
    _takeShot(appController, "toggleToFramed.png")

# Test that selection highlighting works properly in usdview
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testFrameSelection(appController)
