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

# Set all light settings and refresh the view.
def _setLights(mainWindow, ambientChecked, keyChecked, fillChecked, backChecked):
    mainWindow._ui.actionAmbient_Only.setChecked(ambientChecked)
    mainWindow._ambientOnlyClicked(ambientChecked)

    mainWindow._ui.actionKey.setChecked(keyChecked)
    mainWindow._onKeyLightClicked(keyChecked)

    mainWindow._ui.actionFill.setChecked(fillChecked)
    mainWindow._onFillLightClicked(fillChecked)

    mainWindow._ui.actionBack.setChecked(backChecked)
    mainWindow._onBackLightClicked(backChecked)

    mainWindow._stageView.updateGL()

# Take a shot of the viewport and save it to a file.
def _takeShot(mainWindow, fileName):
    viewportShot = mainWindow.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test the ambient light with key/fill/back lights off.
def _testAmbientLightThreeOff(mainWindow):
    _setLights(mainWindow, True, False, False, False)
    _takeShot(mainWindow, "ambient_only_three_off.png")

# Test the ambient light with key/fill/back lights on.
def _testAmbientLightThreeOn(mainWindow):
    _setLights(mainWindow, True, False, False, False)
    _takeShot(mainWindow, "ambient_only_three_on.png")

# Test with all lights off.
def _testNoLights(mainWindow):
    _setLights(mainWindow, False, False, False, False)
    _takeShot(mainWindow, "none.png")

# Test with only the key light.
def _testKeyLights(mainWindow):
    _setLights(mainWindow, False, True, False, False)
    _takeShot(mainWindow, "key.png")

# Test with only the fill light.
def _testFillLights(mainWindow):
    _setLights(mainWindow, False, False, True, False)
    _takeShot(mainWindow, "fill.png")

# Test with only the back light.
def _testBackLights(mainWindow):
    _setLights(mainWindow, False, False, False, True)
    _takeShot(mainWindow, "back.png")

# Test with only the three non-ambient lights.
def _testThreeLights(mainWindow):
    _setLights(mainWindow, False, True, True, True)
    _takeShot(mainWindow, "three.png")

# Test that lights work properly in usdview.
def testUsdviewInputFunction(mainWindow):
    _modifySettings(mainWindow)
    _testAmbientLightThreeOff(mainWindow)
    _testAmbientLightThreeOn(mainWindow)
    _testNoLights(mainWindow)
    _testKeyLights(mainWindow)
    _testFillLights(mainWindow)
    _testBackLights(mainWindow)
    _testThreeLights(mainWindow)

