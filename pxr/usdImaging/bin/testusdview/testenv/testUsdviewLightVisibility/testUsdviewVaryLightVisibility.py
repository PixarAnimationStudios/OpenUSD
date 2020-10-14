#!/pxrpythonsubst
#
# Copyright 2020 Pixar
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
