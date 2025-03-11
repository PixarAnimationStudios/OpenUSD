#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import UsdShade
from pxr import Sdf

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

def _addMaterial(s):
    # Make a Material that contains a UsdPreviewSurface
    material = UsdShade.Material.Define(s, '/Scene/Looks/NewMaterial')

    # Create the surface.
    pbrShader = UsdShade.Shader.Define(s, '/Scene/Looks/NewMaterial/PbrPreview')
    pbrShader.CreateIdAttr("UsdPreviewSurface")
    pbrShader.CreateInput("roughness", Sdf.ValueTypeNames.Float).Set(0.0)
    pbrShader.CreateInput("metallic", Sdf.ValueTypeNames.Float).Set(0.0)
    pbrShader.CreateInput("diffuseColor", Sdf.ValueTypeNames.Color3f).Set((0.0, 0.0, 1.0))
    material.CreateSurfaceOutput().ConnectToSource(pbrShader.ConnectableAPI(),
            "surface")

    # Now bind the Material to the card
    mesh = s.GetPrimAtPath('/Scene/Geom/Plane')
    UsdShade.MaterialBindingAPI.Apply(mesh).Bind(material)

# Test material bindings edits.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    s = appController._dataModel.stage

    appController._takeShot("0.png", waitForConvergence=True)
    
    _addMaterial(s)
    # Wait for usdview to catch up with changes, and since we are not interested
    # in the final image at this point, we are fine not waiting for convergence
    appController._takeShot("1.png", waitForConvergence=True)
