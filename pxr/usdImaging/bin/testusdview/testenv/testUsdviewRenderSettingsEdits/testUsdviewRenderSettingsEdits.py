#!/pxrpythonsubst
#
# Copyright 2023 Pixar
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

# Enable/Disable the Depth of Field attribute on the given RenderProduct
def _updateAttribute(appController, productPath, attrName, attrValue):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    product = stage.GetPrimAtPath(productPath)
    attr = product.GetAttribute(attrName)
    attr.Set(attrValue)


# Test changing the connected SampleFilter.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)

    productPath = '/Render/Product'

    appController._takeShot("DofEnabled.png", waitForConvergence=True)

    # Disable Depth of Field attribute
    _updateAttribute(appController, productPath, 'disableDepthOfField', True)
    appController._takeShot("DofDisabled.png", waitForConvergence=True)
