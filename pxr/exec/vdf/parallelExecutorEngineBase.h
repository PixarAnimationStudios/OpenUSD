//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_PARALLEL_EXECUTOR_ENGINE_BASE_H
#define PXR_EXEC_VDF_PARALLEL_EXECUTOR_ENGINE_BASE_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/evaluationState.h"
#include "pxr/exec/vdf/executorBufferData.h"
#include "pxr/exec/vdf/executorErrorLogger.h"
#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/executionStats.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/networkUtil.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/parallelTaskSync.h"
#include "pxr/exec/vdf/requiredInputsPredicate.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/vector.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/errorTransport.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/taskGraph.h"
#include "pxr/base/work/withScopedParallelism.h"

#include <tbb/concurrent_vector.h>

PXR_NAMESPACE_OPEN_SCOPE

// Use this macro to enable tracing in the executor engine.
#define PEE_TRACE_SCOPE(x)

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfParallelExecutorEngineBase
///
/// The base class for all parallel executor engines. This executor
/// engine evaluates a parallel task graph generated at scheduling time. It
/// evaluates each node and all their invocations in different tasks, which
/// can then run on separate threads. This executor engine does branch multi-
/// threading, as well as strip-mining. It also produces multiple invocations
/// for nodes that mutate a lot of data, potentially spreading the work of a
/// single node across multiple threads.
///
template < typename Derived, typename DataManager >
class VdfParallelExecutorEngineBase
{
    typedef VdfParallelExecutorEngineBase<Derived, DataManager> This;

public:
    /// Noncopyable.
    ///
    VdfParallelExecutorEngineBase(
        const VdfParallelExecutorEngineBase &) = delete;
    VdfParallelExecutorEngineBase &operator=(
        const VdfParallelExecutorEngineBase &) = delete;


    /// Constructor.
    ///
    VdfParallelExecutorEngineBase(
        const VdfExecutorInterface &executor, 
        DataManager *dataManager);

    /// Destructor.
    ///
    virtual ~VdfParallelExecutorEngineBase();

    /// Executes the given \p schedule with a \p computeRequest and an optional
    /// \p errorLogger.
    ///
    void RunSchedule(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        VdfExecutorErrorLogger *errorLogger) {
        RunSchedule(
            schedule, computeRequest, errorLogger,
            [](const VdfMaskedOutput &, size_t){});
    }

    /// Executes the given \p schedule with a \p computeRequest and an optional
    /// \p errorLogger. Concurrently invokes \p callback after evaluation of
    /// each uncached output in the request, and immediatelly after hitting the
    /// cache for cached outputs in the request.
    ///
    template < typename Callback >
    void RunSchedule(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        VdfExecutorErrorLogger *errorLogger,
        Callback &&callback);

protected:
    // The data handle type from the data manager implementation.
    typedef typename DataManager::DataHandle _DataHandle;

    // An integer type for storing the current per-task evaluation stage.
    typedef uint32_t _EvaluationStage;

    // A leaf task, i.e. the entry point for parallel evaluation.
    template < typename Callback >
    class _LeafTask : public WorkTaskGraph::BaseTask
    {
    public:
        _LeafTask(
            This *engine,
            const VdfEvaluationState &state,
            const VdfMaskedOutput &output,
            const size_t requestedIndex,
            Callback &callback) :
            _engine(engine),
            _state(state),
            _output(output),
            _requestedIndex(requestedIndex),
            _callback(callback),
            _evaluationStage(0)
        {}

        // Task execution entry point.
        WorkTaskGraph::BaseTask * execute() override;

    private:
        This *_engine;
        const VdfEvaluationState &_state;
        const VdfMaskedOutput &_output;
        const size_t _requestedIndex;
        Callback &_callback;
        _EvaluationStage _evaluationStage;
    };

    // A scheduled compute task.
    class _ComputeTask : public WorkTaskGraph::BaseTask
    {
    public:
        _ComputeTask(
            This *engine,
            const VdfEvaluationState &state,
            const VdfNode &node,
            VdfScheduleTaskId taskIndex) :
            _engine(engine),
            _state(state),
            _node(node),
            _taskIndex(taskIndex),
            _evaluationStage(0)
        {}

        // Task execution entry point.
        WorkTaskGraph::BaseTask *execute() override;

    private:
        This *_engine;
        const VdfEvaluationState &_state;
        const VdfNode &_node;
        VdfScheduleTaskId _taskIndex;
        _EvaluationStage _evaluationStage;
    };

    // A scheduled inputs task.
    class _InputsTask : public WorkTaskGraph::BaseTask
    {
    public:
        _InputsTask(
            This *engine,
            const VdfEvaluationState &state,
            const VdfNode &node,
            VdfScheduleTaskIndex taskIndex) :
            _engine(engine),
            _state(state),
            _node(node),
            _taskIndex(taskIndex),
            _evaluationStage(0)
        {}

        // Task execution entry point.
        WorkTaskGraph::BaseTask *execute();

    private:
        This *_engine;
        const VdfEvaluationState &_state;
        const VdfNode &_node;
        VdfScheduleTaskIndex _taskIndex;
        _EvaluationStage _evaluationStage;
    };

    // A scheduled keep task.
    class _KeepTask : public WorkTaskGraph::BaseTask
    {
    public:
        _KeepTask(
            This *engine,
            const VdfEvaluationState &state,
            const VdfNode &node,
            VdfScheduleTaskIndex taskIndex) :
            _engine(engine),
            _state(state),
            _node(node),
            _taskIndex(taskIndex),
            _evaluationStage(0)
        {}

        // Task execution entry point.
        WorkTaskGraph::BaseTask *execute();

    private:
        This *_engine;
        const VdfEvaluationState &_state;
        const VdfNode &_node;
        VdfScheduleTaskIndex _taskIndex;
        _EvaluationStage _evaluationStage;
    };

    // A touch-task for touching all outputs between a from-buffer source
    // and a destination output.
    class _TouchTask : public WorkTaskGraph::BaseTask
    {
    public:
        _TouchTask(
            This *engine,
            const VdfOutput &dest,
            const VdfOutput &source) :
            _engine(engine),
            _dest(dest),
            _source(source)
        {}

        // Task execution entry point.
        WorkTaskGraph::BaseTask *execute();

    private:
        This *_engine;
        const VdfOutput &_dest;
        const VdfOutput &_source;
    };

    // A task that invokes all compute tasks scheduled for a particular node. 
    class _ComputeAllTask : public WorkTaskGraph::BaseTask {
    public:
        _ComputeAllTask(
            This *engine, 
            const VdfEvaluationState &state, 
            const VdfNode &node) :
            _engine(engine),
            _state(state),
            _node(node),
            _completed(false)
        {}
        
        WorkTaskGraph::BaseTask *execute();

    private:
        This *_engine;
        const VdfEvaluationState &_state;
        const VdfNode &_node;
        bool _completed;
    };

    // Reset the engine's internal state. Every round of evaluation starts with
    // clean state.
    void _ResetState(const VdfSchedule &schedule);

    // Run a single, requested output. If the output is uncached, this will
    // reset the internal state (if not already done), and add the leaf task to
    // the task list.
    template < typename Callback >
    void _RunOutput(
        const VdfEvaluationState &state,
        const VdfMaskedOutput &maskedOutput,
        const size_t requestedIndex,
        Callback &callback,
        WorkTaskGraph::TaskList *taskList);

    // Spawn the task(s) requested for a given node. These are the tasks spawn
    // as entry points into evaluating the schedule. Remaining tasks will be
    // spawn as input dependencies to these requested tasks.
    void _SpawnRequestedTasks(
        const VdfEvaluationState &state,
        const VdfNode &node,
        WorkTaskGraph::BaseTask *successor,
        WorkTaskGraph::BaseTask **bypass);

    // Spawn a new task, or assign the task to the bypass output parameter,
    // if no task has previously been assigned to bypass. The output
    // parameter can later be used to drive scheduler bypassing in order to
    // reduce scheduling overhead.
    void _SpawnOrBypass(
        WorkTaskGraph::BaseTask *task,
        WorkTaskGraph::BaseTask **bypass);

    // The task execution entry point for the scheduled leaf tasks. These tasks
    // are the main entry points to evaluation. The engine will spawn one leaf
    // task for each uncached requested output. Returns true if the task is not
    // done after returning, and must therefore be recycled for re-execution
    // after all its input dependencies have been completed.
    template < typename Callback >
    bool _ProcessLeafTask(
        WorkTaskGraph::BaseTask *task,
        const VdfEvaluationState &state,
        const VdfMaskedOutput &maskedOutput,
        const size_t requestedIndex,
        Callback &callback,
        _EvaluationStage *evaluationStage,
        WorkTaskGraph::BaseTask **bypass);

    // The task execution entry point for scheduled compute tasks. Returns
    // true if the task is not done after returning, and must therefore be
    // recycled for re-execution after all its input dependencies have been
    // completed.
    bool _ProcessComputeTask(
        WorkTaskGraph::BaseTask *task,
        const VdfEvaluationState &state,
        const VdfNode &node,
        const VdfScheduleComputeTask &scheduleTask,
        _EvaluationStage *evaluationStage,
        WorkTaskGraph::BaseTask **bypass);

    // The task execution entry point for scheduled inputs tasks. Returns
    // true if the task is not done after returning, and must therefore be
    // recycled for re-execution after all its input dependencies have been
    // completed.
    bool _ProcessInputsTask(
        WorkTaskGraph::BaseTask *task,
        const VdfEvaluationState &state,
        const VdfNode &node,
        const VdfScheduleInputsTask &scheduleTask,
        _EvaluationStage *evaluationStage,
        WorkTaskGraph::BaseTask **bypass);

    // The task execution entry point for scheduled keep tasks. Returns
    // true if the task is not done after returning, and must therefore be
    // recycled for re-execution after all its input dependencies have been
    // completed.
    bool _ProcessKeepTask(
        WorkTaskGraph::BaseTask *task,
        const VdfEvaluationState &state,
        const VdfNode &node,
        _EvaluationStage *evaluationStage,
        WorkTaskGraph::BaseTask **bypass);

    // Invokes a keep task, as an input dependency to the successor task.
    // Returns true if the successor must wait for completion of the newly
    // invoked task. If this method returns false, the input dependency
    // has already been fulfilled.
    bool _InvokeKeepTask(
        const VdfScheduleTaskIndex idx,
        const VdfNode &node,
        const VdfEvaluationState &state,
        WorkTaskGraph::BaseTask *successor,
        WorkTaskGraph::BaseTask **bypass);

    // Invokes a touch task, touching all outputs between dest and source. The
    // touching happens in the background. Only the root task synchronizes on
    // this work.
    void _InvokeTouchTask(
        const VdfOutput &dest,
        const VdfOutput &source);

    // Invokes a compute task, as an input dependency to the successor task.
    // Returns true if the successor must wait for completion of the newly
    // invoked task. If this method returns false, the input dependency
    // has already been fulfilled.
    bool _InvokeComputeTask(
        const VdfScheduleTaskId taskIndex,
        const VdfEvaluationState &state,
        const VdfNode &node,
        WorkTaskGraph::BaseTask *successor,
        WorkTaskGraph::BaseTask **bypass);

    // Calls _InvokeComputeTask on an iterable range of tasks.
    template < typename Iterable >
    bool _InvokeComputeTasks(
        const Iterable &tasks,
        const VdfEvaluationState &state,
        const VdfNode &node,
        WorkTaskGraph::BaseTask *successor,
        WorkTaskGraph::BaseTask **bypass);

    // Check whether the output attached to the input dependency has already
    // been cached.
    bool _IsInputDependencyCached(
        VdfScheduleInputDependencyUniqueIndex uniqueIndex,
        const VdfOutput &output,
        const VdfMask &mask);

    // Calls _InvokeComputeTask on a range of tasks specified by input.
    // Alternatively, if input specifies a keep task, this method will invoke
    // the keep task instead.
    bool _InvokeComputeOrKeepTasks(
        const VdfScheduleInputDependency &input,
        const VdfEvaluationState &state,
        WorkTaskGraph::BaseTask *successor,
        WorkTaskGraph::BaseTask **bypass);

    // Calls _InvokeComputeTask on a range of tasks providing values for the
    // specified output. Alternatively, if the values for the specified output
    // are being provided by a keep task, this method will invoke the keep task
    // instead.
    bool _InvokeComputeOrKeepTasks(
        const VdfOutput &output,
        const VdfEvaluationState &state,
        WorkTaskGraph::BaseTask *successor,
        WorkTaskGraph::BaseTask **bypass);

    // Invokes all the compute tasks required to fulfill all prereq
    // dependencies. Returns true if the successor must wait for completion of
    // the newly invoked tasks. If this method returns false, the input
    // dependencies have already been fulfilled.
    bool _InvokePrereqInputs(
        const VdfScheduleInputsTask &scheduleTask,
        const VdfEvaluationState &state,
        WorkTaskGraph::BaseTask *successor,
        WorkTaskGraph::BaseTask **bypass);

    // Invokes all the compute tasks required to fulfill all optional input
    // dependencies (those dependent on the results of prereqs). Returns true
    // if the successor must wait for completion of the newly invoked tasks. If
    // this method returns false, the input dependencies have already been
    // fulfilled.
    bool _InvokeOptionalInputs(
        const VdfScheduleInputsTask &scheduleTask,
        const VdfEvaluationState &state,
        const VdfNode &node,
        WorkTaskGraph::BaseTask *successor,
        WorkTaskGraph::BaseTask **bypass);

    // Invokes all the compute tasks required to fulfill all required input
    // dependencies (those not dependent on prereqs, and read/writes). Returns
    // true if the successor must wait for completion of the newly invoked
    // tasks. If this method returns false, the input dependencies have already
    // been fulfilled.
    bool _InvokeRequiredInputs(
        const VdfScheduleComputeTask &scheduleTask,
        const VdfEvaluationState &state,
        WorkTaskGraph::BaseTask *successor,
        WorkTaskGraph::BaseTask **bypass);

    // Invokes an inputs task, as an input dependency to the successor task.
    // Returns true if the successor must wait for completion of the newly
    // invoked task. If this method returns false, the input dependency
    // has already been fulfilled.
    bool _InvokeInputsTask(
        const VdfScheduleComputeTask &scheduleTask,
        const VdfEvaluationState &state,
        const VdfNode &node,
        WorkTaskGraph::BaseTask *successor,
        WorkTaskGraph::BaseTask **bypass);

    // Invokes a task that prepares a node for execution, as an input
    // dependency to the successor task. Returns true if the successor must
    // wait for completion of the newly invoked task. If this method returns
    // false, the input dependency has already been fulfilled.
    bool _InvokePrepTask(
        const VdfScheduleComputeTask &scheduleTask,
        const VdfEvaluationState &state,
        const VdfNode &node,
        WorkTaskGraph::BaseTask *successor);

    // Prepares a node for execution. Every node has to be prepared exactly
    // once. Nodes with multiple invocations will be prepared by the first
    // compute task that gets to the node preparation stage.
    void _PrepareNode(
        const VdfEvaluationState &state,
        const VdfNode &node);

    // Prepares an output for execution.
    void _PrepareOutput(
        const VdfSchedule &schedule,
        const VdfSchedule::OutputId outputId);

    // Create the cache for the scratch buffer. This will make sure the cache
    // can accomodate all the data denoted by mask.
    void _CreateScratchCache(
        const VdfOutput &output,
        const _DataHandle dataHandle,
        const VdfMask &mask,
        VdfExecutorBufferData *scratchBuffer);

    // Evaluate a node by either invoking its Compute() method, or passing
    // through all data.
    void _EvaluateNode(
        const VdfScheduleComputeTask &scheduleTask,
        const VdfEvaluationState &state,
        const VdfNode &node,
        WorkTaskGraph::BaseTask *successor);

    // Compute a node by invoking its Compute() method.
    void _ComputeNode(
        const VdfScheduleComputeTask &scheduleTask,
        const VdfEvaluationState &state,
        const VdfNode &node);

    // Pass all the read/write data through the node.
    void _PassThroughNode(
        const VdfScheduleComputeTask &scheduleTask,
        const VdfEvaluationState &state,
        const VdfNode &node);

    // Process an output after execution.
    void _ProcessOutput(
        const VdfScheduleComputeTask &scheduleTask,
        const VdfEvaluationState &state,
        const VdfOutput &output,
        const VdfSchedule::OutputId outputId,
        const _DataHandle dataHandle,
        const bool hasAssociatedInput,
        VdfExecutorBufferData *privateBuffer);

    // Prepares a read/write buffer by ensure that the private data is
    // available at the output.
    void _PrepareReadWriteBuffer(
        const VdfOutput &output,
        const VdfSchedule::OutputId outputId,
        const VdfMask &mask,
        const VdfSchedule &schedule,
        VdfExecutorBufferData *privateBuffer);

    // Pass a read/write buffer from the source output to the destination
    // output, or copy the data if required.
    void _PassOrCopyBuffer(
        const VdfOutput &output,
        const VdfOutput &source,
        const VdfMask &inputMask,
        const VdfSchedule &schedule,
        VdfExecutorBufferData *privateBuffer);

    // Pass a read/write buffer from the source buffer to the destination
    // buffer.
    //
    void _PassBuffer(
        VdfExecutorBufferData *fromBuffer,
        VdfExecutorBufferData *toBuffer) const;

    // Copy a read/write buffer from the source output to the destination
    // output.
    void _CopyBuffer(
        const VdfOutput &output,
        const VdfOutput &source,
        const VdfMask &fromMask,
        VdfExecutorBufferData *toData) const;

    // Publish the data in the scratch buffers of this node.
    void _PublishScratchBuffers(
        const VdfSchedule &schedule,
        const VdfNode &node);

    // Copies all of the publicly available data missing from \p haveMask into
    // the scratch buffer and extends the executor cache mask. Returns a pointer
    // to the destination vector if any data was copied.
    VdfVector *_AbsorbPublicBuffer(
        const VdfOutput &output,
        const _DataHandle dataHandle,
        const VdfMask &haveMask);

    // Detects interruption by querying the executor interruption API and
    // calling into the derived engine to do cycle detection. Sets the
    // interruption flag if interruption (or a cycle) has been detected.
    bool _DetectInterruption(
        const VdfEvaluationState &state,
        const VdfNode &node);

    // Returns true if the interruption flag (as determined by
    // _DetectInterruption()) has been set.
    bool _HasDetectedInterruption() const;

    // Create an error transport out of an error mark to enable transferring
    // the errors to the calling thread later on.
    void _TransportErrors(const TfErrorMark &errorMark);

    // Post all the transported errors on the calling thread.
    void _PostTransportedErrors();

    // Returns a reference to the derived class for static polymorphism.
    Derived &_Self() {
        return *static_cast<Derived *>(this);
    }

    // The executor that uses this engine.
    const VdfExecutorInterface &_executor;

    // The data manager populated by this engine.
    DataManager *_dataManager;

    // A task graph for dynamically adding and spawning tasks during execution. 
    WorkTaskGraph _taskGraph;

    // Keep track of which unique input dependencies have had their cached
    // state checked.
    std::unique_ptr<std::atomic<uint8_t>[]> _dependencyState;

