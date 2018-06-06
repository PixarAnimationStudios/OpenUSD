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
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#


#! [PopulateAllSkelRoots]
def PopulateAllSkelRoots(stage, cache):
    rng = stage.Traverse()
    for prim in rng:
        if prim.IsA(UsdSkel.Root):
            cache.Populate(UsdSkel.Root(prim))
            # don't need to iterate further down.
            it.PruneChildren()
#! [PopulateAllSkelRoots]


#! [IterSkels]
def IterSkels(skelRootPrim, skelCache):
    for descendant in Usd.PrimRange(skelRootPrim):
        query = skelCache.GetSkelQuery(descendant)
        if query:
            yield (descendant,query)
#! [IterSkels]


#! [PrintSkelsAndSkinnedPrims]
def PrintSkelsAndSkinnedPrims(skelRoot):
    
    for prim in Usd.PrimRange(skelRoot.GetPrim()):
        query = cache.GetSkelQuery(prim)
        if query:
            print query
            print "Skinnned prims:"

            for skinnedPrim,skinningQuery in cache.ComputeSkinnedPrims(prim):
                print "\t" + str(skinnedPrim.GetPath())
#! [PrintSkelsAndSkinnedPrims]


#! [ComputeSkinnedPoints]
def ComputeSkinnedPoints(pointBased, skelQuery, skinningQuery, time):
    # Query the initial points.
    # The initial points will be in local gprim space.
    points = pointBased.GetPointsAttr().Get()
    if not points:
        return False

    # Compute skinning transforms (in skeleton space!).
    xforms = skelQuery.ComputeSkinningTransforms(time)
    
    # Apply skinning.
    if skinningQuery.ComputeSkinnedPoints(xforms, points, time):
        return points
#! [ComputeSkinnedPoints]


#! [ComputeSkinnedTransform]
def ComputeSkinnedTransform(xformable, skelQuery, skinningQuery, time):
    # Must be rigidly deforming to skin a transform.
    if not skinningQuery.IsRigidlyDeforming():
        return

    # Compute skinning transforms (in skeleton space!).
    xforms = skelQuery.ComputeSkinningTransforms(time)

    # Apply skinning.
    return skinningQuery.ComputeSkinnedTransform(xforms, time)
#! [ComputeSkinnedTransform]
