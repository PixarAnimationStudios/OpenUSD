//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_PARALLEL_EXECUTOR_ENGINE_H
#define PXR_EXEC_VDF_PARALLEL_EXECUTOR_ENGINE_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/parallelExecutorEngineBase.h"

#include "pxr/base/work/taskGraph.h"
#include "pxr/base/work/withScopedParallelism.h"

#include <tbb/concurrent_unordered_map.h>

#include <atomic>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

// Forward declare the speculation executor engine with equivalent traits to
// this executor engine.
template <typename> class VdfParallelSpeculationExecutorEngine;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfParallelExecutorEngine
///
/// A generic, but fully-featured parallel executor engine, deriving from
/// VdfParallelExecutorEngineBase.
/// 
/// This engine does not perform cycle detection.
///
template < typename DataManagerType >
class VdfParallelExecutorEngine :
    public VdfParallelExecutorEngineBase<
        VdfParallelExecutorEngine<DataManagerType>,
        DataManagerType>
{
public:

    /// The equivalent speculation executor engine. Executor factories can use
    /// this typedef to map from an executor engine to a speculation executor
    /// engine with equivalent traits.
    ///
    typedef
        VdfParallelSpeculationExecutorEngine<DataManagerType>
        SpeculationExecutorEngine;

    /// Base class.
    ///
    typedef
        VdfParallelExecutorEngineBase<
            VdfParallelExecutorEngine<DataManagerType>,
            DataManagerType>
        Base;

    /// Constructor.
    ///
    VdfParallelExecutorEngine(
        const VdfExecutorInterface &executor, 
        DataManagerType *dataManager) :
        Base(executor, dataManager)
    {}

private:

    // Befriend the base class so that it has access to the private methods
    // used for static polymorphism.
    //
    friend Base;

    // This executor engine does not do cycle detection.
    //
    bool _DetectCycle(
        const VdfEvaluationState &state,
        const VdfNode &node);

    // This executor engine supports touching.
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

    // Finalize state after evaluation completed.
    // 
    void _FinalizeEvaluation();

    // Lock the buffer for a given output for mung buffer locking.
    //
    void _LockBuffer(
        const VdfEvaluationState &state,
        const VdfSchedule::OutputId outputId,
        const typename Base::_DataHandle dataHandle,
        const VdfScheduleTaskIndex invocationIndex);

    // Publish all locked buffers. This method must not be invoked before all
    // outputs with locked buffers had their private/scratch buffers published.
    //
    void _PublishLockedBuffers();

    // This structure holds all the data relevant to locked buffers.
    //
    class _LockedData
    {
    public:
        // Constructor.
        //
        _LockedData(
            const VdfOutputSpec &spec,
            const VdfMask &mask,
            const size_t numTasks);

        // Destructor.
        //
        ~_LockedData() {
            delete _value;
        }

        // Merge values into this buffer.
        //
        void Merge(
            const VdfVector &value,
            const VdfMask &mask);

        // Transfer ownership of this buffer.
        //
        void TransferOwnership(VdfExecutorBufferData *destination);

    private:
        // The data locked at this output.
        //
        VdfVector *_value;

        // An array of masks, one for each compute task having merged data
        // into _value.
        //
        // XXX: This should be a small vector to optimize for size = 1.
        //
        std::unique_ptr<VdfMask[]> _masks;

        // The number of entries in the _masks array, which are currently
        // occupied.
        //
        std::atomic<size_t> _num;
    };

    // TBB task wrapping for publishing locked data to public buffers.
    //
    class _PublishLockedDataTask : public WorkTaskGraph::BaseTask 
    {
    public:
        // Constructor. Note that the task takes ownership of lockedData and
        // will destruct the structure and free its memory upon completion.
        //
        _PublishLockedDataTask(
            DataManagerType *dataManager,
            const typename Base::_DataHandle dataHandle,
            _LockedData *lockedData) :
            _dataManager(dataManager),
            _dataHandle(dataHandle),
            _lockedData(lockedData)
        {}

        // Task execution entry point.
        //
        WorkTaskGraph::BaseTask *execute() override;

    private:
        DataManagerType *_dataManager;
        const typename Base::_DataHandle _dataHandle;
        std::unique_ptr<_LockedData> _lockedData;
    };

    // Insert a new entry into the locked data map.
    //
    _LockedData *_InsertLockedData(
        const VdfEvaluationState &state,
        const VdfSchedule::OutputId outputId,
        const typename Base::_DataHandle dataHandle);

    // A map from output data handle to data locked at the output.
    //
    typedef 
        tbb::concurrent_unordered_map<typename Base::_DataHandle, _LockedData *>
        _LockedDataMap;
    _LockedDataMap _lockedDataMap;
};

