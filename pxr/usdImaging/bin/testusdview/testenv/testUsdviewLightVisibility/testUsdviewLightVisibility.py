#!/pxrpythonsubst
#
# Copyright 2020 Pixar
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


from __future__ import print_function
import sys
from pxr.Usdviewq.qt import QtWidgets


# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Turn off the Camera and Dome light.
def _turnLightsOff(appController):
    appController._ui.actionAmbient_Only.setChecked(False)
    appController._ambientOnlyClicked(False)

    appController._ui.actionDomeLight.setChecked(False)
    appController._onDomeLightClicked(False)

    appController._stageView.updateGL()

# Select one or more prim paths, then set visible state of those prims.
def _selectAndSetVisible(appController, visible, paths):
    selection = appController._dataModel.selection
    with selection.batchPrimChanges:
        selection.clearPrims()
        for path in paths:
            selection.addPrimPath(path)

    if visible:
        appController.visSelectedPrims()
        # We must processEvents after every call to activateSelectedPrims() so the
        # activated PrimViewItems can repopulate. (See _primViewUpdateTimer in
        # appController.py)
        QtWidgets.QApplication.processEvents()
    else:
        appController.invisSelectedPrims()

# Test making a single light invisible then make it visible.
def _testSingleVisible(appController):
    _selectAndSetVisible(appController, False, ["/lights/light1"])
    appController._takeShot("singleInvisible1.png")
    _selectAndSetVisible(appController, True, ["/lights/light1"])

    _selectAndSetVisible(appController, False, ["/lights/light2"])
    appController._takeShot("singleInvisible2.png")
    _selectAndSetVisible(appController, True, ["/lights/light2"])

# Test all lights visible/invisible.
def _testAllVisible(appController):
    _selectAndSetVisible(appController, False, ["/lights/light1"])
    _selectAndSetVisible(appController, False, ["/lights/light2"])

    appController._takeShot("allInvisible.png")

    _selectAndSetVisible(appController, True, ["/lights/light1"])
    _selectAndSetVisible(appController, True, ["/lights/light2"])

    appController._takeShot("allVisible.png")


# Test that the complexity setting works properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _turnLightsOff(appController)
    _testSingleVisible(appController)
    _testAllVisible(appController)
