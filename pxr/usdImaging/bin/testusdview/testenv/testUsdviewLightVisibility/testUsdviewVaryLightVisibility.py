#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#


from __future__ import print_function
import sys

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Turn off the Camera and Dome light.
def _turnLightsOff(appController):
    appController._ui.actionAmbient_Only.setChecked(False)
    appController._ambientOnlyClicked(False)

    appController._ui.actionDomeLight.setChecked(False)
    appController._onDomeLightClicked(False)

    appController._stageView.updateGL()

# Test light visibility varying over time.
def _testVaryingVisibility(appController):

    appController.setFrame(0)
    appController._takeShot("visible.png")
    appController.setFrame(5)
    appController._takeShot("invisible.png")


# Test that the complexity setting works properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _turnLightsOff(appController)

    _testVaryingVisibility(appController)
