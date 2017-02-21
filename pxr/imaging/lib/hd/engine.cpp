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
#include "pxr/imaging/hd/engine.h"

#include "pxr/imaging/gal/delegateRegistry.h"
#include "pxr/imaging/gal/delegate.h"

#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderDelegateRegistry.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderIndexManager.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/shader.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/tokens.h"

#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


// Placeholder instead for the default delegates for an unintalized value
// as the empty token is valid.
const TfToken HdEngine::_UninitalizedId = TfToken("__UNINIT__");


HdEngine::HdEngine() 
 : _activeContexts()
 , _inactiveContexts()
 , _activeContextsDirty(false)
 , _defaultRenderDelegateId(_UninitalizedId)
 , _defaultGalDelegateId(_UninitalizedId)
 , _resourceRegistry(&HdResourceRegistry::GetInstance())
 , _taskContext()
{
}

HdEngine::~HdEngine()
{
    if (!_activeContexts.empty() ||
        !_inactiveContexts.empty()) {
        TF_CODING_ERROR("Shutting down hydra with contexts still alive");

        while (!_activeContexts.empty())
        {
            _DeleteContext(&_activeContexts.front());
        }

        while (!_inactiveContexts.empty())
        {
            _DeleteContext(&_inactiveContexts.front());
        }

    }
}

// static
void
HdEngine::GetRenderDelegateDescs(HfPluginDelegateDescVector *pDelegates)
{
    if (pDelegates == nullptr) {
        TF_CODING_ERROR("Null pointer passed to GetRenderDelegateDescs");
        return;
    }

    HdRenderDelegateRegistry::GetInstance().GetDelegateDescs(pDelegates);
}

// static
void
HdEngine::GetGalDelegateDescs(HfPluginDelegateDescVector *pDelegates)
{
    if (pDelegates == nullptr) {
        TF_CODING_ERROR("Null pointer passed to GetRenderDelegateDescs");
        return;
    }

    GalDelegateRegistry::GetInstance().GetDelegateDescs(pDelegates);
}

void
HdEngine::SetDefaultRenderDelegateId(const TfToken &renderDelegateId)
{
    HdRenderDelegateRegistry &renderRegistry =
                                        HdRenderDelegateRegistry::GetInstance();

    if (renderDelegateId.IsEmpty()) {
        _InitalizeDefaultRenderDelegateId();
    } else  if (renderRegistry.IsRegisteredDelegate(renderDelegateId)) {
        _defaultRenderDelegateId = renderDelegateId;
    } else {
        TF_CODING_ERROR("Unknown render delegate id while setting default : %s",
                        renderDelegateId.GetText());
    }
}

void
HdEngine::SetDefaultGalDelegateId(const TfToken &galDelegateId)
{
    GalDelegateRegistry &galRegistry = GalDelegateRegistry::GetInstance();

    if (galDelegateId.IsEmpty()) {
        _InitalizeDefaultGalDelegateId();
    } else if (galDelegateId == HdDelegateTokens->none ) {
        // Special None token.
        _defaultGalDelegateId = HdDelegateTokens->none;
    } else if (galRegistry.IsRegisteredDelegate(galDelegateId)) {
        _defaultGalDelegateId = galDelegateId;
    } else {
        TF_CODING_ERROR("Unknown Gal delegate id while setting default: %s",
                        galDelegateId.GetText());
    }
}

const TfToken &
HdEngine::GetDefaultRenderDelegateId()
{
    if (_defaultRenderDelegateId == _UninitalizedId)
    {
        _InitalizeDefaultRenderDelegateId();
    }

    return _defaultRenderDelegateId;
}

const TfToken &
HdEngine::GetDefaultGalDelegateId()
{
    if (_defaultGalDelegateId == _UninitalizedId)
    {
        _InitalizeDefaultGalDelegateId();
    }

    return _defaultGalDelegateId;
}

