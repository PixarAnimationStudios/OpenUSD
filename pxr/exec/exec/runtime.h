//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_RUNTIME_H
#define PXR_EXEC_EXEC_RUNTIME_H

#include "pxr/pxr.h"

#include "pxr/exec/ef/maskedSubExecutor.h"
#include "pxr/exec/vdf/dataManagerFacade.h"
#include "pxr/exec/vdf/maskedOutputVector.h"

#include <memory>
#include <tuple>

PXR_NAMESPACE_OPEN_SCOPE

class EfLeafNodeCache;
class EfPageCacheStorage;
class EfTime;
class EfTimeInterval;
class EfTimeInputNode;
class VdfExecutorErrorLogger;
class VdfExecutorInterface;
class VdfNode;
class VdfRequest;
class VdfSchedule;

/// Owns the main executor and related data structure for managing computed and
/// cached values.
///
class Exec_Runtime
{
public:
    Exec_Runtime(
        EfTimeInputNode &timeNode,
        EfLeafNodeCache &leafNodeCache);
    ~Exec_Runtime();

    // Non-copyable and non-movable.
    Exec_Runtime(const Exec_Runtime &) = delete;
    Exec_Runtime& operator=(const Exec_Runtime &) = delete;

    /// Returns a facade of the main executor's data manager, providing read
    /// access to previously computed and cached values.
    /// 
    const VdfDataManagerFacade GetDataManager();

    /// Sets the time on the executor data manager.
    ///
    /// Returns a tuple containing a boolean indicating whether the time has
    /// changed relative to the previously set time, along with the previous
    /// time value.
    /// 
    /// \note
    /// This method does not perform time invalidation on the executor.
    /// 
    std::tuple<bool, EfTime> SetTime(
        const EfTimeInputNode &timeNode,
        const EfTime &time);

    /// Explicitly invalidates all executor state that depends on the topology
    /// of the data-flow network.
    /// 
    /// This must be called explicitly after topology changes in a manner that
    /// does not also increment the data-flow network version. For example,
    /// changing the time-dependency flag on an input node.
    /// 
    void InvalidateTopologicalState();

    /// Invalidates the computed output values in the \p invalidationRequest,
    /// along with all values that depend on these outputs.
    /// 
    /// This method implicitly invalidates executor state dependent on the
    /// topology of the data-flow network, if the data-flow network version has
    /// changed.
    /// 
    void InvalidateExecutor(const VdfMaskedOutputVector &invalidationRequest);

    /// Invalidates the time-varying computed values in the
    /// \p invalidationRequest over the provided \p timeInterval, along with all
    /// dependent values.
    ///
    void InvalidatePageCache(
        const VdfMaskedOutputVector &invalidationRequest,
        const EfTimeInterval &timeInterval);

    /// Deletes all of \p node 's computed and cached values.
    void DeleteData(const VdfNode &node);

    /// Performs evaluation with the provided \p schedule and \p computeRequest
    /// and caches all computed values.
    /// 
    void ComputeValues(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest);

    /// Performs evaluation with the provided \p schedule and \p computeRequest
    /// using a temporary subexecutor that is initialized with the provided
    /// overrides.
    ///
    /// The overrides are specified by \p overriddenOutputs and
    /// \p overriddenValues. These vectors must be equal in length, because the
    /// output at index i in \p overriddenOutputs will assume an override value
    /// from index i in \p overriddenValues.
    ///
    /// It is the caller's responsibility to ensure that the override value
    /// has the same type as the output being overridden.
    ///
    /// This returns a unique pointer to the subexecutor used for evaluation
    /// which stores the results of the computation.
    ///
    std::unique_ptr<VdfExecutorInterface> ComputeWithOverrides(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        const VdfMaskedOutputVector &overriddenOutputs,
        const std::vector<VtValue> &overriddenValues);

private:
    // Reports any executor errors raised during evaluation.
    void _ReportExecutorErrors(const VdfExecutorErrorLogger &errorLogger) const;

private:
    std::unique_ptr<VdfExecutorInterface> _executor;
    size_t _executorTopologicalStateVersion;
    std::unique_ptr<EfPageCacheStorage> _cacheStorage;
    std::unique_ptr<EfMaskedSubExecutor> _overridesExecutor;
    size_t _overridesExecutorNetworkVersion;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
