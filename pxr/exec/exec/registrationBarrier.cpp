//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/registrationBarrier.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

void
Exec_RegistrationBarrier::SetFullyConstructed()
{
    // Callers of GetInstance() can now safely return a fully-constructed
    // registry.
    bool wasFullyConstructed;
    {
        // Even though _isFullyConstructed is an atomic, we still need to
        // protect its update with a lock on the mutex, or else other threads
        // might enter a wait state after we've notified the condition
        // variable.
        std::lock_guard lock(_isFullyConstructedMutex);
        wasFullyConstructed = _isFullyConstructed.exchange(
            true, std::memory_order_release);
    }
    _isFullyConstructedConditionVariable.notify_all();
    TF_VERIFY(!wasFullyConstructed,
              "SetFullyConstructed must only be called once");
}

void
Exec_RegistrationBarrier::_WaitUntilFullyConstructed()
{
    TRACE_FUNCTION();

    std::unique_lock lock(_isFullyConstructedMutex);
    _isFullyConstructedConditionVariable.wait(lock, [this] {
        return _isFullyConstructed.load(std::memory_order_acquire);
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
