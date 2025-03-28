#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Set the field of view and refresh the view.
def _setFOV(appController, fov):
    appController._dataModel.viewSettings.freeCamera.fov = fov
    appController._stageView.updateGL()

# Test 45 degree FOV.
def _test45Degree(appController):
    _setFOV(appController, 45)
    appController._takeShot("fov45.png")

# Test 60 degree FOV.
def _test60Degree(appController):
    _setFOV(appController, 60)
    appController._takeShot("fov60.png")

# Test 90 degree FOV.
def _test90Degree(appController):
    _setFOV(appController, 90)
    appController._takeShot("fov90.png")

# Test that the FOV setting work properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _test45Degree(appController)
    _test60Degree(appController)
    _test90Degree(appController)
