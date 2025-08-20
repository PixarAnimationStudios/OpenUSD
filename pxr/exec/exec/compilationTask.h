//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_COMPILATION_TASK_H
#define PXR_EXEC_EXEC_COMPILATION_TASK_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/exec/exec/compilationState.h"
#include "pxr/exec/exec/compilerTaskSync.h"

#include <atomic>
#include <cstdint>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_OutputKey;

/// Base class for parallel compilation tasks.
class Exec_CompilationTask
{
public:
    virtual ~Exec_CompilationTask();

    /// Registers an additional dependency.
    /// 
    /// As long as there are unfulfilled dependencies, this task will not be
    /// re-run to continue its next phase(s).
    /// 
    void AddDependency() {
        _numDependents.fetch_add(1, std::memory_order_acquire);
    }

    /// Removes a dependency after it has been fulfilled.
    ///
    /// Returns the new number of unfulfilled dependencies. If the return value
    /// is `0`, this task can be re-run to continue its next phase(s). The
    /// caller is responsible for re-running the task.
    ///
    int RemoveDependency() {
        return _numDependents.fetch_sub(1, std::memory_order_release) - 1;
    }

    /// Executes the task.
    ///
    /// The caller adds 1 to the \p depth to track the depth of recursive
    /// invocations of operator().
    /// 
    void operator()(int depth = 0) const;

protected:
    class TaskPhases;
    class TaskDependencies;

    /// All compilation tasks are heap allocated and must be constructed through
    /// NewTask() and NewSubtask().
    /// 
    explicit Exec_CompilationTask(Exec_CompilationState &compilationState)
        : _parent(nullptr)
        , _numDependents(0)
        , _taskPhase(0)
        , _compilationState(compilationState)
    {}

    /// Main entry point of a compilation task to be implemented in the
    /// derived class.
    /// 
    /// Note, we deliberately chose mutable reference types for these
    /// parameters, as opposed to pointers as required for output parameters per
    /// our coding conventions. We attempt to optimize for readability in the
    /// overrides (of which there will be many) compared to the very short (and
    /// likely not frequently changing) call site in execute(). The spirit
    /// behind the convention is to bring clarity to what is an output parameter
    /// at the call site, which is often more important, but irrelevant in this
    /// particular case.
    /// 
    virtual void _Compile(Exec_CompilationState &, TaskPhases &) = 0;

    /// Called from the _Compile method in the derived class to indicate that
    /// the task identified by \c key has been completed. This must be called
    /// *after* the task published its results.
    /// 
    void _MarkDone(const Exec_OutputKey::Identity &key);

private:
    // The parent task, if this is a sub-task. nullptr for top-level tasks.
    Exec_CompilationTask *_parent;

    // Reference count denoting the number of unfulfilled dependencies
    std::atomic<int> _numDependents;

    // Current task phase
    uint32_t _taskPhase;

    // State persistent to one round of compilation
    Exec_CompilationState &_compilationState;
};

/// Manages the task dependencies established during task phases.
class Exec_CompilationTask::TaskDependencies
{
public:
    /// Constructs and runs a new subtask and establishes the subtask as a
    /// dependency of the calling task. The calling task's _Compile method will
    /// automatically be re-executed once all dependencies have been fulfilled.
    /// 
    template<class TaskType, class ... Args>
    void NewSubtask(Exec_CompilationState &state, Args&&... args);

    /// Claims a subtask identified by the provided \p key as a dependency. If
    /// the claimed subtask has already been claimed by another task, the
    /// calling task will establish a dependency on the subtask and the _Compile
    /// method will automatically be re-executed once all dependencies have been
    /// fulfilled.
    /// 
    Exec_CompilerTaskSync::ClaimResult ClaimSubtask(
        const Exec_OutputKey::Identity &key);

private:
    friend class Exec_CompilationTask::TaskPhases;

    TaskDependencies(
        Exec_CompilationTask *task,
        Exec_CompilationState &compilationState)
        : _task(task)
        , _compilationState(compilationState)
        , _nextSubtask(nullptr)
        , _hasDependencies(false)
    {}

