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

# Update the connected Sample Filter.
def _updateSampleFilterConnection(filterPaths, appController):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    renderSettings = stage.GetPrimAtPath('/Render/RenderSettings')
    sampleFilterAttr = renderSettings.GetAttribute('outputs:ri:sampleFilters')
    sampleFilterAttr.SetConnections(filterPaths)

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

    filter1out = '/Render/MurkFilter1.outputs:result'
    filter2out = '/Render/MurkFilter2.outputs:result'
    filter2 = '/Render/MurkFilter2'
    conFarDistAttrName = "inputs:ri:conFarDist"

    appController._takeShot("firstFilter.png", waitForConvergence=True)

    _updateSampleFilterConnection([filter2out], appController)
    appController._takeShot("secondFilter.png", waitForConvergence=True)

    _updateSampleFilterParam(filter2, conFarDistAttrName, 1, appController)
    appController._takeShot("secondFilter_modified.png", waitForConvergence=True)

    _updateSampleFilterParam(filter2, conFarDistAttrName, 50, appController)
    _updateSampleFilterConnection([filter1out, filter2out], appController)
    appController._takeShot("multiFilters1.png", waitForConvergence=True)

    _updateSampleFilterConnection([filter2out, filter1out], appController)
    appController._takeShot("multiFilters2.png", waitForConvergence=True)
