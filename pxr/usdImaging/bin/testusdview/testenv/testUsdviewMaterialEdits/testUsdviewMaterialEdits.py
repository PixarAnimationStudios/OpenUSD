#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

# Update mesh binding.
def _updateMeshBinding(path, appController):
    s = appController._dataModel.stage
    l = s.GetSessionLayer()
    s.SetEditTarget(l)

    mesh = s.GetPrimAtPath('/Scene/Geom/Plane')
    material = UsdShade.Material(s.GetPrimAtPath(path))
    UsdShade.MaterialBindingAPI.Apply(mesh).Bind(material)

# Test material bindings edits.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _updateMeshBinding('/Scene/Looks/MainMaterial_1', appController)
    appController._takeShot("1.png")
    _updateMeshBinding('/Scene/Looks/MainMaterial_2', appController)
    appController._takeShot("2.png")
    _updateMeshBinding('/Scene/Looks/MainMaterial_1', appController)
    appController._takeShot("3.png")
