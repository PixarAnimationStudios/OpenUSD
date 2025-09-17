#!/pxrpythonsubst
#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import UsdShade

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

def _updateDisplayFilterTarget(filterPaths, appController):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    renderSettings = stage.GetPrimAtPath('/Render/RenderSettings')
    displayFilterRel = renderSettings.GetRelationship('ri:displayFilters')
    displayFilterRel.SetTargets(filterPaths)

def _updateSampleFilterTarget(filterPaths, appController):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    renderSettings = stage.GetPrimAtPath('/Render/RenderSettings')
    sampleFilterRel = renderSettings.GetRelationship('ri:sampleFilters')
    sampleFilterRel.SetTargets(filterPaths)

def testUsdviewInputFunction(appController):
    _modifySettings(appController)

    appController._takeShot("both.png", waitForConvergence=True)

    _updateDisplayFilterTarget([], appController)
    appController._takeShot("sampleOnly.png", waitForConvergence=True)

    _updateSampleFilterTarget([], appController)
    _updateDisplayFilterTarget(['/Render/DisplayFilter'], appController)
    appController._takeShot("displayOnly.png", waitForConvergence=True)
