//
// Copyright 2017 Pixar
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
#include "pxr/imaging/hdSt/drawTarget.h"

#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdStDrawTargetTokens, HDST_DRAW_TARGET_TOKENS);

HdStDrawTarget::HdStDrawTarget(SdfPath const &id)
    : HdSprim(id)
    , _enabled(true)
    , _resolution(512, 512)
{
}

HdStDrawTarget::~HdStDrawTarget() = default;

/*virtual*/
void
HdStDrawTarget::Sync(HdSceneDelegate *sceneDelegate,
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
            sceneDelegate->Get(id, HdStDrawTargetTokens->enable);

        // Optional attribute.
        _enabled = vtValue.GetWithDefault<bool>(true);
    }

    if (bits & DirtyDTCamera) {
        const VtValue vtValue =
            sceneDelegate->Get(id, HdStDrawTargetTokens->camera);
        _drawTargetRenderPassState.SetCamera(vtValue.Get<SdfPath>());
    }

    if (bits & DirtyDTResolution) {
        const VtValue vtValue =
            sceneDelegate->Get(id, HdStDrawTargetTokens->resolution);
        
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
            sceneDelegate->Get(id, HdStDrawTargetTokens->aovBindings);
        _drawTargetRenderPassState.SetAovBindings(
            aovBindingsValue.GetWithDefault<HdRenderPassAovBindingVector>(
                {}));
    }
    
    if (bits & DirtyDTDepthPriority) {
        const VtValue depthPriorityValue =
            sceneDelegate->Get(id, HdStDrawTargetTokens->depthPriority);
        _drawTargetRenderPassState.SetDepthPriority(
            depthPriorityValue.GetWithDefault<HdDepthPriority>(
                HdDepthPriorityNearest));
    }
        
    if (bits & DirtyDTCollection) {
        const VtValue vtValue =
            sceneDelegate->Get(id, HdStDrawTargetTokens->collection);

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
HdStDrawTarget::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

/*static*/
void
HdStDrawTarget::GetDrawTargets(HdRenderIndex* const renderIndex,
                               HdStDrawTargetPtrVector * const drawTargets)
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
            drawTargets->push_back(static_cast<HdStDrawTarget *>(drawTarget));
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

