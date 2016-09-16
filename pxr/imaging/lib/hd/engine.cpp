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

#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/task.h"

#include <sstream>

HdEngine::HdEngine() 
 : _resourceRegistry(&HdResourceRegistry::GetInstance())
 , _context()
{
}

HdEngine::~HdEngine()
{
    /*NOTHING*/
}

void 
HdEngine::SetTaskContextData(const TfToken &id, VtValue &data)
{
    // See if the token exists in the context and if not add it.
    std::pair<HdTaskContext::iterator, bool> result = _context.emplace(id, data);
    if (not result.second) {
        // Item wasn't new, so need to update it
        result.first->second = data;
    }
}

void
HdEngine::RemoveTaskContextData(const TfToken &id)
{
    _context.erase(id);
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

    // Sync the draw targets
    index.SyncDrawTargets();

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
        if (not TF_VERIFY(*it)) {
            continue;
        }
        (*it)->Sync(&_context);
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
        (*it)->Execute(&_context);
    }
}