HdContext *
HdEngine::CreateContextWithDefaults()
{
    // Create Default Render Delegate
    HdRenderDelegateRegistry &renderRegistry =
                                        HdRenderDelegateRegistry::GetInstance();

    const TfToken &defaultRenderDelegateId = GetDefaultRenderDelegateId();
    HdRenderDelegate *renderDelegate =
                      renderRegistry.GetRenderDelegate(defaultRenderDelegateId);


    // Create Default Gal
    GalDelegateRegistry &galRegistry = GalDelegateRegistry::GetInstance();

    TfToken defaultGalId;
    if (renderDelegate) {
        defaultGalId = renderDelegate->GetDefaultGalId();
    }
    if (defaultGalId.IsEmpty()) {
        defaultGalId = GetDefaultGalDelegateId();
    }

    GalDelegate *galDelegate = nullptr;
    // Gal is optional, so default could be empty.
    if (defaultGalId.IsEmpty()) {
        defaultGalId = HdDelegateTokens->none;
    } else {
        // Gal could be explicitly disabled.
        if (defaultGalId != HdDelegateTokens->none)
        {
            galDelegate = galRegistry.GetGalDelegate(defaultGalId);

            // However if specified as a default it is expected to be there,
            // so this is a failure.
            if (!galDelegate) {
                renderRegistry.ReleaseDelegate(renderDelegate);
                return nullptr;
            }
        }
    }

    // Create Default Render Index.
    Hd_RenderIndexManager &riMgr = Hd_RenderIndexManager::GetInstance();
    HdRenderIndex *index = riMgr.CreateRenderIndex();


    HdContext *context = nullptr;
    if (renderDelegate &&
        index          &&
        (defaultGalId == HdDelegateTokens->none || galDelegate)) {

        context = _CreateContext(renderDelegate,
                                 galDelegate,
                                 index);
    }

    // If an error happened cleanup.
    if (context == nullptr) {
        // Registries and Managers are expected to handle null pointers.
        renderRegistry.ReleaseDelegate(renderDelegate);
        galRegistry.ReleaseDelegate(galDelegate);
        riMgr.ReleaseRenderIndex(index);
        return nullptr;
    }

    // Provide the Render Index with the Render Delegate, so it can create
    // prims using the delegate
    index->SetRenderDelegate(renderDelegate);

    if (!index->CreateFallbackPrims()) {
        DestroyContext(context);
        return nullptr;
    }

    return context;
}

HdContext *
HdEngine::CreateSharedContext(HdContext *srcContext)
{
    if (srcContext == nullptr) {
        TF_CODING_ERROR("Null context passed to CreateSharedContext");
        return nullptr;
    }

    HdRenderDelegate *renderDelegate = srcContext->GetRenderDelegate();
    GalDelegate      *galDelegate    = srcContext->GetGalDelegate();
    HdRenderIndex    *index          = srcContext->GetRenderIndex();

    // Gal's optional.
    if ((renderDelegate == nullptr) ||
        (index          == nullptr)) {
        return nullptr;
    }

    HdRenderDelegateRegistry &renderRegistry =
                                        HdRenderDelegateRegistry::GetInstance();
    GalDelegateRegistry &galRegistry = GalDelegateRegistry::GetInstance();
    Hd_RenderIndexManager &riMgr = Hd_RenderIndexManager::GetInstance();


    renderRegistry.AddDelegateReference(renderDelegate);
    riMgr.AddRenderIndexReference(index);

    if (galDelegate != nullptr) {
        galRegistry.AddDelegateReference(galDelegate);
    }

    HdContext *context = _CreateContext(renderDelegate,
                                        galDelegate,
                                        index);

    // If an error happened cleanup.
    if (context == nullptr) {
        renderRegistry.ReleaseDelegate(renderDelegate);
        galRegistry.ReleaseDelegate(galDelegate);
        riMgr.ReleaseRenderIndex(index);
    }

    return context;
}

