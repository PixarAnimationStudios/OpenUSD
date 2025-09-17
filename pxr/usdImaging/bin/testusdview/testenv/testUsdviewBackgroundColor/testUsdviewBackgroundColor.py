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

# Set the background color and refresh the view.
def _setBackgroundColorAction(appController, action):
    action.setChecked(True)
    appController._changeBgColor(action)
    appController._stageView.updateGL()

# Test with a black background color.
def _testBlackBackground(appController):
    _setBackgroundColorAction(appController, appController._ui.actionBlack)
    appController._takeShot("black.png")

# Test with a dark grey background color.
def _testGreyDarkBackground(appController):
    _setBackgroundColorAction(appController, appController._ui.actionGrey_Dark)
    appController._takeShot("grey_dark.png")

# Test with a light grey background color.
def _testGreyLightBackground(appController):
    _setBackgroundColorAction(appController, appController._ui.actionGrey_Light)
    appController._takeShot("grey_light.png")

# Test with a white background color.
def _testWhiteBackground(appController):
    _setBackgroundColorAction(appController, appController._ui.actionWhite)
    appController._takeShot("white.png")

# Test that the background color setting works properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testBlackBackground(appController)
    _testGreyDarkBackground(appController)
    _testGreyLightBackground(appController)
    _testWhiteBackground(appController)
