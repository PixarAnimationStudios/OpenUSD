#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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

# Test with only the camera light.
def _testCameraLight(appController):
    _setLights(appController, True, False, False)
    appController._takeShot("camera.png")

# Test with all lights off.
def _testNoLights(appController):
    _setLights(appController, False, False, False)
    appController._takeShot("noLights.png")

# Test with only the dome light when camera visibility is on.
def _testDomeLight(appController):
    _setLights(appController, False, True, True)
    appController._takeShot("dome.png")

# Test with only the dome light when camera visibility is off.
def _testDomeLightCamVis(appController):
    _setLights(appController, False, True, False)
    appController._takeShot("domeCamVis.png")

# Test with both the dome and camera lights.
def _testBothLights(appController):
    _setLights(appController, True, True, True)
    appController._takeShot("bothLights.png")

# Test that lights work properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testCameraLight(appController)
    _testNoLights(appController)
    _testDomeLight(appController)
    _testDomeLightCamVis(appController)
    _testBothLights(appController)
