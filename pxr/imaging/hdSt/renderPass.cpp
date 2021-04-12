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
#include "pxr/imaging/hdSt/renderPass.h"

#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/glUtils.h"
#include "pxr/imaging/hdSt/indirectDrawBatch.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/renderParam.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"

#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/gf/frustum.h"


// XXX We do not want to include specific HgiXX backends, but we need to do
// this temporarily until Storm has transitioned fully to Hgi.
#include "pxr/imaging/hgiGL/graphicsCmds.h"

PXR_NAMESPACE_OPEN_SCOPE

void
_ExecuteDraw(
    HdStCommandBuffer* cmdBuffer,
    HdStRenderPassStateSharedPtr const& stRenderPassState,
    HdStResourceRegistrySharedPtr const& resourceRegistry)
{
    cmdBuffer->ExecuteDraw(stRenderPassState, resourceRegistry);
}

unsigned int
_GetDrawBatchesVersion(HdRenderIndex *renderIndex)
{
    HdStRenderParam *stRenderParam = static_cast<HdStRenderParam*>(
        renderIndex->GetRenderDelegate()->GetRenderParam());

    return stRenderParam->GetDrawBatchesVersion();
}

unsigned int
_GetMaterialTagsVersion(HdRenderIndex *renderIndex)
{
    HdStRenderParam *stRenderParam = static_cast<HdStRenderParam*>(
        renderIndex->GetRenderDelegate()->GetRenderParam());

    return stRenderParam->GetMaterialTagsVersion();
}


HdSt_RenderPass::HdSt_RenderPass(HdRenderIndex *index,
                                 HdRprimCollection const &collection)
    : HdRenderPass(index, collection)
    , _lastSettingsVersion(0)
    , _useTinyPrimCulling(false)
    , _collectionVersion(0)
    , _materialTagsVersion(0)
    , _collectionChanged(false)
    , _drawItemCount(0)
    , _drawItemsChanged(false)
    , _hgi(nullptr)
{
    HdStRenderDelegate* renderDelegate =
        static_cast<HdStRenderDelegate*>(index->GetRenderDelegate());
    _hgi = renderDelegate->GetHgi();
}

HdSt_RenderPass::~HdSt_RenderPass()
{
}

size_t
HdSt_RenderPass::GetDrawItemCount() const
{
    // Note that returning '_drawItems.size()' is only correct during Prepare.
    // During Execute _drawItems is cleared in SwapDrawItems().
    // For that reason we return the cached '_drawItemCount' here.
    return _drawItemCount;
}

void
HdSt_RenderPass::_Prepare(TfTokenVector const &renderTags)
{
    _PrepareDrawItems(renderTags);
}

static
const GfVec3i &
_GetFramebufferSize(const HgiGraphicsCmdsDesc &desc)
{
    for (const HgiTextureHandle &color : desc.colorTextures) {
        return color->GetDescriptor().dimensions;
    }
    if (desc.depthTexture) {
        return desc.depthTexture->GetDescriptor().dimensions;
    }
    
    static const GfVec3i fallback(0);
    return fallback;
}

static
GfVec4i
_FlipViewport(const GfVec4i &viewport,
              const GfVec3i &framebufferSize)
{
    const int height = framebufferSize[1];
    if (height > 0) {
        return GfVec4i(viewport[0],
                       height - (viewport[1] + viewport[3]),
                       viewport[2],
                       viewport[3]);
    } else {
        return viewport;
    }
}

static
GfVec4i
_ToVec4i(const GfVec4f &v)
{
    return GfVec4i(int(v[0]), int(v[1]), int(v[2]), int(v[3]));
}

static
GfVec4i
_ToVec4i(const GfRect2i &r)
{
    return GfVec4i(r.GetMinX(),  r.GetMinY(),
                   r.GetWidth(), r.GetHeight());
}

static
GfVec4i
_ComputeViewport(HdRenderPassStateSharedPtr const &renderPassState,
                 const HgiGraphicsCmdsDesc &desc,
                 const bool flip)
{
    const CameraUtilFraming &framing = renderPassState->GetFraming();
    if (framing.IsValid()) {
        // Use data window for clients using the new camera framing
        // API.
        const GfVec4i viewport = _ToVec4i(framing.dataWindow);
        if (flip) {
            // Note that in OpenGL, the coordinates for the viewport
            // are y-Up but the camera framing is y-Down.
            return _FlipViewport(viewport, _GetFramebufferSize(desc));
        } else {
            return viewport;
        }
    }

    // For clients not using the new camera framing API, fallback
    // to the viewport they specified.
    return _ToVec4i(renderPassState->GetViewport());
}

