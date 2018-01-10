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
def _modifySettings(appController):
    appController._viewSettingsDataModel.showBBoxes = False
    appController._viewSettingsDataModel.showHUD = False

# Set all light settings and refresh the view.
def _setLights(appController, ambientChecked, keyChecked, fillChecked, backChecked):
    appController._ui.actionAmbient_Only.setChecked(ambientChecked)
    appController._ambientOnlyClicked(ambientChecked)

    appController._ui.actionKey.setChecked(keyChecked)
    appController._onKeyLightClicked(keyChecked)

    appController._ui.actionFill.setChecked(fillChecked)
    appController._onFillLightClicked(fillChecked)

    appController._ui.actionBack.setChecked(backChecked)
    appController._onBackLightClicked(backChecked)

    appController._stageView.updateGL()

# Take a shot of the viewport and save it to a file.
def _takeShot(appController, fileName):
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test the ambient light with key/fill/back lights off.
def _testAmbientLightThreeOff(appController):
    _setLights(appController, True, False, False, False)
    _takeShot(appController, "ambient_only_three_off.png")

# Test the ambient light with key/fill/back lights on.
def _testAmbientLightThreeOn(appController):
    _setLights(appController, True, False, False, False)
    _takeShot(appController, "ambient_only_three_on.png")

# Test with all lights off.
def _testNoLights(appController):
    _setLights(appController, False, False, False, False)
    _takeShot(appController, "none.png")

# Test with only the key light.
def _testKeyLights(appController):
    _setLights(appController, False, True, False, False)
    _takeShot(appController, "key.png")

# Test with only the fill light.
def _testFillLights(appController):
    _setLights(appController, False, False, True, False)
    _takeShot(appController, "fill.png")

# Test with only the back light.
def _testBackLights(appController):
    _setLights(appController, False, False, False, True)
    _takeShot(appController, "back.png")

# Test with only the three non-ambient lights.
def _testThreeLights(appController):
    _setLights(appController, False, True, True, True)
    _takeShot(appController, "three.png")

# Test that lights work properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testAmbientLightThreeOff(appController)
    _testAmbientLightThreeOn(appController)
    _testNoLights(appController)
    _testKeyLights(appController)
    _testFillLights(appController)
    _testBackLights(appController)
    _testThreeLights(appController)

