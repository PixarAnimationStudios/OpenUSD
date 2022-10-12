//
// Copyright 2022 Pixar
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

#include "pxr/pxr.h"

#include "pxr/base/tf/bigRWMutex.h"

PXR_NAMESPACE_OPEN_SCOPE


TfBigRWMutex::TfBigRWMutex()
    : _states(std::make_unique<_LockState []>(NumStates))
    , _writerWaiting(false)
{
}

int
TfBigRWMutex::_AcquireReadContended(int stateIndex)
{
  retry:
    // First check _writerWaiting and wait until we see that set to false if
    // need be.
    while (_writerWaiting == true) {
        std::this_thread::yield();
    }
    
    // Now try to bump the reader count on our state index.  If we see a write
    // lock state, go back to waiting for any pending writer.  If we fail to
    // bump the count, move to the next slot (and wrap around).
    for (int i = (stateIndex + 1) % NumStates;
         ;
         i = (stateIndex + 1) % NumStates) {
        _LockState &lockState = _states[i];
        
        int stateVal = lockState.state;
        if (stateVal == WriteLocked) {
            std::this_thread::yield();
            goto retry;
        }
        
        // Otherwise try to increment the count.
        if (lockState.state.compare_exchange_strong(stateVal, stateVal+1)) {
            // Success!  Record the state we used to mark this lock as
            // acquired.
            return i;
        }
        // Otherwise we advance to the next state index and try there.
    }
}

void
TfBigRWMutex::_AcquireWrite()
{
    // First, we need to take _writerWaiting from false -> true.
    bool writerWaits = false;
    while (!_writerWaiting.compare_exchange_weak(writerWaits, true)) {
        std::this_thread::yield();
        writerWaits = false;
    }
    
    // Now, we need to wait for all pending readers to finish and lock out any
    // new ones.
    for (_LockState *lockState = _states.get(),
             *end = _states.get() + NumStates; lockState != end;
         ++lockState) {
        
        int expected = NotLocked;
        while (!lockState->state.compare_exchange_weak(expected, WriteLocked)) {
            std::this_thread::yield();
            expected = NotLocked;
        }
    }
}

void
TfBigRWMutex::_ReleaseWrite()
{
    // Restore all the read lock states to 0 and set writerWaits to false.
    for (_LockState *lockState = _states.get(),
             *end = _states.get() + NumStates; lockState != end;
         ++lockState) {
        lockState->state = NotLocked;
    }
    _writerWaiting = false;
}
    
PXR_NAMESPACE_CLOSE_SCOPE
