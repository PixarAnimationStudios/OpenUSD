//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/bigRWMutex.h"

PXR_NAMESPACE_OPEN_SCOPE


TfBigRWMutex::TfBigRWMutex()
    : _states(std::make_unique<_LockState []>(NumStates))
    , _writerActive(false)
{
}

void
TfBigRWMutex::_AcquireReadContended(int stateIndex)
{
    // First check _writerActive and wait until we see that set to false.
    while (true) {
        if (_writerActive) {
            std::this_thread::yield();
        }
        else if (_states[stateIndex].mutex.TryAcquireRead()) {
            break;
        }
    }
}

void
TfBigRWMutex::_AcquireWrite()
{
    while (_writerActive.exchange(true) == true) {
        // Another writer is active, wait to see false and retry.
        do {
            std::this_thread::yield();
        } while (_writerActive);
    }

    // Use the staged-acquire API that TfSpinRWMutex supplies so that we can
    // acquire the write locks while simultaneously waiting for readers on the
    // other locks to complete.  Otherwise we'd have to wait for all pending
    // readers on the Nth lock before beginning to take the N+1th lock.
    TfSpinRWMutex::_StagedAcquireWriteState
        stageStates[NumStates] { TfSpinRWMutex::_StageNotAcquired };

    bool allAcquired;
    do {
        allAcquired = true;
        for (int i = 0; i != NumStates; ++i) {
            stageStates[i] =
                _states[i].mutex._StagedAcquireWriteStep(stageStates[i]);
            allAcquired &= (stageStates[i] == TfSpinRWMutex::_StageAcquired);
        }
    } while (!allAcquired);
}

void
TfBigRWMutex::_ReleaseWrite()
{
    _writerActive = false;
    
    // Release all the write locks.
    for (_LockState *lockState = _states.get(),
             *end = _states.get() + NumStates; lockState != end;
         ++lockState) {
        lockState->mutex.ReleaseWrite();
    }
}
    
PXR_NAMESPACE_CLOSE_SCOPE
