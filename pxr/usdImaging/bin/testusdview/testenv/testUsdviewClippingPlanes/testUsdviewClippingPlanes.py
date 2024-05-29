#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Gf

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

# Set the clipping plane overrides and update the view.
def _setClippingPlaneOverrides(appController, near=None, far=None):
    appController._dataModel.viewSettings.freeCameraOverrideNear = near
    appController._dataModel.viewSettings.freeCameraOverrideFar = far
    appController._stageView.updateGL()

# Test with no overrides (calculated clipping planes are used).
def _testNoOverride(appController):
    _setClippingPlaneOverrides(appController)
    appController._takeShot("no_override.png")

# Test with a near override.
def _testOverrideNear(appController):
    _setClippingPlaneOverrides(appController, near=2.8)
    appController._takeShot("override_near.png")

# Test with a far override.
def _testOverrideFar(appController):
    _setClippingPlaneOverrides(appController, far=4.8)
    appController._takeShot("override_far.png")

# Test with overrides for both near and far clipping planes.
def _testOverrideBoth(appController):
    _setClippingPlaneOverrides(appController, near=2.8, far=4.8)
    appController._takeShot("override_both.png")

# Move the camera forward so the front sphere is clipped, then recompute clipping.
def _testRecomputeClipping(appController):
    _setClippingPlaneOverrides(appController)

    appController._dataModel.viewSettings.freeCamera.dist = 2.7
    appController._stageView.updateGL()

    appController._takeShot("before_recompute.png")

    appController._stageView.computeAndSetClosestDistance()
    appController._stageView.updateGL()

    appController._takeShot("after_recompute.png")

# Test that clipping plane settings work properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testNoOverride(appController)
    _testOverrideNear(appController)
    _testOverrideFar(appController)
    _testOverrideBoth(appController)
    _testRecomputeClipping(appController)