///////////////////////////////////////////////////////////////////////////////

template < typename DataManagerType >
inline bool
VdfParallelExecutorEngine<DataManagerType>::_DetectCycle(
    const VdfEvaluationState &state,
    const VdfNode &node)
{
    return false;
}

template < typename DataManagerType >
void
VdfParallelExecutorEngine<DataManagerType>::_Touch(
    const VdfOutput &output)
{
    Base::_dataManager->DataManagerType::Base::Touch(output);
}

template < typename DataManagerType >
void
VdfParallelExecutorEngine<DataManagerType>::_FinalizeOutput(
    const VdfEvaluationState &state,
    const VdfOutput &output,
    const VdfSchedule::OutputId outputId,
    const typename Base::_DataHandle dataHandle,
    const VdfScheduleTaskIndex invocationIndex,
    const VdfOutput *passToOutput)
{
    // Does this buffer require locking?
    if (passToOutput && Base::_dataManager->HasInvalidationTimestampMismatch(
            dataHandle,
            Base::_dataManager->GetDataHandle(passToOutput->GetId()))) {
        _LockBuffer(state, outputId, dataHandle, invocationIndex);
    }
}

template < typename DataManagerType >
void
VdfParallelExecutorEngine<DataManagerType>::_FinalizeEvaluation()
{
    _PublishLockedBuffers();
}

template < typename DataManagerType >
void
VdfParallelExecutorEngine<DataManagerType>::_LockBuffer(
    const VdfEvaluationState &state,
    const VdfSchedule::OutputId outputId,
    const typename Base::_DataHandle dataHandle,
    const VdfScheduleTaskIndex invocationIndex)
{
    PEE_TRACE_SCOPE("VdfParallelExecutorEngine::_LockBuffer");

    // Get the locked data structure for the given output as identified by
    // its data handle. If no locked data exists for that output, insert a
    // new one into the map.
    typename _LockedDataMap::iterator it = _lockedDataMap.find(dataHandle);
    _LockedData *lockedData = it == _lockedDataMap.end()
        ? _InsertLockedData(state, outputId, dataHandle)
        : it->second;

    // Get the private buffer data. This is the buffer that we want to lock.
    VdfExecutorBufferData *privateBuffer =
        Base::_dataManager->GetPrivateBufferData(dataHandle);

    // Retrieve the lock mask based on whether the buffer is being locked for
    // a node with multiple invocations, or just a single invocation. The
    // lock mask is the relevant request mask in either case.
    const VdfSchedule &schedule = state.GetSchedule();
    const VdfMask &lockMask = !VdfScheduleTaskIsInvalid(invocationIndex)
        ? schedule.GetRequestMask(invocationIndex)
        : schedule.GetRequestMask(outputId);

    // Merge the private buffer data into the locked data structure.
    lockedData->Merge(*privateBuffer->GetExecutorCache(), lockMask);
}

template < typename DataManagerType >
void
VdfParallelExecutorEngine<DataManagerType>::_PublishLockedBuffers()
{
    // Bail out if there is no locked data to publish.
    if (_lockedDataMap.empty()) {
        return;
    }

    PEE_TRACE_SCOPE("VdfParallelExecutorEngine::_PublishLockedBuffers");

    WorkWithScopedParallelism(
        [&lockedDataMap = _lockedDataMap, &taskGraph = Base::_taskGraph,
            &dataManager = Base::_dataManager] {
        // For each entry in the locked data map, spawn a new task to publish
        // the locked data.
        for (const auto &data : lockedDataMap) {
            // Allocate a new task responsible for publishing the data. Note
            // that the _PublishLockedDataTask will take ownership of the locked
            // data structure, and is responsible for deallocating it.
            _PublishLockedDataTask * const task =
                taskGraph.template AllocateTask<_PublishLockedDataTask>(
                    dataManager, data.first, data.second);
            taskGraph.RunTask(task);
        }

        // Clear the map, while the data is still being published.
        lockedDataMap.clear();

        // Wait for all the publishing to complete.
        taskGraph.Wait();
    });
}

