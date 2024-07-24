#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Usd, UsdGeom

# section 1
stage = Usd.Stage.Open("RefExample.usda")
assert([x for x in stage.Traverse()] == [stage.GetPrimAtPath("/refSphere"), 
    stage.GetPrimAtPath("/refSphere/world"), stage.GetPrimAtPath("/refSphere2"), 
    stage.GetPrimAtPath("/refSphere2/world")])

# section 2
assert([x for x in stage.Traverse() if UsdGeom.Sphere(x)] == 
        [stage.GetPrimAtPath("/refSphere/world"), 
         stage.GetPrimAtPath("/refSphere2/world")])

# section 3
treeIter = iter(Usd.PrimRange.PreAndPostVisit(stage.GetPseudoRoot()))
    
treeIterExpectedResults = [(stage.GetPrimAtPath("/"), False),
        (stage.GetPrimAtPath("/refSphere"), False),
        (stage.GetPrimAtPath("/refSphere/world"), False),
        (stage.GetPrimAtPath("/refSphere/world"), True),
        (stage.GetPrimAtPath("/refSphere"), True),
        (stage.GetPrimAtPath("/refSphere2"), False),
        (stage.GetPrimAtPath("/refSphere2/world"), False),
        (stage.GetPrimAtPath("/refSphere2/world"), True),
        (stage.GetPrimAtPath("/refSphere2"), True),
        (stage.GetPrimAtPath("/"), True)]

treeIterActualResults = [(x, treeIter.IsPostVisit()) for x in treeIter]
assert treeIterExpectedResults == treeIterActualResults

# section 4
# Deactivate the prim as we would in usdview
ref2Prim = stage.GetPrimAtPath('/refSphere2')
stage.SetEditTarget(stage.GetSessionLayer())
Usd.Prim.SetActive(ref2Prim, False)

assert ([x for x in stage.Traverse()] == [stage.GetPrimAtPath("/refSphere"), 
    stage.GetPrimAtPath("/refSphere/world")])
