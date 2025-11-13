#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function
from pxr.Usdviewq.common import Sdf, Usd
from pxr import Gf, UsdGeom, UsdVol

imageIdx = 0

def _capture(appController):
    global imageIdx
    filename = f"volumeEdits_{imageIdx}.png"
    print(f"Capturing {filename}")
    appController._takeShot(filename, waitForConvergence=True)
    imageIdx = imageIdx + 1

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

def _testVolumeAndFieldEdits(appController):
    stage = appController._dataModel.stage

    volume = UsdVol.Volume(stage.GetPrimAtPath("/Volume"))
    assert(volume)

    # Volume prim has a field:density relationship. Get the targeted field prim.
    assert(volume.HasFieldRelationship("density"))
    fieldPath = volume.GetFieldPath("density")
    print(f"Volume {volume.GetPath()} has field:density relationship to {fieldPath}")
    field = UsdVol.OpenVDBAsset(stage.GetPrimAtPath(fieldPath))
    assert(field)
    _capture(appController) # _0

    # Change the filepath of the field asset.
    filePathAttr = field.GetFilePathAttr()
    assert(filePathAttr)
    curAssetPath = filePathAttr.Get()
    newFilePath = "smoke_2.vdb"
    print(f"Updating file path of field {fieldPath} from {curAssetPath} to {newFilePath}")
    filePathAttr.Set(Sdf.AssetPath(authoredPath=newFilePath))
    _capture(appController) # _1

    # Also modify the authored display color to verify that updates to the 
    # volume prim are reflected.
    pvDisplayColor = UsdGeom.PrimvarsAPI(volume.GetPrim()).GetPrimvar(UsdGeom.Tokens.primvarsDisplayColor)
    assert(pvDisplayColor)
    print(f"Updating displayColor of volume {volume.GetPath()} to yellow")
    pvDisplayColor.Set([Gf.Vec3f(1.0, 1.0, 0.0)])
    _capture(appController) # _2

    # Revert the filepath change.
    print(f"Reverting file path of field {fieldPath} to {curAssetPath}")
    filePathAttr.Set(curAssetPath)
    _capture(appController) # _3

    # Change the field wired to the volume.
    newFieldPath = "/Volume/Smoke_2"
    print(f"Updating volume {volume.GetPath()} field:density relationship from {fieldPath} to {newFieldPath}")
    volume.CreateFieldRelationship("density", newFieldPath)
    _capture(appController) # _4

    # Change the display color again.
    print(f"Updating displayColor of volume {volume.GetPath()} to magenta")
    pvDisplayColor.Set([Gf.Vec3f(1.0, 0.0, 1.0)])
    _capture(appController) # _5

def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testVolumeAndFieldEdits(appController)
