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

# Update the targeted Sample Filter.
def _updateSampleFilterTargets(filterPaths, appController):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    renderSettings = stage.GetPrimAtPath('/Render/RenderSettings')
    sampleFilterRel = renderSettings.GetRelationship('ri:sampleFilters')
    sampleFilterRel.SetTargets(filterPaths)

def _updateSampleFilterParam(filterPath, attrName, attrValue, appController):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    sampleFilter = stage.GetPrimAtPath(filterPath)
    sampleFilterParam = sampleFilter.GetAttribute(attrName)
    sampleFilterParam.Set(attrValue)


# Test changing the connected SampleFilter.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)

    filter1 = '/Render/MurkFilter1'
    filter2 = '/Render/MurkFilter2'
    conFarDistAttrName = "inputs:ri:conFarDist"

    appController._takeShot("firstFilter.png", waitForConvergence=True)

    _updateSampleFilterTargets([filter2], appController)
    appController._takeShot("secondFilter.png", waitForConvergence=True)

    _updateSampleFilterParam(filter2, conFarDistAttrName, 1, appController)
    appController._takeShot("secondFilter_modified.png", waitForConvergence=True)

    _updateSampleFilterParam(filter2, conFarDistAttrName, 50, appController)
    _updateSampleFilterTargets([filter1, filter2], appController)
    appController._takeShot("multiFilters1.png", waitForConvergence=True)

    _updateSampleFilterTargets([filter2, filter1], appController)
    appController._takeShot("multiFilters2.png", waitForConvergence=True)
