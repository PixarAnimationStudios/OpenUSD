#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Usdviewq.common import Usd, UsdGeom
from pxr import Vt, Gf

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

def _fixInfPoint(appController):
    stage = appController._dataModel.stage
    mesh = UsdGeom.Mesh(stage.GetPrimAtPath("/PentagonWithInfPoint"))
    points = mesh.GetPointsAttr().Get()
    points[2] = Gf.Vec3f(0.5, 0, 0.5)
    mesh.GetPointsAttr().Set(points)

    appController._stageView.updateView(resetCam=True, forceComputeBBox=True)
    appController._takeShot("fixedGeom.png")

# Test rendering of a scene wherein an infinite point (in all dimensions) is
# authored on a prim, but extents aren't.
# The freecam's frustum and transform is set based on the stage bounds, which
# would be infinite in this case, resulting in a xform that can't be
# orthonormalized. This test validates handling of such a case, and "fixes"
# the infinite point.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    appController._takeShot("infGeom.png")
    _fixInfPoint(appController)
