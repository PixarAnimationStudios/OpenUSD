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

from pxr.Usdviewq.common import Usd, UsdGeom
from pxr import Vt, Gf

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

# Take a shot of the viewport and save it to a file.
def _takeShot(appController, fileName):
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

def _fixInfPoint(appController):
    stage = appController._dataModel.stage
    mesh = UsdGeom.Mesh(stage.GetPrimAtPath("/PentagonWithInfPoint"))
    points = mesh.GetPointsAttr().Get()
    points[2] = Gf.Vec3f(0.5, 0, 0.5)
    mesh.GetPointsAttr().Set(points)

    appController._stageView.updateView(resetCam=True, forceComputeBBox=True)
    _takeShot(appController, "fixedGeom.png")

# Test rendering of a scene wherein an infinite point (in all dimensions) is
# authored on a prim, but extents aren't.
# The freecam's frustum and transform is set based on the stage bounds, which
# would be infinite in this case, resulting in a xform that can't be
# orthonormalized. This test validates handling of such a case, and "fixes"
# the infinite point.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _takeShot(appController, "infGeom.png")
    _fixInfPoint(appController)
