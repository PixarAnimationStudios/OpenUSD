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

#include "pxr/pxr.h"

#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/diagnostic.h"

#include "pxr/base/arch/debugger.h"
#include "pxr/base/arch/demangle.h"

PXR_NAMESPACE_OPEN_SCOPE

int
Tf_RefPtr_UniqueChangedCounter::_AddRef(TfRefBase const *refBase)
{
    // Read the current value, and try to CAS it one greater if it looks like we
    // won't take it 1 -> 2.  If we're successful at this, we can skip the whole
    // locking business.  If instead the current count is 1, we pay the price
    // with the locking stuff.
    auto &counter = refBase->GetRefCount()._counter;

    int prevCount = counter.load();
    while (prevCount != 1) {
        if (counter.compare_exchange_weak(prevCount, prevCount+1)) {
            return prevCount;
        }
    }

    // prevCount is 1 or it changed in the meantime.
    TfRefBase::UniqueChangedListener const &listener =
        TfRefBase::_uniqueChangedListener;
    listener.lock();
    prevCount = refBase->GetRefCount()._FetchAndAdd(1);
    if (prevCount == 1) {
        listener.func(refBase, false);
    }
    listener.unlock();
    
    return prevCount;
}

bool
Tf_RefPtr_UniqueChangedCounter::_RemoveRef(TfRefBase const *refBase)
{
    // Read the current value, and try to CAS it one less if it looks like we
    // won't take it 2 -> 1.  If we're successful at this, we can skip the whole
    // locking business.  If instead the current count is 2, we pay the price
    // with the locking stuff.
    auto &counter = refBase->GetRefCount()._counter;

    int prevCount = counter.load();
    while (prevCount != 2) {
        if (counter.compare_exchange_weak(prevCount, prevCount-1)) {
            return prevCount == 1;
        }
    }

    // prevCount is 2 or it changed in the meantime.
    TfRefBase::UniqueChangedListener const &listener =
        TfRefBase::_uniqueChangedListener;
    listener.lock();
    prevCount = refBase->GetRefCount()._FetchAndAdd(-1);
    if (prevCount == 2) {
        listener.func(refBase, true);
    }
    listener.unlock();
    
    return prevCount == 1;
}

bool Tf_RefPtr_UniqueChangedCounter::_AddRefIfNonzero(
    TfRefBase const *refBase)
{
    // Read the current value, and try to CAS it one greater if it looks like we
    // won't take it 1 -> 2.  If we're successful at this, we can skip the whole
    // locking business.  If instead the current count is 1, we pay the price
    // with the locking stuff.
    auto &counter = refBase->GetRefCount()._counter;

    int prevCount = counter.load();
    while (prevCount != 0 && prevCount != 1) {
        if (counter.compare_exchange_weak(prevCount, prevCount+1)) {
            return true;
        }
    }

    // If we saw a 0, return false.
    if (prevCount == 0) {
        return false;
    }

    // Otherwise we saw a 1, so we may have to lock & invoke the unique changed
    // counter.
    TfRefBase::UniqueChangedListener const &listener =
        TfRefBase::_uniqueChangedListener;
    listener.lock();
    prevCount = counter.load();
    while (true) {
        if (prevCount == 0) {
            listener.unlock();
            return false;
        }
        if (prevCount == 1) {
            listener.func(refBase, false);
            counter.store(2, std::memory_order_relaxed);
            listener.unlock();
            return true;
        }
        if (counter.compare_exchange_weak(prevCount, prevCount+1)) {
            listener.unlock();
            return true;
        }
    }
    // Unreachable...
    listener.unlock();
    return true;
}

void
Tf_PostNullSmartPtrDereferenceFatalError(
    const TfCallContext &ctx,
    const char *typeName)
{
    Tf_DiagnosticHelper(ctx, TF_DIAGNOSTIC_FATAL_ERROR_TYPE)
        .IssueFatalError("attempted member lookup on NULL %s",
                         ArchGetDemangled(typeName).c_str());
    ArchAbort();
}

PXR_NAMESPACE_CLOSE_SCOPE
