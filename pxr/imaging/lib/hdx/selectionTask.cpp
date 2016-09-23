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

// -------------------------------------------------------------------------- //

typedef std::vector<HdBufferSourceSharedPtr> HdBufferSourceSharedPtrVector;

HdxSelectionTask::HdxSelectionTask(HdSceneDelegate* delegate,
                                                   SdfPath const& id)
    : HdSceneTask(delegate, id)
    , _lastVersion(-1)
    , _offsetMin(0)
    , _offsetMax(-1)
    , _hasSelection(false)
    , _selOffsetBar(nullptr)
    , _selValueBar(nullptr)
{
    _params = {false, GfVec4f(), GfVec4f(), GfVec4f()};
}

void
HdxSelectionTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

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
    HdChangeTracker::DirtyBits bits = changeTracker.GetTaskDirtyBits(id);
    HdResourceRegistry* resourceRegistry = &HdResourceRegistry::GetInstance();

    bool paramsChanged = bits & HdChangeTracker::DirtyParams;
    if (paramsChanged) {
        _GetSceneDelegateValue(HdTokens->params, &_params);
    }

    HdxSelectionTrackerSharedPtr sel;
    if (_GetTaskContextData(ctx, HdxTokens->selectionState, &sel)) {
        sel->Sync(&index);
    }

    if (sel and (paramsChanged or sel->GetVersion() != _lastVersion)) {

        _lastVersion = sel->GetVersion();
        VtIntArray offsets;
        VtIntArray values;
        
        _hasSelection = sel->GetBuffers(&index, &offsets);
        if (not _selOffsetBar) {

            HdBufferSpecVector offsetSpecs;
            offsetSpecs.push_back(HdBufferSpec(
                                    HdxTokens->hdxSelectionBuffer, GL_INT, 1));
            _selOffsetBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                                /*role*/HdxTokens->selection,
                                                offsetSpecs);

            HdBufferSpecVector uniformSpecs;
            uniformSpecs.push_back(
                        HdBufferSpec(HdxTokens->selColor, GL_FLOAT, 4));
            uniformSpecs.push_back(
                        HdBufferSpec(HdxTokens->selLocateColor, GL_FLOAT, 4));
            uniformSpecs.push_back(
                        HdBufferSpec(HdxTokens->selMaskColor, GL_FLOAT, 4));
            _selUniformBar = resourceRegistry->AllocateUniformBufferArrayRange(
                                                /*role*/HdxTokens->selection,
                                                uniformSpecs);
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
        uniformSources.push_back(HdBufferSourceSharedPtr(
                new HdVtBufferSource(HdxTokens->selMaskColor,
                                     VtValue(_params.maskColor))));
        resourceRegistry->AddSources(_selUniformBar, uniformSources);

        //
        // Offsets
        //
        HdBufferSourceSharedPtr offsetSource(
                new HdVtBufferSource(HdxTokens->hdxSelectionBuffer,
                                     VtValue(offsets)));
        resourceRegistry->AddSource(_selOffsetBar, offsetSource);
    }

    if (_params.enableSelection and _hasSelection) {
        (*ctx)[HdxTokens->selectionOffsets] = _selOffsetBar;
        (*ctx)[HdxTokens->selectionUniforms] = _selUniformBar;
    } else {
        (*ctx)[HdxTokens->selectionOffsets] = VtValue();
        (*ctx)[HdxTokens->selectionUniforms] = VtValue();
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
    out << pv.locateColor << " ";
    out << pv.maskColor;
    return out;
}

bool operator==(const HdxSelectionTaskParams& lhs,
                const HdxSelectionTaskParams& rhs) {
    return lhs.enableSelection == rhs.enableSelection
        && lhs.selectionColor == rhs.selectionColor 
        && lhs.locateColor == rhs.locateColor
        && lhs.maskColor == rhs.maskColor;
}

bool operator!=(const HdxSelectionTaskParams& lhs,
                const HdxSelectionTaskParams& rhs) {
    return not(lhs == rhs);
}
