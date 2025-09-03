//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_MASKED_SUB_EXECUTOR_H
#define PXR_EXEC_EF_MASKED_SUB_EXECUTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"

#include "pxr/exec/vdf/datalessExecutor.h"
#include "pxr/exec/vdf/executorFactory.h"
#include "pxr/exec/vdf/speculationExecutor.h"
#include "pxr/exec/vdf/types.h"

#include <tbb/concurrent_unordered_map.h>

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorErrorLogger;
class VdfSchedule;

///////////////////////////////////////////////////////////////////////////////
///
/// \class EfMaskedSubExecutor.
///
/// \brief This sub-executor masks the parent executor. It is a dataless
/// executor, i.e. it does not hold on to any data caches. It does, however,
/// support invalidation and locally trackes invalidation state. If an output is
/// invalid on this executor, the call to GetOutputValue() will not look
/// up the data cache on the parent executor, and will instead return NULL.
/// Thus, the EfMaskedSubExecutor allows for correctly tracking invalidation
/// without affecting the invalidation state on the parent executor, potentially
/// messing with mung buffer locking, or stomping on existing buffers.
///
class EF_API_TYPE EfMaskedSubExecutor : public VdfDatalessExecutor
{
    // Executor factory.
    typedef
        VdfExecutorFactory<
            EfMaskedSubExecutor,
            VdfSpeculationExecutor<
                VdfSpeculationExecutorEngine,
                VdfDataManagerVector< 
                    VdfDataManagerDeallocationMode::Background > > > 
        _Factory;

public:

    /// Constructor
    ///
    /// Note, this executor must be constructed with a parent executor present,
    /// because it dispatches the calls to GetOutputValue to the parent.
    ///
    EF_API
    EfMaskedSubExecutor(const VdfExecutorInterface *parentExecutor);

    /// Destructor
    ///
    EF_API
    virtual ~EfMaskedSubExecutor();

    /// Factory construction.
    ///
    const VdfExecutorFactoryBase &GetFactory() const override {
        return _factory;
    }

    /// Duplicates the output data associated with \p sourceOutput and copies
    /// it to \p destOutput.
    ///
    EF_API
    virtual void DuplicateOutputData(
        const VdfOutput &sourceOutput,
        const VdfOutput &destOutput);

    /// Indicates whether this executor contains data.
    ///
    /// Note, that this method always returns \c false on this executor. We do
    /// this in order to trick invalidation into thinking that there is always
    /// data living on this executor. This allows us to push invalidation
    /// through the entire network and record the invalidation state, without
    /// regard for what state the parent executor is in.
    ///
    virtual bool IsEmpty() const {
        return false;
    }

    /// Returns \c true if the invalidation timestamps mismatch between the
    /// \p source and \p dest outputs. This information is used to determine
    /// whether to lock the source output for mung buffer locking.
    ///
    /// Although, this executor does store invalidation state, we refer to the
    /// parent executor to look up invalidation timestamps.
    /// 
    virtual bool HasInvalidationTimestampMismatch(
        const VdfOutput &source, 
        const VdfOutput &dest) const {
        const VdfExecutorInterface *parentExecutor = GetParentExecutor();
        return
            parentExecutor &&
            parentExecutor->HasInvalidationTimestampMismatch(source, dest);
    }

protected:

    // This executor supports invalidation. Any invalid output will not be
    // read from the parent executor.
    //
    virtual bool _InvalidateOutput(
        const VdfOutput &output,
        const VdfMask &invalidationMask);

    // This executor does not store temporary data caches, instead the locally
    // stored invalidation state will be cleared out.
    //
    virtual void _ClearData();

private:

    // Running this executor is not supported.
    //
    virtual void _Run(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        VdfExecutorErrorLogger *errorLogger);

    // Returns an output value for reading.
    //
    inline virtual const VdfVector *_GetOutputValueForReading(
        const VdfOutput &output,
        const VdfMask &mask) const override;

    // Returns \c true if the output is already invalid for the given
    // \p invalidationMask.
    //
    virtual bool _IsOutputInvalid(
        const VdfId outputId,
        const VdfMask &invalidationMask) const override;

    // The factory shared amongst executors of this type.
    //
    static const _Factory _factory;

    // A set of invalid outputs. Note, that after creating this executor, all
    // outputs are considered valid. As outputs become invalid, they are added
    // to the set of invalid outputs.
    //
    using _InvalidOutputs = tbb::concurrent_unordered_map<VdfId, VdfMask>;
    _InvalidOutputs _invalidOutputs;
};

///////////////////////////////////////////////////////////////////////////////

const VdfVector *
EfMaskedSubExecutor::_GetOutputValueForReading(
    const VdfOutput &output,
    const VdfMask &mask) const
{
    // If the output has not been invalidated on this executor, return the
    // value stored at the parent executor. Otherwise, return NULL.
    const VdfExecutorInterface *parentExecutor = GetParentExecutor();
    _InvalidOutputs::const_iterator it =
        _invalidOutputs.find(output.GetId());
    if (parentExecutor && 
        (it == _invalidOutputs.end() || !it->second.Overlaps(mask))) {
        return parentExecutor->GetOutputValue(output, mask);
    }
    return NULL;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
