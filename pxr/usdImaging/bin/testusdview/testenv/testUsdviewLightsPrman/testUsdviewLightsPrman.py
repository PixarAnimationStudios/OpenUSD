#!/pxrpythonsubst
#
# Copyright 2021 Pixar
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
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Set all light settings and refresh the view.
def _setLights(appController, ambientChecked, domeChecked, domeCamVis):
    appController._ui.actionAmbient_Only.setChecked(ambientChecked)
    appController._ambientOnlyClicked(ambientChecked)

    appController._ui.actionDomeLight.setChecked(domeChecked)
    appController._onDomeLightClicked(domeChecked)

    appController._ui.actionDomeLightTexturesVisible.setChecked(domeCamVis)
    appController._onDomeLightTexturesVisibleClicked(domeCamVis)

# Test with all lights off.
def _testNoLights(appController):
    _setLights(appController, False, False, False)
    appController._takeShot("noLightsPrman.png", waitForConvergence=True)

# Test with only the camera light.
def _testCameraLight(appController):
    _setLights(appController, True, False, False)
    appController._takeShot("cameraPrman.png", waitForConvergence=True)

# Test with only the dome light when camera visibility is on.
def _testDomeLight(appController):
    _setLights(appController, False, True, True)
    appController._takeShot("domePrman.png", waitForConvergence=True)

# Test with only the dome light when camera visibility is off.
def _testDomeLightCamVis(appController):
    _setLights(appController, False, True, False)
    appController._takeShot("domeCamVisPrman.png", waitForConvergence=True)

# Test with both the dome and camera lights.
def _testBothLights(appController):
    _setLights(appController, True, True, True)
    appController._takeShot("bothLightsPrman.png", waitForConvergence=True)

# Test that lights work properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testNoLights(appController)
    _testCameraLight(appController)
    _testDomeLight(appController)
    _testDomeLightCamVis(appController)
    _testBothLights(appController)
