//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/plugin/LoFi/drawTarget.h"

#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(LoFiDrawTargetTokens, LOFI_DRAW_TARGET_TOKENS);

LoFiDrawTarget::LoFiDrawTarget(SdfPath const &id)
    : HdSprim(id)
    , _enabled(true)
    , _resolution(512, 512)
{
}

LoFiDrawTarget::~LoFiDrawTarget() = default;

/*virtual*/
void
LoFiDrawTarget::Sync(HdSceneDelegate *sceneDelegate,
                     HdRenderParam   *renderParam,
                     HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    SdfPath const &id = GetId();
    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    const HdDirtyBits bits = *dirtyBits;

    if (bits & DirtyDTEnable) {
        const VtValue vtValue =
            sceneDelegate->Get(id, LoFiDrawTargetTokens->enable);

        // Optional attribute.
        _enabled = vtValue.GetWithDefault<bool>(true);
    }

    if (bits & DirtyDTCamera) {
        const VtValue vtValue =
            sceneDelegate->Get(id, LoFiDrawTargetTokens->camera);
        _drawTargetRenderPassState.SetCamera(vtValue.Get<SdfPath>());
    }

    if (bits & DirtyDTResolution) {
        const VtValue vtValue =
            sceneDelegate->Get(id, LoFiDrawTargetTokens->resolution);
        
        // The resolution is needed to set the viewport and compute the
        // camera projection matrix (more precisely, to do the aspect ratio
        // adjustment).
        //
        // Note that it is also stored in the render buffers. This is
        // somewhat redundant but it would be complicated for the draw
        // target to reach through to the render buffers to get the
        // resolution and that conceptually, the view port and camera
        // projection matrix are different from the texture
        // resolution.
        _resolution = vtValue.Get<GfVec2i>();
    }

    if (bits & DirtyDTAovBindings) {
        const VtValue aovBindingsValue =
            sceneDelegate->Get(id, LoFiDrawTargetTokens->aovBindings);
        _drawTargetRenderPassState.SetAovBindings(
            aovBindingsValue.GetWithDefault<HdRenderPassAovBindingVector>(
                {}));
    }
    
    if (bits & DirtyDTDepthPriority) {
        const VtValue depthPriorityValue =
            sceneDelegate->Get(id, LoFiDrawTargetTokens->depthPriority);
        _drawTargetRenderPassState.SetDepthPriority(
            depthPriorityValue.GetWithDefault<HdDepthPriority>(
                HdDepthPriorityNearest));
    }
        
    if (bits & DirtyDTCollection) {
        const VtValue vtValue =
            sceneDelegate->Get(id, LoFiDrawTargetTokens->collection);

        const HdRprimCollection collection = vtValue.Get<HdRprimCollection>();

        TfToken const &collectionName = collection.GetName();

        HdChangeTracker& changeTracker =
                         sceneDelegate->GetRenderIndex().GetChangeTracker();

        if (_collection.GetName() != collectionName) {
            // Make sure collection has been added to change tracker
            changeTracker.AddCollection(collectionName);
        }

        // Always mark collection dirty even if added - as we don't
        // know if this is a re-add.
        changeTracker.MarkCollectionDirty(collectionName);

        _drawTargetRenderPassState.SetRprimCollection(collection);
        _collection = collection;
    }

    *dirtyBits = Clean;
}

// virtual
HdDirtyBits
LoFiDrawTarget::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

/*static*/
void
LoFiDrawTarget::GetDrawTargets(HdRenderIndex* const renderIndex,
                               LoFiDrawTargetPtrVector * const drawTargets)
{
    HF_MALLOC_TAG_FUNCTION();

    if (!renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->drawTarget)) {
        return;
    }

    const SdfPathVector paths = renderIndex->GetSprimSubtree(
        HdPrimTypeTokens->drawTarget, SdfPath::AbsoluteRootPath());

    for (const SdfPath &path : paths) {
        if (HdSprim * const drawTarget =
                renderIndex->GetSprim(HdPrimTypeTokens->drawTarget, path)) {
            drawTargets->push_back(static_cast<LoFiDrawTarget *>(drawTarget));
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