    // The structures that orchestrate synchronization for the different task
    // types.
    //
    // XXX: We should explore folding all these into a single instance.
    std::atomic<bool> _resetState;
    VdfParallelTaskSync _computeTasks;
    VdfParallelTaskSync _inputsTasks;
    VdfParallelTaskSync _prepTasks;
    VdfParallelTaskSync _keepTasks;

    // Keep a record of errors to post to the calling thread.
    tbb::concurrent_vector<TfErrorTransport> _errors;

    // Stores the interruption signal as determined by _DetectInterruption.
    std::atomic<bool> _isInterrupted;
};

///////////////////////////////////////////////////////////////////////////////

template < typename Derived, typename DataManager >
VdfParallelExecutorEngineBase<Derived, DataManager>::
    VdfParallelExecutorEngineBase(
        const VdfExecutorInterface &executor, 
        DataManager *dataManager) :
        _executor(executor),
        _dataManager(dataManager),
        _resetState(),
        _computeTasks(&_taskGraph),
        _inputsTasks(&_taskGraph),
        _prepTasks(&_taskGraph),
        _keepTasks(&_taskGraph),
        _isInterrupted()
{
}

template < typename Derived, typename DataManager >
VdfParallelExecutorEngineBase<Derived, DataManager>::
    ~VdfParallelExecutorEngineBase()
{
}

