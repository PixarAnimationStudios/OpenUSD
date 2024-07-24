#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import print_function

from pxr.Usdviewq.common import Usd, UsdGeom
from pxr import Vt, Gf, Sdf

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

def _createPoints(stage):
    # define the points Prim on the stage
    points = UsdGeom.Points.Define(stage, Sdf.Path("/Scene/points"))

    positions = []
    for x in range(-5, 5):
        positions.append(Gf.Vec3f(x, 0, 10))

    points.CreatePointsAttr().Set(Vt.Vec3fArray(positions))
    return points

def _widths(points, widthSize):
    return [ widthSize * (1.0 + i / 10.0)
             for i in range(0, len(points.GetPointsAttr().Get())) ]

def _createWidths(points, widthSize):
    # initialize the widths attribute to a specific value
    attr = points.CreateWidthsAttr()
    attr.Set(_widths(points, widthSize))

def _createWidthsPrimvar(points, widthSize):
    api = UsdGeom.PrimvarsAPI(points)
    attr = api.CreatePrimvar(UsdGeom.Tokens.widths,
                             Sdf.ValueTypeNames.FloatArray,
                             UsdGeom.Tokens.vertex)
    attr.Set(_widths(points, widthSize))

def _removeWidthsPrimvar(points):
    api = UsdGeom.PrimvarsAPI(points)
    api.RemovePrimvar(UsdGeom.Tokens.widths)

def _testPointsWidthsAttr(appController):
    # create the initial set of points that we will be changing the widths on
    points = _createPoints(appController._dataModel.stage)

    # take a screen shot of the points with their default widths
    appController._takeShot("start.png")

    # now author the widths attribute to a non-default width size
    _createWidths(points, 0.25)
    appController._takeShot("set_point_widths_025.png")

    # change widths to a different value
    _createWidths(points, 0.5)
    appController._takeShot("set_point_widths_050.png")

    # now author the widths primvar to override the widths attr
    _createWidthsPrimvar(points, 0.75)
    appController._takeShot("set_point_widths_primvar_075.png")

    # remove the widths primvar to revert back to the widths attr
    _removeWidthsPrimvar(points)
    appController._takeShot("end.png")

# This test adds and removes primvars that are used/unused by the material.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testPointsWidthsAttr(appController)
