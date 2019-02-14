//
// Copyright 2019 Pixar
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
#include "pxr/usdImaging/usdSkelImaging/skelRootAdapter.h"
#include "pxr/usdImaging/usdSkelImaging/skeletonAdapter.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/gprimAdapter.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/cache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdSkelImagingSkelRootAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}


UsdSkelImagingSkelRootAdapter::~UsdSkelImagingSkelRootAdapter()
{}

/*virtual*/
SdfPath
UsdSkelImagingSkelRootAdapter::Populate(
    const UsdPrim& prim,
    UsdImagingIndexProxy* index,
    const UsdImagingInstancerContext* instancerContext)
{
    if(!TF_VERIFY(prim.IsA<UsdSkelRoot>())) {
        return SdfPath();
    }

    // Find skeletons and skinned prims under this skel root.
    UsdSkelRoot skelRoot(prim);
    UsdSkelCache skelCache;
    skelCache.Populate(skelRoot);
    
    std::vector<UsdSkelBinding> bindings;
    if (!skelCache.ComputeSkelBindings(skelRoot, &bindings)) {
        return SdfPath();
    }
    if (bindings.empty()) {
        return SdfPath();
    }

    // Use the skeleton adapter to inject hydra computation prims for each
    // target of a skeleton.
    for (const auto& binding : bindings) {
        const UsdSkelSkeleton& skel = binding.GetSkeleton();

        UsdImagingPrimAdapterSharedPtr adapter =
            _GetPrimAdapter(skel.GetPrim());
        TF_VERIFY(adapter);

        auto skelAdapter = boost::dynamic_pointer_cast<
            UsdSkelImagingSkeletonAdapter> (adapter);
        TF_VERIFY(skelAdapter);

        // Define a new binding that only contains skinnable prims
        // that have a bound prim adapter.
        VtArray<UsdSkelSkinningQuery> skinningQueries;
        skinningQueries.reserve(binding.GetSkinningTargets().size());

        for (const auto& skinningQuery : binding.GetSkinningTargets()) {
            UsdPrim const& skinnedPrim = skinningQuery.GetPrim();

            // Register the SkeletonAdapter for each skinned prim, effectively
            // hijacking all processing to go via it.
            UsdImagingPrimAdapterSharedPtr skinnedPrimAdapter =
                _GetPrimAdapter(skinnedPrim);
            if (!skinnedPrimAdapter) {
                // This prim is technically considered skinnable,
                // but an adapter may not be registered for the prim type.
                continue;
            }

            UsdImagingInstancerContext hijackContext;
            if (instancerContext) {
                hijackContext = *instancerContext;
            }
            hijackContext.instancerAdapter = skelAdapter;
            skinnedPrimAdapter->Populate(skinnedPrim, index, &hijackContext);

            skinningQueries.push_back(skinningQuery);
        }
        // We don't have a way to figure out all the skinned prims that are
        // bound to a skeleton when processing it (in the SkeletonAdapter).
        // We can do this with the SkelRoot and let the SkeletonAdapter know
        // about it.
        skelAdapter->RegisterSkelBinding(
            UsdSkelBinding(skel, skinningQueries));
    }

    return SdfPath();
}

/*virtual*/
void
UsdSkelImagingSkelRootAdapter::TrackVariability(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    HdDirtyBits* timeVaryingBits,
    const UsdImagingInstancerContext* instancerContext) const
{
    // The SkeletonAdapter is registered for skeletons and skinned prims, so
    // there's no work to be done here.
}

/*virtual*/
void
UsdSkelImagingSkelRootAdapter::UpdateForTime(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    const UsdImagingInstancerContext* instancerContext) const
{
    // The SkeletonAdapter is registered for skeletons and skinned prims, so
    // there's no work to be done here.
}

/*virtual*/
HdDirtyBits
UsdSkelImagingSkelRootAdapter::ProcessPropertyChange(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    const TfToken& propertyName)
{
    // The SkeletonAdapter is registered for skeletons and skinned prims, so
    // there's no work to be done here.
    // Note: Subtree visibility is handled by the delegate.
    return HdChangeTracker::Clean;
}

/*virtual*/
void
UsdSkelImagingSkelRootAdapter::MarkDirty(const UsdPrim& prim,
                                          const SdfPath& cachePath,
                                          HdDirtyBits dirty,
                                          UsdImagingIndexProxy* index)
{
    // The SkeletonAdapter is registered for skeletons and skinned prims, so
    // there's no work to be done here.
}

/*virtual*/
void
UsdSkelImagingSkelRootAdapter::_RemovePrim(const SdfPath& cachePath,
                                           UsdImagingIndexProxy* index)
{
    // The SkeletonAdapter is registered for skeletons and skinned prims, so
    // there's no work to be done here.
}

PXR_NAMESPACE_CLOSE_SCOPE
