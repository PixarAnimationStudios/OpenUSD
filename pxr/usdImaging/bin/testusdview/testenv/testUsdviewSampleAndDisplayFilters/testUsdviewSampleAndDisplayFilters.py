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

def _updateDisplayFilterConnection(filterPaths, appController):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    renderSettings = stage.GetPrimAtPath('/Render/RenderSettings')
    displayFilterAttr = renderSettings.GetAttribute('outputs:ri:displayFilters')
    displayFilterAttr.SetConnections(filterPaths)

def _updateSampleFilterConnection(filterPaths, appController):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    renderSettings = stage.GetPrimAtPath('/Render/RenderSettings')
    sampleFilterAttr = renderSettings.GetAttribute('outputs:ri:sampleFilters')
    sampleFilterAttr.SetConnections(filterPaths)

def testUsdviewInputFunction(appController):
    _modifySettings(appController)

    appController._takeShot("both.png", waitForConvergence=True)

    _updateDisplayFilterConnection([], appController)
    appController._takeShot("sampleOnly.png", waitForConvergence=True)

    _updateSampleFilterConnection([], appController)
    _updateDisplayFilterConnection(['/Render/DisplayFilter.outputs:result'], appController)
    appController._takeShot("displayOnly.png", waitForConvergence=True)
