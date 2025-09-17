#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Usdviewq.qt import QtWidgets
from pxr.Usdviewq.selectionDataModel import ALL_INSTANCES
from pxr.Usdviewq.common import SelectionHighlightModes

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.selHighlightMode = (
        SelectionHighlightModes.NEVER)

def _emitFrameAction(appController):
    appController._frameSelection()

def _emitToggleFrameAction(appController):
    appController._ui.actionToggle_Framed_View.triggered.emit() 
    QtWidgets.QApplication.processEvents()

# Test frame selection and toggling between cached frame views.
def _testFrameSelection(appController):

    # Frame the front sphere.

    appController._dataModel.selection.setPrimPath("/frontSphere", ALL_INSTANCES)
    _emitFrameAction(appController)

    appController._takeShot("framed.png")

    # Toggle back to the start camera.

    _emitToggleFrameAction(appController)
    appController._takeShot("toggleToStart.png")

    # Rotate the start camera.

    appController._dataModel.viewSettings.freeCamera.rotTheta = 90
    appController._stageView.updateGL()

    appController._takeShot("rotatedStart.png")

    # Toggle back to the framed camera, and make sure it did not rotate as well.

    _emitToggleFrameAction(appController)
    appController._takeShot("toggleToFramed.png")

# Test that selection highlighting works properly in usdview
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testFrameSelection(appController)
