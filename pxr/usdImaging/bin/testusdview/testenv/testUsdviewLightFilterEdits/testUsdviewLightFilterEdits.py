#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Usdviewq.common import Sdf, Usd, UsdShade

imageIdx = 0

def _capture(appController):
    global imageIdx
    filename = f"lightFilterEdits_{imageIdx}.png"
    print(f"Capturing {filename}")
    appController._takeShot(filename, waitForConvergence=True)
    imageIdx = imageIdx + 1

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

def _getLightFiltersRel(light):
    return light.GetRelationship("light:filters")

def _testLightFilterEdits(appController):
    stage = appController._dataModel.stage

    light = stage.GetPrimAtPath("/light")
    filtersRel = _getLightFiltersRel(light)
    assert(filtersRel)
    assert(len(filtersRel.GetTargets()) == 0) # initially no filters
    _capture(appController) # _0

    filters = stage.GetPrimAtPath("/Filters").GetChildren()
    assert(len(filters) == 3)

    for f in filters:
        print(f"Connecting filter {f.GetPath()} to light")
        # In addition to taking effect, this should update dependencies such
        # that edits on the filter invalidate the light.
        filtersRel.AddTarget(f.GetPath())
        _capture(appController) # _1,_3,_5

        # Edit an attribute on the filter. Validate that this takes effect.
        # Resulting imange should show a smaller radius than before.
        radiusAttr = f.GetAttribute("inputs:ri:lightFilter:radius")
        curRadius = radiusAttr.Get()
        newRadius = curRadius * 0.5
        print(f"Editing filter {f.GetPath()} radius from {curRadius} to {newRadius}")
        radiusAttr.Set(newRadius)
        _capture(appController) # _2,_4,_6
    
    # Remove the first filter.
    filtersRel.RemoveTarget(filters[0].GetPath())
    _capture(appController) # _7

def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testLightFilterEdits(appController)
