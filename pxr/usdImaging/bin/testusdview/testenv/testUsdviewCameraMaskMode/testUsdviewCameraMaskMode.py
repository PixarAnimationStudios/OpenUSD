#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Usdviewq.qt import QtWidgets
import string

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Set the mask mode and refresh the view.
def _setCameraMaskModeAction(appController, action, outline=False):
    action.setChecked(True)
    appController._updateCameraMaskMenu()

    appController._ui.actionCameraMask_Outline.setChecked(outline)
    appController._updateCameraMaskOutlineMenu()

    QtWidgets.QApplication.processEvents()

def _getRendererAppendedImageName(appController, filename):
    rendererName = appController._stageView.rendererDisplayName.lower()
    imageName = filename + "_" + rendererName + ".png"
    return imageName

# Test with no masking.
def _testNoMask(appController):
    _setCameraMaskModeAction(appController, appController._ui.actionCameraMask_None)
    imgName = _getRendererAppendedImageName(appController, "none")
    # We are using a progressive renderer, so wait for images to converge
    appController._takeShot(imgName, waitForConvergence=True)

# Test with outline enabled but no masking.
def _testOutline(appController):
    _setCameraMaskModeAction(appController, appController._ui.actionCameraMask_None, outline=True)
    imgName = _getRendererAppendedImageName(appController, "outline")
    # We are using a progressive renderer, so wait for images to converge
    appController._takeShot(imgName, waitForConvergence=True)

# Test with partial masking.
def _testPartial(appController):
    _setCameraMaskModeAction(appController, appController._ui.actionCameraMask_Partial)
    imgName = _getRendererAppendedImageName(appController, "partial")
    # We are using a progressive renderer, so wait for images to converge
    appController._takeShot(imgName, waitForConvergence=True)

# Test with full masking.
def _testFull(appController):
    _setCameraMaskModeAction(appController, appController._ui.actionCameraMask_Full)
    imgName = _getRendererAppendedImageName(appController, "full")
    # We are using a progressive renderer, so wait for images to converge
    appController._takeShot(imgName, waitForConvergence=True)

# Test that the complexity setting works properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testNoMask(appController)
    _testOutline(appController)
    _testPartial(appController)
    _testFull(appController)
