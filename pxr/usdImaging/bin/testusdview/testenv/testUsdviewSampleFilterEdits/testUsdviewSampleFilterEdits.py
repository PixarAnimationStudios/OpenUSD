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
