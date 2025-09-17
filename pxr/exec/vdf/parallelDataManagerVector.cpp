//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/parallelDataManagerVector.h"

#include "pxr/exec/vdf/dataManagerAllocator.h"

#include "pxr/base/tf/staticData.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

// The executor data vector allocator shared accross instances of this class.
static
    TfStaticData<Vdf_DataManagerAllocator<Vdf_ParallelExecutorDataVector>>
    _allocator;

VdfParallelDataManagerVector::~VdfParallelDataManagerVector()
{
    _allocator->DeallocateLater(_data);
}

void
VdfParallelDataManagerVector::Resize(const VdfNetwork &network)
{
    // Allocate a new Vdf_ParallelExecutorDataVector if necessary. 
    if (!_data) {
        _data = _allocator->Allocate(network);
    }

    // Otherwise, make sure to resize our current instance.
    else {
        _data->Resize(network);
    }
}

void
VdfParallelDataManagerVector::ClearDataForOutput(const VdfId outputId)
{
    // Clear the data associated with the given output (if it exists).
    if (_data) {
        const DataHandle dataHandle = _data->GetDataHandle(outputId);
        if (IsValidDataHandle(dataHandle)) {
            _data->Reset(dataHandle, outputId);
        }
    }
}

void
VdfParallelDataManagerVector::Clear()
{
    if (!_data) {
        return;
    }

    TRACE_FUNCTION();

    _data->Clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
