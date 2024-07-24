#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import UsdShade, Gf

def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

def _updatePointInstancerVariant(appController, newVariant):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    pointInstancer = stage.GetPrimAtPath('/PointInstancer')
    variantSet = pointInstancer.GetVariantSet('InstanceVariation')
    variantSet.SetVariantSelection(newVariant)

def testUsdviewInputFunction(appController):
    _modifySettings(appController)

    _updatePointInstancerVariant(appController, 'B')
    appController._takeShot("variantB.png")

    _updatePointInstancerVariant(appController, 'A')
    appController._takeShot("variantA.png")
