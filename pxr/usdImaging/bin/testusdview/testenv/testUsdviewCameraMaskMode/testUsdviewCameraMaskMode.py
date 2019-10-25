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

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Set the mask mode and refresh the view.
def _setCameraMaskModeAction(appController, action, outline=False):
    action.setChecked(True)
    appController._updateCameraMaskMenu()

    appController._ui.actionCameraMask_Outline.setChecked(outline)
    appController._updateCameraMaskOutlineMenu()

    QtWidgets.QApplication.processEvents()

# Take a shot of the viewport and save it to a file.
def _takeShot(appController, fileName):
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test with no masking.
def _testNoMask(appController):
    _setCameraMaskModeAction(appController, appController._ui.actionCameraMask_None)
    _takeShot(appController, "none.png")

# Test with outline enabled but no masking.
def _testOutline(appController):
    _setCameraMaskModeAction(appController, appController._ui.actionCameraMask_None, outline=True)
    _takeShot(appController, "outline.png")

# Test with partial masking.
def _testPartial(appController):
    _setCameraMaskModeAction(appController, appController._ui.actionCameraMask_Partial)
    _takeShot(appController, "partial.png")

# Test with full masking.
def _testFull(appController):
    _setCameraMaskModeAction(appController, appController._ui.actionCameraMask_Full)
    _takeShot(appController, "full.png")

# Test that the complexity setting works properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testNoMask(appController)
    _testOutline(appController)
    _testPartial(appController)
    _testFull(appController)