void
HdSt_RenderPass::_Execute(HdRenderPassStateSharedPtr const &renderPassState,
                          TfTokenVector const& renderTags)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Downcast render pass state
    HdStRenderPassStateSharedPtr stRenderPassState =
        std::dynamic_pointer_cast<HdStRenderPassState>(
        renderPassState);
    TF_VERIFY(stRenderPassState);

    // Validate and update draw batches.
    _PrepareCommandBuffer(renderTags);

    // CPU frustum culling (if chosen)
    _FrustumCullCPU(stRenderPassState);

    // Downcast the resource registry
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::dynamic_pointer_cast<HdStResourceRegistry>(
        GetRenderIndex()->GetResourceRegistry());
    TF_VERIFY(resourceRegistry);

    _cmdBuffer.PrepareDraw(stRenderPassState, resourceRegistry);

    // Create graphics work to render into aovs.
    const HgiGraphicsCmdsDesc desc =
        stRenderPassState->MakeGraphicsCmdsDesc(GetRenderIndex());
    HgiGraphicsCmdsUniquePtr gfxCmds = _hgi->CreateGraphicsCmds(desc);
    if (!TF_VERIFY(gfxCmds)) {
        return;
    }
    HdRprimCollection const &collection = GetRprimCollection();
    std::string passName = "HdSt_RenderPass: " +
        collection.GetMaterialTag().GetString();
    gfxCmds->PushDebugGroup(passName.c_str());

    gfxCmds->SetViewport(
        _ComputeViewport(
            renderPassState,
            desc,
            /* flip = */ _hgi->GetAPIName() == HgiTokens->OpenGL));

    HdStCommandBuffer* cmdBuffer = &_cmdBuffer;
    HgiGLGraphicsCmds* glGfxCmds = 
        dynamic_cast<HgiGLGraphicsCmds*>(gfxCmds.get());

    // XXX: The Bind/Unbind calls below set/restore GL state.
    // This will be reworked to use Hgi.
    stRenderPassState->Bind();

    if (glGfxCmds) {
        // XXX Tmp code path to allow non-hgi code to insert functions into
        // HgiGL ops-stack. Will be removed once Storms uses Hgi everywhere
        auto executeDrawOp = [cmdBuffer, stRenderPassState, resourceRegistry] {
            _ExecuteDraw(cmdBuffer, stRenderPassState, resourceRegistry);
        };
        glGfxCmds->InsertFunctionOp(executeDrawOp);
    } else {
        _ExecuteDraw(cmdBuffer, stRenderPassState, resourceRegistry);
    }

    if (gfxCmds) {
        gfxCmds->PopDebugGroup();
        _hgi->SubmitCmds(gfxCmds.get());
    }

    stRenderPassState->Unbind();
}

void
HdSt_RenderPass::_MarkCollectionDirty()
{
    // Force any cached data based on collection to be refreshed.
    _collectionChanged = true;
    _collectionVersion = 0;
}

void
HdSt_RenderPass::_PrepareDrawItems(TfTokenVector const& renderTags)
{
    HD_TRACE_FUNCTION();

    HdChangeTracker const &tracker = GetRenderIndex()->GetChangeTracker();
    HdRprimCollection const &collection = GetRprimCollection();

    const int collectionVersion =
        tracker.GetCollectionVersion(collection.GetName());

    const int renderTagVersion =
        tracker.GetRenderTagVersion();

    const unsigned int materialTagsVersion =
        _GetMaterialTagsVersion(GetRenderIndex());

    const bool collectionChanged = _collectionChanged ||
        (_collectionVersion != collectionVersion);

    const bool renderTagsChanged = _renderTagVersion != renderTagVersion;

    const bool materialTagsChanged =
        _materialTagsVersion != materialTagsVersion;

    if (collectionChanged || renderTagsChanged || materialTagsChanged) {
        HD_PERF_COUNTER_INCR(HdPerfTokens->collectionsRefreshed);

        if (TfDebug::IsEnabled(HDST_DRAW_ITEM_GATHER)) {
            if (collectionChanged) {
                TfDebug::Helper::Msg(
                    "CollectionChanged: %s (repr = %s, version = %d -> %d)\n",
                        collection.GetName().GetText(),
                        collection.GetReprSelector().GetText(),
                        _collectionVersion,
                        collectionVersion);
            }

            if (renderTagsChanged) {
                TfDebug::Helper::Msg("RenderTagsChanged (version = %d -> %d)\n",
                        _renderTagVersion, renderTagVersion);
            }
            if (materialTagsChanged) {
                TfDebug::Helper::Msg(
                    "MaterialTagsChanged (version = %d -> %d)\n",
                    _materialTagsVersion, materialTagsVersion);
            }
        }

        _drawItems = GetRenderIndex()->GetDrawItems(collection, renderTags);
        _drawItemCount = _drawItems.size();
        _drawItemsChanged = true;

        _collectionVersion = collectionVersion;
        _collectionChanged = false;

        _renderTagVersion = renderTagVersion;
        _materialTagsVersion = materialTagsVersion;
    }
}

