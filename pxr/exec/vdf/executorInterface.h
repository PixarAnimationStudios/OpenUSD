//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTOR_API_H
#define PXR_EXEC_VDF_EXECUTOR_API_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/maskedOutputVector.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/hashset.h"

#include <atomic>
#include <memory>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

class VdfMask;
class VdfNetwork;
class VdfOutput;
class VdfRequest;
class VdfSchedule;
class VdfVector;
class VdfExecutionStats;
class VdfExecutorErrorLogger;
class VdfExecutorFactoryBase;
class VdfExecutorInvalidator;
class VdfExecutorObserver;

///////////////////////////////////////////////////////////////////////////////
///
/// \brief Abstract base class for classes that execute a VdfNetwork to compute
/// a requested set of values.
///

class VDF_API_TYPE VdfExecutorInterface
{
public:
    /// Noncopyable.
    ///
    VdfExecutorInterface(const VdfExecutorInterface &) = delete;
    VdfExecutorInterface &operator=(const VdfExecutorInterface &) = delete;

    /// Destructor.
    ///
    VDF_API
    virtual ~VdfExecutorInterface();

    /// \name Evaluation
    /// @{

    /// Executes the \p schedule.
    ///
    VDF_API
    void Run(
        const VdfSchedule &schedule, 
        VdfExecutorErrorLogger *errorLogger = NULL);

    /// Executes the \p schedule.
    ///
    /// \p computeRequest must be a subset of the scheduled request. If the
    /// full, scheduled request should be computed, then \p computeRequest
    /// should be set to schedule.GetRequest().
    ///
    VDF_API
    void Run(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        VdfExecutorErrorLogger *errorLogger = NULL);

    /// @}


    /// \name Factory Construction
    /// @{

    /// Returns a factory class facilitating the construction of new executors
    /// that share traits with this executor instance.
    ///
    virtual const VdfExecutorFactoryBase &GetFactory() const = 0; 

    /// @}


    /// \name Executor Observer Notification
    /// @{

    /// Can be called by clients to register a VdfExecutorObserver
    /// with this executor.
    ///
    VDF_API
    void RegisterObserver(const VdfExecutorObserver *observer) const;

    /// Must be called by clients to unregister a
    /// VdfExecutorObserver, which has been previously registered
    /// with RegisterObserver().
    ///
    VDF_API
    void UnregisterObserver(const VdfExecutorObserver *observer) const;

    /// @}


    /// \name Cache Management
    /// @{

    /// Resize the executor to accomodate data for the given \p network.
    ///
    virtual void Resize(const VdfNetwork &network) {}

    /// Sets the cached value for a given \p output.
    ///
    virtual void SetOutputValue(
        const VdfOutput &output,
        const VdfVector &value,
        const VdfMask &mask) = 0;

    /// Transfers ownership of the \p value to the given \p output. Returns
    /// \c true if the transfer of ownership was successful. If the transfer of
    /// ownership was successful, the executor assumes responsibility for the
    /// lifetime of \p value. Otherwise, the call site maintains this
    /// responsibility.
    ///
    virtual bool TakeOutputValue(
        const VdfOutput &output,
        VdfVector *value,
        const VdfMask &mask) = 0;

    /// Returns the cached value for a given \p output if it has a cache
    /// that contains all values specified by \p mask.  Otherwise, returns
    /// NULL.
    ///
    const VdfVector *GetOutputValue(
        const VdfOutput &output,
        const VdfMask &mask) const {
        return _GetOutputValueForReading(output, mask);
    }

    /// Duplicates the output data associated with \p sourceOutput and copies
    /// it to \p destOutput. 
    ///
    virtual void DuplicateOutputData(
        const VdfOutput &sourceOutput,
        const VdfOutput &destOutput) = 0;

    /// @}


    /// \name Executor Hierarchy Management
    /// @{

    /// Returns the parent executor, if any.
    ///
    const VdfExecutorInterface *GetParentExecutor() const {
        return _parentExecutor;
    }

    /// Sets the parent executor.
    ///
    /// This method also inherits the execution stats from the parent executor,
    /// unless the executor already has its execution stats set.
    ///
    /// XXX: We need to get rid of this public API, since most executors do not
    ///      support changing out the parent executor after construction.
    ///
    VDF_API
    void SetParentExecutor(const VdfExecutorInterface *parentExecutor);