HdContext *
HdEngine::CreateContext(const TfToken &renderDelegateId,
                        const TfToken &galDelegateId,
                        HdRenderIndex *index)
{
    if (renderDelegateId.IsEmpty() ||
        galDelegateId.IsEmpty()    ||
        (index == nullptr)) {
        TF_CODING_ERROR("Failed to specify delegates to use or render index");
        return nullptr;
    }

    HdRenderDelegateRegistry &renderRegistry =
                                        HdRenderDelegateRegistry::GetInstance();
    GalDelegateRegistry &galRegistry = GalDelegateRegistry::GetInstance();
    Hd_RenderIndexManager &riMgr = Hd_RenderIndexManager::GetInstance();

    // Check render index compatibility
    const TfToken &riRenderDelegateType = index->GetRenderDelegateType();
    if ((!riRenderDelegateType.IsEmpty()) &&
        (riRenderDelegateType != renderDelegateId)) {
        TF_CODING_ERROR("Render Index is bound to render delegate id %s, "
                        "which conflicts with request id %s",
                        riRenderDelegateType.GetText(),
                        renderDelegateId.GetText());
        return nullptr;
    }


    HdRenderDelegate *renderDelegate =
                             renderRegistry.GetRenderDelegate(renderDelegateId);

    if (!riMgr.AddRenderIndexReference(index)) {
        // Index not registered, we shouldn't use it.
        index = nullptr;
    }

    GalDelegate *galDelegate = nullptr;
    if (galDelegateId != HdDelegateTokens->none)
    {
        galDelegate = galRegistry.GetGalDelegate(galDelegateId);

    }


    HdContext *context = nullptr;
    // Gal is optional.
    if ((renderDelegate != nullptr) &&
        (index != nullptr) &&
        ((galDelegate != nullptr) ||
         (galDelegateId == HdDelegateTokens->none))) {

        context = _CreateContext(renderDelegate,
                                 galDelegate,
                                 index);
    }

    // If an error happened cleanup.
    if (context == nullptr) {
        // Report Error source
        if (renderDelegate == nullptr) {
            TF_CODING_ERROR("Render Delegate %s not found",
                            renderDelegateId.GetText());
        }

        if ((galDelegate == nullptr) &&
            (galDelegateId != HdDelegateTokens->none)) {
            TF_CODING_ERROR("Gal Delegate %s not found",
                            galDelegateId.GetText());
        }

        renderRegistry.ReleaseDelegate(renderDelegate);
        galRegistry.ReleaseDelegate(galDelegate);
        riMgr.ReleaseRenderIndex(index);

        return nullptr;
    }

    // Provide the Render Index with the Render Delegate, so it can create
    // prims using the delegate.
    //
    // The assignment of the render delegate to the render index, initializes
    // that render index with render delegate specific state, so we want
    // to ensure that the context creation was success before doing this
    // and we also don't want to repeat the operation if it has already been
    // performed.
    if (riRenderDelegateType.IsEmpty()) {
        index->SetRenderDelegate(renderDelegate);

        if (!index->CreateFallbackPrims()) {
            DestroyContext(context);
            return nullptr;
        }
    }

    return context;
}

void
HdEngine::DestroyContext(HdContext *context)
{
    if (context == nullptr) {
        // This doesn't throw an error, so that it can be called in the
        // event of a failure.
        return;
    }

    HdRenderDelegate *renderDelegate = context->GetRenderDelegate();
    GalDelegate      *galDelegate    = context->GetGalDelegate();
    HdRenderIndex    *index          = context->GetRenderIndex();

    _DeleteContext(context);

    HdRenderDelegateRegistry &renderRegistry =
                                        HdRenderDelegateRegistry::GetInstance();
    GalDelegateRegistry &galRegistry = GalDelegateRegistry::GetInstance();
    Hd_RenderIndexManager &riMgr = Hd_RenderIndexManager::GetInstance();

    renderRegistry.ReleaseDelegate(renderDelegate);
    galRegistry.ReleaseDelegate(galDelegate);
    riMgr.ReleaseRenderIndex(index);
}

void
HdEngine::ActivateContext(HdContext *context)
{
    if (context == nullptr) {
        TF_CODING_ERROR("Null context passed to activate context");
        return;
    }

    // As we don't know what list the context was originally in, assume it was
    // inactive.
    context->unlink();
    _activeContexts.push_front(*context);
    _activeContextsDirty = true;
}

