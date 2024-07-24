#!/pxrpythonsubst
#
# Copyright 2023 Pixar
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
