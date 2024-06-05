#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# The OCIO env var is set to the test.ocio config in the test directory before
# it is run.

from pxr.Usdviewq.common import ColorCorrectionModes

def _useOCIO(appController):
    appController._dataModel.viewSettings.colorCorrectionMode = ColorCorrectionModes.OPENCOLORIO
    appController._dataModel.viewSettings.showHUD = False
    # The first view ("Gamma 2.2" will be the default view)
    appController._takeShot("colorCorrectionOCIO_g22.png")
    # XXX Add support for testing color spaces and looks.
    appController._dataModel.viewSettings.setOcioSettings(
        colorSpace = None, display = "rec709g22", view = "Linear")
    appController._takeShot("colorCorrectionOCIO_linear.png")
        

def _useFallback(appController):
    appController._dataModel.viewSettings.colorCorrectionMode = ColorCorrectionModes.SRGB
    appController._takeShot("colorCorrectionSRGB.png")

def _disableColorCorrection(appController):
    appController._dataModel.viewSettings.colorCorrectionMode = ColorCorrectionModes.DISABLED
    appController._takeShot("colorCorrectionDisabled.png")

# Set the background color and refresh the view.
def _setBackgroundColorAction(appController, action):
    action.setChecked(True)
    appController._changeBgColor(action)
    appController._stageView.updateGL()

# Test with a dark grey background color.
def _testGreyDarkBackground(appController):
    _setBackgroundColorAction(appController, appController._ui.actionGrey_Dark)

# Test OpenColorIO in UsdView
def testUsdviewInputFunction(appController):
    _testGreyDarkBackground(appController)
    _useOCIO(appController)
    _useFallback(appController)
    _disableColorCorrection(appController)