void
HdEngine::DeactivateContext(HdContext *context)
{
    if (context == nullptr) {
        TF_CODING_ERROR("Null context passed to deactivate context");
        return;
    }

    // As we don't know what list the context was originally in, assume it was
    // active.
    context->unlink();
    _inactiveContexts.push_front(*context);
    _activeContextsDirty = true;
}

// static
HdRenderIndex *
HdEngine::CreateRenderIndex()
{
    Hd_RenderIndexManager &riMgr = Hd_RenderIndexManager::GetInstance();

    return riMgr.CreateRenderIndex();
}

// static
void
HdEngine::AddRenderIndexReference(HdRenderIndex *renderIndex)
{
    if (renderIndex == nullptr) {
        TF_CODING_ERROR("Null context passed to add render index reference");
        return;
    }

    Hd_RenderIndexManager &riMgr = Hd_RenderIndexManager::GetInstance();

    riMgr.AddRenderIndexReference(renderIndex);
}

// static
void
HdEngine::ReleaseRenderIndex(HdRenderIndex *renderIndex)
{
    if (renderIndex == nullptr) {
        // No error reported as releasing null in the event of an error
        // is desirable use-case.
        return;
    }

    Hd_RenderIndexManager &riMgr = Hd_RenderIndexManager::GetInstance();

    riMgr.ReleaseRenderIndex(renderIndex);
}

void 
HdEngine::SetTaskContextData(const TfToken &id, VtValue &data)
{
    // See if the token exists in the context and if not add it.
    std::pair<HdTaskContext::iterator, bool> result =
                                                 _taskContext.emplace(id, data);
    if (!result.second) {
        // Item wasn't new, so need to update it
        result.first->second = data;
    }
}

void
HdEngine::RemoveTaskContextData(const TfToken &id)
{
    _taskContext.erase(id);
}

void
HdEngine::_InitCaps() const
{
    // Make sure we initialize caps in main thread.
    HdRenderContextCaps::GetInstance();
}

void 
HdEngine::Draw(HdRenderIndex& index,
               HdRenderPassSharedPtr const &renderPass,
               HdRenderPassStateSharedPtr const &renderPassState)
{
    // DEPRECATED : use Execute() instead

    // XXX: remaining client codes are
    //
    //   HdxIntersector::Query
    //   Hd_TestDriver::Draw
    //   UsdImaging_TestDriver::Draw
    //   PxUsdGeomGL_TestDriver::Draw
    //   HfkCurvesVMP::draw
    //

    HD_TRACE_FUNCTION();
    _InitCaps();

    // Sync the scene state prims
    index.SyncSprims();

    renderPass->Sync();
    renderPassState->Sync();

    // Process pending dirty lists.
    index.SyncAll();

    // Commit all pending source data.
    _resourceRegistry->Commit();

    if (index.GetChangeTracker().IsGarbageCollectionNeeded()) {
        _resourceRegistry->GarbageCollect();
        index.GetChangeTracker().ClearGarbageCollectionNeeded();
        index.GetChangeTracker().MarkAllCollectionsDirty();
    }

    _resourceRegistry->GarbageCollectDispatchBuffers();

    renderPassState->Bind();
    renderPass->Execute(renderPassState);
    renderPassState->Unbind();
}

