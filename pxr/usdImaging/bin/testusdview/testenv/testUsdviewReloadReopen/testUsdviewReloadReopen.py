#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Usdviewq.qt import QtWidgets
from pxr.Usdviewq.common import SelectionHighlightModes

# XXX We will probably want harness-level facilities for this, as it
# will be required for any tests that do stage mutations, as we only
# update the viewport on a timer in response, so as not to over-refresh
def _waitForRefresh():
    import time
    time.sleep(0.5)
    QtWidgets.QApplication.processEvents()

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.selHighlightMode = (
        SelectionHighlightModes.NEVER)

# We use framing in this test both to test that reloading preserves
# camera position, and because it will trigger a redraw after stage
# mutations, which do not (yet).
def _emitFrameAction(appController):
    appController._frameSelection()

def _emitReload_All_LayersAction(appController):
    appController._ui.actionReload_All_Layers.triggered.emit() 
    QtWidgets.QApplication.processEvents()
    # This is a stage mutation, so must wait...
    _waitForRefresh()

def _emitReopen_StageAction(appController):
    appController._ui.actionReopen_Stage.triggered.emit() 
    QtWidgets.QApplication.processEvents()

#
# Test that Reloading All Layers resets file-backed layers while preserving
# camera posiiton, and that Reopen Stage fully resets the scene and camera.
#
def _testReloadReopen(appController):
    from pxr import Usd, UsdGeom

    #
    # Frame the front sphere, and color it red
    #
    appController._dataModel.selection.setPrimPath("/frontSphere")
    stage = appController._dataModel.stage
    sphere = UsdGeom.Sphere(stage.GetPrimAtPath("/frontSphere"))
    with Usd.EditContext(stage, stage.GetRootLayer()):
        sphere.CreateDisplayColorAttr([(1, 0, 0)])
    _emitFrameAction(appController)
    # Finally, clear selection so the red really shows
    appController._dataModel.selection.clear()

    appController._takeShot("coloredAndFramed.png")

    #
    # Reloading should set the sphere back to gray (because we authored into
    # its root layer), but otherwise not change the view
    #
    _emitReload_All_LayersAction(appController)
    appController._takeShot("reloaded.png")

    #
    # Reopening the stage should completely reset the view
    #
    _emitReopen_StageAction(appController)
    appController._takeShot("reopened.png")

def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testReloadReopen(appController)
