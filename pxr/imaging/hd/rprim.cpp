//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/rprim.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"

PXR_NAMESPACE_OPEN_SCOPE


HdRprim::HdRprim(SdfPath const& id)
    : _instancerId()
    , _materialId()
    , _sharedData(HdDrawingCoord::DefaultNumSlots,
                  /*visible=*/true)
{
    _sharedData.rprimID = id;
}

HdRprim::~HdRprim() = default;

// -------------------------------------------------------------------------- //
///                 Rprim Hydra Engine API : Pre-Sync & Sync-Phase
// -------------------------------------------------------------------------- //

bool
HdRprim::CanSkipDirtyBitPropagationAndSync(HdDirtyBits bits) const
{
    // For invisible prims, we'd like to avoid syncing data, which involves:
    // (a) the scene delegate pulling data post dirty-bit propagation 
    // (b) the rprim processing its dirty bits and
    // (c) the rprim committing resource updates to the GPU
    // 
    // However, the current design adds a draw item for a repr during repr 
    // initialization (see _InitRepr) even if a prim may be invisible, which
    // requires us go through the sync process to avoid tripping other checks.
    // 
    // XXX: We may want to avoid this altogether, or rethink how we approach
    // the two workflow scenarios:
    // ( i) objects that are always invisible (i.e., never loaded by the user or
    // scene)
    // (ii) vis-invis'ing objects
    //  
    // For now, we take the hit of first repr initialization (+ sync) and avoid
    // time-varying updates to the invisible prim.
    // 
    // Note: If the sync is skipped, the dirty bits in the change tracker
    // remain the same.
    bool skip = false;

    HdDirtyBits mask = (HdChangeTracker::DirtyVisibility |
                        HdChangeTracker::NewRepr);

    if (!IsVisible() && !(bits & mask)) {
        // By setting the propagated dirty bits to Clean, we effectively 
        // disable delegate and rprim sync
        skip = true;
        HD_PERF_COUNTER_INCR(HdPerfTokens->skipInvisibleRprimSync);
    }

    return skip;
}

HdDirtyBits
HdRprim::PropagateRprimDirtyBits(HdDirtyBits bits)
{
    // If the dependent computations changed - assume all
    // primvars are dirty
    if (bits & HdChangeTracker::DirtyComputationPrimvarDesc) {
        bits |= (HdChangeTracker::DirtyPoints  |
                 HdChangeTracker::DirtyNormals |
                 HdChangeTracker::DirtyWidths  |
                 HdChangeTracker::DirtyPrimvar);
    }

    // when refine level changes, topology becomes dirty.
    // XXX: can we remove DirtyDisplayStyle then?
    if (bits & HdChangeTracker::DirtyDisplayStyle) {
        bits |=  HdChangeTracker::DirtyTopology;
    }

    // if topology changes, all dependent bits become dirty.
    if (bits & HdChangeTracker::DirtyTopology) {
        bits |= (HdChangeTracker::DirtyPoints  |
                 HdChangeTracker::DirtyNormals |
                 HdChangeTracker::DirtyPrimvar);
    }

    // Let subclasses propagate bits
    return _PropagateDirtyBits(bits);
}

void
HdRprim::InitRepr(HdSceneDelegate* delegate,
                  TfToken const &reprToken,
                  HdDirtyBits *dirtyBits)
{
    _InitRepr(reprToken, dirtyBits);
}

// -------------------------------------------------------------------------- //
///                 Rprim Hydra Engine API : Execute-Phase
// -------------------------------------------------------------------------- //
const HdRepr::DrawItemUniquePtrVector &
HdRprim::GetDrawItems(TfToken const& reprToken) const
{
    if (HdReprSharedPtr const repr = _GetRepr(reprToken)) {
        return repr->GetDrawItems();
    }

    static HdRepr::DrawItemUniquePtrVector empty;

    TF_CODING_ERROR("Rprim has no draw items for repr %s", reprToken.GetText());

    return empty;
}

