#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Sdf, UsdShade
from pixar.Usdviewq.usdviewApi import UsdviewApi

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

def _getStage(appController):
    stage = appController._dataModel.stage
    stage.SetEditTarget(stage.GetSessionLayer())
    return stage

# Add (or retarget) the named coordinate system binding on the given prim.
def _bindCoordSys(primPath, name, targetPath, appController):
    stage = _getStage(appController)
    prim = stage.GetPrimAtPath(primPath)
    UsdShade.CoordSysAPI.Apply(prim, name).Bind(Sdf.Path(targetPath))

# Remove the named coordinate system binding from the given prim.
def _unbindCoordSys(primPath, name, appController):
    stage = _getStage(appController)
    prim = stage.GetPrimAtPath(primPath)
    UsdShade.CoordSysAPI(prim, name).BlockBinding()

def _setActive(primPath, active, appController):
    stage = _getStage(appController)
    stage.GetPrimAtPath(primPath).SetActive(active)

def _flush(appController):
    # XXX Set this to true to generate baseline images, by starting
    # a fresh render each time instead of incrementally editing the scene.
    renderFromScratch = False
    if renderFromScratch:
        usdviewApi = UsdviewApi(appController)
        usdviewApi.SetViewportRenderer('HdStormRendererPlugin')
        usdviewApi.SetViewportRenderer('HdPrmanLoaderRendererPlugin')

# Test adding and removing coordinate system bindings, and deactivating
# and reactivating the parent prim of the binding targets.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)

    customRow = '/World/CustomRow'
    worldSpaceRow = '/World/WorldSpaceRow'
    coordSystems = '/World/CoordSystems'

    _flush(appController)
    appController._takeShot("0_initial.png", waitForConvergence=True)

    # Remove the Custom binding from CustomRow.
    _unbindCoordSys(customRow, 'Custom', appController)
    _flush(appController)
    appController._takeShot("1_removeBinding.png", waitForConvergence=True)

    # Add the Custom binding back, targeting a different coordinate system.
    _bindCoordSys(customRow, 'Custom', coordSystems + '/Alternate',
                  appController)
    _flush(appController)
    appController._takeShot("2_addBinding.png", waitForConvergence=True)

    # Add a World binding on WorldSpaceRow that overrides the inherited
    # World binding from /World.
    _bindCoordSys(worldSpaceRow, 'World', coordSystems + '/Custom',
                  appController)
    _flush(appController)
    appController._takeShot("3_addOverridingBinding.png",
                            waitForConvergence=True)

    # Deactivate the parent of all binding targets.
    _setActive(coordSystems, False, appController)
    _flush(appController)
    appController._takeShot("4_deactivateTargets.png", waitForConvergence=True)

    # Reactivate the parent of the binding targets; this should match
    # addOverridingBinding.png.
    _setActive(coordSystems, True, appController)
    _flush(appController)
    appController._takeShot("5_reactivateTargets.png", waitForConvergence=True)

    # Remove the overriding World binding; WorldSpaceRow should revert to
    # the inherited World binding.
    _unbindCoordSys(worldSpaceRow, 'World', appController)
    _flush(appController)
    appController._takeShot("6_removeOverridingBinding.png",
                            waitForConvergence=True)

    # Rotate the Custom coordinate system.
    stage = _getStage(appController)
    alternateCoordSys = stage.GetPrimAtPath('/World/CoordSystems/Alternate')
    alternateRotateX = alternateCoordSys.GetAttribute('xformOp:rotateX')
    alternateRotateX.Set(90)
    _flush(appController)
    appController._takeShot("7_rotateCustom.png",
                            waitForConvergence=True)
