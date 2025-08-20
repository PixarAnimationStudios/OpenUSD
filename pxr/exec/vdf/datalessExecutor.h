//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_DATALESS_EXECUTOR_H
#define PXR_EXEC_VDF_DATALESS_EXECUTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/executorInterface.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfOutput;
class VdfVector;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfDatalessExecutor
///
/// \brief An abstract base class for executors, which do not store any data
/// at all. This class mainly serves the purpose of abstracting away error
/// handling on dataless executors, when methods that are supposed to mutate
/// data are called.
///

class VDF_API_TYPE VdfDatalessExecutor : public VdfExecutorInterface
{
public:

    /// Destructor
    ///
    VDF_API
    virtual ~VdfDatalessExecutor();

    /// Sets the cached value for a given \p output.
    ///
    /// This is not supported on this type of executor.
    ///
    VDF_API
    virtual void SetOutputValue(
        const VdfOutput &output,
        const VdfVector &value,
        const VdfMask &mask);

    /// Transfers ownership of \p value to the given \p output.
    /// 
    /// This is not supported on this type of executor.
    ///
    VDF_API
    virtual bool TakeOutputValue(
        const VdfOutput &output,
        VdfVector *value,
        const VdfMask &mask);

    /// Returns \c true of the data manager is empty.
    ///
    /// This type of executor is always considered empty, since it does not
    /// hold any data.
    ///
    virtual bool IsEmpty() const {
        return true;
    }

protected:

    /// Protected default constructor
    ///
    VDF_API
    VdfDatalessExecutor();

    /// Returns a value for the cache that flows across \p connection.
    ///
    virtual const VdfVector *_GetInputValue(
        const VdfConnection &connection,
        const VdfMask &mask) const override {
        return NULL;
    }

    /// Returns an output value for reading.
    ///
    virtual const VdfVector *_GetOutputValueForReading(
        const VdfOutput &output,
        const VdfMask &mask) const override {
        return NULL;
    }

    /// Returns an output value for writing.
    ///
    virtual VdfVector *_GetOutputValueForWriting(
        const VdfOutput &output) const override {
        return NULL;
    }

    /// Clears the data for a specific output on this executor.
    ///
    /// This has no effect on this type of executor.
    ///
    void _ClearDataForOutput(
        const VdfId outputId, const VdfId nodeId) override {}

    /// Clears all the data caches associated with any output in the network.
    ///
    /// This has no effect on this type of executor.
    ///
    virtual void _ClearData() {}

    /// Called before invalidation begins to update the timestamp that will be
    /// written for every VdfOutput visited during invalidation.  This timestamp
    /// is later used to identify outputs for mung buffer locking.
    ///
    /// This has no effect on this type of executor.
    ///
    virtual void _UpdateInvalidationTimestamp() {}

    /// Called to set destOutput's buffer output to be a reference to the 
    /// buffer output of sourceOutput.
    ///
    /// This is not supported on this type of executor.
    ///
    VDF_API
    virtual void _SetReferenceOutputValue(
        const VdfOutput &destOutput,
        const VdfOutput &sourceOutput,
        const VdfMask &sourceMask) const;

    /// Mark the output as having been visited.  This is only to be used by
    /// the speculation engine to tell its parent executor that an output 
    /// has been visited and should be marked for invalidation.
    ///
    /// This has no effect on this type of executor.
    ///
    virtual void _TouchOutput(const VdfOutput &output) const {}

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
