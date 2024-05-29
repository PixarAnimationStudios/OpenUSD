//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    , _params({false, false, 0.5, GfVec4f(), GfVec4f()})
    , _selOffsetBar(nullptr)
    , _selUniformBar(nullptr)
    , _pointColorsBufferSize(0)
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

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxSelectionTask::Prepare(HdTaskContext* ctx,
                          HdRenderIndex* renderIndex)
{
    HdxSelectionTrackerSharedPtr sel;
    if (_GetTaskContextData(ctx, HdxTokens->selectionState, &sel)) {
        // Update the Hydra selection held by the tracker. This hook point
        // allows applications to transform their notion of selected 
        // objects into Hydra selection entries.
        sel->UpdateSelection(renderIndex);
    }

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
                    HdBufferArrayUsageHintBitsStorage);
        }

        const VtVec4fArray ptColors = sel->GetSelectedPointColors(renderIndex);
        const size_t numPtColors = ptColors.size();
        if (!_selUniformBar ||
            numPtColors > _pointColorsBufferSize) {

            // Allocate space for a small number of colors to avoid shader
            // permutations from different number of selected point colors.
            constexpr size_t s_minNumPointColors = 5;
            _pointColorsBufferSize =
                std::max<size_t>(numPtColors, s_minNumPointColors);

            HdBufferSpecVector uniformSpecs;
            uniformSpecs.emplace_back(HdxTokens->selColor,
                                      HdTupleType { HdTypeFloatVec4, 1 });
            uniformSpecs.emplace_back(HdxTokens->selLocateColor,
                                      HdTupleType { HdTypeFloatVec4, 1 });
            uniformSpecs.emplace_back(HdxTokens->occludedSelectionOpacity,
                                      HdTupleType { HdTypeFloat, 1 });
            uniformSpecs.emplace_back(HdxTokens->selectionPointColors,
                                      HdTupleType { HdTypeFloatVec4,
                                                    _pointColorsBufferSize});
            _selUniformBar = 
                hdStResourceRegistry->AllocateUniformBufferArrayRange(
                    /*role*/HdxTokens->selection,
                    uniformSpecs,
                    HdBufferArrayUsageHintBitsUniform);
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
                    VtValue(_params.occludedSelectionOpacity)),
                std::make_shared<HdVtBufferSource>(
                    HdxTokens->selectionPointColors,
                    VtValue(ptColors),
                    ptColors.size())
            });

        //
        // Offsets
        //
        VtIntArray offsets;
        _hasSelection = sel->GetSelectionOffsetBuffer(
                renderIndex,
                _params.enableSelectionHighlight,
                _params.enableLocateHighlight,
                &offsets);
        hdStResourceRegistry->AddSource(
            _selOffsetBar,
            std::make_shared<HdVtBufferSource>(
                HdxTokens->hdxSelectionBuffer,
                VtValue(offsets)));
    }

    (*ctx)[HdxTokens->selectionOffsets] = _selOffsetBar;
    (*ctx)[HdxTokens->selectionUniforms] = _selUniformBar;
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
    out << pv.enableSelectionHighlight << " ";
    out << pv.enableLocateHighlight << " ";
    out << pv.selectionColor << " ";
    out << pv.locateColor;
    return out;
}

bool operator==(const HdxSelectionTaskParams& lhs,
                const HdxSelectionTaskParams& rhs) {
    return lhs.enableSelectionHighlight == rhs.enableSelectionHighlight
        && lhs.enableLocateHighlight == rhs.enableLocateHighlight
        && lhs.selectionColor == rhs.selectionColor 
        && lhs.locateColor == rhs.locateColor;
}

bool operator!=(const HdxSelectionTaskParams& lhs,
                const HdxSelectionTaskParams& rhs) {
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE

