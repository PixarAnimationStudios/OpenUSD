#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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
