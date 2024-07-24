//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

HdInstancer::HdInstancer(HdSceneDelegate* delegate,
                         SdfPath const& id)
    : _delegate(delegate)
    , _id(id)
    , _parentId()
{
}

HdInstancer::~HdInstancer() = default;

/* static */
int
HdInstancer::GetInstancerNumLevels(HdRenderIndex& index,
                                   HdRprim const& rprim)
{
    // Walk up the instancing hierarchy to figure out how many levels of
    // instancing the passed-in rprim has.

    int instancerLevels = 0;
    SdfPath parent = rprim.GetInstancerId();
    HdInstancer *instancer = nullptr;
    while (!parent.IsEmpty()) {
        instancerLevels++;
        instancer = index.GetInstancer(parent);
        TF_VERIFY(instancer);
        parent = instancer ? instancer->GetParentId()
            : SdfPath::EmptyPath();
    }
    return instancerLevels;
}

/* static */
TfTokenVector const &
HdInstancer::GetBuiltinPrimvarNames()
{
    static const TfTokenVector primvarNames = {
        HdInstancerTokens->instanceTransforms,
        HdInstancerTokens->instanceRotations,
        HdInstancerTokens->instanceScales,
        HdInstancerTokens->instanceTranslations
    };
    return primvarNames;
}

void
HdInstancer::Sync(HdSceneDelegate *sceneDelegate,
                  HdRenderParam   *renderParam,
                  HdDirtyBits     *dirtyBits)
{
}

void
HdInstancer::Finalize(HdRenderParam *renderParam)
{
}

void
HdInstancer::_SyncInstancerAndParents(HdRenderIndex &renderIndex,
                                      SdfPath const& instancerId)
{
    HdRenderParam *renderParam =
        renderIndex.GetRenderDelegate()->GetRenderParam();
    HdChangeTracker& tracker = renderIndex.GetChangeTracker();
    SdfPath id = instancerId;
    while (!id.IsEmpty()) {
        HdInstancer *instancer = renderIndex.GetInstancer(id);
        if (!TF_VERIFY(instancer)) {
            return;
        }

        std::lock_guard<std::mutex> lock(instancer->_instanceLock);
        HdDirtyBits dirtyBits = tracker.GetInstancerDirtyBits(id);
        if (dirtyBits != HdChangeTracker::Clean) {
            instancer->Sync(instancer->GetDelegate(), renderParam, &dirtyBits);
            tracker.MarkInstancerClean(id);
        }
        
        id = instancer->GetParentId();
    }
}

void
HdInstancer::_UpdateInstancer(HdSceneDelegate *delegate,
                              HdDirtyBits *dirtyBits)
{
    if (HdChangeTracker::IsInstancerDirty(*dirtyBits, GetId())) {
        SdfPath const& parentId = delegate->GetInstancerId(GetId());
        if (parentId == _parentId) {
            return;
        }

        // If we have a new instancer ID, we need to update the dependency
        // map and also update the stored instancer ID.
        HdChangeTracker &tracker =
            delegate->GetRenderIndex().GetChangeTracker();
        if (!_parentId.IsEmpty()) {
            tracker.RemoveInstancerInstancerDependency(_parentId, GetId());
        }
        if (!parentId.IsEmpty()) {
            tracker.AddInstancerInstancerDependency(parentId, GetId());
        }
        _parentId = parentId;
    }
}

HdDirtyBits
HdInstancer::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::DirtyTransform |
           HdChangeTracker::DirtyPrimvar |
           HdChangeTracker::DirtyInstanceIndex |
           HdChangeTracker::DirtyInstancer;
}

PXR_NAMESPACE_CLOSE_SCOPE