    /// @}


    /// \name Invalidation
    /// @{

    /// Invalidates the network, starting from the masked outputs in
    /// \p request.
    ///
    /// Performs an optimized vectorized traversal.
    /// 
    VDF_API
    void InvalidateValues(
        const VdfMaskedOutputVector &invalidationRequest);

    /// Invalidate all state depending on network topology. This must be
    /// called after changes to the network have been made.
    ///
    /// XXX:exec
    /// I believe that we should not have this kind of API exposed.  Clients
    /// should not have to know about the internal state kept in the 
    /// executors.  This is very similiar to schedules and perhaps should be
    /// treated like schedules by being registered with the networks that
    /// they depend on.  A generalized mechanism in VdfNetwork might be nice.
    ///
    VDF_API
    void InvalidateTopologicalState();

    /// Clears the executors buffers
    ///
    VDF_API
    void ClearData();

    /// Clears the executor buffers for a specific output
    ///
    VDF_API
    void ClearDataForOutput(const VdfId outputId, const VdfId nodeId);

    /// Returns \c true if the executor buffers are empty.
    ///
    virtual bool IsEmpty() const = 0;

    /// @}


    /// \name Mung Buffer Locking Invalidation Timestamps
    /// @{

    /// Increment this executor's invalidation timestamp for mung
    /// buffer locking.
    ///
    void IncrementExecutorInvalidationTimestamp() {
        ++_executorInvalidationTimestamp;
    }

    /// Inherit the invalidation timestamp from another executor.
    ///
    void InheritExecutorInvalidationTimestamp(const VdfExecutorInterface &executor) {
        _executorInvalidationTimestamp = 
            executor.GetExecutorInvalidationTimestamp();
    }

    /// Returns this executor's invalidation timestamp
    ///
    VdfInvalidationTimestamp GetExecutorInvalidationTimestamp() const {
        return _executorInvalidationTimestamp;
    }

    /// Returns \c true, if the invalidation timestamps between the \p source
    /// and \p dest outputs do not match, i.e. the source output should be
    /// mung buffer locked.
    ///
    virtual bool HasInvalidationTimestampMismatch(
        const VdfOutput &source, 
        const VdfOutput &dest) const = 0;

    /// @}


    /// \name Executor Interruption
    /// @{

    /// Set the interruption flag.
    ///
    void SetInterruptionFlag(const std::atomic_bool *interruptionFlag) {
        _interruptionFlag = interruptionFlag;
    }

    /// Returns the interruption flag
    ///
    const std::atomic_bool *GetInterruptionFlag() const {
        return _interruptionFlag;
    }

    /// Returns whether or not the executor has been interrupted, if the 
    /// executor supports interruption. If interruption is not supported, i.e.
    /// no interruption flag has been set, this will always return \c false.
    ///
    bool HasBeenInterrupted() const {
        return _interruptionFlag && _interruptionFlag->load();
    }

    /// @}


    /// \name Diagnostic Support
    /// @{

    /// Sets an execution stats object.
    ///
    /// When \p stats is not NULL, then execution statistics will be gathered
    /// into the object.  When NULL, execution statistics will not be gathered.
    ///
    void SetExecutionStats(VdfExecutionStats *stats) {
        _stats = stats;
    }

    /// Returns the Execution Stats object, if any.
    ///
    VdfExecutionStats *GetExecutionStats() const {
        return _stats;
    }

    /// @}


protected:

    /// Protected default constructor.
    ///
    VDF_API
    VdfExecutorInterface();

    /// Construct with a parent executor.
    ///
    VDF_API
    explicit VdfExecutorInterface(const VdfExecutorInterface *parentExecutor);

    /// Run this executor with the given \p schedule and \p request.
    ///
    virtual void _Run(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        VdfExecutorErrorLogger *errorLogger) = 0;

    /// Returns a value for the cache that flows across \p connection.
    ///
    virtual const VdfVector *_GetInputValue(
        const VdfConnection &connection,
        const VdfMask &mask) const = 0;

    /// Returns an output value for reading.
    ///
    virtual const VdfVector *_GetOutputValueForReading(
        const VdfOutput &output,
        const VdfMask &mask) const = 0;

    /// Returns an output value for writing.
    ///
    virtual VdfVector *_GetOutputValueForWriting(
        const VdfOutput &output) const = 0;

