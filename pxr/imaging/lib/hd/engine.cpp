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
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/tokens.h"

#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


HdEngine::HdEngine() 
 : _taskContext()
{
}

HdEngine::~HdEngine()
{
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
HdEngine::Execute(HdRenderIndex& index, HdTaskSharedPtrVector const &tasks)
{
    // --------------------------------------------------------------------- //
    // DATA DISCOVERY PHASE
    // --------------------------------------------------------------------- //
    // Discover all required input data needed to render the required render
    // prim representations. At this point, we must read enough data to
    // establish the resource dependency graph, but we do not yet populate CPU-
    // or GPU-memory with data.

    // As a result of the next call, the resource registry will be populated
    // with both BufferSources that need to be resolved (possibly generating
    // data on the CPU) and computations to run on the CPU/GPU.

    TF_DEBUG(HD_ENGINE_PHASE_INFO).Msg(
            "\n"
            "==============================================================\n"
            "      HdEngine [Data Discovery Phase](RenderIndex::SyncAll)   \n"
            "--------------------------------------------------------------\n");

    index.SyncAll(tasks, &_taskContext);

    // --------------------------------------------------------------------- //
    // DATA COMMIT PHASE
    // --------------------------------------------------------------------- //
    // Having acquired handles to the data needed to update various resources,
    // we let the render delegate 'commit' these resources. These resources may
    // reside either on the CPU/GPU/both; that depends on the render delegate
    // implementation.
    TF_DEBUG(HD_ENGINE_PHASE_INFO).Msg(
            "\n"
            "==============================================================\n"
            " HdEngine [Data Commit Phase](RenderDelegate::CommitResources)\n"
            "--------------------------------------------------------------\n");
    
    HdRenderDelegate *renderDelegate = index.GetRenderDelegate();
    renderDelegate->CommitResources(&index.GetChangeTracker());

    // --------------------------------------------------------------------- //
    // EXECUTE PHASE
    // --------------------------------------------------------------------- //
    // Having updated all the necessary data buffers, we can finally execute
    // the rendering tasks.
    TF_DEBUG(HD_ENGINE_PHASE_INFO).Msg(
            "\n"
            "==============================================================\n"
            "             HdEngine [Execute Phase](Task::Execute)          \n"
            "--------------------------------------------------------------\n");

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

    // Dirty all materials
    SdfPathVector materials = index.GetSprimSubtree(HdPrimTypeTokens->material,
                                                    SdfPath::AbsoluteRootPath());

    for (SdfPathVector::iterator materialIt  = materials.begin();
                                 materialIt != materials.end();
                               ++materialIt) {

        tracker.MarkSprimDirty(*materialIt, HdChangeTracker::AllDirty);
    }

    // Invalidate shader cache in Resource Registry.
    index.GetResourceRegistry()->InvalidateShaderRegistry();

    // Fallback material
    HdMaterial *material = static_cast<HdMaterial *>(
                        index.GetFallbackSprim(HdPrimTypeTokens->material));
    material->Reload();

    // Note: Several Shaders are not currently captured in this
    // - Lighting Shaders
    // - Render Pass Shaders
    // - Culling Shader
}

PXR_NAMESPACE_CLOSE_SCOPE