template < typename DataManagerType >
typename VdfParallelExecutorEngine<DataManagerType>::_LockedData *
VdfParallelExecutorEngine<DataManagerType>::_InsertLockedData(
    const VdfEvaluationState &state,
    const VdfSchedule::OutputId outputId,
    const typename Base::_DataHandle dataHandle)
{
    // Get the output and node.
    const VdfSchedule &schedule = state.GetSchedule();
    const VdfOutput &output = *schedule.GetOutput(outputId);
    const VdfNode &node = output.GetNode();

    // Get all the compute tasks for this node.
    VdfSchedule::TaskIdRange tasks = schedule.GetComputeTaskIds(node);

    // Construct a new instance of the locked data structure.
    _LockedData *newData = new _LockedData(
        output.GetSpec(), schedule.GetRequestMask(outputId), tasks.size());

    // Attempt to insert the newly allocated locked data structure into the
    // map. Note that this will fail if a parallel thread got to inserting
    // an instance for the same output data handle first.
    std::pair<typename _LockedDataMap::iterator, bool> res =
        _lockedDataMap.insert(std::make_pair(dataHandle, newData));

    // If the insertion failed, because another thread got to inserting the
    // data first, we need to free the new locked data structure instance we
    // just allocated.
    if (!res.second) {
        delete newData;
    }

    // The insertion suceeded if this is the first time that this output is
    // being locked during this round of evaluation. In that case, and if the
    // node has more than a single compute task, make sure that all compute
    // tasks will be run. Otherwise, cache hits on other outputs could result in
    // some of these compute tasks not being invoked, and cause the locked
    // buffer to be incomplete.
    else if (tasks.size() > 1) {
        Base::_taskGraph.RunTask(
            Base::_taskGraph.template AllocateTask<
                typename Base::_ComputeAllTask>(this, state, node));
    }

    // Return a pointer to the locked data structure instance (either the newly
    // inserted one, or an existing one).
    return res.first->second;
}

template < typename DataManagerType >
WorkTaskGraph::BaseTask *
VdfParallelExecutorEngine<DataManagerType>::_PublishLockedDataTask::execute()
{
    // We are going to publish the locked data to the public buffer.
    VdfExecutorBufferData *publicBuffer =
        _dataManager->GetPublicBufferData(_dataHandle);

    // Transfer ownership of the locked data to the public buffer.
    _lockedData->TransferOwnership(publicBuffer);

    // No scheduler bypass.
    return nullptr;
}

template < typename DataManagerType >
VdfParallelExecutorEngine<DataManagerType>::_LockedData::_LockedData(
    const VdfOutputSpec &spec,
    const VdfMask &mask,
    const size_t numTasks) :
    _num(0)
{
    // Allocate a new VdfVector for the given output.
    _value = spec.AllocateCache();

    // Make sure the VdfVector is appropriately sized in order to accommodate
    // all the data as indicated by the mask.
    spec.ResizeCache(_value, mask.GetBits());

    // Allocate an array of masks - one for each task. Note that not all tasks
    // will necessarily end up locking any data, but pre-sizing the array for
    // the worst case allows us to remain lockless.
    _masks.reset(new VdfMask[numTasks]);
}

template < typename DataManagerType >
void
VdfParallelExecutorEngine<DataManagerType>::_LockedData::Merge(
    const VdfVector &value,
    const VdfMask &mask)
{
    // Merge all the data from the source vector into the internal vector.
    _value->Merge(value, mask);

    // Store the mask to denote the data entries we just copied. We will
    // accumulate all the masks when we later publish the locked buffer.
    const size_t maskIdx = _num.fetch_add(1, std::memory_order_release);
    _masks[maskIdx] = mask;
}

template < typename DataManagerType >
void
VdfParallelExecutorEngine<DataManagerType>::_LockedData::TransferOwnership(
    VdfExecutorBufferData *destination)
{
    // If there is no data locked at the output, bail out early.
    const size_t num = _num.load(std::memory_order_acquire);
    if (num == 0) {
        return;
    }

    // If there is only a single mask locked at this output, and the 
    // destination buffer does not have its own cache, we can straight up
    // transfer ownership to the destination. This is the fast path.
    if (num == 1 && !destination->GetExecutorCache()) {
        destination->SetExecutorCacheMask(_masks[0]);
        destination->TakeOwnership(_value);
        _value = nullptr;
        return;
    }

    // Accumulate all the masks locked for this output.
    VdfMask::Bits unionBits(_masks[0].GetBits());
    for (size_t i = 1; i < num; ++i) {
        unionBits |= _masks[i].GetBits();
    }

    // If the destination buffer already has a cache, we need to merge the
    // locked cache into the existing cache, and append the locked mask to
    // the cache mask. The destination buffer does not take ownership of the
    // local VdfVector in this case.
    if (VdfVector *value = destination->GetExecutorCache()) {
        value->Merge(*_value, unionBits);
        unionBits |= destination->GetExecutorCacheMask().GetBits();
    }

    // If the destination buffer has no cache, we can simply transfer ownership
    // of the local VdfVector. Don't forget to reset the local VdfVector
    // pointer to prevent double-freeing the memory (this constructor and
    // the VdfExecutorBufferData owner).
    else {
        destination->TakeOwnership(_value);
        _value = nullptr;
    }

    // Finally, apply the executor cache mask using the accumulated mask.
    destination->SetExecutorCacheMask(VdfMask(unionBits));
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
