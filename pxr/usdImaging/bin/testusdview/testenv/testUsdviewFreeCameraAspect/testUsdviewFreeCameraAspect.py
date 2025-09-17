#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

def _test185Aspect(appController):
    appController._dataModel.viewSettings.lockFreeCameraAspect = True
    appController._dataModel.viewSettings.freeCamera.aspectRatio = 1.85
    appController._stageView.updateGL()
    appController._takeShot("aspect185.png", cropToAspectRatio=True)

def _test239Aspect(appController):
    appController._dataModel.viewSettings.lockFreeCameraAspect = True
    appController._dataModel.viewSettings.freeCamera.aspectRatio = 2.39
    appController._stageView.updateGL()
    appController._takeShot("aspect239.png", cropToAspectRatio=True)

def _test05Aspect(appController):
    appController._dataModel.viewSettings.lockFreeCameraAspect = True
    appController._dataModel.viewSettings.freeCamera.aspectRatio = 0.5
    appController._stageView.updateGL()
    appController._takeShot("aspect05.png", cropToAspectRatio=True)


# Test that the FOV setting work properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _test185Aspect(appController)
    _test239Aspect(appController)
    _test05Aspect(appController)
