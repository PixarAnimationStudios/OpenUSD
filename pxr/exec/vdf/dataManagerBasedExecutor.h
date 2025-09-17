//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_DATA_MANAGER_BASED_EXECUTOR_H
#define PXR_EXEC_VDF_DATA_MANAGER_BASED_EXECUTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/vector.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfDataManagerBasedExecutor
///
/// Base class for executors that use a data manager.
///
/// This implements much of the API from VdfExecutorInterface that simply passes
/// the work on to a data manager.
///

template<typename DataManagerType, typename BaseClass>
class VdfDataManagerBasedExecutor : public BaseClass
{
public:

    /// Default constructor.
    ///
    VdfDataManagerBasedExecutor() {}

    /// Construct with a parent executor.
    ///
    explicit VdfDataManagerBasedExecutor(
        const VdfExecutorInterface *parentExecutor) :
        BaseClass(parentExecutor)
    {}

    /// Destructor.
    ///
    virtual ~VdfDataManagerBasedExecutor() {}

    /// Resize the executor data manager to accommodate the given \p network.
    ///
    virtual void Resize(const VdfNetwork &network) override {
        _dataManager.Resize(network);
    }

    /// Sets the cached value for a given \p output.
    ///
    /// If the output already contains data, it will be merged with the new
    /// data as indicated by \p value and \p mask.
    ///
    virtual void SetOutputValue(
        const VdfOutput &output,
        const VdfVector &value,
        const VdfMask &mask) {
        _dataManager.SetOutputValue(output, value, mask);
    }

    /// Transfers the \p value to the given \p output.
    ///
    virtual bool TakeOutputValue(
        const VdfOutput &output,
        VdfVector *value,
        const VdfMask &mask) {
        return _dataManager.TakeOutputValue(output, value, mask);
    }

    /// Duplicates the output data associated with \p sourceOutput and copies
    /// it to \p destOutput.
    ///
    virtual void DuplicateOutputData(
        const VdfOutput &sourceOutput,
        const VdfOutput &destOutput) {
        _dataManager.DuplicateOutputData(sourceOutput, destOutput);
    }

    /// Returns \c true of the data manager is empty.
    ///
    virtual bool IsEmpty() const {
        return _dataManager.IsEmpty();
    }

    /// Returns \c true, if the invalidation timestamps between the \p source
    /// and \p dest outputs do not match, i.e. the source output should be
    /// mung buffer locked.
    virtual bool HasInvalidationTimestampMismatch(
        const VdfOutput &source, 
        const VdfOutput &dest) const {
        return _dataManager.HasInvalidationTimestampMismatch(
            _dataManager.GetDataHandle(source.GetId()),
            _dataManager.GetDataHandle(dest.GetId()));
    }

protected:

    /// Returns value for the cache that flows across \p connection.
    ///
    virtual const VdfVector *_GetInputValue(
        const VdfConnection &connection,
        const VdfMask &mask) const override {
        return _dataManager.GetInputValue(connection, mask);
    }

    /// Returns an output value for reading.
    ///
    virtual const VdfVector *_GetOutputValueForReading(
        const VdfOutput &output,
        const VdfMask &mask) const override {
        return _dataManager.GetOutputValueForReading(
            _dataManager.GetDataHandle(output.GetId()), mask);
    }

    /// Returns an output value for writing.
    ///
    virtual VdfVector *_GetOutputValueForWriting(
        const VdfOutput &output) const override {
        return _dataManager.GetOrCreateOutputValueForWriting(
            output, _dataManager.GetDataHandle(output.GetId()));
    }

    /// Clears the data for a specific output on this executor.
    /// 
    void _ClearDataForOutput(
        const VdfId outputId, const VdfId nodeId) override {
        _dataManager.ClearDataForOutput(outputId);
    }

    /// Returns \c true if the output is already invalid for the given
    /// \p invalidationMask.
    /// 
    virtual bool _IsOutputInvalid(
        const VdfId outputId,
        const VdfMask &invalidationMask) const override {
        return _dataManager.IsOutputInvalid(outputId, invalidationMask);
    }

    /// Called during invalidation to mark outputs as invalid and determine
    /// when the traversal can terminate early.
    ///
    /// \p incomingInput is the VdfInput by which this \p output was reached
    /// during invalidation traversal, or NULL if \p output is one of the
    /// first-traversed outputs.
    ///
    /// Returns \c true if there was anything to invalidate and \c false if
    /// \p output was already invalid.
    ///
    virtual bool _InvalidateOutput(
        const VdfOutput &output,
        const VdfMask &invalidationMask)
    {
        return _dataManager.InvalidateOutput(output, invalidationMask);
    }

    /// Called before invalidation begins to update the timestamp that will be
    /// written for every VdfOutput visited during invalidation.  This timestamp
    /// is later used to identify outputs for mung buffer locking.
    ///
    virtual void _UpdateInvalidationTimestamp()
    {
        _dataManager.UpdateInvalidationTimestamp(
            BaseClass::GetExecutorInvalidationTimestamp());
    }

    /// Called to set destOutput's buffer output to be a reference to the 
    /// buffer output of sourceOutput.
    ///
    virtual void _SetReferenceOutputValue(
        const VdfOutput &destOutput,
        const VdfOutput &sourceOutput,
        const VdfMask &sourceMask) const {
        // XXX: We are getting the cached output value from the executor, which
        //      may give us a pointer into the parent executor data manager. We
        //      cannot take ownership of values stored outside of this
        //      executor's data manager. We have to come up with a way to
        //      support reference outputs as a core concept! 
        const VdfVector *sourceValue =
            _GetOutputValueForReading(sourceOutput, sourceMask);
        _dataManager.SetReferenceOutputValue(sourceValue, destOutput.GetId());
    }

    /// Mark the output as having been visited.  This is only to be used by
    /// the speculation engine to tell its parent executor that an output 
    /// has been visited and should be marked for invalidation.
    ///
    virtual void _TouchOutput(const VdfOutput &output) const override
    {
        _dataManager.DataManagerType::Base::Touch(output);
    }


protected:

    // This object manages the data needed for this executor, including all
    // the cached output values.
    DataManagerType _dataManager;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
