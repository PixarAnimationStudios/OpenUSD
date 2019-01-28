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

def _modifySettings(appController):
    import os
    cwd = os.getcwd()
    os.environ["OCIO"] = cwd + "/test.ocio"
    appController._dataModel.viewSettings.colorCorrectionMode = "openColorIO"
    appController._dataModel.viewSettings.showHUD = False

# Set the background color and refresh the view.
def _setBackgroundColorAction(appController, action):
    action.setChecked(True)
    appController._changeBgColor(action)
    appController._stageView.updateGL()

# Take a shot of the viewport and save it to a file.
def _takeShot(appController, fileName):
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test with a dark grey background color.
def _testGreyDarkBackground(appController):
    _setBackgroundColorAction(appController, appController._ui.actionGrey_Dark)
    _takeShot(appController, "ocio_grey_dark.png")

# Test OpenColorIO in UsdView
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testGreyDarkBackground(appController)
