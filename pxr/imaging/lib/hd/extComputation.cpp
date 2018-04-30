//
// Copyright 2017 Pixar
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

#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/extComputationContext.h"
#include "pxr/imaging/hd/compExtCompInputSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/base/tf/envSetting.h"

#include "pxr/base/arch/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HD_ENABLE_SHARED_EXT_COMPUTATION_DATA, 0,
                      "Enable sharing of ext computation data buffers");

/* static */
bool
HdExtComputation::_IsEnabledSharedExtComputationData()
{
    static bool enabled =
        (TfGetEnvSetting(HD_ENABLE_SHARED_EXT_COMPUTATION_DATA) == 1);
    return enabled;
}

HdExtComputation::HdExtComputation(SdfPath const &id)
 : HdSprim(id)
 , _dispatchCount(0)
 , _elementCount(0)
 , _sceneInputNames()
 , _computationInputs()
 , _computationOutputs()
 , _gpuKernelSource()
{
}

void
HdExtComputation::Sync(HdSceneDelegate *sceneDelegate,
                       HdRenderParam   *renderParam,
                       HdDirtyBits     *dirtyBits)
{
    HdExtComputation::_Sync(sceneDelegate, renderParam, dirtyBits);
}

void
HdExtComputation::_Sync(HdSceneDelegate *sceneDelegate,
                        HdRenderParam   *renderParam,
                        HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("HdExtComputation::Sync\n");

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    HdDirtyBits bits = *dirtyBits;

    if (bits & DirtyInputDesc) {
        TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("    dirty inputs\n");
        
        _sceneInputNames =
                sceneDelegate->GetExtComputationSceneInputNames(GetID());
        _computationInputs =
                sceneDelegate->GetExtComputationInputDescriptors(GetID());
    }

    if (bits & DirtyOutputDesc) {
        _computationOutputs =
                sceneDelegate->GetExtComputationOutputDescriptors(GetID());
    }

    if (bits & DirtyDispatchCount) {
        VtValue vtDispatchCount =
                sceneDelegate->Get(GetID(), HdTokens->dispatchCount);
        // For backward compatibility, allow the dispatch count to be empty.
        if (!vtDispatchCount.IsEmpty()) {
            _dispatchCount = vtDispatchCount.Get<size_t>();
        } else {
            _dispatchCount = 0;
        }
    }

    if (bits & DirtyElementCount) {
        VtValue vtElementCount =
                sceneDelegate->Get(GetID(), HdTokens->elementCount);
        // For backward compatibility, allow the element count to be empty.
        if (!vtElementCount.IsEmpty()) {
            _elementCount = vtElementCount.Get<size_t>();
        } else {
            _elementCount = 0;
        }
    }

    if (bits & DirtyKernel) {
        _gpuKernelSource = sceneDelegate->GetExtComputationKernel(GetID());
        TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("    GpuKernelSource = '%s'\n",
                _gpuKernelSource.c_str());
        // XXX we should update any created GPU computations as well
        // with the new kernel if we want to provide a good editing flow.
    }

    *dirtyBits = Clean;
}

HdDirtyBits
HdExtComputation::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

VtValue
HdExtComputation::Get(TfToken const &token) const
{
    return VtValue();
}

size_t
HdExtComputation::GetDispatchCount() const
{
    return (_dispatchCount > 0 ? _dispatchCount : _elementCount);
}

TfTokenVector
HdExtComputation::GetOutputNames() const
{
    TfTokenVector result;
    result.reserve(GetComputationOutputs().size());
    for (const HdExtComputationOutputDescriptor & outputDesc:
                GetComputationOutputs()) {
        result.push_back(outputDesc.name);
    }
    return result;
}

bool
HdExtComputation::IsInputAggregation() const
{
    // Computations with no outputs act as input aggregators, i.e.
    // schedule inputs for resolution, but don't directly schedule
    // execution of a computation.
    return GetComputationOutputs().empty();
}

PXR_NAMESPACE_CLOSE_SCOPE
