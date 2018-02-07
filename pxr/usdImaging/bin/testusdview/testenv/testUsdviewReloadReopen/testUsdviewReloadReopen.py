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

# Take a shot of the viewport and save it to a file.
def _takeShot(appController, fileName):
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# We use framing in this test both to test that reloading preserves
# camera position, and because it will trigger a redraw after stage
# mutations, which do not (yet).
def _emitFrameAction(appController):
    appController._ui.actionFrame_Selection.triggered.emit() 
    QtWidgets.QApplication.processEvents()

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

    _takeShot(appController, "coloredAndFramed.png")

    #
    # Reloading should set the sphere back to gray (because we authored into
    # its root layer), but otherwise not change the view
    #
    _emitReload_All_LayersAction(appController)
    _takeShot(appController, "reloaded.png")

    #
    # Reopening the stage should completely reset the view
    #
    _emitReopen_StageAction(appController)
    _takeShot(appController, "reopened.png")

def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testReloadReopen(appController)
