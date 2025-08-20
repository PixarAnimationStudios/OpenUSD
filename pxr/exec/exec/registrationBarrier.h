//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_REGISTRATION_BARRIER_H
#define PXR_EXEC_EXEC_REGISTRATION_BARRIER_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/base/arch/hints.h"

#include <atomic>
#include <condition_variable>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

/// Helper to prevent races when populating singleton registries.
///
/// TfSingleton already serializes construction of singleton instances.
/// However, for singleton registries that support concurrent access, this
/// presents a subtle problem.  Registry functions that insert entries into
/// the registry need access to the singleton instance so the instance must be
/// made available before subscription.  However, doing so allows threads
/// performing lookups to access the registry before registry functions have
/// completed.
///
/// To use a registration barrier, the registry must have a clear distinction
/// between operations that add entries to the registry and operations that
/// query results from the registry.  Registries should provide a mutable
/// accessor for registration, e.g. `GetInstanceForRegistration()`, which may
/// be private, and a const accessor used for querying the registry,
/// e.g. `GetInstance()`.  Only functions that add entries via the mutable
/// accessor may be called during registry function subscription.  Entering
/// `GetInstance()` during subscription will result in a deadlock.
///
/// Example:
///
/// ```{.cpp}
/// Registry&
/// Registry::GetInstanceForRegistration()
/// {
///     return TfSingleton<Registry>::GetInstance();
/// }
///
/// const Registry&
/// Registry::GetInstance()
/// {
///     Registry& instance = TfSingleton<Registry>::GetInstance();
///     instance._registrationBarrier->WaitUntilFullyConstructed();
///     return instance;
/// }
///
/// Registry::Registry()
///     : _registrationBarrier(std::make_unique<Exec_RegistrationBarrier>())
/// {
///     // Perform any internal work to prepare the registry to accept
///     // registration.
///
///     // Make the instance available for registration.
///     TfSingleton<Registry>::SetInstanceConstructed(*this);
///
///     // Subscribe to registry functions.
///     TfRegistryManager::SubscribeTo<Registry>();
///
///     // Make the instance available for queries.
///     _registrationBarrier->SetFullyConstructed();
/// }
/// ```
///
class Exec_RegistrationBarrier
{
public:
    /// Waits until the instance is ready for all clients.
    void WaitUntilFullyConstructed() {
        if (ARCH_LIKELY(_isFullyConstructed.load(std::memory_order_acquire))) {
            return;
        }

        _WaitUntilFullyConstructed();
    }

    /// Indicate that the instance ready for all clients.
    void SetFullyConstructed();

private:
    // Slow path for waiting on _isFullyConstructed.
    //
    // The vast majority of the calls to WaitUntilFullyConstructed will occur
    // after the instance is fully constructed and take the early return.  As
    // such, the wait implementation involving a mutex and condition variable
    // is placed in this function to maximize the likelihood of the compiler
    // inlining the initial check.
    //
    void _WaitUntilFullyConstructed();

private:
    // Indicates that the registry instance is fully constructed.
    std::atomic<bool> _isFullyConstructed{false};

    // Used by non-registration clients to wait until the instance is fully
    // constructed.
    std::mutex _isFullyConstructedMutex;
    std::condition_variable _isFullyConstructedConditionVariable;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
