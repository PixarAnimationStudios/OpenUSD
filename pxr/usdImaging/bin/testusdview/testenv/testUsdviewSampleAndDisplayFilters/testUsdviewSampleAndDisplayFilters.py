#!/pxrpythonsubst
#
# Copyright 2022 Pixar
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
