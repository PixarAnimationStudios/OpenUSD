#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

def _modifySettings(appController):
    import os
    cwd = os.getcwd()
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.cullBackfaces = True

# Set the background color and refresh the view.
def _setBackgroundColorAction(appController, action):
    action.setChecked(True)
    appController._changeBgColor(action)
    appController._stageView.updateGL()

# Test with a dark grey background color.
def _testGreyDarkBackground(appController):
    _setBackgroundColorAction(appController, appController._ui.actionGrey_Dark)
    appController._takeShot("additive.png")

# Test Additive Transparency in UsdView
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testGreyDarkBackground(appController)
