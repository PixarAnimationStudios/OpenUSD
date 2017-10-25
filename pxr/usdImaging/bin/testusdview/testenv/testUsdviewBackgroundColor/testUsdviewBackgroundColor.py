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

# Set the background color and refresh the view.
def _setBackgroundColorAction(mainWindow, action):
    action.setChecked(True)
    mainWindow._changeBgColor(action)
    mainWindow._stageView.updateGL()

# Take a shot of the viewport and save it to a file.
def _takeShot(mainWindow, fileName):
    viewportShot = mainWindow.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test with a black background color.
def _testBlackBackground(mainWindow):
    _setBackgroundColorAction(mainWindow, mainWindow._ui.actionBlack)
    _takeShot(mainWindow, "black.png")

# Test with a dark grey background color.
def _testGreyDarkBackground(mainWindow):
    _setBackgroundColorAction(mainWindow, mainWindow._ui.actionGrey_Dark)
    _takeShot(mainWindow, "grey_dark.png")

# Test with a light grey background color.
def _testGreyLightBackground(mainWindow):
    _setBackgroundColorAction(mainWindow, mainWindow._ui.actionGrey_Light)
    _takeShot(mainWindow, "grey_light.png")

# Test with a white background color.
def _testWhiteBackground(mainWindow):
    _setBackgroundColorAction(mainWindow, mainWindow._ui.actionWhite)
    _takeShot(mainWindow, "white.png")

# Test that the background color setting works properly in usdview.
def testUsdviewInputFunction(mainWindow):
    _modifySettings(mainWindow)
    _testBlackBackground(mainWindow)
    _testGreyDarkBackground(mainWindow)
    _testGreyLightBackground(mainWindow)
    _testWhiteBackground(mainWindow)
