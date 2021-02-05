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
#include "pxr/imaging/hdx/selectionTask.h"

#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hdSt/resourceRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE


// -------------------------------------------------------------------------- //

using HdBufferSourceSharedPtrVector = std::vector<HdBufferSourceSharedPtr>;

HdxSelectionTask::HdxSelectionTask(HdSceneDelegate* delegate,
                                   SdfPath const& id)
    : HdTask(id)
    , _lastVersion(-1)
    , _hasSelection(false)
    , _params({false, 0.5, GfVec4f(), GfVec4f()})
    , _selOffsetBar(nullptr)
    , _selUniformBar(nullptr)
    , _selPointColorsBar(nullptr)
{
}

HdxSelectionTask::~HdxSelectionTask() = default;

void
HdxSelectionTask::Sync(HdSceneDelegate* delegate,
                       HdTaskContext* ctx,
                       HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        _GetTaskParams(delegate, &_params);

        // We track the version of selection tracker in the task
        // to see if we need need to update the uniform buffers.
        // As the params have changed, we also need to force an
        // update of the uniform buffers.  We don't have access to
        // the selection tracker (as it is in the task context)
        // so we reset the version to -1 to cause a version mismatch
        // and force the uniform update.
        _lastVersion = -1;
    }

    // Update the selected objects on the tracker. This hook point
    // allows applications to transform their notion of selected 
    // objects into Hydra rprims. This is done during the Sync phase
    // as a preparatory step to render selected prims in a separate
    // task, where the collection for the render pass needs to be
    // created during Sync.
    HdxSelectionTrackerSharedPtr sel;
    if (_GetTaskContextData(ctx, HdxTokens->selectionState, &sel)) {
        sel->UpdateSelection(&(delegate->GetRenderIndex()));
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxSelectionTask::Prepare(HdTaskContext* ctx,
                          HdRenderIndex* renderIndex)
{
    HdxSelectionTrackerSharedPtr sel;
    _GetTaskContextData(ctx, HdxTokens->selectionState, &sel);

    HdStResourceRegistrySharedPtr const& hdStResourceRegistry =
        std::dynamic_pointer_cast<HdStResourceRegistry>(
            renderIndex->GetResourceRegistry());

    // Only Storm supports buffer array range. Without its registry
    // there's nowhere to put selection state, so don't compute it.
    if (!hdStResourceRegistry) {
        return;
    }

    if (sel && (sel->GetVersion() != _lastVersion)) {

        _lastVersion = sel->GetVersion();
        
        if (!_selOffsetBar) {

            HdBufferSpecVector offsetSpecs;
            offsetSpecs.emplace_back(HdxTokens->hdxSelectionBuffer,
                                     HdTupleType { HdTypeInt32, 1 });
            _selOffsetBar = 
                hdStResourceRegistry->AllocateSingleBufferArrayRange(
                    /*role*/HdxTokens->selection,
                    offsetSpecs,
                    HdBufferArrayUsageHint());
        }

        if (!_selUniformBar) {
            HdBufferSpecVector uniformSpecs;
            uniformSpecs.emplace_back(HdxTokens->selColor,
                                      HdTupleType { HdTypeFloatVec4, 1 });
            uniformSpecs.emplace_back(HdxTokens->selLocateColor,
                                      HdTupleType { HdTypeFloatVec4, 1 });
            uniformSpecs.emplace_back(HdxTokens->occludedSelectionOpacity,
                                      HdTupleType { HdTypeFloat, 1 });
            _selUniformBar = 
                hdStResourceRegistry->AllocateUniformBufferArrayRange(
                    /*role*/HdxTokens->selection,
                    uniformSpecs,
                    HdBufferArrayUsageHint());
        }

        if (!_selPointColorsBar) {
            HdBufferSpecVector colorSpecs;
            colorSpecs.emplace_back(HdxTokens->selectionPointColors,
                                      HdTupleType { HdTypeFloatVec4, 1 });
            _selPointColorsBar =
                hdStResourceRegistry->AllocateSingleBufferArrayRange(
                    /*role*/HdxTokens->selection,
                    colorSpecs,
                    HdBufferArrayUsageHint());
        }

        //
        // Uniforms
        //
        hdStResourceRegistry->AddSources(
            _selUniformBar,
            {
                std::make_shared<HdVtBufferSource>(
                    HdxTokens->selColor,
                    VtValue(_params.selectionColor)),
                std::make_shared<HdVtBufferSource>(
                    HdxTokens->selLocateColor,
                    VtValue(_params.locateColor)),
                std::make_shared<HdVtBufferSource>(
                    HdxTokens->occludedSelectionOpacity,
                    VtValue(_params.occludedSelectionOpacity))
            });

        //
        // Offsets
        //
        VtIntArray offsets;
        _hasSelection = sel->GetSelectionOffsetBuffer(renderIndex,
                _params.enableSelection, &offsets);
        hdStResourceRegistry->AddSource(
            _selOffsetBar,
            std::make_shared<HdVtBufferSource>(
                HdxTokens->hdxSelectionBuffer,
                VtValue(offsets)));

        //
        // Point Colors
        //
        const VtVec4fArray ptColors = sel->GetSelectedPointColors();
        hdStResourceRegistry->AddSource(
            _selPointColorsBar,
            std::make_shared<HdVtBufferSource>(
                HdxTokens->selectionPointColors,
                VtValue(ptColors)));
    }

    (*ctx)[HdxTokens->selectionOffsets] = _selOffsetBar;
    (*ctx)[HdxTokens->selectionUniforms] = _selUniformBar;
    (*ctx)[HdxTokens->selectionPointColors] = _selPointColorsBar;
}

void
HdxSelectionTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Note that selectionTask comes after renderTask.
}


// -------------------------------------------------------------------------- //
// VtValue requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out,
                         const HdxSelectionTaskParams& pv)
{
    out << pv.enableSelection << " ";
    out << pv.selectionColor << " ";
    out << pv.locateColor;
    return out;
}

bool operator==(const HdxSelectionTaskParams& lhs,
                const HdxSelectionTaskParams& rhs) {
    return lhs.enableSelection == rhs.enableSelection
        && lhs.selectionColor == rhs.selectionColor 
        && lhs.locateColor == rhs.locateColor;
}

bool operator!=(const HdxSelectionTaskParams& lhs,
                const HdxSelectionTaskParams& rhs) {
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE

