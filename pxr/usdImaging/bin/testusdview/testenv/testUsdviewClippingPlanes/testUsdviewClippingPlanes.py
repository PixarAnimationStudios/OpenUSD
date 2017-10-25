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

from pxr import Gf

# Remove any unwanted visuals from the view.
def _modifySettings(mainWindow):
    mainWindow.showBBoxes = False
    mainWindow.showHUD = False

# Set the clipping plane overrides and update the view.
def _setClippingPlaneOverrides(mainWindow, near=None, far=None):
    mainWindow._stageView.overrideNear = near
    mainWindow._stageView.overrideFar = far
    mainWindow._stageView.updateGL()

# Take a shot of the viewport and save it to a file.
def _takeShot(mainWindow, fileName):
    viewportShot = mainWindow.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test with no overrides (calculated clipping planes are used).
def _testNoOverride(mainWindow):
    _setClippingPlaneOverrides(mainWindow)
    _takeShot(mainWindow, "no_override.png")

# Test with a near override.
def _testOverrideNear(mainWindow):
    _setClippingPlaneOverrides(mainWindow, near=2.8)
    _takeShot(mainWindow, "override_near.png")

# Test with a far override.
def _testOverrideFar(mainWindow):
    _setClippingPlaneOverrides(mainWindow, far=4.8)
    _takeShot(mainWindow, "override_far.png")

# Test with overrides for both near and far clipping planes.
def _testOverrideBoth(mainWindow):
    _setClippingPlaneOverrides(mainWindow, near=2.8, far=4.8)
    _takeShot(mainWindow, "override_both.png")

# Move the camera forward so the front sphere is clipped, then recompute clipping.
def _testRecomputeClipping(mainWindow):
    _setClippingPlaneOverrides(mainWindow)

    mainWindow.freeCamera.dist = 2.7
    mainWindow._stageView.updateGL()

    _takeShot(mainWindow, "before_recompute.png")

    mainWindow._stageView.computeAndSetClosestDistance()
    mainWindow._stageView.updateGL()

    _takeShot(mainWindow, "after_recompute.png")

# Test that clipping plane settings work properly in usdview.
def testUsdviewInputFunction(mainWindow):
    _modifySettings(mainWindow)
    _testNoOverride(mainWindow)
    _testOverrideNear(mainWindow)
    _testOverrideFar(mainWindow)
    _testOverrideBoth(mainWindow)
    _testRecomputeClipping(mainWindow)
