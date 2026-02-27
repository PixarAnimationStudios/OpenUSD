//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_REQUEST_TRACKER_H
#define PXR_EXEC_EXEC_REQUEST_TRACKER_H

#include "pxr/pxr.h"

#include "pxr/base/tf/pxrTslRobinMap/robin_set.h"
#include "pxr/base/tf/spinMutex.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

struct Exec_AttributeValueInvalidationResult;
struct Exec_DisconnectedInputsInvalidationResult;
struct Exec_MetadataInvalidationResult;
class Exec_RequestImpl;
class Exec_TimeChangeInvalidationResult;

/// Maintains a (non-owning) set of outstanding requests.
class Exec_RequestTracker
{
public:
    /// Expires all outstanding requests.
    ///
    /// Destroying the tracker invalidates all request indices over all time
    /// and resets internal request state.  Expired requests will not receive
    /// any future invalidation.  This does not delete impl objects as these
    /// are owned exclusively by their request.
    ///
    ~Exec_RequestTracker();

    /// Add \p impl to the collection of outstanding requets.
    ///
    /// The tracker does not take ownership of the request impl.  It is
    /// responsible for notifying the request of value, topological, and time
    /// changes.
    ///
    void Insert(Exec_RequestImpl *impl);

    /// Remove \p impl from the collection of outstanding requests.
    ///
    /// The request will no longer receive change notification.
    ///
    void Remove(Exec_RequestImpl *impl);

    /// Notify all requests of invalid computed values as a consequence of
    /// attribute authored value invalidation.
    /// 
    void DidInvalidateComputedValues(
        const Exec_AttributeValueInvalidationResult &invalidationResult);

    /// Notify all requests of invalid computed values as a consequence of
    /// metadata authored value invalidation.
    /// 
    void DidInvalidateComputedValues(
        const Exec_MetadataInvalidationResult &invalidationResult);

    /// Notify all requests of invalid computed values as a consequence of
    /// uncompilation.
    /// 
    void DidInvalidateComputedValues(
        const Exec_DisconnectedInputsInvalidationResult &invalidationResult);

    /// Notify all requests to invalidate value keys that don't have a compiled
    /// leaf node.
    ///
    void DidInvalidateUnknownValues();

    /// Notify all requests of time having changed.
    void DidChangeTime(
        const Exec_TimeChangeInvalidationResult &invalidationResult) const;

    /// Invoke \p f on each tracked request.
    ///
    /// \p f is executed with the request mutex held so it must not re-enter
    /// the request tracker.  The method may execute \p f for multiple
    /// requests concurrently.
    ///
    template <typename Function>
    void ParallelForEachRequest(Function &&f) const;

private:
    mutable TfSpinMutex _requestsMutex;
    pxr_tsl::robin_set<Exec_RequestImpl *> _requests;
};

template <typename Function>
void
Exec_RequestTracker::ParallelForEachRequest(Function &&f) const
{
    TfSpinMutex::ScopedLock lock{_requestsMutex};
    WorkWithScopedParallelism([&] {
        WorkParallelForEach(
            _requests.begin(), _requests.end(), 
            [func=std::forward<Function>(f)](Exec_RequestImpl *const impl) {
                func(*impl);
            });
    });
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