    /// Returns \c true if the output is already invalid for the given
    /// \p invalidationMask.
    ///
    virtual bool _IsOutputInvalid(
        const VdfId outputId,
        const VdfMask &invalidationMask) const = 0;

    /// Called during invalidation to mark outputs as invalid and determine
    /// when the traversal can terminate early.
    ///
    /// Returns \c true if there was anything to invalidate and \c false if
    /// \p output was already invalid.
    ///
    virtual bool _InvalidateOutput(
        const VdfOutput &output,
        const VdfMask &invalidationMask) = 0;

    /// This method is called as a pre-processing step before an
    /// InvalidateValues() call. The method will return \c true if
    /// \p processedRequest is to override the originally supplied
    /// \p invalidationRequest for executor invalidation.
    ///
    virtual bool _PreProcessInvalidation(
        const VdfMaskedOutputVector &invalidationRequest,
        VdfMaskedOutputVector *processedRequest) {
        return false;
    }

    /// Called before invalidation begins to update the timestamp that will be
    /// written for every VdfOutput visited during invalidation.  This timestamp
    /// is later used to identify outputs for mung buffer locking.
    ///
    virtual void _UpdateInvalidationTimestamp() = 0;

    /// Virtual implementation of the ClearData call. This may be
    /// overridden by classes, which derive from VdfExecutorInterface.
    ///
    VDF_API
    virtual void _ClearData();

    /// Virtual implementation of the ClearDataForOutput call. This may be
    /// overridden by classes, which derive from VdfExecutorInterface.
    ///
    VDF_API
    virtual void _ClearDataForOutput(const VdfId outputId, const VdfId nodeId);

    /// Called to set destOutput's buffer output to be a reference to the 
    /// buffer output of sourceOutput.
    ///
    virtual void _SetReferenceOutputValue(
        const VdfOutput &destOutput,
        const VdfOutput &sourceOutput,
        const VdfMask &sourceMask) const = 0;

    /// Mark the output as having been visited.  This is only to be used by
    /// the speculation engine to tell its parent executor that an output 
    /// has been visited and should be marked for invalidation.
    ///
    virtual void _TouchOutput(const VdfOutput &output) const = 0;

private:

    // VdfContext needs access to _SetReferenceOutputValue,
    // _GetOutputValueForReading, _GetOutputValueForWriting and _LogWarning.
    friend class VdfContext;

    // VdfIterator needs friend access to _GetInputValue,
    // _GetOutputValueForReading and _GetOutputValueForWriting.
    friend class VdfIterator;

    // VdfSpeculationNode needs friend access to _GetOutputValueForWriting.
    friend class VdfSpeculationNode;

    // These classes need access to _TouchOutput.
    template<template <typename> class E, class D>
    friend class VdfSpeculationExecutor;
    template<class T> friend class VdfSpeculationExecutorEngine;
    template<class T> friend class VdfPullBasedExecutorEngine;
    template<class T> friend class VdfParallelSpeculationExecutorEngine;

    // VdfExecutorInvalidator needs access to _InvalidateOutput.
    friend class VdfExecutorInvalidator;

    // The optional invalidator, responsible for invalidating output state
    // and temporary buffers for outputs and their dependent outputs.
    std::unique_ptr<VdfExecutorInvalidator> _invalidator;

    // Optional, externally provided (i.e. not owned by the executor) object
    // to keep track of execution statistics.
    VdfExecutionStats *_stats;

    // Keeps track of the VdfExecutorObservers registered with this executor
    typedef TfHashSet<const VdfExecutorObserver *, TfHash> _Observers;
    mutable _Observers _observers;

    // Access to the _observers set can happen from multiple threads.  The
    // scenario where this happens is when background execution is interrupted
    // with sharing enabled.  Interuption iterates over the _observers lists in
    // order to notify.  That notification causes sharing node to release
    // sub-executors via a worker thread.  That also does unregister from the
    // _observers set while the main thread is still using the set to interrupt.
    mutable std::recursive_mutex _observersLock;   

    // The executor stores its own invalidation timestamp.
    // This timestamp will be applied to the data manager upon
    // invalidating values.
    VdfInvalidationTimestamp _executorInvalidationTimestamp;

    // Optional parent executor.
    const VdfExecutorInterface *_parentExecutor;

    // Interruption flag
    const std::atomic_bool *_interruptionFlag;

};

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
