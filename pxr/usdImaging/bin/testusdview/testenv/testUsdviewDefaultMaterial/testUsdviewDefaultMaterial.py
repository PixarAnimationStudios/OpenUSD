#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Set all light settings and refresh the view.
def _setLights(appController, ambientChecked, domeChecked):
    appController._ui.actionAmbient_Only.setChecked(ambientChecked)
    appController._ambientOnlyClicked(ambientChecked)

    appController._ui.actionDomeLight.setChecked(domeChecked)
    appController._onDomeLightClicked(domeChecked)

    appController._stageView.updateGL()

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

    _setLights(appController, True, True)

# Set the default material and update the view.
def _setDefaultMaterial(appController, ambient, specular):
    appController._dataModel.viewSettings.defaultMaterialAmbient = ambient
    appController._dataModel.viewSettings.defaultMaterialSpecular = specular
    appController._stageView.updateGL()

# Test with no ambient and no specular reflection.
def _testNone(appController):
    _setDefaultMaterial(appController, 0, 0)
    appController._takeShot("none.png")

# Test with high ambient and no specular reflection.
def _testAmbient(appController):
    _setDefaultMaterial(appController, 1, 0)
    appController._takeShot("ambient.png")

# Test with no ambient and high specular reflection.
def _testSpecular(appController):
    _setDefaultMaterial(appController, 0, 1)
    appController._takeShot("specular.png")

# Test with high ambient and high specular reflection.
def _testBoth(appController):
    _setDefaultMaterial(appController, 1, 1)
    appController._takeShot("both.png")

# Test that default materials work properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testNone(appController)
    _testAmbient(appController)
    _testSpecular(appController)
    _testBoth(appController)