void
HdSt_RenderPass::_PrepareCommandBuffer(TfTokenVector const& renderTags)
{
    HD_TRACE_FUNCTION();

    // -------------------------------------------------------------------
    // SCHEDULE PREPARATION
    // -------------------------------------------------------------------
    // We know what must be drawn and that the stream needs to be updated,
    // so iterate over each prim, cull it and schedule it to be drawn.

    const int batchVersion = _GetDrawBatchesVersion(GetRenderIndex());

    // It is optional for a render task to call RenderPass::Prepare() to
    // update the drawItems during the prepare phase. We ensure our drawItems
    // are always up-to-date before building the command buffers.
    _PrepareDrawItems(renderTags);

    // Rebuild draw batches based on new draw items
    if (_drawItemsChanged) {
        _cmdBuffer.SwapDrawItems(
            // Downcast the HdDrawItem entries to HdStDrawItems:
            reinterpret_cast<std::vector<HdStDrawItem const*>*>(&_drawItems),
            batchVersion);

        _drawItemsChanged = false;
        size_t itemCount = _cmdBuffer.GetTotalSize();
        HD_PERF_COUNTER_SET(HdTokens->totalItemCount, itemCount);
    } else {
        // validate command buffer to not include expired drawItems,
        // which could be produced by migrating BARs at the new repr creation.
        _cmdBuffer.RebuildDrawBatchesIfNeeded(batchVersion);
    }

    // -------------------------------------------------------------------
    // RENDER SETTINGS
    // -------------------------------------------------------------------
    HdRenderDelegate *renderDelegate = GetRenderIndex()->GetRenderDelegate();
    int currentSettingsVersion = renderDelegate->GetRenderSettingsVersion();
    if (_lastSettingsVersion != currentSettingsVersion) {
        _lastSettingsVersion = currentSettingsVersion;
        _useTinyPrimCulling = renderDelegate->GetRenderSetting<bool>(
            HdStRenderSettingsTokens->enableTinyPrimCulling, false);
    }

    _cmdBuffer.SetEnableTinyPrimCulling(_useTinyPrimCulling);
}

void
HdSt_RenderPass::_FrustumCullCPU(
    HdStRenderPassStateSharedPtr const &renderPassState)
{
    // This process should be moved to HdSt_DrawBatch::PrepareDraw
    // to be consistent with GPU culling.

    GlfContextCaps const &caps = GlfContextCaps::GetInstance();
    HdChangeTracker const &tracker = GetRenderIndex()->GetChangeTracker();

    const bool
       skipCulling = TfDebug::IsEnabled(HDST_DISABLE_FRUSTUM_CULLING) ||
           (caps.multiDrawIndirectEnabled
               && HdSt_IndirectDrawBatch::IsEnabledGPUFrustumCulling());
    bool freezeCulling = TfDebug::IsEnabled(HD_FREEZE_CULL_FRUSTUM);

    if(skipCulling) {
        // Since culling state is stored across renders,
        // we need to update all items visible state
        _cmdBuffer.SyncDrawItemVisibility(tracker.GetVisibilityChangeCount());

        TF_DEBUG(HD_DRAWITEMS_CULLED).Msg("CULLED: skipped\n");
    }
    else {
        if (!freezeCulling) {
            // Re-cull the command buffer.
            _cmdBuffer.FrustumCull(renderPassState->GetCullMatrix());
        }

        if (TfDebug::IsEnabled(HD_DRAWITEMS_CULLED)) {
            TF_DEBUG(HD_DRAWITEMS_CULLED).Msg("CULLED: %zu drawItems\n",
                                              _cmdBuffer.GetCulledSize());
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