void
HdEngine::Execute(HdRenderIndex& index, HdTaskSharedPtrVector const &tasks)
{
    // The following order is important, be careful.
    //
    // If _SyncGPU updates topology varying prims, it triggers both:
    //   1. changing drawing coordinate and bumps up the global collection
    //      version to invalidate the (indirect) batch.
    //   2. marking garbage collection needed so that the unused BAR
    //      resources will be reclaimed.
    //   Also resizing ranges likely cause the buffer reallocation
    //   (==drawing coordinate changes) anyway.
    //
    // Note that the garbage collection also changes the drawing coordinate,
    // so the collection should be invalidated in that case too.
    //
    // Once we reflect all conditions which provoke the batch recompilation
    // into the collection dirtiness, we can call
    // HdRenderPass::GetCommandBuffer() to get the right batch.

    _InitCaps();

    // Sync the scene state prims
    index.SyncSprims();

    // --------------------------------------------------------------------- //
    // DATA DISCOVERY PHASE
    // --------------------------------------------------------------------- //
    // Discover all required input data needed to render the required render
    // prim representations. At this point, we must read enough data to
    // establish the resource dependency graph, but we do not yet populate CPU-
    // nor GPU-memory with data.

    // As a result of the next call, the resource registry will be populated
    // with both BufferSources that need to be resolved (possibly generating
    // data on the CPU) and computations to run on the GPU.

    // could be in parallel... but how?
    // may be just gathering dirtyLists at first, and then index->sync()?
    TF_FOR_ALL(it, tasks) {
        if (!TF_VERIFY(*it)) {
            continue;
        }
        (*it)->Sync(&_taskContext);
    }

    // Process all pending dirty lists
    index.SyncAll();

    // --------------------------------------------------------------------- //
    // RESOLVE, COMPUTE & COMMIT PHASE
    // --------------------------------------------------------------------- //
    // All the required input data is now resident in memory, next we must: 
    //
    //     1) Execute compute as needed for normals, tessellation, etc.
    //     2) Commit resources to the GPU.
    //     3) Update any scene-level acceleration structures.

    // Commit all pending source data.
    _resourceRegistry->Commit();

    if (index.GetChangeTracker().IsGarbageCollectionNeeded()) {
        _resourceRegistry->GarbageCollect();
        index.GetChangeTracker().ClearGarbageCollectionNeeded();
        index.GetChangeTracker().MarkAllCollectionsDirty();
    }

    // see bug126621. currently dispatch buffers need to be released
    //                more frequently than we expect.
    _resourceRegistry->GarbageCollectDispatchBuffers();

    TF_FOR_ALL(it, tasks) {
        (*it)->Execute(&_taskContext);
    }
}

void
HdEngine::ReloadAllShaders(HdRenderIndex& index)
{
    HdChangeTracker &tracker = index.GetChangeTracker();

    // 1st dirty all rprims, so they will trigger shader reload
    tracker.MarkAllRprimsDirty(HdChangeTracker::AllDirty);

    // Dirty all surface shaders
    SdfPathVector shaders = index.GetSprimSubtree(HdPrimTypeTokens->shader,
                                                  SdfPath::EmptyPath());

    for (SdfPathVector::iterator shaderIt  = shaders.begin();
                                 shaderIt != shaders.end();
                               ++shaderIt) {

        tracker.MarkSprimDirty(*shaderIt, HdChangeTracker::AllDirty);
    }

    // Invalidate Geometry shader cache in Resource Registry.
    _resourceRegistry->InvalidateGeometricShaderRegistry();

    // Fallback Shader
    HdShader *shader = static_cast<HdShader *>(
                              index.GetFallbackSprim(HdPrimTypeTokens->shader));
    shader->Reload();


    // Note: Several Shaders are not currently captured in this
    // - Lighting Shaders
    // - Render Pass Shaders
    // - Culling Shader

}

HdContext *
HdEngine::_CreateContext(HdRenderDelegate *renderDelegate,
                         GalDelegate      *galDelegate,
                         HdRenderIndex    *index)
{
    HdContext *context = new HdContext(renderDelegate, galDelegate, index);

    if (!TF_VERIFY(context != nullptr)) {
        return nullptr;
    }

    _activeContexts.push_front(*context);
    return context;
}

void
HdEngine::_DeleteContext(HdContext *context)
{
    context->unlink();
    delete context;
}

void
HdEngine::_InitalizeDefaultRenderDelegateId()
{
    HdRenderDelegateRegistry &renderRegistry =
                                        HdRenderDelegateRegistry::GetInstance();

    _defaultRenderDelegateId = renderRegistry.GetDefaultDelegateId();
}

void
HdEngine::_InitalizeDefaultGalDelegateId()
{
    GalDelegateRegistry &galRegistry = GalDelegateRegistry::GetInstance();

    _defaultGalDelegateId = galRegistry.GetDefaultDelegateId();
}

PXR_NAMESPACE_CLOSE_SCOPE

