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

# Remove any unwanted visuals from the view.
def _modifySettings(mainWindow):
    mainWindow.showBBoxes = False
    mainWindow.showHUD = False

    # Ambient isn't visible to perceptual diff with direct camera lighting, so
    # use the key light instead. This makes ambient visible in one half of the
    # sphere and specular visible in the other half.
    _setLights(mainWindow, False, True, False, False)

# Set the default material and update the view.
def _setDefaultMaterial(mainWindow, ambient, specular):
    mainWindow.defaultMaterialAmbient = ambient
    mainWindow.defaultMaterialSpecular = specular
    mainWindow._stageView.updateGL()

# Take a shot of the viewport and save it to a file.
def _takeShot(mainWindow, fileName):
    viewportShot = mainWindow.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test with no ambient and no specular reflection.
def _testNone(mainWindow):
    _setDefaultMaterial(mainWindow, 0, 0)
    _takeShot(mainWindow, "none.png")

# Test with high ambient and no specular reflection.
def _testAmbient(mainWindow):
    _setDefaultMaterial(mainWindow, 1, 0)
    _takeShot(mainWindow, "ambient.png")

# Test with no ambient and high specular reflection.
def _testSpecular(mainWindow):
    _setDefaultMaterial(mainWindow, 0, 1)
    _takeShot(mainWindow, "specular.png")

# Test with high ambient and high specular reflection.
def _testBoth(mainWindow):
    _setDefaultMaterial(mainWindow, 1, 1)
    _takeShot(mainWindow, "both.png")

# Test that default materials work properly in usdview.
def testUsdviewInputFunction(mainWindow):
    _modifySettings(mainWindow)
    _testNone(mainWindow)
    _testAmbient(mainWindow)
    _testSpecular(mainWindow)
    _testBoth(mainWindow)
