//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/parallelTaskSync.h"

PXR_NAMESPACE_OPEN_SCOPE

void
VdfParallelTaskSync::Reset(const size_t num)
{
    // Rewind the waitlists, so memory allocated to waitlist nodes does not
    // grow beyond this point.
    _waitlists.Rewind();

    // If the size of the graph has changed, we need to allocate sufficiently
    // large heap memory.
    if (num > _num) {
        _state.reset(new std::atomic<uint8_t>[num]);
        _waiting.reset(new VdfParallelTaskWaitlist::HeadPtr[num]);
        _num = num;
    }

    // We need to clear out the task states and waiting queue heads. Note that
    // this is not an atomic operation.
    if (num) {
        char *const state = reinterpret_cast<char*>(_state.get());
        memset(state, 0, sizeof(std::atomic<uint8_t>) * num);
        char *const waiting = reinterpret_cast<char*>(_waiting.get());
        memset(waiting, 0, sizeof(VdfParallelTaskWaitlist::HeadPtr) * num);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
