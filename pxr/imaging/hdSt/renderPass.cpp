//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/renderPass.h"

#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/drawItemsCache.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/renderParam.h"
#include "pxr/imaging/hdSt/renderPassState.h"

#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/hgi.h"

#include "pxr/imaging/hd/renderDelegate.h"

#include "pxr/base/tf/envSetting.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDST_ENABLE_DRAW_ITEMS_CACHE, false,
                "Enable usage of the draw items cache in Storm.");

static bool
_IsDrawItemsCacheEnabled()
{
    static const bool enabled = TfGetEnvSetting(HDST_ENABLE_DRAW_ITEMS_CACHE);
    return enabled;
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

unsigned int
_GetGeomSubsetDrawItemsVersion(HdRenderIndex *renderIndex)
{
    HdStRenderParam *stRenderParam = static_cast<HdStRenderParam*>(
        renderIndex->GetRenderDelegate()->GetRenderParam());

    return stRenderParam->GetGeomSubsetDrawItemsVersion();
}

HdSt_RenderPass::HdSt_RenderPass(HdRenderIndex *index,
                                 HdRprimCollection const &collection)
    : HdRenderPass(index, collection)
    , _lastSettingsVersion(0)
    , _useTinyPrimCulling(false)
    , _collectionVersion(0)
    , _rprimRenderTagVersion(0)
    , _taskRenderTagsVersion(0)
    , _materialTagsVersion(0)
    , _geomSubsetDrawItemsVersion(0)
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

bool
HdSt_RenderPass::HasDrawItems(TfTokenVector const &renderTags) const
{
    // Note that using the material tag and render tags is not a sufficient
    // filter. The collection paths also matter for computing the correct 
    // subset.  So this method may produce false positives, but serves its 
    // purpose of identifying when work can be skipped due to definite lack of
    // draw items that pass the material tag and render tags filter.
    const HdStRenderParam * const renderParam =
        static_cast<HdStRenderParam *>(
            GetRenderIndex()->GetRenderDelegate()->GetRenderParam());

    return renderParam->HasMaterialTag(GetRprimCollection().GetMaterialTag()) &&
        (renderTags.empty() || renderParam->HasAnyRenderTag(renderTags));
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
    _UpdateCommandBuffer(renderTags);

    // Downcast the resource registry
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::dynamic_pointer_cast<HdStResourceRegistry>(
        GetRenderIndex()->GetResourceRegistry());
    TF_VERIFY(resourceRegistry);

    // Create graphics work to handle the prepare steps.
    // This does not have any AOVs since it only writes intermediate buffers.
    HgiGraphicsCmdsUniquePtr prepareGfxCmds =
        _hgi->CreateGraphicsCmds(HgiGraphicsCmdsDesc());
    if (!TF_VERIFY(prepareGfxCmds)) {
        return;
    }

    HdRprimCollection const &collection = GetRprimCollection();
    std::string prepareName = "HdSt_RenderPass: Prepare " +
        collection.GetMaterialTag().GetString();

    prepareGfxCmds->PushDebugGroup(prepareName.c_str());

    _cmdBuffer.PrepareDraw(prepareGfxCmds.get(),
                           stRenderPassState, GetRenderIndex());

    prepareGfxCmds->PopDebugGroup();
    _hgi->SubmitCmds(prepareGfxCmds.get());

    // Create graphics work to render into aovs.
    const HgiGraphicsCmdsDesc desc =
        stRenderPassState->MakeGraphicsCmdsDesc(GetRenderIndex());
    HgiGraphicsCmdsUniquePtr gfxCmds = _hgi->CreateGraphicsCmds(desc);
    if (!TF_VERIFY(gfxCmds)) {
        return;
    }

    std::string passName = "HdSt_RenderPass: " +
        collection.GetMaterialTag().GetString();

    gfxCmds->PushDebugGroup(passName.c_str());

    gfxCmds->SetViewport(stRenderPassState->ComputeViewport());

    // Camera state needs to be updated once per pass (not per batch).
    stRenderPassState->ApplyStateFromCamera();

    _cmdBuffer.ExecuteDraw(gfxCmds.get(), stRenderPassState, resourceRegistry);

    gfxCmds->PopDebugGroup();
    _hgi->SubmitCmds(gfxCmds.get());
}

void
HdSt_RenderPass::_MarkCollectionDirty()
{
    // Force any cached data based on collection to be refreshed.
    _collectionChanged = true;
    _collectionVersion = 0;
}

static
HdStDrawItemsCachePtr
_GetDrawItemsCache(HdRenderIndex *renderIndex)
{
    HdStRenderDelegate* renderDelegate =
        static_cast<HdStRenderDelegate*>(renderIndex->GetRenderDelegate());
    return renderDelegate->GetDrawItemsCache();
}

void
HdSt_RenderPass::_UpdateDrawItems(TfTokenVector const& renderTags)
{
    HD_TRACE_FUNCTION();

    HdRprimCollection const &collection = GetRprimCollection();
    if (_IsDrawItemsCacheEnabled()) {
        HdStDrawItemsCachePtr cache = _GetDrawItemsCache(GetRenderIndex());

        HdDrawItemConstPtrVectorSharedPtr cachedEntry =
            cache->GetDrawItems(
                collection, renderTags, GetRenderIndex(), _drawItems);
        
        if (_drawItems != cachedEntry) {
            _drawItems = cachedEntry;
            _drawItemsChanged = true;
            _drawItemCount = _drawItems->size();
        }
        // We don't rely on this state when using the cache. Reset always.
        _collectionChanged = false;

        return;
    }

    HdChangeTracker const &tracker = GetRenderIndex()->GetChangeTracker();

    const int collectionVersion =
        tracker.GetCollectionVersion(collection.GetName());

    const int rprimRenderTagVersion = tracker.GetRenderTagVersion();

    const unsigned int materialTagsVersion =
        _GetMaterialTagsVersion(GetRenderIndex());

    const unsigned int geomSubsetDrawItemsVersion =
        _GetGeomSubsetDrawItemsVersion(GetRenderIndex());

    const bool collectionChanged = _collectionChanged ||
        (_collectionVersion != collectionVersion);

    const bool rprimRenderTagChanged = _rprimRenderTagVersion != rprimRenderTagVersion;

    const bool materialTagsChanged =
        _materialTagsVersion != materialTagsVersion;

    const bool geomSubsetDrawItemsChanged =
        _geomSubsetDrawItemsVersion != geomSubsetDrawItemsVersion;

    const int taskRenderTagsVersion = tracker.GetTaskRenderTagsVersion();
    bool taskRenderTagsChanged = false;
    if (_taskRenderTagsVersion != taskRenderTagsVersion) {
        _taskRenderTagsVersion = taskRenderTagsVersion;
        if (_prevRenderTags != renderTags) {
            _prevRenderTags = renderTags;
            taskRenderTagsChanged = true;
        }
    }

    if (collectionChanged ||
        rprimRenderTagChanged ||
        materialTagsChanged ||
        geomSubsetDrawItemsChanged ||
        taskRenderTagsChanged) {

        if (TfDebug::IsEnabled(HDST_DRAW_ITEM_GATHER)) {
            if (collectionChanged) {
                TfDebug::Helper::Msg(
                    "CollectionChanged: %s (repr = %s, version = %d -> %d)\n",
                        collection.GetName().GetText(),
                        collection.GetReprSelector().GetText(),
                        _collectionVersion,
                        collectionVersion);
            }

            if (rprimRenderTagChanged) {
                TfDebug::Helper::Msg("RprimRenderTagChanged (version = %d -> %d)\n",
                        _rprimRenderTagVersion, rprimRenderTagVersion);
            }
            if (materialTagsChanged) {
                TfDebug::Helper::Msg(
                    "MaterialTagsChanged (version = %d -> %d)\n",
                    _materialTagsVersion, materialTagsVersion);
            }
            if (geomSubsetDrawItemsChanged) {
                TfDebug::Helper::Msg(
                    "GeomSubsetDrawItemsChanged (version = %d -> %d)\n",
                    _geomSubsetDrawItemsVersion, geomSubsetDrawItemsVersion);
            }
            if (taskRenderTagsChanged) {
                TfDebug::Helper::Msg( "TaskRenderTagsChanged\n" );
            }
        }

        const HdStRenderParam * const renderParam =
            static_cast<HdStRenderParam *>(
                GetRenderIndex()->GetRenderDelegate()->GetRenderParam());
        if (renderParam->HasMaterialTag(collection.GetMaterialTag())) {
            _drawItems = std::make_shared<HdDrawItemConstPtrVector>(
                GetRenderIndex()->GetDrawItems(collection, renderTags));
            HD_PERF_COUNTER_INCR(HdStPerfTokens->drawItemsFetched);
        } else {
            // No need to even call GetDrawItems when we know that
            // there is no prim with the desired material tag.
            _drawItems = std::make_shared<HdDrawItemConstPtrVector>();
        }
        _drawItemCount = _drawItems->size();
        _drawItemsChanged = true;

        _collectionVersion = collectionVersion;
        _collectionChanged = false;

        _rprimRenderTagVersion = rprimRenderTagVersion;
        _materialTagsVersion = materialTagsVersion;
        _geomSubsetDrawItemsVersion = geomSubsetDrawItemsVersion;
    }
}

void
HdSt_RenderPass::_UpdateCommandBuffer(TfTokenVector const& renderTags)
{
    HD_TRACE_FUNCTION();

    // -------------------------------------------------------------------
    // SCHEDULE PREPARATION
    // -------------------------------------------------------------------
    // We know what must be drawn and that the stream needs to be updated,
    // so iterate over each prim, cull it and schedule it to be drawn.

    // Ensure that the drawItems are always up-to-date before building the
    // command buffers.
    _UpdateDrawItems(renderTags);

    const int batchVersion = _GetDrawBatchesVersion(GetRenderIndex());
    // Rebuild draw batches based on new draw items
    if (_drawItemsChanged) {
        _cmdBuffer.SetDrawItems(_drawItems, batchVersion, _hgi);

        _drawItemsChanged = false;
        size_t itemCount = _cmdBuffer.GetTotalSize();
        HD_PERF_COUNTER_SET(HdTokens->totalItemCount, itemCount);
    } else {
        // validate command buffer to not include expired drawItems,
        // which could be produced by migrating BARs at the new repr creation.
        _cmdBuffer.RebuildDrawBatchesIfNeeded(batchVersion, _hgi);
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

PXR_NAMESPACE_CLOSE_SCOPE
