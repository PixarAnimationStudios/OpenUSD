#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Set all light settings and refresh the view.
def _setLights(appController, ambientChecked, domeChecked):
    appController._ui.actionAmbient_Only.setChecked(ambientChecked)
    appController._ambientOnlyClicked(ambientChecked)

    appController._ui.actionDomeLight.setChecked(domeChecked)
    appController._onDomeLightClicked(domeChecked)

    appController._stageView.updateGL()

# Test with only the dome light.
def _testDomeLight(appController):
    _setLights(appController, False, True)
    appController._takeShot("domeYup.png")

# Test with only the dome and camera lights.
def _testBothLights(appController):
    _setLights(appController, True, True)
    appController._takeShot("bothLightsYup.png")

# Test that lights work properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testDomeLight(appController)
    _testBothLights(appController)
