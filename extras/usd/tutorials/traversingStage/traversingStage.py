#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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
treeIter = Usd.TreeIterator.PreAndPostVisit(stage.GetPseudoRoot())
    
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
