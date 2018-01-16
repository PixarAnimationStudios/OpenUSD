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

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

    # Ambient isn't visible to perceptual diff with direct camera lighting, so
    # use the key light instead. This makes ambient visible in one half of the
    # sphere and specular visible in the other half.
    _setLights(appController, False, True, False, False)

# Set the default material and update the view.
def _setDefaultMaterial(appController, ambient, specular):
    appController._dataModel.viewSettings.defaultMaterialAmbient = ambient
    appController._dataModel.viewSettings.defaultMaterialSpecular = specular
    appController._stageView.updateGL()

# Take a shot of the viewport and save it to a file.
def _takeShot(appController, fileName):
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test with no ambient and no specular reflection.
def _testNone(appController):
    _setDefaultMaterial(appController, 0, 0)
    _takeShot(appController, "none.png")

# Test with high ambient and no specular reflection.
def _testAmbient(appController):
    _setDefaultMaterial(appController, 1, 0)
    _takeShot(appController, "ambient.png")

# Test with no ambient and high specular reflection.
def _testSpecular(appController):
    _setDefaultMaterial(appController, 0, 1)
    _takeShot(appController, "specular.png")

# Test with high ambient and high specular reflection.
def _testBoth(appController):
    _setDefaultMaterial(appController, 1, 1)
    _takeShot(appController, "both.png")

# Test that default materials work properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testNone(appController)
    _testAmbient(appController)
    _testSpecular(appController)
    _testBoth(appController)
