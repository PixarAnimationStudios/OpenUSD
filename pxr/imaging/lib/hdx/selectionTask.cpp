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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdx/selectionTask.h"

#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE


// -------------------------------------------------------------------------- //

typedef std::vector<HdBufferSourceSharedPtr> HdBufferSourceSharedPtrVector;

HdxSelectionTask::HdxSelectionTask(HdSceneDelegate* delegate,
                                   SdfPath const& id)
    : HdSceneTask(delegate, id)
    , _lastVersion(-1)
    , _hasSelection(false)
    , _selOffsetBar(nullptr)
    , _selUniformBar(nullptr)
    , _selPointColorsBar(nullptr)
{
    _params = {false, GfVec4f(), GfVec4f()};
}

void
HdxSelectionTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Note that selectionTask comes after renderTask.
}

void
HdxSelectionTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();

    SdfPath const& id = GetId();
    HdSceneDelegate* delegate = GetDelegate();
    HdRenderIndex& index = delegate->GetRenderIndex();
    HdChangeTracker& changeTracker = index.GetChangeTracker();
    HdDirtyBits bits = changeTracker.GetTaskDirtyBits(id);
    HdResourceRegistrySharedPtr const& resourceRegistry = 
        index.GetResourceRegistry();

    bool paramsChanged = bits & HdChangeTracker::DirtyParams;
    if (paramsChanged) {
        _GetSceneDelegateValue(HdTokens->params, &_params);
    }

    HdxSelectionTrackerSharedPtr sel;
    if (_GetTaskContextData(ctx, HdxTokens->selectionState, &sel)) {
        sel->Sync(&index);
    }

    // If the resource registry doesn't support uniform or single bars,
    // there's nowhere to put selection state, so don't compute it.
    if (!(resourceRegistry->HasSingleStorageAggregationStrategy()) ||
        !(resourceRegistry->HasUniformAggregationStrategy())) {
        return;
    }

    if (sel && (paramsChanged || sel->GetVersion() != _lastVersion)) {

        _lastVersion = sel->GetVersion();
        
        if (!_selOffsetBar) {

            HdBufferSpecVector offsetSpecs;
            offsetSpecs.emplace_back(HdxTokens->hdxSelectionBuffer,
                                     HdTupleType { HdTypeInt32, 1 });
            _selOffsetBar = resourceRegistry->AllocateSingleBufferArrayRange(
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
            _selUniformBar = resourceRegistry->AllocateUniformBufferArrayRange(
                                                /*role*/HdxTokens->selection,
                                                uniformSpecs,
                                                HdBufferArrayUsageHint());
        }

        if (!_selPointColorsBar) {
            HdBufferSpecVector colorSpecs;
            colorSpecs.emplace_back(HdxTokens->selectionPointColors,
                                      HdTupleType { HdTypeFloatVec4, 1 });
            _selPointColorsBar =
                resourceRegistry->AllocateSingleBufferArrayRange(
                                                /*role*/HdxTokens->selection,
                                                colorSpecs,
                                                HdBufferArrayUsageHint());
        }

        //
        // Uniforms
        //
        HdBufferSourceSharedPtrVector uniformSources;
        uniformSources.push_back(HdBufferSourceSharedPtr(
                new HdVtBufferSource(HdxTokens->selColor,
                                     VtValue(_params.selectionColor))));
        uniformSources.push_back(HdBufferSourceSharedPtr(
                new HdVtBufferSource(HdxTokens->selLocateColor,
                                     VtValue(_params.locateColor))));
        resourceRegistry->AddSources(_selUniformBar, uniformSources);

        //
        // Offsets
        //
        VtIntArray offsets;
        _hasSelection = sel->GetSelectionOffsetBuffer(&index, &offsets);
        HdBufferSourceSharedPtr offsetSource(
                new HdVtBufferSource(HdxTokens->hdxSelectionBuffer,
                                     VtValue(offsets)));
        resourceRegistry->AddSource(_selOffsetBar, offsetSource);

        //
        // Point Colors
        //
        VtVec4fArray ptColors = sel->GetSelectedPointColors();
        HdBufferSourceSharedPtr ptColorSource(
                new HdVtBufferSource(HdxTokens->selectionPointColors,
                                     VtValue(ptColors)));
        resourceRegistry->AddSource(_selPointColorsBar, ptColorSource);
    }

    if (_params.enableSelection && _hasSelection) {
        (*ctx)[HdxTokens->selectionOffsets] = _selOffsetBar;
        (*ctx)[HdxTokens->selectionUniforms] = _selUniformBar;
        (*ctx)[HdxTokens->selectionPointColors] = _selPointColorsBar;
    } else {
        (*ctx)[HdxTokens->selectionOffsets] = VtValue();
        (*ctx)[HdxTokens->selectionUniforms] = VtValue();
        (*ctx)[HdxTokens->selectionPointColors] = VtValue();
    }
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