    bool _HasDependencies() const {
        return _hasDependencies;
    }

    Exec_CompilationTask *_GetNextSubtask() const {
        return _nextSubtask;
    }

private:
    Exec_CompilationTask *const _task;
    Exec_CompilationState &_compilationState;
    Exec_CompilationTask *_nextSubtask;
    bool _hasDependencies;
};

template<class TaskType, class ... Args>
void
Exec_CompilationTask::TaskDependencies::NewSubtask(
    Exec_CompilationState &state, Args&&... args)
{
    _hasDependencies = true;

    // TODO: We need a small-object task allocator.
    // Tasks manage their own lifetime, and delete themselves after completion.
    TaskType *const subTask = new TaskType(state, std::forward<Args>(args)...);
    subTask->_parent = _task;
    subTask->_parent->AddDependency();

    // If there is already a next sub-task recorded, run it now! Then record
    // this new subtask as the one to run next. This will ensure that the last
    // sub-task is the one eventually returned by _GetNextSubtask().
    if (_nextSubtask) {
        Exec_CompilationState::OutputTasksAccess::_Get(&state).Run(
            _nextSubtask);
    }
    _nextSubtask = subTask;
}

/// Manages the callables associated with task phases.
/// 
/// Sequentially advances through phases, putting the task to "sleep" between
/// phases while there are unfulfilled dependencies, and then automatically
/// re-executing the _Compile method with the next phase once all dependencies
/// have been fulfilled.
/// 
class Exec_CompilationTask::TaskPhases
{
public:
    /// Invokes the callables in order, each denoting a task phase.
    template<typename... Callables>
    void Invoke(Callables&&... callables) {
        _nextTask = _InvokeOnePhase(0, std::forward<Callables>(callables)...);
    }

private:
    friend class Exec_CompilationTask;

    TaskPhases(
        Exec_CompilationTask *task,
        Exec_CompilationState &compilationState,
        uint32_t &taskPhase)
        : _task(task)
        , _compilationState(compilationState)
        , _nextTask(nullptr)
        , _taskPhase(taskPhase)
    {}

    Exec_CompilationTask *_InvokeOnePhase(uint32_t);

    template<typename Callable, typename... Tail>
    Exec_CompilationTask *_InvokeOnePhase(
        uint32_t i,
        Callable&& callable,
        Tail&&... tail);

    Exec_CompilationTask *_GetNextTask() const {
        return _nextTask;
    }

private:
    Exec_CompilationTask *const _task;
    Exec_CompilationState &_compilationState;
    Exec_CompilationTask *_nextTask;
    uint32_t &_taskPhase;
};

inline
Exec_CompilationTask *
Exec_CompilationTask::TaskPhases::_InvokeOnePhase(uint32_t)
{
    // Returning nullptr here indicates the task is complete, and there is no
    // next task to run.
    return nullptr;
}

template<typename Callable, typename... Tail>
Exec_CompilationTask *
Exec_CompilationTask::TaskPhases::_InvokeOnePhase(
    uint32_t i,
    Callable&& callable,
    Tail&&... tail)
{
    // If this is the active stage, invoke the callable.
    if (i >= _taskPhase) {
        // Construct the TaskDependencies instance and invoke the callable
        TaskDependencies taskDependencies(_task, _compilationState);
        std::forward<Callable>(callable)(taskDependencies);

        // Advance to the next stage.
        ++_taskPhase;

        // If dependencies were established, return here and put the task to
        // "sleep" until the last fulfilled dependency re-runs it, starting at
        // the next phase.
        // 
        // Return a pointer to the next sub-task to invoke immediately. If there
        // are no recorded sub-tasks, but this task is incomplete and must
        // continue, we return a pointer to this task instead.
        if (taskDependencies._HasDependencies()) {
            Exec_CompilationTask *const nextSubtask =
                taskDependencies._GetNextSubtask();
            return nextSubtask ? nextSubtask : _task;
        }
    }

    // Invoke the next stage if we haven't returned yet.
    return _InvokeOnePhase(i + 1, std::forward<Tail>(tail)...);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
