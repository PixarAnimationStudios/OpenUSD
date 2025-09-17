#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
