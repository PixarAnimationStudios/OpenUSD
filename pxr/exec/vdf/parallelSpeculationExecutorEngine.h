//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_PARALLEL_SPECULATION_EXECUTOR_ENGINE_H
#define PXR_EXEC_VDF_PARALLEL_SPECULATION_EXECUTOR_ENGINE_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/parallelExecutorEngineBase.h"
#include "pxr/exec/vdf/speculationExecutorBase.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfParallelSpeculationExecutorEngine
///
/// An executor engine used for parallel speculation node evaluation,
/// deriving from VdfParallelExecutorEngineBase. The engine does not support
/// arena execution. The reason being that the parent executor engine will
/// already be executing tasks inside of an arena. The engine does also not
/// need to do any touching, or buffer locking, due to its temporary lifetime.
/// It does, however, do cycle detection.
///
template < typename DataManagerType >
class VdfParallelSpeculationExecutorEngine :
    public VdfParallelExecutorEngineBase<
        VdfParallelSpeculationExecutorEngine<DataManagerType>,
        DataManagerType>
{
public:

    /// Base class.
    ///
    typedef
        VdfParallelExecutorEngineBase<
            VdfParallelSpeculationExecutorEngine<DataManagerType>,
            DataManagerType>
        Base;

    /// Constructor.
    ///
    VdfParallelSpeculationExecutorEngine(
        const VdfSpeculationExecutorBase &speculationExecutor, 
        DataManagerType *dataManager);

private:

    // Befriend the base class so that it has access to the private methods
    // used for static polymorphism.
    //
    friend Base;

    // Detect a cycle by looking at the current node and determining whether it
    // is the same node that started the speculation.
    //
    bool _DetectCycle(
        const VdfEvaluationState &state,
        const VdfNode &node);

    // This executor engine does not need to do any touching on itself. It does,
    // however, touch output buffers on the parent executor.
    //
    void _Touch(const VdfOutput &output);

    // Finalize the output before publishing any buffers.
    //
    void _FinalizeOutput(
        const VdfEvaluationState &state,
        const VdfOutput &output,
        const VdfSchedule::OutputId outputId,
        const typename Base::_DataHandle dataHandle,
        const VdfScheduleTaskIndex invocationIndex,
        const VdfOutput *passToOutput);

    // Finalize any state after evaluation completed.
    //
    void _FinalizeEvaluation() {}

    // The first non-speculation parent executor to transfer buffers to.
    //
    VdfExecutorInterface *_writeBackExecutor;
};

///////////////////////////////////////////////////////////////////////////////

template < typename DataManagerType >
VdfParallelSpeculationExecutorEngine<DataManagerType>::
VdfParallelSpeculationExecutorEngine(
    const VdfSpeculationExecutorBase &speculationExecutor, 
    DataManagerType *dataManager) :
    Base(speculationExecutor, dataManager),
    _writeBackExecutor(const_cast<VdfExecutorInterface *>(
        speculationExecutor.GetNonSpeculationParentExecutor()))
{

}

template < typename DataManagerType >
bool
VdfParallelSpeculationExecutorEngine<DataManagerType>::_DetectCycle(
    const VdfEvaluationState &state,
    const VdfNode &node)
{
    // This engine is always constructed with a speculation executor (c.f.
    // constructor).
    const VdfSpeculationExecutorBase &speculationExecutor = 
        static_cast<const VdfSpeculationExecutorBase &>(state.GetExecutor());

    // If the node to execute is the same speculation node that ran this
    // executor engine, evaluation is trapped in a cycle.
    return speculationExecutor.IsSpeculatingNode(&node);
}

template < typename DataManagerType >
void
VdfParallelSpeculationExecutorEngine<DataManagerType>::_Touch(
    const VdfOutput &output)
{
    // The speculation executor engine doesn't need to touch locally, but we
    // need to dispatch the call to th executor and its parents.
    Base::_executor._TouchOutput(output);
}

template < typename DataManagerType >
void
VdfParallelSpeculationExecutorEngine<DataManagerType>::_FinalizeOutput(
    const VdfEvaluationState &state,
    const VdfOutput &output,
    const VdfSchedule::OutputId outputId,
    const typename Base::_DataHandle dataHandle,
    const VdfScheduleTaskIndex invocationIndex,
    const VdfOutput *passToOutput)
{
    // Only write back buffers for outputs which do not pass their buffers.
    if (passToOutput) {
        return;
    }

    // Bail out of the output does not have ownership over the cache. We can't
    // transfer ownership we don't have in the first place.
    VdfExecutorBufferData *privateBuffer =
        Base::_dataManager->GetPrivateBufferData(dataHandle);
    if (!privateBuffer->HasOwnership()) {
        return;
    }

    // Bail out if the write back executor already contains all the data.
    const VdfMask &mask = privateBuffer->GetExecutorCacheMask();
    if (_writeBackExecutor->GetOutputValue(output, mask)) {
        return;
    }

    // Attempt to transfer ownership of the buffer to the write back executor.
    // Relinquish ownership of the private buffer if this operation succeeds:
    // The write back executor will now own this buffer instead.
    VdfVector *value = privateBuffer->GetExecutorCache();
    if (_writeBackExecutor->TakeOutputValue(output, value, mask)) {
        privateBuffer->YieldOwnership();
    } 
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif