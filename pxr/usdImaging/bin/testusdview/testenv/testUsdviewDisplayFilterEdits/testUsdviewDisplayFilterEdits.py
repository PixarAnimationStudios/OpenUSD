#!/pxrpythonsubst
#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import UsdShade, Gf

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

# Update the connected Display Filter.
def _updateDisplayFilterConnection(filterPaths, appController):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    renderSettings = stage.GetPrimAtPath('/Render/RenderSettings')
    displayFilterAttr = renderSettings.GetAttribute('outputs:ri:displayFilters')
    displayFilterAttr.SetConnections(filterPaths)

def _updateDisplayFilterParam(filterPath, attrName, attrValue, appController):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    displayFilter = stage.GetPrimAtPath(filterPath)
    displayFilterParam = displayFilter.GetAttribute(attrName)
    displayFilterParam.Set(attrValue)


# Test changing the connected DisplayFilter.
def testUsdviewInputFunction(appController):
    from os import cpu_count
    print('******CPU COUNT: {}'.format(cpu_count()))
    _modifySettings(appController)

    filter1out = '/Render/DisplayFilter1.outputs:result'
    filter2out = '/Render/DisplayFilter2.outputs:result'
    filter2 = '/Render/DisplayFilter2'
    bgColorAttrName = "inputs:ri:backgroundColor"

    appController._takeShot("firstFilter.png", waitForConvergence=True)
    
    _updateDisplayFilterConnection([filter2out], appController)
    appController._takeShot("secondFilter.png", waitForConvergence=True)

    _updateDisplayFilterParam(filter2, bgColorAttrName, Gf.Vec3f(0, 1, 1), appController)
    appController._takeShot("secondFilter_modified.png", waitForConvergence=True)

    _updateDisplayFilterParam(filter2, bgColorAttrName, Gf.Vec3f(1, 0, 1), appController)
    _updateDisplayFilterConnection([filter1out, filter2out], appController)
    appController._takeShot("multiFilters1.png", waitForConvergence=True)

    _updateDisplayFilterConnection([filter2out, filter1out], appController)
    appController._takeShot("multiFilters2.png", waitForConvergence=True)
