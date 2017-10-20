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

from pxr import Sdf, UsdImagingGL

# Remove any unwanted visuals from the view and set complexity.
def _modifySettings(mainWindow):
    mainWindow.showBBoxes = False
    mainWindow.showHUD = False

# Select a render mode and update the view.
def _setRenderModeAction(mainWindow, action):
    action.setChecked(True)
    mainWindow._changeRenderMode(action)
    mainWindow._stageView.updateGL()

# Take a shot of the viewport and save it to a file.
def _takeShot(mainWindow, fileName):
    viewportShot = mainWindow.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test wireframe.
def _testWireframe(mainWindow):
    _setRenderModeAction(mainWindow, mainWindow._ui.actionWireframe)
    _takeShot(mainWindow, "render_mode_wireframe.png")

# Test wireframe on surface.
def _testWireframeOnSurface(mainWindow):
    _setRenderModeAction(mainWindow, mainWindow._ui.actionWireframeOnSurface)
    _takeShot(mainWindow, "render_mode_wireframe_on_surface.png")

# Test smooth shading.
def _testSmoothShaded(mainWindow):
    _setRenderModeAction(mainWindow, mainWindow._ui.actionSmooth_Shaded)
    _takeShot(mainWindow, "render_mode_smooth.png")

# Test flat shading.
def _testFlatShading(mainWindow):
    _setRenderModeAction(mainWindow, mainWindow._ui.actionFlat_Shaded)
    _takeShot(mainWindow, "render_mode_flat.png")

# Test points.
def _testPoints(mainWindow):
    _setRenderModeAction(mainWindow, mainWindow._ui.actionPoints)
    _takeShot(mainWindow, "render_mode_points.png")

# Test geometry only shading.
def _testGeomOnly(mainWindow):
    _setRenderModeAction(mainWindow, mainWindow._ui.actionGeom_Only)
    _takeShot(mainWindow, "geom_only.png")

# Test geometry smooth shading.
def _testGeomSmooth(mainWindow):
    _setRenderModeAction(mainWindow, mainWindow._ui.actionGeom_Smooth)
    _takeShot(mainWindow, "geom_smooth.png")

# Test geometry flat shading.
def _testGeomFlat(mainWindow):
    _setRenderModeAction(mainWindow, mainWindow._ui.actionGeom_Flat)
    _takeShot(mainWindow, "geom_flat.png")

# Test hidden-surface wireframe.
def _testHiddenSurfaceWireframe(mainWindow):
    _setRenderModeAction(mainWindow, mainWindow._ui.actionHidden_Surface_Wireframe)
    _takeShot(mainWindow, "render_mode_hidden_surface_wireframe.png")

# Test that selection highlighting works properly in usdview
def testUsdviewInputFunction(mainWindow):
    _modifySettings(mainWindow)
    _testWireframe(mainWindow)
    _testWireframeOnSurface(mainWindow)
    _testSmoothShaded(mainWindow)
    _testFlatShading(mainWindow)
    _testPoints(mainWindow)
    _testGeomOnly(mainWindow)
    _testGeomSmooth(mainWindow)
    _testGeomFlat(mainWindow)
    _testHiddenSurfaceWireframe(mainWindow)
