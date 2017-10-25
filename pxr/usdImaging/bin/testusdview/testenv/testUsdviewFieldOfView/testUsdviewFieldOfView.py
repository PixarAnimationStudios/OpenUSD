#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

# Remove any unwanted visuals from the view.
def _modifySettings(mainWindow):
    mainWindow.showBBoxes = False
    mainWindow.showHUD = False

# Set the field of view and refresh the view.
def _setFOV(mainWindow, fov):
    mainWindow.freeCamera.fov = fov
    mainWindow._stageView.updateGL()

# Take a shot of the viewport and save it to a file.
def _takeShot(mainWindow, fileName):
    viewportShot = mainWindow.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test 45 degree FOV.
def _test45Degree(mainWindow):
    _setFOV(mainWindow, 45)
    _takeShot(mainWindow, "fov45.png")

# Test 60 degree FOV.
def _test60Degree(mainWindow):
    _setFOV(mainWindow, 60)
    _takeShot(mainWindow, "fov60.png")

# Test 90 degree FOV.
def _test90Degree(mainWindow):
    _setFOV(mainWindow, 90)
    _takeShot(mainWindow, "fov90.png")

# Test that the FOV setting work properly in usdview.
def testUsdviewInputFunction(mainWindow):
    _modifySettings(mainWindow)
    _test45Degree(mainWindow)
    _test60Degree(mainWindow)
    _test90Degree(mainWindow)
