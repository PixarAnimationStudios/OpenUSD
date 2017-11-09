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

# Remove any unwanted visuals from the view.
def _modifySettings(mainWindow):
    mainWindow.showBBoxes = False
    mainWindow.showHUD = False

# Set the mask mode and refresh the view.
def _setCameraMaskModeAction(mainWindow, action, outline=False):
    action.setChecked(True)
    mainWindow._ui.actionCameraMask_Outline.setChecked(outline)
    mainWindow._updateCameraMaskMenu()

# Take a shot of the viewport and save it to a file.
def _takeShot(mainWindow, fileName):
    viewportShot = mainWindow.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test with no masking.
def _testNoMask(mainWindow):
    _setCameraMaskModeAction(mainWindow, mainWindow._ui.actionCameraMask_None)
    _takeShot(mainWindow, "none.png")

# Test with outline enabled but no masking.
def _testOutline(mainWindow):
    _setCameraMaskModeAction(mainWindow, mainWindow._ui.actionCameraMask_None, outline=True)
    _takeShot(mainWindow, "outline.png")

# Test with partial masking.
def _testPartial(mainWindow):
    _setCameraMaskModeAction(mainWindow, mainWindow._ui.actionCameraMask_Partial)
    _takeShot(mainWindow, "partial.png")

# Test with full masking.
def _testFull(mainWindow):
    _setCameraMaskModeAction(mainWindow, mainWindow._ui.actionCameraMask_Full)
    _takeShot(mainWindow, "full.png")

# Test that the complexity setting works properly in usdview.
def testUsdviewInputFunction(mainWindow):
    _modifySettings(mainWindow)
    _testNoMask(mainWindow)
    _testOutline(mainWindow)
    _testPartial(mainWindow)
    _testFull(mainWindow)
