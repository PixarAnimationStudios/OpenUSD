//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/animQuery.h"
#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/skinningQuery.h"

PXR_NAMESPACE_OPEN_SCOPE


//! [PopulateAllSkelRoots]
void PopulateAllSkelRoots(const UsdStagePtr& stage,
                          UsdSkelCache* cache)
{
    auto range = stage->Traverse();
    for(auto it = range.begin(); it != range.end(); ++it) {
        if(it->IsA<UsdSkelRoot>()) {
            cache->Populate(UsdSkelRoot(*it));
            // don't need to iterate further down.
            it.PruneChildren();
        }
    }
}
//! [PopulateAllSkelRoots]


//! [FindSkels]
void FindSkels(const UsdPrim& skelRootPrim,
               UsdSkelCache* cache,
               std::vector<std::pair<UsdPrim,UsdSkelSkeletonQuery> >* skels)
{
    for(const auto& descendant : UsdPrimRange(skelRootPrim)) {
        if(UsdSkelSkeletonQuery skelQuery = cache->GetSkelQuery(descendant)) {
            skels->emplace_back(descendant, skelQuery);
        }
    }
}
//! [FindSkels]


//! [PrintSkelsAndSkinnedPrims]
void PrintSkelsAndSkinnedPrims(const UsdSkelRoot& root, UsdSkelCache* cache)
{
    for(const auto& prim : UsdPrimRange(root.GetPrim())) {
        if(UsdSkelSkeletonQuery skelQuery = cache.GetSkelQuery(prim)) {

            std::cout << skelQuery.GetDescription() << std::endl
                      << "Skinned prims:" << std::endl;

            std::vector<std::pair<UsdPrim,UsdSkelSkinningQuery> > skinnedPrims;
            if(cache.ComputeSkinnedPrims(prim, &skinnedPrims)) {
                for(const auto& pair : skinnedPrims) {
                    std::cout << '\t' << pair.first.GetPath() << std::endl;
                }
            }
        }
    }
}
//! [PrintSkelsAndSkinnedPrims]


//! [PrintSkelsAndSkinningInfoForMesh]
void _PrintSkelAndSkinningInfoForMesh(const UsdPrim& meshPrim)
{
    // Cache data is populated from the view of the skel root. Find it.
    if(UsdSkelRoot root = UsdSkelRoot::Find(meshPrim)) {

        // XXX: Usually this cache would be shared by a process.
        UsdSkelCache cache;
        
        if(!cache.Populate(root))
            return;

        UsdSkelSkeletonQuery skelQuery = cache.GetInheritedSkelQuery(meshPrim);
        std::cout << skelQuery.GetDescription() << std::endl;
    }
}
//! [PrintSkelsAndSkinningInfoForMesh]


//! [ComputeSkinnedPoints]
bool ComputeSkinnedPoints(const UsdGeomPointBased& pointBased,
                          const UsdSkelSkeletonQuery& skelQuery,
                          const UsdSkelSkinningQuery& skinningQuery,
                          UsdTimeCode time,
                          VtVec3fArray* points)
{
    // Query the initial points.
    // The initial points will be in local gprim space.
    if(!pointBased.GetPointsAttr().Get(points)) {
        return false;
    }

    // Compute skinning transforms (in skeleton space!).
    VtMatrix4dArray skinningXforms;
    if(!skelQuery.ComputeSkinningTransforms(&skinningXforms, time)) {
        return false;
    }

    // Apply skinning.
    return skinningQuery.ComputeSkinnedPoints(skinningXforms, points, time);
//! [ComputeSkinnedPoints]


//! [ComputeSkinnedTransform]
bool ComputeSkinnedTransform(const UsdGeomXformable& xformable,
                             const UsdSkelSkeletonQuery& skelQuery,
                             const UsdSkelSkinningQuery& skinningQuery,
                             UsdTimeCode time,
                             GfMatrix4d* xform)
{
    // Must be rigidly deforming to skin a transform.
    if(!skinningQuery.IsRigidlyDeforming()) {
        return false;
    }

    // Compute skinning transforms (in skeleton space!).
    VtMatrix4dArray skinningXforms;
    if(!skelQuery.ComputeSkinningTransforms(&skinningXforms, time)) {
        return false;
    }

    // Apply skinning.
    return skinningQuery.ComputeSkinnedTransform(skinningXforms, xform, time);
}
//! [ComputeSkinnedTransform]



PXR_NAMESPACE_CLOSE_SCOPE
