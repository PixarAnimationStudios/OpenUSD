//
// Copyright 2018 Pixar
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

#include "pxr/imaging/hdSt/extComputation.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hd/extComputationContext.h"
#include "pxr/imaging/hd/compExtCompInputSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStExtComputation::HdStExtComputation(SdfPath const &id)
 : HdExtComputation(id)
 , _inputRange()
{
}

void
HdStExtComputation::Sync(HdSceneDelegate *sceneDelegate,
                         HdRenderParam   *renderParam,
                         HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg(
        "HdStExtComputation::Sync %s\n", GetID().GetText());

    HdExtComputation::_Sync(sceneDelegate, renderParam, dirtyBits);

    // We only commit GPU resources when directly executing a GPU computation
    // or when aggregating inputs for a downstream computation.
    if (GetGpuKernelSource().empty() && !IsInputAggregation()) {
        return;
    }

    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    HdStResourceRegistrySharedPtr const & resourceRegistry =
        boost::dynamic_pointer_cast<HdStResourceRegistry>(
                              renderIndex.GetResourceRegistry());

    HdBufferSourceVector inputs;
    for (TfToken const & inputName: GetSceneInputs()) {
        VtValue inputValue = sceneDelegate->Get(GetID(), inputName);
        size_t arraySize =
            inputValue.IsArrayValued() ? inputValue.GetArraySize() : 1;
        HdBufferSourceSharedPtr inputSource = HdBufferSourceSharedPtr(
                    new HdVtBufferSource(inputName, inputValue, arraySize));
        inputs.push_back(inputSource);
    }

    _inputRange.reset();
    if (!inputs.empty()) {
        HdBufferSpecVector bufferSpecs;
        HdBufferSpec::AddBufferSpecs(&bufferSpecs, inputs);

        _inputRange = resourceRegistry->AllocateShaderStorageBufferArrayRange(
                HdTokens->primVar, bufferSpecs);

        resourceRegistry->AddSources(_inputRange, inputs);
    }
}

VtValue
HdStExtComputation::Get(TfToken const &token) const
{
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
