#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function
from pxr.Usdviewq.common import Sdf, Usd, UsdShade

imageIdx = 0

def _capture(appController):
    global imageIdx
    appController._takeShot(f"materialBindings_{imageIdx}.png")
    imageIdx = imageIdx + 1

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

def _testBindingEdits(appController):
    stage = appController._dataModel.stage

    # Below, we make edits to the first (left most) column of cubes.
    # The collection material binding at /Scene/Column0 has a 
    # "weakerThanDescendants" strength. The direct binding at each of its cube 
    # children wins (being lower in namespace).
    # Toggle the collection binding's strength.
    col0 = stage.GetPrimAtPath("/Scene/Column0")
    mbApi = UsdShade.MaterialBindingAPI(col0)
    cbRels = mbApi.GetCollectionBindingRels()
    assert(len(cbRels) == 1)
    cbRel = cbRels[0]
    assert(mbApi.GetMaterialBindingStrength(cbRel) == UsdShade.Tokens.weakerThanDescendants)
    mbApi.SetMaterialBindingStrength(cbRel, UsdShade.Tokens.strongerThanDescendants)

    _capture(appController) # _1

    mbApi.SetMaterialBindingStrength(cbRel, UsdShade.Tokens.weakerThanDescendants)
    _capture(appController) # _2 should match _0.

    # Make the collection binding stronger again.
    # Edit direct binding on /Scene/Column0/Cube1. This should have no effect
    # since the collection binding is stronger.
    mbApi.SetMaterialBindingStrength(cbRel, UsdShade.Tokens.strongerThanDescendants)
    cube = stage.GetPrimAtPath("/Scene/Column0/Cube1")
    cubeMbApi = UsdShade.MaterialBindingAPI(cube)
    cubeMbApi.GetDirectBindingRel().SetTargets(
        [Sdf.Path("/Materials/Mat1")]) # Green color
    _capture(appController) # _3 should match _1.

    # Edit the prims targeted by the collection used by the collection binding.
    # Note that we don't modify the collection binding here.
    colApi = Usd.CollectionAPI(col0, "cubes")
    assert(colApi)
    assert(colApi.GetIncludesRel().GetTargets() == [Sdf.Path("/Scene/Column0")])
    # Target 2 of the 5 cubes in the column. 
    colApi.GetIncludesRel().SetTargets(
        [Sdf.Path("/Scene/Column0/Cube2"), Sdf.Path("/Scene/Column0/Cube3")])
    _capture(appController) # _4

    # Edit the collection binding strength so that the direct bindings win.
    # Cube1 should now look green from the earlier edit. Rest should be red.
    mbApi.SetMaterialBindingStrength(cbRel, UsdShade.Tokens.weakerThanDescendants)
    _capture(appController) # _5

    # One last time, toggle its strength.
    mbApi.SetMaterialBindingStrength(cbRel, UsdShade.Tokens.strongerThanDescendants)
    _capture(appController) # _6 should match _4.

    # Change material for the collection binding.
    assert(len(cbRel.GetTargets()) == 2)
    cbRel.SetTargets(
        [Sdf.Path('/Scene/Column0.collection:cubes'),
         Sdf.Path('/Materials/Mat6')]) # Yellow
    _capture(appController) # _7

    # Author a path-expression based collection and a collection material
    # binding at /Scene that overrides the material bound to cubes in rows 
    # 0 and 2.
    scene = stage.GetPrimAtPath("/Scene")
    colApi = Usd.CollectionAPI.Apply(scene, "foo")
    colApi.GetMembershipExpressionAttr().Set(
        Sdf.PathExpression("//*0 + //*2")) # All leaf prims ending in 0 or 2.
    mbApi = UsdShade.MaterialBindingAPI.Apply(scene)
    mat = UsdShade.Material(stage.GetPrimAtPath("/Materials/Mat4")) # purple
    mbApi.Bind(colApi, mat, "fooMat", UsdShade.Tokens.strongerThanDescendants)
    _capture(appController) # _8

def testUsdviewInputFunction(appController):
    _modifySettings(appController)

    # Scene consists of cubes in a 5x4 grid. The initial colors of the columns
    # should be: [red, green, blue, yellow]
    _capture(appController) # _0
    _testBindingEdits(appController)