// -------------------------------------------------------------------------- //
///                     Rprim Hydra Engine API : Cleanup
// -------------------------------------------------------------------------- //
void
HdRprim::Finalize(HdRenderParam *renderParam)
{
}

// -------------------------------------------------------------------------- //
///                              Rprim Data API
// -------------------------------------------------------------------------- //
void
HdRprim::SetPrimId(int32_t primId)
{
    _primId = primId;
    // Don't set DirtyPrimID here, to avoid undesired variability tracking.
}

void 
HdRprim::SetMaterialId(SdfPath const& materialId)
{
    _materialId = materialId;
}

bool
HdRprim::IsDirty(HdChangeTracker &changeTracker) const
{
    return changeTracker.IsRprimDirty(GetId());
}

void
HdRprim::UpdateReprSelector(HdSceneDelegate* delegate,
                            HdDirtyBits *dirtyBits)
{
    if (HdChangeTracker::IsReprDirty(*dirtyBits, GetId())) {
        _authoredReprSelector = delegate->GetReprSelector(GetId());
        *dirtyBits &= ~HdChangeTracker::DirtyRepr;
    }
}

void
HdRprim::UpdateRenderTag(HdSceneDelegate *delegate,
                         HdRenderParam *renderParam)
{
    _renderTag = delegate->GetRenderTag(GetId());
}

// -------------------------------------------------------------------------- //
///                             Rprim Shared API
// -------------------------------------------------------------------------- //
HdReprSharedPtr const &
HdRprim::_GetRepr(TfToken const &reprToken) const
{
    _ReprVector::const_iterator reprIt =
        std::find_if(_reprs.begin(), _reprs.end(),
                     _ReprComparator(reprToken));
    if (reprIt == _reprs.end()) {
        TF_CODING_ERROR("_InitRepr() should be called for repr %s on prim %s.",
                        reprToken.GetText(), GetId().GetText());
        static const HdReprSharedPtr ERROR_RETURN;
        return ERROR_RETURN;
    }
    return reprIt->second;
}

void
HdRprim::_UpdateVisibility(HdSceneDelegate* delegate,
                           HdDirtyBits *dirtyBits)
{
    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, GetId())) {
        _sharedData.visible = delegate->GetVisible(GetId());
    }
}

void
HdRprim::_UpdateInstancer(HdSceneDelegate* delegate,
                          HdDirtyBits *dirtyBits)
{
    if (HdChangeTracker::IsInstancerDirty(*dirtyBits, GetId())) {
        SdfPath const& instancerId = delegate->GetInstancerId(GetId());
        if (instancerId == _instancerId) {
            return;
        }

        // If we have a new instancer ID, we need to update the dependency
        // map and also update the stored instancer ID.
        HdChangeTracker &tracker =
            delegate->GetRenderIndex().GetChangeTracker();
        if (!_instancerId.IsEmpty()) {
            tracker.RemoveInstancerRprimDependency(_instancerId, GetId());
        }
        if (!instancerId.IsEmpty()) {
            tracker.AddInstancerRprimDependency(instancerId, GetId());
        }
        _instancerId = instancerId;
    }
}

VtMatrix4dArray
HdRprim::GetInstancerTransforms(HdSceneDelegate* delegate)
{
    SdfPath instancerId = _instancerId;
    VtMatrix4dArray transforms;

    HdRenderIndex &renderIndex = delegate->GetRenderIndex();

    while (!instancerId.IsEmpty()) {
        transforms.push_back(delegate->GetInstancerTransform(instancerId));
        HdInstancer *instancer = renderIndex.GetInstancer(instancerId);
        if (instancer) {
            instancerId = instancer->GetParentId();
        } else {
            instancerId = SdfPath();
        }
    }
    return transforms;
}

PXR_NAMESPACE_CLOSE_SCOPE
