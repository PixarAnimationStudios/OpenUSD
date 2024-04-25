#!/pxrpythonsubst
#
# Copyright 2024 Pixar
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

# Set builtin light settings and refresh the view.
def _setBuiltinLights(appController, cameraLight, domeLight, domeCamVis):
    appController._ui.actionAmbient_Only.setChecked(cameraLight)
    appController._ambientOnlyClicked(cameraLight)

    appController._ui.actionDomeLight.setChecked(domeLight)
    appController._onDomeLightClicked(domeLight)

    appController._ui.actionDomeLightTexturesVisible.setChecked(domeCamVis)
    appController._onDomeLightTexturesVisibleClicked(domeCamVis)

# Set SceneLights enabled or disabled and refresh the view
def _setSceneLights(appController, sceneLightsEnabled):
    appController._ui.actionEnable_Scene_Lights.setChecked(sceneLightsEnabled)
    appController._toggleEnableSceneLights()

def _getRendererAppendedImageName(appController, filename):
    rendererName = appController._stageView.rendererDisplayName
    imageName = filename + "_" + rendererName + ".png"
    print(" -", imageName)
    return imageName


# Test the camera light with scene lights disabled 
def _testCameraLight(appController):
    _setBuiltinLights(appController, True, False, False)
    _setSceneLights(appController, False)
    filename = _getRendererAppendedImageName(appController, "camera_noScene")
    appController._takeShot(filename, waitForConvergence=True)

# Test the camera light with scene lights enabled 
def _testCamera_SceneLight(appController):
    _setBuiltinLights(appController, True, False, False)
    _setSceneLights(appController, True)
    filename = _getRendererAppendedImageName(appController, "camera_scene")
    appController._takeShot(filename, waitForConvergence=True)

# Test dome light with scene lights disabled
def _testDomeVis(appController):
    _setBuiltinLights(appController, False, True, True)
    _setSceneLights(appController, False)
    filename = _getRendererAppendedImageName(appController, "domeVis_noScene")
    appController._takeShot(filename, waitForConvergence=True)

# Test dome light with scene lights disabled
def _testDomeVis_SceneLight(appController):
    _setBuiltinLights(appController, False, True, True)
    _setSceneLights(appController, True)
    filename = _getRendererAppendedImageName(appController, "domeVis_scene")
    appController._takeShot(filename, waitForConvergence=True)

# Test dome light with scene lights disabled
def _testDomeInvis(appController):
    _setBuiltinLights(appController, False, True, False)
    _setSceneLights(appController, False)
    filename = _getRendererAppendedImageName(appController, "domeInvis_noScene")
    appController._takeShot(filename, waitForConvergence=True)

# Test dome light with scene lights disabled
def _testDomeInvis_SceneLight(appController):
    _setBuiltinLights(appController, False, True, False)
    _setSceneLights(appController, True)
    filename = _getRendererAppendedImageName(appController, "domeInvis_scene")
    appController._takeShot(filename, waitForConvergence=True)

# Test scene lights with out any built in lights 
def _testSceneLights(appController):
    _setBuiltinLights(appController, False, False, False)
    _setSceneLights(appController, True)
    filename = _getRendererAppendedImageName(appController, "sceneLights")
    appController._takeShot(filename, waitForConvergence=True)

    # Edit the light color of the scene light
    s = appController._dataModel.stage
    l = s.GetSessionLayer()
    s.SetEditTarget(l)
    light = s.GetPrimAtPath('/Lights/light1')
    light.GetAttribute('inputs:color').Set((1,0,0))
    filename = _getRendererAppendedImageName(appController, "sceneLights_edit")
    appController._takeShot(filename, waitForConvergence=True)
    
# Test no built in or scene lights 
def _testNoLights(appController):
    _setBuiltinLights(appController, False, False, False)
    _setSceneLights(appController, False)
    filename = _getRendererAppendedImageName(appController, "noLights")
    appController._takeShot(filename, waitForConvergence=True)


# Test that lights work properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)

    # Builtin Camera light with and without scene lights
    _testCameraLight(appController)
    _testCamera_SceneLight(appController)

    # Builtin Dome light (camera Vis and invis) with and without scene lights 
    _testDomeVis_SceneLight(appController)
    _testDomeVis(appController)
    _testDomeInvis(appController)
    _testDomeInvis_SceneLight(appController)

    # no builtin lights with and without scene lights
    _testSceneLights(appController)
    _testNoLights(appController)
