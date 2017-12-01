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
def _modifySettings(appController):
    appController.showBBoxes = False
    appController.showHUD = False

# Set the clipping plane overrides and update the view.
def _setClippingPlaneOverrides(appController, near=None, far=None):
    appController._stageView.overrideNear = near
    appController._stageView.overrideFar = far
    appController._stageView.updateGL()

# Take a shot of the viewport and save it to a file.
def _takeShot(appController, fileName):
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test with no overrides (calculated clipping planes are used).
def _testNoOverride(appController):
    _setClippingPlaneOverrides(appController)
    _takeShot(appController, "no_override.png")

# Test with a near override.
def _testOverrideNear(appController):
    _setClippingPlaneOverrides(appController, near=2.8)
    _takeShot(appController, "override_near.png")

# Test with a far override.
def _testOverrideFar(appController):
    _setClippingPlaneOverrides(appController, far=4.8)
    _takeShot(appController, "override_far.png")

# Test with overrides for both near and far clipping planes.
def _testOverrideBoth(appController):
    _setClippingPlaneOverrides(appController, near=2.8, far=4.8)
    _takeShot(appController, "override_both.png")

# Move the camera forward so the front sphere is clipped, then recompute clipping.
def _testRecomputeClipping(appController):
    _setClippingPlaneOverrides(appController)

    appController.freeCamera.dist = 2.7
    appController._stageView.updateGL()

    _takeShot(appController, "before_recompute.png")

    appController._stageView.computeAndSetClosestDistance()
    appController._stageView.updateGL()

    _takeShot(appController, "after_recompute.png")

# Test that clipping plane settings work properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testNoOverride(appController)
    _testOverrideNear(appController)
    _testOverrideFar(appController)
    _testOverrideBoth(appController)
    _testRecomputeClipping(appController)
