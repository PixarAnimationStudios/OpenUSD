//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/extComputation.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HD_ENABLE_SHARED_EXT_COMPUTATION_DATA, 1,
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

HdExtComputation::~HdExtComputation() = default;

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

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }
    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("HdExtComputation::Sync for %s"
        " (dirty bits = 0x%x)\n", GetId().GetText(), *dirtyBits);

    HdDirtyBits bits = *dirtyBits;

    if (bits & DirtyInputDesc) {
        TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("    dirty inputs\n");
        
        _sceneInputNames =
                sceneDelegate->GetExtComputationSceneInputNames(GetId());
        _computationInputs =
                sceneDelegate->GetExtComputationInputDescriptors(GetId());
    }

    if (bits & DirtyOutputDesc) {
        _computationOutputs =
                sceneDelegate->GetExtComputationOutputDescriptors(GetId());
    }

    if (bits & DirtyDispatchCount) {
        VtValue vtDispatchCount =
                sceneDelegate->GetExtComputationInput(GetId(),
                                                      HdTokens->dispatchCount);
        // For backward compatibility, allow the dispatch count to be empty.
        if (!vtDispatchCount.IsEmpty()) {
            _dispatchCount = vtDispatchCount.Get<size_t>();
        } else {
            _dispatchCount = 0;
        }
    }

    if (bits & DirtyElementCount) {
        VtValue vtElementCount =
                sceneDelegate->GetExtComputationInput(GetId(),
                                                      HdTokens->elementCount);
        // For backward compatibility, allow the element count to be empty.
        if (!vtElementCount.IsEmpty()) {
            _elementCount = vtElementCount.Get<size_t>();
        } else {
            _elementCount = 0;
        }
    }

    if (bits & DirtyKernel) {
        _gpuKernelSource = sceneDelegate->GetExtComputationKernel(GetId());
        TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("    GpuKernelSource = '%s'\n",
                _gpuKernelSource.c_str());
        // XXX we should update any created GPU computations as well
        // with the new kernel if we want to provide a good editing flow.
    }

    // Clear processed bits
    *dirtyBits &= ~(DirtyInputDesc | DirtyOutputDesc | DirtyDispatchCount |
                     DirtyElementCount | DirtyKernel);
    // XXX: DirtyCompInput isn't used yet.
    *dirtyBits &= ~DirtyCompInput;
}

HdDirtyBits
HdExtComputation::GetInitialDirtyBitsMask() const
{
    return AllDirty;
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