template < typename Derived, typename DataManager >
template < typename Callback >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::RunSchedule(
    const VdfSchedule &schedule,
    const VdfRequest &computeRequest,
    VdfExecutorErrorLogger *errorLogger,
    Callback &&callback)
{
    TRACE_SCOPE("VdfParallelExecutorEngineBase::RunSchedule");

    // Make sure the data manager is appropriately sized.
    _dataManager->Resize(*schedule.GetNetwork());

    // Indicate that the internal state has not yet been reset.
    _resetState.store(false, std::memory_order_relaxed);

    // The persistent evaluation state.
    VdfEvaluationState state(_executor, schedule, errorLogger);

    // Build an indexed view ontop of the compute request. We will use this
    // view for random access into the compute request in a parallel for-loop.
    VdfRequest::IndexedView view(computeRequest);

    // Perform all the work of spawning and waiting on tasks with scoped
    // parallelism, in order to prevent evaluation tasks from being stolen in
    // unrelated loops.
    VdfParallelExecutorEngineBase<Derived, DataManager> *engine = this;
    WorkWithScopedParallelism([engine, &state, &view, &callback] {
        // Collect all the leaf tasks, which are the entry point for evaluation.
        // We will later spawn all these tasks together.
        WorkTaskGraph::TaskLists taskLists;

        // Run all the outputs in parallel. This will reset the internal state,
        // if necessary, and collect all the leaf tasks for uncached outputs.
        WorkParallelForN(
            view.GetSize(),
            [engine, &state, &view, &callback, &taskLists]
            (size_t b, size_t e) {
                WorkTaskGraph::TaskList *taskList = &taskLists.local();
                for (size_t i = b; i != e; ++i) {
                    if (const VdfMaskedOutput *maskedOutput = view.Get(i)) {
                        engine->_RunOutput(
                            state, *maskedOutput, i, callback, taskList);
                    }
                }
            });

        // Now, spawn all the leaf tasks for uncached outputs. We need to first
        // check the cache for all requested outputs, before even running the
        // first uncached one. Otherwise, we could get cache hits for outputs
        // that were just computed, failing to invoke the callback. 
        engine->_taskGraph.RunLists(taskLists);

        // Now, wait for all the tasks to complete.
        {
            TRACE_SCOPE(
                "VdfParallelExecutorEngineBase::RunSchedule "
                "(wait for parallel tasks)");
            engine->_taskGraph.Wait();
        }
    });

    // Allow the derived executor engine to finalize state after evaluation
    // completed.
    _Self()._FinalizeEvaluation();

    // Reset the interruption signal.
    _isInterrupted.store(false, std::memory_order_relaxed);

    // Post all transported errors on the calling thread.
    _PostTransportedErrors();
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_ResetState(
    const VdfSchedule &schedule)
{
    TRACE_FUNCTION();

    // Each input dependency is uniquely indexed in the schedule, and each
    // input dependency may be required by more than a single node / invocation.
    // We only check state of each input dependency once, cache the result,
    // and then re-use that cache for subsequent lookups.
    const size_t numUniqueDeps = schedule.GetNumUniqueInputDependencies();
    _dependencyState.reset(new std::atomic<uint8_t>[numUniqueDeps]);
    char *const dependencyState =
        reinterpret_cast<char*>(_dependencyState.get());
    memset(dependencyState, 0,
           sizeof(std::atomic<uint8_t>) * numUniqueDeps);

    // Reset the task synchronization structures for all the different types
    // of tasks.
    _computeTasks.Reset(schedule.GetNumComputeTasks());
    _inputsTasks.Reset(schedule.GetNumInputsTasks());
    _prepTasks.Reset(schedule.GetNumPrepTasks());
    _keepTasks.Reset(schedule.GetNumKeepTasks());
}

template < typename Derived, typename DataManager >
template < typename Callback >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_RunOutput(
    const VdfEvaluationState &state,
    const VdfMaskedOutput &maskedOutput,
    const size_t requestedIndex,
    Callback &callback,
    WorkTaskGraph::TaskList *taskList)
{
    // The output and mask for the output to run.
    const VdfOutput &output = *maskedOutput.GetOutput();
    const VdfMask &mask = maskedOutput.GetMask();

    // Check whether the output already has a value cached. If that's the case
    // we do not need to run the output, but we must invoke the callback to
    // notify the client side that evaluation of the requested output has
    // completed.
    if (_executor.GetOutputValue(output, mask)) {
        callback(maskedOutput, requestedIndex);
        return;
    }

    // If the output is uncached we need to eventually run its leaf task. This
    // means that we need the internal state to be reset. Attempt to do that
    // now, if it hasn't already happened.
    bool isReset = _resetState.load(std::memory_order_relaxed);
    if (!isReset && _resetState.compare_exchange_strong(isReset, true)) {
        _ResetState(state.GetSchedule());
    }

    // Then allocate a leaf task and add it to the task list. We will spawn it
    // later along with all other leaf tasks.
    WorkTaskGraph::BaseTask * task = 
       _taskGraph.AllocateTask< _LeafTask<Callback> >(
            this, state, maskedOutput, requestedIndex, callback);
    taskList->push_back(task);
}

template < typename Derived, typename DataManager >
template < typename Callback >
WorkTaskGraph::BaseTask *
VdfParallelExecutorEngineBase<Derived, DataManager>::
    _LeafTask<Callback>::execute()
{
    // Bump the ref count to 1, because as child tasks finish executing before 
    // returning from this function, we don't want this task to get re-executed 
    // prematurely. 
    AddChildReference();

    // Dedicate one task for scheduler bypass to reduce scheduling overhead.
    WorkTaskGraph::BaseTask *bypass = nullptr;

    // Process the scheduled task, and recycle this task for re-execution if
    // requested. Note that this will implicitly decrement the ref count.
    if (_engine->_ProcessLeafTask(
        this, _state, _output, _requestedIndex, _callback, &_evaluationStage, 
            &bypass)) {
        _RecycleAsContinuation();
    } 

    // If the task is done and does not require re-execution we will have to
    // manually decrement the task's ref count here in order to undo the
    // increment above.
    else {
        RemoveChildReference();
    }

    // Return a task for scheduler bypassing, if any.
    return bypass;
}

template < typename Derived, typename DataManager >
WorkTaskGraph::BaseTask *
VdfParallelExecutorEngineBase<Derived, DataManager>::_ComputeTask::execute()
{
    // Create an error mark, so that we can later detect if any errors have
    // been posted, and transport them to the calling thread.
    TfErrorMark errorMark;

    // Bump the ref count to 1, because as child tasks finish executing before
    // returning from this function, we don't want this task to get re-executed
    // prematurely.
    AddChildReference();

    // Dedicate one task for scheduler bypass to reduce scheduling overhead.
    WorkTaskGraph::BaseTask *bypass = nullptr;

    // Get the scheduled task.
    const VdfScheduleComputeTask &scheduleTask =
        _state.GetSchedule().GetComputeTask(_taskIndex);

    // Process the scheduled task, and recycle this task for re-execution if
    // requested. Note that this will implicitly decrement the ref count.
    if (_engine->_ProcessComputeTask(
            this, _state, _node, scheduleTask, &_evaluationStage, &bypass)) {
        _RecycleAsContinuation();
    }

    // If the task is done and does not require re-execution, mark it as done.
    // If the task is not being recycled, we will have to manually decrement
    // its ref count.
    else {
        _engine->_computeTasks.MarkDone(_taskIndex);
        RemoveChildReference();
    }

    // If any errors have been recorded, transport them so that they can later
    // be posted to the calling thread.
    if (!errorMark.IsClean()) {
        _engine->_TransportErrors(errorMark);
    }

    // Return a task for scheduler bypassing, if any.
    return bypass;
}

template < typename Derived, typename DataManager >
WorkTaskGraph::BaseTask *
VdfParallelExecutorEngineBase<Derived, DataManager>::_InputsTask::execute()
{
    // Bump the ref count to 1, because as child tasks finish executing before
    // returning from this function, we don't want this task to get re-executed
    // prematurely.
    AddChildReference();

    // Dedicate one task for scheduler bypass to reduce scheduling overhead.
    WorkTaskGraph::BaseTask *bypass = nullptr;

    // Get the scheduled task.
    const VdfScheduleInputsTask &scheduleTask =
        _state.GetSchedule().GetInputsTask(_taskIndex);

    // Process the scheduled task, and recycle this task for re-execution if
    // requested. Note that this will implicitly decrement the ref count.
    if (_engine->_ProcessInputsTask(
            this, _state, _node, scheduleTask, &_evaluationStage, &bypass)) {
        _RecycleAsContinuation();
    } 

    // If the task is done and does not require re-execution, mark it as done.
    // We will have to manually decrement the task's ref count here.
    else {
        _engine->_inputsTasks.MarkDone(_taskIndex);
        RemoveChildReference();
    }

    // Return a task for scheduler bypassing, if any.
    return bypass;
}

template < typename Derived, typename DataManager >
WorkTaskGraph::BaseTask *
VdfParallelExecutorEngineBase<Derived, DataManager>::_KeepTask::execute()
{
    // Bump the ref count to 1, because as child tasks finish executing before
    // returning from this function, we don't want this task to get re-executed
    // prematurely.
    AddChildReference();

    // Dedicate one task for scheduler bypass to reduce scheduling overhead.
    WorkTaskGraph::BaseTask *bypass = nullptr;

    // Process the scheduled task, and recycle this task for re-execution if
    // requested. Note that this will implicitly decrement the ref count.
    if (_engine->_ProcessKeepTask(
            this, _state, _node, &_evaluationStage, &bypass)) {
        _RecycleAsContinuation();
    } 

    // If the task is done and does not require re-execution, mark it as done.
    // We will have to manually decrement the task's ref count here.
    else {
        _engine->_keepTasks.MarkDone(_taskIndex);
        RemoveChildReference();
    }

    // Return a task for scheduler bypassing, if any.
    return bypass;
}

template < typename Derived, typename DataManager >
WorkTaskGraph::BaseTask *
VdfParallelExecutorEngineBase<Derived, DataManager>::_TouchTask::execute()
{
    // Touch all the output buffers between the source output and the
    // destination output, not including the source output itself.
    const VdfOutput *output = VdfGetAssociatedSourceOutput(_dest);
    while (output && output != &_source) {
        _engine->_Self()._Touch(*output);
        output = VdfGetAssociatedSourceOutput(*output);
    }

    // No scheduler bypass.
    return nullptr;
}

template < typename Derived, typename DataManager >
WorkTaskGraph::BaseTask *
VdfParallelExecutorEngineBase<Derived, DataManager>::_ComputeAllTask::execute()
{
    if (_completed) {
        return nullptr;
    }

    // Bump the ref count to 1, because as child tasks finish executing before
    // returning from this function, we don't want this task to get re-executed
    // prematurely.
    AddChildReference();

    // Invoke all the compute tasks associated with the given node.
    const bool invoked = _engine->_InvokeComputeTasks(
        _state.GetSchedule().GetComputeTaskIds(_node),
        _state, _node, this, nullptr);

    // If any compute tasks were invoked, recycle this task for re-execution.
    // This task will not perform any work upon re-execution, but we use its
    // ref count to synchronize completion of all the compute tasks.
    // Note that recycling will implicitly decrement the ref count.
    if (invoked) {
        _RecycleAsContinuation();
        _completed = true;
    }

    // If the task is done and does not require re-execution, manually decrement
    // the ref count here.
    else {
        RemoveChildReference();
    }

    // No scheduler bypass.
    return nullptr;
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_SpawnRequestedTasks(
    const VdfEvaluationState &state,
    const VdfNode &node,
    WorkTaskGraph::BaseTask *successor,
    WorkTaskGraph::BaseTask **bypass)
{
    // Get the compute tasks associated with the requested node.
    const VdfSchedule &schedule = state.GetSchedule();
    VdfSchedule::TaskIdRange tasks = schedule.GetComputeTaskIds(node);

    // Note that we only actually spawn requested tasks, if the task indices
    // have been claimed successfully. If the task has already been claimed as
    // an input dependency, then the root task will already synchronize on its
    // completion. Otherwise, if the task has already been completed, there
    // isn't anything more to do.

    // If this node has just a single compute task, it can't possible have a
    // keep task. Otherwise, check if the node has a keep task. If so, we need
    // to make sure to spawn the keep task, such that the kept data (the
    // requested data) will be published.
    if (tasks.size() > 1) {
        const VdfScheduleTaskIndex keepTaskIndex =
            schedule.GetKeepTaskIndex(node);
        if (!VdfScheduleTaskIsInvalid(keepTaskIndex)) {
            if (_keepTasks.Claim(keepTaskIndex, successor) ==
                    VdfParallelTaskSync::State::Claimed) {
                _KeepTask *task = 
                    successor->AllocateChild<_KeepTask>(
                        this, state, node, keepTaskIndex);
                _SpawnOrBypass(task, bypass);
            }
            return;
        }
    }
 
    // If there is no keep task, spawn all of the node's compute tasks.
    for (const VdfScheduleTaskId computeTaskIndex : tasks) {
        if (_computeTasks.Claim(computeTaskIndex, successor) ==
                VdfParallelTaskSync::State::Claimed) {
            _ComputeTask *task = 
                successor->AllocateChild<_ComputeTask>(
                    this, state, node, computeTaskIndex);
            _SpawnOrBypass(task, bypass);
        }
    }
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_SpawnOrBypass(
    WorkTaskGraph::BaseTask *task,
    WorkTaskGraph::BaseTask **bypass)
{
    // If bypass has already been assigned a value, spawn the specified task.
    // Otherwise, assign the task to bypass, and later use it to drive the
    // scheduler bypass optimization.

    if (!bypass || *bypass) {
        _taskGraph.RunTask(task);
    } else {
        *bypass = task;
    }
}

template < typename Derived, typename DataManager >
template < typename Callback >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_ProcessLeafTask(
    WorkTaskGraph::BaseTask *task,
    const VdfEvaluationState &state,
    const VdfMaskedOutput &maskedOutput,
    const size_t requestedIndex,
    Callback &callback,
    _EvaluationStage *evaluationStage,
    WorkTaskGraph::BaseTask **bypass)
{
    // The evaluation stages this task can be in.
    enum {
        EvaluationStageSpawn,
        EvaluationStageCallback
    };

    // Handle the current evaluation stage.
    switch (*evaluationStage) {

        // Spawn all the requested tasks, and recycle this task for
        // re-evaluation. Once the requested tasks have been completed, we will
        // re-run this task in the callback stage.
        case EvaluationStageSpawn: {
            const VdfNode &node = maskedOutput.GetOutput()->GetNode();
            _SpawnRequestedTasks(state, node, task, bypass);
            *evaluationStage = EvaluationStageCallback;
            return true;
        }

        // Invoke the callback. This will happen once the requested tasks have
        // run and the output cache has been populated.
        case EvaluationStageCallback: {
            callback(maskedOutput, requestedIndex);
        }
    }

    return false;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_ProcessComputeTask(
    WorkTaskGraph::BaseTask *task,
    const VdfEvaluationState &state,
    const VdfNode &node,
    const VdfScheduleComputeTask &scheduleTask,
    _EvaluationStage *evaluationStage,
    WorkTaskGraph::BaseTask **bypass)
{
    // The evaluation stages this task can be in.
    enum {
        EvaluationStageInputs,
        EvaluationStagePrepNode,
        EvaluationStageEvaluateNode
    };

    // Handle the current evaluation stage.
    switch (*evaluationStage) {

        // Input dependencies.
        case EvaluationStageInputs: {
            // Handle interruption detection during the first stage of
            // evaluation, and bail out if interruption has been detected. This
            // covers the outbound path (finding inputs) of the traversal.
            if (_DetectInterruption(state, node)) {
                return false;
            }

            // Log execution stats for required input dependencies.
            VdfExecutionStats::ScopedEvent scopedEvent(
                _executor.GetExecutionStats(),
                node, VdfExecutionStats::NodeRequiredInputsEvent);

            // Invoke the required reads and the inputs task, if applicable.
            const bool invokedRequireds =
                _InvokeRequiredInputs(scheduleTask, state, task, bypass);
            const bool invokedInputsTask =
                _InvokeInputsTask(scheduleTask, state, node, task, bypass);

            // If we just invoked any requireds, or an inputs task: Re-execute
            // this task once the input dependencies have been fulfilled.
            if (invokedRequireds || invokedInputsTask) {
                *evaluationStage = EvaluationStagePrepNode;
                return true;
            }
        }

        // Node preparation.
        case EvaluationStagePrepNode: {
            // Also detect interruption before actually prepping and running the
            // node. If interruption has been detected, there is no need to
            // prep or evaluate this node. This covers the inbound path
            // (evaluating nodes once inputs are available) of the traversal.
            if (_DetectInterruption(state, node)) {
                return false;
            }

            // If we did in fact invoke a separate prep task: Re-execute this
            // task once the prep task has been completed.
            if (_InvokePrepTask(scheduleTask, state, node, task)) {
                *evaluationStage = EvaluationStageEvaluateNode;
                return true;
            }
        }

        // Node (invocation) evaluation.
        case EvaluationStageEvaluateNode: {
            // We really only want to evaluate this node if no interruption has
            // been detected. Otherwise, we would be trying to dereference
            // output buffers, which may not available due to bailing out from
            // interruption.
            if (_HasDetectedInterruption()) {
                return false;
            }

            // Evaluate the node, i.e. compute or pass through.
            _EvaluateNode(scheduleTask, state, node, task);
        }
    }

    // No more re-execution required: We are done!
    return false;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_ProcessInputsTask(
    WorkTaskGraph::BaseTask *task,
    const VdfEvaluationState &state,
    const VdfNode &node,
    const VdfScheduleInputsTask &scheduleTask,
    _EvaluationStage *evaluationStage,
    WorkTaskGraph::BaseTask **bypass)
{
    // The evaluation stages this task can be in.
    enum {
        EvaluationStagePrereqs,
        EvaluationStageOptionals,
        EvaluationStageDone
    };

    // Log execution stats for the inputs task.
    VdfExecutionStats::ScopedEvent scopedEvent(
        _executor.GetExecutionStats(),
        node, VdfExecutionStats::NodeInputsTaskEvent);

    // Handle the current evaluation stage.
    switch (*evaluationStage) {

        // Prereq inputs.
        case EvaluationStagePrereqs: {
            // If we did in fact invoke any compute tasks for prereqs:
            // Re-execute this task once the input dependencies has been
            // fulfilled.
            if (_InvokePrereqInputs(
                    scheduleTask, state, task, bypass)) {
                *evaluationStage = EvaluationStageOptionals;
                return true;
            }
        }

        // Optional inputs (those dependent on prereq values).
        case EvaluationStageOptionals: {
            // If interruption has been detected, we have to bail from this
            // task. This is to prevent us from reading prereq input values,
            // which may have ended in interruption (and therefore are not
            // available for reading), when determining which optional inputs
            // to run.
            if (_HasDetectedInterruption()) {
                return false;
            }

            // If we did in fact invoke any compute tasks for optionals:
            // Re-execute this task once the input dependencies has been
            // fulfilled.
            if (_InvokeOptionalInputs(
                    scheduleTask, state, node, task, bypass)) {
                *evaluationStage = EvaluationStageDone;
                return true;
            }
        }
    }

    // No more re-execution required: We are done!
    return false;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_ProcessKeepTask(
    WorkTaskGraph::BaseTask *task,
    const VdfEvaluationState &state,
    const VdfNode &node,
    _EvaluationStage *evaluationStage,
    WorkTaskGraph::BaseTask **bypass)
{
    // The evaluation stages this task can be in.
    enum {
        EvaluationStageKeep,
        EvaluationStagePublish
    };

    // Get the current schedule. We'll need it for all possible evaluation
    // stages below.
    const VdfSchedule &schedule = state.GetSchedule();

    // Handle the current evaluation stage.
    switch (*evaluationStage) {

        // Run all tasks contributing to the kept buffer.
        case EvaluationStageKeep: {
            VdfSchedule::TaskIdRange tasks = schedule.GetComputeTaskIds(node);
            TF_DEV_AXIOM(!tasks.empty());

            // Look at all the compute tasks associated with the node keeping
            // the data. There should be at least one contributing to the kept
            // buffer.
            bool invoked = false;
            for (const VdfScheduleTaskId taskId : tasks) {
                const VdfScheduleComputeTask &computeTask =
                    schedule.GetComputeTask(taskId);

                // If this compute task contributes to the kept buffer, invoke
                // it, and remember that we just invoked a task.
                if (computeTask.flags.hasKeep) {
                    invoked |= _InvokeComputeTask(
                        taskId, state, node, task, bypass);
                }
            }

            // If we invoked at least one task, we'll re-execute this task
            // once all the input dependencies have been fulfilled.
            if (invoked) {
                *evaluationStage = EvaluationStagePublish;
                return true;
            }
        }

        // Publish the kept buffers.
        case EvaluationStagePublish: {
            // Make sure not to publish anything after interruption.
            if (_HasDetectedInterruption()) {
                return false;
            }

            // Publish the scratch buffers now containing the kept data.
            _PublishScratchBuffers(schedule, node);
        }
    }

    return false;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_InvokeKeepTask(
    const VdfScheduleTaskIndex idx,
    const VdfNode &node,
    const VdfEvaluationState &state,
    WorkTaskGraph::BaseTask *successor,
    WorkTaskGraph::BaseTask **bypass)
{
    // Attempt to claim the keep task.
    VdfParallelTaskSync::State claimState = _keepTasks.Claim(idx, successor);

    // If the task has been claimed successfully, i.e. we are the first to claim
    // it as an input dependency, go ahead and spawn a corresponding TBB task.
    if (claimState == VdfParallelTaskSync::State::Claimed) {
        _KeepTask *task = successor->AllocateChild<_KeepTask>(
            this, state, node, idx);
        _SpawnOrBypass(task, bypass);
    } 

    // If the task isn't done already (i.e. we just claimed it, or were
    // instructed to wait for its completion) return false.
    return claimState != VdfParallelTaskSync::State::Done;
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_InvokeTouchTask(
    const VdfOutput &dest,
    const VdfOutput &source)
{
    // Allocate a new touch task and spawn it. Note that only the root task has
    // to wait for completion of this task, since this is purely background
    // work.

    _TouchTask *task = _taskGraph.AllocateTask<_TouchTask>(
        this, dest, source);
    _taskGraph.RunTask(task);
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_InvokeComputeTask(
    const VdfScheduleTaskId taskIndex,
    const VdfEvaluationState &state,
    const VdfNode &node,
    WorkTaskGraph::BaseTask *successor,
    WorkTaskGraph::BaseTask **bypass)
{
    // Attempt to claim the compute task.
    VdfParallelTaskSync::State claimState = VdfParallelTaskSync::State::Claimed;
    claimState = _computeTasks.Claim(taskIndex, successor);

    // If the task has been claimed successfully, i.e. we are the first to claim
    // it as an input dependency, go ahead and spawn a corresponding TBB task.
    if (claimState == VdfParallelTaskSync::State::Claimed) {
        _ComputeTask *task = 
            successor->AllocateChild<_ComputeTask>(
                this, state, node, taskIndex);
        _SpawnOrBypass(task, bypass);
    }

    // If the task isn't done already (i.e. we just claimed it, or were
    // instructed to wait for its completion) return false.
    return claimState != VdfParallelTaskSync::State::Done;
}

template < typename Derived, typename DataManager >
template < typename Iterable >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_InvokeComputeTasks(
    const Iterable &tasks,
    const VdfEvaluationState &state,
    const VdfNode &node,
    WorkTaskGraph::BaseTask *successor,
    WorkTaskGraph::BaseTask **bypass)
{
    // Invoke all compute tasks within the iterable range.
    bool invoked = false;
    for (const VdfScheduleTaskId taskId : tasks) {
        invoked |= _InvokeComputeTask(taskId, state, node, successor, bypass);
    }

    // Return true if any tasks have been invoked.
    return invoked;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_IsInputDependencyCached(
    VdfScheduleInputDependencyUniqueIndex uniqueIndex,
    const VdfOutput &output,
    const VdfMask &mask)
{
    enum {
        StateUndecided,
        StateCached,
        StateUncached
    };

    // Figure out what state this dependency is currently in.
    std::atomic<uint8_t> *state = &_dependencyState[uniqueIndex];
    uint8_t currentState = state->load(std::memory_order_relaxed);

    // If we haven't yet decided whether this dependency has been cached or not,
    // check now.
    if (currentState == StateUndecided) {
        // Determine the cache state.
        const bool isCached = _executor.GetOutputValue(output, mask);
        const uint8_t newState = isCached ? StateCached : StateUncached;

        // Store the new state, but only if it has not changed (e.g. updated by
        // a different thread) in the meantime. If the CAS below fails,
        // currentState will be updated with the new state.
        if (state->compare_exchange_strong(currentState, newState)) {
            return isCached;
        }
    }

    // Return true if the dependency has been cached.
    return currentState == StateCached;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_InvokeComputeOrKeepTasks(
    const VdfScheduleInputDependency &input,
    const VdfEvaluationState &state,
    WorkTaskGraph::BaseTask *successor,
    WorkTaskGraph::BaseTask **bypass)
{
    // Check if the input dependency has already been fulfilled by looking up
    // the relevant output data in the executor caches. If the data is there,
    // we don't need to worry about invoking any tasks. Note, that if we decide
    // to invoke the corresponding task, we commit to running all the tasks for
    // all the invocations of the node! That's why we cache the result of
    // determining the output cache state the first time. This avoids a
    // correctness problem where the parent executor publishes the requested
    // output data after at least one invocations has already been invoked,
    // and subsequent invocations would then fail to run, because the data is
    // now available.
    if (_IsInputDependencyCached(input.uniqueIndex, input.output, input.mask)) {
        return false;
    }

    // Get the current schedule.
    const VdfSchedule &schedule = state.GetSchedule();

    // Get an iterable range of compute tasks for this input dependency.
    VdfSchedule::TaskIdRange tasks = schedule.GetComputeTaskIds(input);

    // Retrieve the node ad the source end of the input dependency.
    const VdfNode &node = input.output.GetNode();

    // Invoke the relevant compute tasks, if any.
    bool invoked = _InvokeComputeTasks(tasks, state, node, successor, bypass);

    // If there are no compute tasks, and the dependency is instead for a keep
    // task, invoke that keep task instead.
    const VdfScheduleTaskIndex keepTask = input.computeOrKeepTaskId;
    if (input.computeTaskNum == 0 && !VdfScheduleTaskIsInvalid(keepTask)) {
        invoked |= _InvokeKeepTask(keepTask, node, state, successor, bypass);
    }

    // Return true if any tasks have been invoked.
    return invoked;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_InvokeComputeOrKeepTasks(
    const VdfOutput &output,
    const VdfEvaluationState &state,
    WorkTaskGraph::BaseTask *successor,
    WorkTaskGraph::BaseTask **bypass)
{
    // Get the current schedule.
    const VdfSchedule &schedule = state.GetSchedule();

    // If the output is not scheduled, there is no need to invoke a task.
    VdfSchedule::OutputId oid = schedule.GetOutputId(output);
    if (!oid.IsValid()) {
        return false;
    }

    // Is the output already cached? If that's the case there is no need to
    // invoke any tasks.
    const VdfMask &requestMask = schedule.GetRequestMask(oid);
    if (_executor.GetOutputValue(output, requestMask)) {
        return false;
    }

    // Retrieve the node at the source end of the input dependency.
    const VdfNode &node = output.GetNode();

    // Get an iterable range of tasks for this input dependency.
    VdfSchedule::TaskIdRange tasks = schedule.GetComputeTaskIds(node);

    // Invoke all the dependent tasks.
    bool invoked = _InvokeComputeTasks(tasks, state, node, successor, bypass);

    // If there are no compute tasks, and the dependency is instead for a keep
    // task, invoke that keep task instead.
    const VdfScheduleTaskIndex keepTask = schedule.GetKeepTaskIndex(node);
    if (!VdfScheduleTaskIsInvalid(keepTask)) {
        invoked |= _InvokeKeepTask(keepTask, node, state, successor, bypass);
    }

    // Return true if any tasks have been invoked.
    return invoked;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_InvokePrereqInputs(
    const VdfScheduleInputsTask &scheduleTask,
    const VdfEvaluationState &state,
    WorkTaskGraph::BaseTask *successor,
    WorkTaskGraph::BaseTask **bypass)
{
    PEE_TRACE_SCOPE("VdfParallelExecutorEngineBase::_InvokePrereqInputs");

    // If there are no prereqs dependencies, bail out.
    if (!scheduleTask.prereqsNum) {
        return false;
    }

    // Get a range of input dependencies to fulfill to satisfy the prereqs.
    VdfSchedule::InputDependencyRange prereqs =
        state.GetSchedule().GetPrereqInputDependencies(scheduleTask);

    // Iterate over all the prereq dependencies, and invoke the relevant
    // compute and/or keep tasks.
    bool invoked = false;
    for (const VdfScheduleInputDependency &i : prereqs) {
        invoked |= _InvokeComputeOrKeepTasks(i, state, successor, bypass);
    }

    // Return true if any tasks have been invoked.
    return invoked;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_InvokeOptionalInputs(
    const VdfScheduleInputsTask &scheduleTask,
    const VdfEvaluationState &state,
    const VdfNode &node,
    WorkTaskGraph::BaseTask *successor,
    WorkTaskGraph::BaseTask **bypass)
{
    PEE_TRACE_SCOPE("VdfParallelExecutorEngineBase::_InvokeOptionalInputs");

    // If there are no dependencies, bail out.
    if (!scheduleTask.optionalsNum) {
        return false;
    }

    // Get the schedule from the state.
    const VdfSchedule &schedule = state.GetSchedule();

    // Get the read dependencies from the schedule.
    VdfSchedule::InputDependencyRange inputs =
        schedule.GetOptionalInputDependencies(scheduleTask);

    // Ask the node for its required inputs.
    VdfRequiredInputsPredicate inputsPredicate =
        node.GetRequiredInputsPredicate(VdfContext(state, node));

    // If the node does not require any inputs, bail out.
    if (!inputsPredicate.HasRequiredReads()) {
        return false;
    }

    // Have any tasks been invoked?
    bool invoked = false;

    // If all inputs are required, simply invoke tasks for each one of the
    // required input dependencies. We do not need to do any task inversion in
    // this case, which is great.
    if (inputsPredicate.RequiresAllReads()) {
        for (const VdfScheduleInputDependency &i : inputs) {
            invoked |= _InvokeComputeOrKeepTasks(i, state, successor, bypass);
        }
    }

    // If only a subset of the inputs is required, we need to invert the
    // required inputs into compute tasks, and invoke those.
    else {
        PEE_TRACE_SCOPE("Task Inversion");

        // Find all the compute tasks for all the source outputs on all
        // connections on required inputs. Then, invoke those tasks. Note, that
        // the schedule will only contain compute tasks for nodes that have
        // also been scheduled, so there is no need to check if a source output
        // has been scheduled, here.
        for (const VdfScheduleInput &scheduleInput : schedule.GetInputs(node)) {
            if (inputsPredicate.IsRequiredRead(*scheduleInput.input)) {
                invoked |= _InvokeComputeOrKeepTasks(
                    *scheduleInput.source, state, successor, bypass);
            }
        }
    }

    // Return true if any compute tasks have been invoked.
    return invoked;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_InvokeRequiredInputs(
    const VdfScheduleComputeTask &scheduleTask,
    const VdfEvaluationState &state,
    WorkTaskGraph::BaseTask *successor,
    WorkTaskGraph::BaseTask **bypass)
{
    PEE_TRACE_SCOPE("VdfParallelExecutorEngineBase::_InvokeRequiredInputs");

    // Get the current schedule.
    const VdfSchedule &schedule = state.GetSchedule();

    // Get an iterable range of required input dependencies for this task.
    VdfSchedule::InputDependencyRange requireds =
        schedule.GetRequiredInputDependencies(scheduleTask);

    // Invoke the compute tasks for all required input dependencies.
    bool invoked = false;
    for (const VdfScheduleInputDependency &i : requireds) {
        invoked |= _InvokeComputeOrKeepTasks(i, state, successor, bypass);
    }

    // Returns true if any compute tasks have been invoked.
    return invoked;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_InvokeInputsTask(
    const VdfScheduleComputeTask &scheduleTask,
    const VdfEvaluationState &state,
    const VdfNode &node,
    WorkTaskGraph::BaseTask *successor,
    WorkTaskGraph::BaseTask **bypass)
{
    PEE_TRACE_SCOPE("VdfParallelExecutorEngineBase::_InvokeInputsTask");

    // Check if this compute task has a valid inputs task, and bail out
    // if that's not the case.
    const VdfScheduleTaskIndex inputsTaskIndex = scheduleTask.inputsTaskIndex;
    if (VdfScheduleTaskIsInvalid(inputsTaskIndex)) {
        return false;
    }

    // Attempt to claim the inputs task.
    VdfParallelTaskSync::State claimState =
        _inputsTasks.Claim(inputsTaskIndex, successor);

    // If the inputs task has been successfully claimed, i.e. we are the first
    // to claim this task, go ahead an allocate and spawn a TBB task.
    if (claimState == VdfParallelTaskSync::State::Claimed) {
        _InputsTask *task =
            successor->AllocateChild<_InputsTask>(
                this, state, node, inputsTaskIndex);
        _SpawnOrBypass(task, bypass);
    }

    // If the task isn't done already (i.e. we just claimed it, or were
    // instructed to wait for its completion) return false.
    return claimState != VdfParallelTaskSync::State::Done;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_InvokePrepTask(
    const VdfScheduleComputeTask &scheduleTask,
    const VdfEvaluationState &state,
    const VdfNode &node,
    WorkTaskGraph::BaseTask *successor)
{
    PEE_TRACE_SCOPE("VdfParallelExecutorEngineBase::_InvokePrepTask");

    // Check if this compute task has a valid prep task. If it does not have
    // a valid prep task, we still have to prepare the node. However, since
    // there is no separate task for node preparation, we know that there is
    // only one claimant for this task, and we can therefore simply call into
    // _PrepareNode. It's not necessary to update and synchronization
    // structure at this point, and we can also return false, because no
    // task has been invoked,
    const VdfScheduleTaskIndex prepTaskIndex = scheduleTask.prepTaskIndex;
    if (VdfScheduleTaskIsInvalid(prepTaskIndex)) {
        _PrepareNode(state, node);
        return false;
    }

    PEE_TRACE_SCOPE("VdfParallelExecutorEngineBase::_InvokePrepTask (task)");

    // If there is a separate task for node preparation, attempt to claim it.
    VdfParallelTaskSync::State claimState =
        _prepTasks.Claim(prepTaskIndex, successor);

    // If the prep task has been successfully claimed, i.e. we are the first
    // to claim this task, go ahead and do the preparation. Note, that it's
    // not necessary to actually do the invocation in a separate task. We can
    // return false here, because no task was 
    if (claimState == VdfParallelTaskSync::State::Claimed) {
        _PrepareNode(state, node);
        _prepTasks.MarkDone(prepTaskIndex);
        return false;
    }

    // If we were instructed to wait for this task to complete, return true.
    // Otherwise the task had already been completed, and we don't need to
    // synchronize on it.
    return claimState == VdfParallelTaskSync::State::Wait;
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_PrepareNode(
    const VdfEvaluationState &state,
    const VdfNode &node)
{
    PEE_TRACE_SCOPE("VdfParallelExecutorEngineBase::_PrepareNode");

    // Log execution stats for node preparation.
    VdfExecutionStats::ScopedEvent scopedEvent(
        _executor.GetExecutionStats(),
        node, VdfExecutionStats::NodePrepareEvent);

    // Prepare each one of the scheduled outputs.
    const VdfSchedule &schedule = state.GetSchedule();
    VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, schedule, node) {
        _PrepareOutput(schedule, outputId);
    }
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_PrepareOutput(
    const VdfSchedule &schedule,
    const VdfSchedule::OutputId outputId)
{
    // Get the VdfOutput for this scheduled output.
    const VdfOutput &output = *schedule.GetOutput(outputId);

    // Mark the output as having been touched during evaluation. We defer this
    // work to the derived class, because the executor engine may or may not
    // be required to actually do any touching.
    _Self()._Touch(output);

    // Retrieve the data handle.
    _DataHandle dataHandle =
        _dataManager->GetOrCreateDataHandle(output.GetId());

    // Reset the private buffer, and assign the request mask.
    const VdfMask &requestMask = schedule.GetRequestMask(outputId);
    VdfExecutorBufferData *privateBuffer =
        _dataManager->GetPrivateBufferData(dataHandle);
    privateBuffer->ResetExecutorCache(requestMask);

    // For associated outputs, make sure the private data is available, before
    // we start writing to it from multiple threads. This will make sure that
    // the buffer has been passed or copied down from the source output.
    if (output.GetAssociatedInput()) {
        _PrepareReadWriteBuffer(
            output, outputId, requestMask, schedule, privateBuffer);
    }

    // Reset the scratch buffer, and assign the keep mask, if any.
    const VdfMask &keepMask = schedule.GetKeepMask(outputId);
    VdfExecutorBufferData *scratchBuffer =
        _dataManager->GetScratchBufferData(dataHandle);
    scratchBuffer->ResetExecutorCache(keepMask);

    // Make sure the scratch buffer is available, and sized appropriately
    // to accommodate all the kept data, without having to resize the
    // buffer (which would not be thread-safe). We will subsequently be
    // populating this scratch buffer, and that may happen from multiple
    // threads!
    if (!keepMask.IsEmpty()) {
        _CreateScratchCache(output, dataHandle, keepMask, scratchBuffer);
    }
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_CreateScratchCache(
    const VdfOutput &output,
    const _DataHandle dataHandle,
    const VdfMask &mask,
    VdfExecutorBufferData *scratchBuffer)
{
    VdfExecutorBufferData *publicBuffer =
        _dataManager->GetPublicBufferData(dataHandle);
    const VdfMask &publicMask = publicBuffer->GetExecutorCacheMask();

    // If there is no public data at the output, the size of the scratch cache
    // is determined by the mask alone.
    if (publicMask.IsEmpty() || publicMask.IsAllZeros()) {
        _dataManager->CreateOutputCache(output, scratchBuffer, mask.GetBits());
    }

    // If there is public data at the output, we are later going to absorb that
    // data into the scratch cache. Hence, we will make sure that the buffer is
    // sized to accomodate both the specified mask, and the publicMask.
    else {
        VdfMask::Bits unionBits(
            mask.GetSize(),
            std::min(mask.GetFirstSet(), publicMask.GetFirstSet()),
            std::max(mask.GetLastSet(), publicMask.GetLastSet()));
        _dataManager->CreateOutputCache(output, scratchBuffer, unionBits);
    }
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_EvaluateNode(
    const VdfScheduleComputeTask &scheduleTask,
    const VdfEvaluationState &state,
    const VdfNode &node,
    WorkTaskGraph::BaseTask *successor)
{
    PEE_TRACE_SCOPE("VdfParallelExecutorEngineBase::_EvaluateNode");

    // Log execution stats for node evaluation.
    VdfExecutionStats::ScopedMallocEvent scopedEvent(
        _executor.GetExecutionStats(),
        node, VdfExecutionStats::NodeEvaluateEvent);

    // Compute the node, if it is affective.
    if (scheduleTask.flags.isAffective) {
        _ComputeNode(scheduleTask, state, node);
    }

    // If the node is not affective, make sure that all its data has been
    // passed through.
    else {
        _PassThroughNode(scheduleTask, state, node);
    }
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_ComputeNode(
    const VdfScheduleComputeTask &scheduleTask,
    const VdfEvaluationState &state,
    const VdfNode &node)
{
    PEE_TRACE_SCOPE("VdfParallelExecutorEngineBase::_ComputeNode");

    // Log an event indicating this node has been computed.
    if (VdfExecutionStats *stats = _executor.GetExecutionStats()) {
        stats->LogTimestamp(VdfExecutionStats::NodeDidComputeEvent, node);
    }

    // Execute the node callback. Make sure to also pass the invocation index,
    // to the VdfContext. The node may not have multiple invocations, i.e. the
    // invocation index may be VdfScheduleTaskInvalid.
    node.Compute(VdfContext(state, node, scheduleTask.invocationIndex));

    // If interruption occurred while the callback was running, the data
    // produced by the callback may not all be correct. If this happens, we
    // want to avoid processing any of the outputs since doing so may publish
    // results to the buffers.
    if (_DetectInterruption(state, node)) {
        return;
    }

    // We need to finalize all the scheduled outputs. This will take care of
    // populating scratch buffers with kept data, as well as publishing any
    // output data, for example.
    const VdfSchedule &schedule = state.GetSchedule();
    VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, schedule, node) {
        const VdfOutput &output = *schedule.GetOutput(outputId);

        // Retrieve the data handle for this output.
        _DataHandle dataHandle = _dataManager->GetDataHandle(output.GetId());
        TF_DEV_AXIOM(_dataManager->IsValidDataHandle(dataHandle));

        // Get the private executor buffer.
        VdfExecutorBufferData *privateBuffer = 
            _dataManager->GetPrivateBufferData(dataHandle);

        // Check to see if the node did indeed produce values for this output.
        // The node callback is expected to produce buffers for all the
        // scheduled outputs. By definition, read/write outputs will always
        // have produced a value, even if that value was just an unmodified
        // pass-through.
        if (!privateBuffer->GetExecutorCache()) {
            // No output value: Spit out a warning.
            TF_WARN(
                "No value set for output " + output.GetDebugName() +
                " of type " + output.GetSpec().GetType().GetTypeName() +
                " named " + output.GetName().GetString());

            // Fill the output with a default value.
            VdfExecutionTypeRegistry::FillVector(
                output.GetSpec().GetType(),
                schedule.GetRequestMask(outputId).GetSize(),
                _dataManager->GetOrCreateOutputValueForWriting(
                    output, dataHandle));
        }

        // Make sure the output has been processed. This will take care of
        // keeping all the relevant data, as well as publishing buffers for
        // consumption by dependents.
        const bool hasAssociatedInput = output.GetAssociatedInput();
        _ProcessOutput(
            scheduleTask, state, output, outputId, dataHandle,
            hasAssociatedInput, privateBuffer);
    }
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_PassThroughNode(
    const VdfScheduleComputeTask &scheduleTask,
    const VdfEvaluationState &state,
    const VdfNode &node)
{
    PEE_TRACE_SCOPE("VdfParallelExecutorEngineBase::_PassThroughNode");

    // Iterate over all the scheduled outputs on a node, and make sure that
    // they have been properly processed.
    const VdfSchedule &schedule = state.GetSchedule();
    VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, schedule, node) {
        const VdfOutput &output = *schedule.GetOutput(outputId);

        // Retrieve the data handle for this output.
        _DataHandle dataHandle = _dataManager->GetDataHandle(output.GetId());
        TF_DEV_AXIOM(_dataManager->IsValidDataHandle(dataHandle));

        // Get the private executor buffer.
        VdfExecutorBufferData *privateBuffer = 
            _dataManager->GetPrivateBufferData(dataHandle);

        // Make sure the output has been processed. This will take care of
        // keeping all the relevant data, as well as publishing buffers for
        // consumption by dependents.
        const bool hasAssociatedInput = output.GetAssociatedInput();
        _ProcessOutput(
            scheduleTask, state, output, outputId, dataHandle,
            hasAssociatedInput, privateBuffer);
    }
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_ProcessOutput(
    const VdfScheduleComputeTask &scheduleTask,
    const VdfEvaluationState &state,
    const VdfOutput &output,
    const VdfSchedule::OutputId outputId,
    const _DataHandle dataHandle,
    const bool hasAssociatedInput,
    VdfExecutorBufferData *privateBuffer)
{
    // Is this a node have multiple invocations? If the invocation index is set
    // to VdfScheduleTaskInvalid, the node does only have one invocations.
    const VdfScheduleTaskIndex invocationIndex = scheduleTask.invocationIndex;
    const bool hasMultipleInvocations =
        !VdfScheduleTaskIsInvalid(invocationIndex);

    // Does this output pass its buffer?
    const VdfSchedule &schedule = state.GetSchedule();
    const VdfOutput *passToOutput = schedule.GetPassToOutput(outputId);

    // Allow the derived engine to finalize the output data before
    // publishing any buffers.
    _Self()._FinalizeOutput(
        state, output, outputId, dataHandle, invocationIndex, passToOutput);

    // If this output does not pass its buffer, we need to make sure to
    // publish the entire private buffer to make it available for all
    // dependents.
    if (!passToOutput) {
        // Can't publish here, if there are multiple invocations scheduled
        // for the same node. We should never schedule multiple invocations for
        // nodes that don't pass their buffers.
        TF_DEV_AXIOM(!hasMultipleInvocations);

        // Absorb any publicly available data, which is not also available in
        // the private buffer. Note that the missing data will be written to
        // the scratch buffer. The private buffer may still be in use by other
        // node invocations, and doing the merging is a potentially destructive
        // (i.e. racy) operation.
        const VdfMask &privateMask = privateBuffer->GetExecutorCacheMask();
        VdfVector *scratchBuffer =
            _AbsorbPublicBuffer(output, dataHandle, privateMask);

        // If publicly available data has been absorbed into the scratch buffer,
        // also copy the private buffer there, and then publish the whole
        // shebang.
        if (scratchBuffer) {
            scratchBuffer->Merge(
                *privateBuffer->GetExecutorCache(), privateMask);
            _dataManager->PublishScratchBufferData(dataHandle);
        }

        // If no data has been written to the scratch buffer, we can simply
        // publish the private buffer.
        else {
            _dataManager->PublishPrivateBufferData(dataHandle);
        }
    }

    // We are passing this buffer, so let's see if we need to keep anything.
    else {
        // Get the scratch buffer data.
        VdfExecutorBufferData *scratchBuffer =
            _dataManager->GetScratchBufferData(dataHandle);

        // If a scratch buffer has been prepared for this output, then make
        // sure to keep the relevant data currently in the private buffer.
        if (VdfVector *scratchValue = scratchBuffer->GetExecutorCache()) {
            // Get the keep mask. If the node has multiple invocations, this
            // should be the keep mask relevant to the current invocation.
            const VdfMask &keepMask = hasMultipleInvocations
                ? schedule.GetKeepMask(invocationIndex)
                : schedule.GetKeepMask(outputId);

            // Merge the relevant data into the scratch buffer. Note that the
            // scratch buffer must be appropriately sized to accommodate all the
            // data. Otherwise, Merge will expand the buffer, which is not
            // thread-safe. Making sure that the buffer is appropriately sized
            // is the responsibility of node preparation.
            {
                PEE_TRACE_SCOPE(
                    "VdfParallelExecutorEngineBase::_FinalizeOutput (keep)");
                scratchValue->Merge(
                    *privateBuffer->GetExecutorCache(), keepMask);
            }

            // If this is not a node invocation, publish the scratch buffer
            // right here. This way, we can avoid creating a separate keep task
            // for any node that has only one compute task in the first place.
            if (!hasMultipleInvocations) {
                _AbsorbPublicBuffer(
                    output, dataHandle, scratchBuffer->GetExecutorCacheMask());
                _dataManager->PublishScratchBufferData(dataHandle);
            }
        }
    }
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_PrepareReadWriteBuffer(
    const VdfOutput &output,
    const VdfSchedule::OutputId outputId,
    const VdfMask &mask,
    const VdfSchedule &schedule,
    VdfExecutorBufferData *privateBuffer)
{
    // If there is a from-buffer output, pass straight from the from-buffer
    // source. Also make sure to touch any output in between, but we can do
    // that in a separate, background task.
    if (const VdfOutput *source = schedule.GetFromBufferOutput(outputId)) {
        _PassOrCopyBuffer(output, *source, mask, schedule, privateBuffer);
        _InvokeTouchTask(output, *source);
        return;
    }

    // XXX: Don't do this connection nonsense here. All this information can
    //      be stored in the schedule.

    const VdfInput *input = output.GetAssociatedInput();
    const size_t numInputNodes = input->GetNumConnections();

    // If there is exactly one input, we can pass or copy that buffer down.
    if (numInputNodes == 1 && !(*input)[0].GetMask().IsAllZeros()) {
        const VdfOutput &source = (*input)[0].GetSourceOutput();
        _PassOrCopyBuffer(output, source, mask, schedule, privateBuffer);
        return;
    }

    // If we have no inputs, a buffer cannot be passed. Instead, create a
    // brand new one.
    _dataManager->CreateOutputCache(output, privateBuffer);
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_PassOrCopyBuffer(
    const VdfOutput &output,
    const VdfOutput &source,
    const VdfMask &inputMask,
    const VdfSchedule &schedule,
    VdfExecutorBufferData *privateBuffer)
{
    // Decide whether to pass or copy the buffer from the source output.
    bool passBuffer = false;

    // If the source data handle is valid...
    _DataHandle sourceHandle = _dataManager->GetDataHandle(source.GetId());
    if (_dataManager->IsValidDataHandle(sourceHandle)) {

        // ... and the destination is the pass-to output of the source ...
        const VdfSchedule::OutputId sourceOid = schedule.GetOutputId(source);
        if (schedule.GetPassToOutput(sourceOid) == &output) {

            // ... and the cache lookup resulted in a cache miss (i.e. the
            // output value had to be computed by evaluating the corresponding
            // compute tasks.) Pass the buffer down from the source output,
            // instead of copying it.
            const VdfScheduleInputDependencyUniqueIndex uniqueIndex =
                schedule.GetUniqueIndex(sourceOid);
            TF_DEV_AXIOM(uniqueIndex != VdfScheduleTaskInvalid);
            passBuffer = !_IsInputDependencyCached(
                uniqueIndex, source, inputMask);
        }
    }

    // Pass the buffer from the source output. This is the fast path.
    if (passBuffer) {
        VdfExecutorBufferData *sourcePrivateBuffer =
            _dataManager->GetPrivateBufferData(sourceHandle);
        _PassBuffer(sourcePrivateBuffer, privateBuffer);
    }

    // Copy the buffer instead.
    else {
        _CopyBuffer(output, source, inputMask, privateBuffer);
    }
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_PassBuffer(
    VdfExecutorBufferData *fromBuffer,
    VdfExecutorBufferData *toBuffer) const
{
    VdfVector *sourceValue = fromBuffer->GetExecutorCache();
    TF_DEV_AXIOM(sourceValue);

    // Pass the data along. Assume ownership of the source vector
    // and relinquish the ownership at the source private buffer. 
    toBuffer->TakeOwnership(sourceValue);
    fromBuffer->YieldOwnership();
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_CopyBuffer(
    const VdfOutput &output,
    const VdfOutput &source,
    const VdfMask &fromMask,
    VdfExecutorBufferData *toBuffer) const
{
    PEE_TRACE_SCOPE("VdfParallelExecutorEngineBase::_CopyBuffer");

    // Note that we must look up the data through the executor, instead of the
    // data manager, because we may have initially received a cache hit by
    // looking up the executor. The data may live on the parent executor, for
    // example, instead of the local data manager.
    const VdfVector *sourceVector = _executor.GetOutputValue(source, fromMask);
    if (!sourceVector) {
        VDF_FATAL_ERROR(
            source.GetNode(), "No cache for output " + source.GetDebugName());
    }

    // Create a new output cache at the destination output, and copy all the
    // data from the source output.
    VdfVector *destValue = _dataManager->CreateOutputCache(output, toBuffer);
    destValue->Copy(*sourceVector, fromMask);
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_PublishScratchBuffers(
    const VdfSchedule &schedule,
    const VdfNode &node)
{
    // Iterate over all the outputs scheduled on this node.
    VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, schedule, node) {
        const VdfOutput &output = *schedule.GetOutput(outputId);

        // Get the data handle for this output.
        const _DataHandle dataHandle =
            _dataManager->GetDataHandle(output.GetId());
        TF_DEV_AXIOM(_dataManager->IsValidDataHandle(dataHandle));

        // Retrieve the scratch buffer.
        VdfExecutorBufferData *scratchBuffer =
            _dataManager->GetScratchBufferData(dataHandle);

        // If the scratch buffer contains any data, absorb the public data still
        // living on this output, and publish the whole shebang. 
        if (const VdfVector* value = scratchBuffer->GetExecutorCache()) {
            _AbsorbPublicBuffer(
                output, dataHandle, scratchBuffer->GetExecutorCacheMask());
            _dataManager->PublishScratchBufferData(dataHandle);
        }
    }
}

template < typename Derived, typename DataManager >
VdfVector *
VdfParallelExecutorEngineBase<Derived, DataManager>::_AbsorbPublicBuffer(
    const VdfOutput &output,
    const _DataHandle dataHandle,
    const VdfMask &haveMask)
{ 
    // Get the public buffer value and mask.
    const VdfExecutorBufferData *publicBuffer =
        _dataManager->GetPublicBufferData(dataHandle);
    const VdfVector *publicValue = publicBuffer->GetExecutorCache();
    const VdfMask &publicMask = publicBuffer->GetExecutorCacheMask();

    // If there is no public data available, or all that data is already
    // included in the destination mask, bail out.
    if (!publicValue || publicMask.IsEmpty() || publicMask == haveMask) {
        return nullptr;
    }

    // Determine the mask of data to copy from the public buffer, and bail out
    // if there is no data to copy.
    const VdfMask::Bits mergeBits = publicMask.GetBits() - haveMask.GetBits();
    if (mergeBits.AreAllUnset()) {
        return nullptr;
    }

    // The destination buffer is the scratch buffer.
    VdfExecutorBufferData *scratchBuffer =
        _dataManager->GetScratchBufferData(dataHandle);

    // Let's make sure the scratch buffer has an executor cache to write into,
    // and create a new one if it doesn't.
    VdfVector *scratchValue = scratchBuffer->GetExecutorCache();
    const VdfMask extendedMask = publicMask | haveMask;
    if (!scratchValue) {
        scratchValue = _dataManager->CreateOutputCache(
            output, scratchBuffer, extendedMask.GetBits());
    }

    // Merge the public value into the scratch buffer. We only merge the missing
    // elements, in order to avoid redundant copies. Also make sure that the
    // cache mask has been properly extended.
    scratchValue->Merge(*publicValue, mergeBits);
    scratchBuffer->SetExecutorCacheMask(extendedMask);
    return scratchValue;
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::_DetectInterruption(
    const VdfEvaluationState &state,
    const VdfNode &node)
{
    // First, call into the derived engine to detect any cycles. If the engine
    // gets trapped in a cycle we need to interrupted the engine, such that we
    // do not get stuck in an infinite loop.
    const bool hasCycle = _Self()._DetectCycle(state, node);

    // If either a cycle has been detected, or the interruption API on the
    // executor returns that the executor has been interrupted, we need to set
    // the internal interruption flag. _HasDetectedInterruption() will then be
    // queried at various stages of evaluation, which allows us to gracefully
    // wind down the engine.
    if (hasCycle || _executor.HasBeenInterrupted()) {
        _isInterrupted.store(true, std::memory_order_relaxed);
        return true;
    }

    // This will return true if the interruption flag has previously been set.
    return _HasDetectedInterruption();
}

template < typename Derived, typename DataManager >
bool
VdfParallelExecutorEngineBase<Derived, DataManager>::
_HasDetectedInterruption() const
{
    return _isInterrupted.load(std::memory_order_relaxed);
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_TransportErrors(
    const TfErrorMark &errorMark)
{
    TfErrorTransport transport = errorMark.Transport();
    _errors.grow_by(1)->swap(transport);
}

template < typename Derived, typename DataManager >
void
VdfParallelExecutorEngineBase<Derived, DataManager>::_PostTransportedErrors()
{
    if (_errors.empty()) {
        return;
    }

    // Post all the transported errors on the calling thread.
    for (TfErrorTransport &errorTransport : _errors) {
        errorTransport.Post();
    }

    // Clear the transported errors container.
    _errors.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
