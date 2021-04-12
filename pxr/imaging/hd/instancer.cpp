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
        HdInstancerTokens->instanceTransform,
        HdInstancerTokens->rotate,
        HdInstancerTokens->scale,
        HdInstancerTokens->translate
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
    SdfPath id = instancerId;
    while (!id.IsEmpty()) {
        HdInstancer *instancer = renderIndex.GetInstancer(id);
        if (!TF_VERIFY(instancer)) {
            return;
        }

        HdDirtyBits dirtyBits =
            renderIndex.GetChangeTracker().GetInstancerDirtyBits(id);

        if (dirtyBits != HdChangeTracker::Clean) {
            std::lock_guard<std::mutex> lock(instancer->_instanceLock);
            dirtyBits =
                renderIndex.GetChangeTracker().GetInstancerDirtyBits(id);
            instancer->Sync(instancer->GetDelegate(), renderParam, &dirtyBits);
            renderIndex.GetChangeTracker().MarkInstancerClean(id);
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

