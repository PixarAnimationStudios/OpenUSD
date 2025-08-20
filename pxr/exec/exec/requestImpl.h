//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_REQUEST_IMPL_H
#define PXR_EXEC_EXEC_REQUEST_IMPL_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"
#include "pxr/exec/exec/request.h"
#include "pxr/exec/exec/types.h"

#include "pxr/base/tf/bits.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"
#include "pxr/exec/ef/timeInterval.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/types.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class EfTime;
class Exec_AuthoredValueInvalidationResult;
class Exec_CacheView;
class Exec_DisconnectedInputsInvalidationResult;
class Exec_TimeChangeInvalidationResult;
class Exec_ValueExtractor;
class ExecSystem;
class ExecValueKey;
template <typename> class TfSpan;
class VdfRequest;
class VdfSchedule;

/// Contains data structures necessary to implement exec requests that are
/// independent of scene description.
///
/// Concrete implementations inherit from Exec_RequestImpl to implement any
/// functionality that is specific to the scene description system.
///
class Exec_RequestImpl
{
public:
    /// Notify the request of invalid computed values as a consequence of
    /// authored value invalidation.
    /// 
    void DidInvalidateComputedValues(
        const Exec_AuthoredValueInvalidationResult &invalidationResult);

    /// Notify the request of invalid computed values as a consequence of
    /// uncompilation.
    /// 
    void DidInvalidateComputedValues(
        const Exec_DisconnectedInputsInvalidationResult &invalidationResult);

    /// Notify the request of time having changed.
    void DidChangeTime(
        const Exec_TimeChangeInvalidationResult &invalidationResult);

    /// Expires all request indices and discards the request.
    ///
    /// Sends value invalidation for all indicies over all time and renders
    /// the request unusuable for any future operation.
    ///
    void Expire();

protected:
    EXEC_API
    Exec_RequestImpl(
        ExecSystem *system,
        ExecRequestComputedValueInvalidationCallback &&valueCallback,
        ExecRequestTimeChangeInvalidationCallback &&timeCallback);

    Exec_RequestImpl(const Exec_RequestImpl&) = delete;
    Exec_RequestImpl& operator=(const Exec_RequestImpl&) = delete;

    EXEC_API
    ~Exec_RequestImpl();

    /// Compiles outputs for the value keys in the request.
    EXEC_API
    void _Compile(TfSpan<const ExecValueKey> valueKeys);

    /// Builds the schedule for the request.
    EXEC_API
    void _Schedule();

    /// Computes the value keys in the request.
    EXEC_API
    Exec_CacheView _Compute();

    /// Returns true if the request needs to be compiled.
    ///
    /// A request may skip compilation if its schedule is up-to-date and there
    /// is no pending recompilation in the network.
    ///
    EXEC_API
    bool _RequiresCompilation() const;

    /// Expires the indices in \p expired.
    ///
    /// Invalidation callbacks will be invoked for these indices one final
    /// time.  No values will be extractable and no further invalidation will
    /// be sent for these indices.
    ///
    EXEC_API
    void _ExpireIndices(const ExecRequestIndexSet &expired);

    /// Removes the request from the system.
    ///
    /// This prevents any further notification and releases internal request
    /// data structures.
    ///
    EXEC_API
    void _Discard();

private:
    // Ensures the _leafNodeToIndex map is up-to-date.
    void _BuildLeafNodeToIndexMap();

    // Turns invalid leaf nodes into a set of requested - and not previously
    // invalidated - indices.
    // 
    void _InvalidateLeafOutputs(
        bool isNewlyInvalidInterval,
        TfSpan<const VdfNode *const> leafNodes,
        ExecRequestIndexSet *invalidIndices);

private:
    // The system that issued this request.
    ExecSystem *_system;

    // The compiled leaf output.
    std::vector<VdfMaskedOutput> _leafOutputs;

    // Value extractors corresponding to each requested value.
    std::vector<Exec_ValueExtractor> _extractors;

    // Maps leaf nodes to their index in the array of valueKeys the request
    // was built with.
    pxr_tsl::robin_map<VdfId, size_t> _leafNodeToIndex;

    // The compute request to cache values with.
    std::unique_ptr<VdfRequest> _computeRequest;

    // The schedule to cache values with.
    std::unique_ptr<VdfSchedule> _schedule;

    // The combined time interval for which invalidation has been sent out.
    EfTimeInterval _lastInvalidatedInterval;

    // The combined set of indices for which invalidation has been sent out.
    TfBits _lastInvalidatedIndices;

    // The invalidation callback to invoke when computed values change.
    ExecRequestComputedValueInvalidationCallback _valueCallback;

    // The invalidation callback to invoke when time changes.
    ExecRequestTimeChangeInvalidationCallback _timeCallback;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
