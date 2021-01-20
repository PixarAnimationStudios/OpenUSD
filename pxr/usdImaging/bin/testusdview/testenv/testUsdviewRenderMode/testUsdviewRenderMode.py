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

from pxr import Sdf

# Remove any unwanted visuals from the view and set complexity.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Select a render mode and update the view.
def _setRenderModeAction(appController, action):
    action.setChecked(True)
    appController._changeRenderMode(action)
    appController._stageView.updateGL()

# Test wireframe.
def _testWireframe(appController):
    _setRenderModeAction(appController, appController._ui.actionWireframe)
    appController._takeShot("render_mode_wireframe.png")

# Test wireframe on surface.
def _testWireframeOnSurface(appController):
    _setRenderModeAction(appController, appController._ui.actionWireframeOnSurface)
    appController._takeShot("render_mode_wireframe_on_surface.png")

# Test smooth shading.
def _testSmoothShaded(appController):
    _setRenderModeAction(appController, appController._ui.actionSmooth_Shaded)
    appController._takeShot("render_mode_smooth.png")

# Test flat shading.
def _testFlatShading(appController):
    _setRenderModeAction(appController, appController._ui.actionFlat_Shaded)
    appController._takeShot("render_mode_flat.png")

# Test points.
def _testPoints(appController):
    _setRenderModeAction(appController, appController._ui.actionPoints)
    appController._takeShot("render_mode_points.png")

# Test geometry only shading.
def _testGeomOnly(appController):
    _setRenderModeAction(appController, appController._ui.actionGeom_Only)
    appController._takeShot("geom_only.png")

# Test geometry smooth shading.
def _testGeomSmooth(appController):
    _setRenderModeAction(appController, appController._ui.actionGeom_Smooth)
    appController._takeShot("geom_smooth.png")

# Test geometry flat shading.
def _testGeomFlat(appController):
    _setRenderModeAction(appController, appController._ui.actionGeom_Flat)
    appController._takeShot("geom_flat.png")

# Test hidden-surface wireframe.
def _testHiddenSurfaceWireframe(appController):
    _setRenderModeAction(appController, appController._ui.actionHidden_Surface_Wireframe)
    appController._takeShot("render_mode_hidden_surface_wireframe.png")

# Test that selection highlighting works properly in usdview
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testWireframe(appController)
    _testWireframeOnSurface(appController)
    _testSmoothShaded(appController)
    _testFlatShading(appController)
    _testPoints(appController)
    _testGeomOnly(appController)
    _testGeomSmooth(appController)
    _testGeomFlat(appController)
    _testHiddenSurfaceWireframe(appController)
