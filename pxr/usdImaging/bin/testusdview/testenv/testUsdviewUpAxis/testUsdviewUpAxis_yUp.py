#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True
    appController._stageView.updateGL()

# Test that setting the upAxis to "Y" properly changes the view.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    appController._takeShot("yUp.png")
