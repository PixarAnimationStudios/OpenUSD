//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/requestTracker.h"

#include "pxr/exec/exec/debugCodes.h"
#include "pxr/exec/exec/requestImpl.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

Exec_RequestTracker::~Exec_RequestTracker()
{
    TRACE_FUNCTION();

    TF_DEBUG(EXEC_REQUEST_EXPIRATION)
        .Msg("[Exec_RequestTracker] Expiring %zu requests\n",
             _requests.size());

    for (Exec_RequestImpl * const impl : _requests) {
        impl->Expire();
    }
}

void
Exec_RequestTracker::Insert(Exec_RequestImpl *impl)
{
    bool inserted;
    {
        TfSpinMutex::ScopedLock lock{_requestsMutex};
        inserted = _requests.insert(impl).second;
    }
    TF_VERIFY(inserted);
}

void
Exec_RequestTracker::Remove(Exec_RequestImpl *impl)
{
    bool erased;
    {
        TfSpinMutex::ScopedLock lock{_requestsMutex};
        erased = _requests.erase(impl);
    }
    TF_VERIFY(erased);
}

void
Exec_RequestTracker::DidInvalidateComputedValues(
    const Exec_AuthoredValueInvalidationResult &invalidationResult)
{
    ParallelForEachRequest([&invalidationResult](Exec_RequestImpl &impl) {
        impl.DidInvalidateComputedValues(invalidationResult);
    });
}

void
Exec_RequestTracker::DidInvalidateComputedValues(
    const Exec_DisconnectedInputsInvalidationResult &invalidationResult)
{
    ParallelForEachRequest([&invalidationResult](Exec_RequestImpl &impl) {
        impl.DidInvalidateComputedValues(invalidationResult);
    });
}

void
Exec_RequestTracker::DidChangeTime(
    const Exec_TimeChangeInvalidationResult &invalidationResult) const
{
    ParallelForEachRequest([&invalidationResult](Exec_RequestImpl &impl) {
        impl.DidChangeTime(invalidationResult);
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
