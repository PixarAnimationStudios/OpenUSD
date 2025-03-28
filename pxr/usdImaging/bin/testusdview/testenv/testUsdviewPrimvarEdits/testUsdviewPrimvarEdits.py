#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import print_function

from pxr.UsdAppUtils.complexityArgs import RefinementComplexities
from pxr.Usdviewq.common import RenderModes, Usd, UsdGeom
from pxr import Vt, Gf, Sdf

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

# Set the complexity and refresh the view.
def _setComplexity(appController, complexity):
    appController._dataModel.viewSettings.complexity = complexity
    appController._stageView.updateView()

def _setRenderMode(appController, renderMode):
    appController._dataModel.viewSettings.renderMode = renderMode
    appController._stageView.updateView()

def _CreatePrimvar(prim, primvarName, primvarType, interpToken, value):
    pv = UsdGeom.PrimvarsAPI(prim).CreatePrimvar(
        primvarName, primvarType, interpToken)
    if not pv:
        print("Failed to create primvar", str(primvarName))
        return
    pv.Set(value)

def _RemovePrimvar(prim, primvarName):
    # XXX: The UsdGeom API doesn't have a DestroyPrimvar method yet.
    prim.RemoveProperty('primvars:' + str(primvarName))

def _testAddRemovePrimvarUsedByMaterial(appController):
    _setRenderMode(appController, RenderModes.SMOOTH_SHADED)
    appController._takeShot("start.png")
    stage = appController._dataModel.stage
    prim = stage.GetPrimAtPath("/Scene/Geom/Plane")
    mesh = UsdGeom.Imageable(prim)
    if not mesh:
        print("Prim", str(prim.GetPath()), "is not imageable.")
        return

    # The preview surface material has a primvar reader node with the
    # varname 'myColor' that has a fallback value of blue.
    # XXX Adding the primvar results in a resync currently in usdImaging, due to
    # limitations in Hydra Storm. This will change in the near future.
    
    # Adding the primvar expected by the material should result in the authored
    # value (green) being used instead of the fallback (blue)
    _CreatePrimvar(mesh, 'myColor', Sdf.ValueTypeNames.Float3, 'constant',
        Gf.Vec3f(0.0, 1.0, 0.0))       
    appController._takeShot("add_fallback_primvar_smooth.png")

    # Change the display mode to ensure repr switching in Storm picks up the
    # primvar
    _setRenderMode(appController, RenderModes.FLAT_SHADED)
    appController._takeShot("add_fallback_primvar_flat.png")

    # XXX Removing the primvar results in a resync in usdImaging, due to
    # limitations in Hydra Storm. This will change in the near future.
    # Removing the primvar should result in the fallback value being used (blue)
    _RemovePrimvar(prim, 'myColor')
    appController._takeShot("remove_fallback_primvar_flat.png")

    _setRenderMode(appController, RenderModes.WIREFRAME_ON_SURFACE)
    appController._takeShot("remove_fallback_primvar_wireOnSurf.png")

def _testAddRemovePrimvarNotUsedByMaterial(appController):
    _setRenderMode(appController, RenderModes.SMOOTH_SHADED)

    stage = appController._dataModel.stage
    prim = stage.GetPrimAtPath("/Scene/Geom/Plane")
    mesh = UsdGeom.Imageable(prim)
    if not mesh:
        print("Prim", str(prim.GetPath()), "is not imageable.")
        return

    # XXX Adding the primvar results in a resync currently in usdImaging, due to
    # limitations in Hydra Storm. This will change in the near future.
    # The primvars below aren't used by the material. If primvar filtering is
    # enabled, they will be ignored and not "make it" to the backend.
    _CreatePrimvar(mesh, 'foo', Sdf.ValueTypeNames.Float, 'constant', 0.1)
    _CreatePrimvar(mesh, 'bar', Sdf.ValueTypeNames.Vector3fArray, 'vertex',
        Vt.Vec3fArray(8))
    appController._takeShot("add_unused_primvars.png")

    _RemovePrimvar(prim, 'foo')
    _RemovePrimvar(prim, 'bar')
    appController._takeShot("remove_unused_primvars.png")

def _testPrimvarReaderVarnameEdit(appController):
    # Test editing the inputs:varname of a primvar reader.
    #
    # In particular, there was a bug in Storm where the other primvar was
    # pruned and thus changing the primvar reader to that other primvar
    # resulted in the primvar not being picked up.
    #
    # https://github.com/PixarAnimationStudios/OpenUSD/issues/2382

    # Setup the stage with two primvars and making the primvar reader
    # consume one of the primvars.
    stage = appController._dataModel.stage
    prim = stage.GetPrimAtPath("/Scene/Geom/Plane")
    mesh = UsdGeom.Imageable(prim)
    _CreatePrimvar(mesh, 'myColorGreen', Sdf.ValueTypeNames.Float3, 'constant',
        Gf.Vec3f(0.0, 1.0, 0.0))
    _CreatePrimvar(mesh, 'myColorBlue', Sdf.ValueTypeNames.Float3, 'constant',
        Gf.Vec3f(0.0, 0.0, 1.0))
    varnameAttr = stage.GetAttributeAtPath("/Scene/Looks/MainMaterial/Primvar.inputs:varname")
    varnameAttr.Set('myColorGreen')
    # Take a snap shot to see whether the setup is correct.
    appController._takeShot('add_primvars_for_varname_edit.png')

    # Now switch to the other primvar.
    varnameAttr.Set('myColorBlue')
    appController._takeShot('primvar_reader_varname_edit.png')

# This test adds and removes primvars that are used/unused by the material.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testAddRemovePrimvarUsedByMaterial(appController)
    _testAddRemovePrimvarNotUsedByMaterial(appController)
    _testPrimvarReaderVarnameEdit(appController)
