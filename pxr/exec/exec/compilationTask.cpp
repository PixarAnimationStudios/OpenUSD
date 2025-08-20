//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/compilationTask.h"

#include "pxr/exec/exec/compilationState.h"

#include "pxr/base/arch/hints.h"

PXR_NAMESPACE_OPEN_SCOPE

static inline void
_RunOrInvoke(
    const Exec_CompilerTaskSync &taskSync,
    Exec_CompilationTask *const task, 
    const int depth)
{
    // We empirically determined a stack depth limit of 50 to preserve the
    // performance optimization gained from recursively invoking tasks, while
    // limiting growth of the stack space.
    // We also performance tested limits of 100 and 200, and were not able to
    // observe a significant performance difference.
    
    if (ARCH_LIKELY(depth < 50)) {
        task->operator()(depth + 1);
    } else {
        taskSync.Run(task);
    }
}

Exec_CompilationTask::~Exec_CompilationTask() = default;

Exec_CompilerTaskSync::ClaimResult
Exec_CompilationTask::TaskDependencies::ClaimSubtask(
    const Exec_OutputKey::Identity &key)
{
    const Exec_CompilerTaskSync::ClaimResult result =
        Exec_CompilationState::OutputTasksAccess::_Get(&_compilationState)
            .Claim(key, _task);
    if (result == Exec_CompilerTaskSync::ClaimResult::Wait) {
        _hasDependencies = true;
    }
    return result;
}

void
Exec_CompilationTask::operator()(const int depth) const
{
    // WorkDispatcher semantics require call operators to be const, but we need
    // to mutate our internal task state.
    Exec_CompilationTask *thisTask = const_cast<Exec_CompilationTask*>(this);

    // Register an additional dependency while this task is running.
    // 
    // This ensures that if sub-tasks complete while this task is still running,
    // the last completed sub-task will not re-run this task and cause it to be
    // re-entrant before we get to the end of this method. We undo this below by
    // calling RemoveDependency().
    thisTask->AddDependency();

    // Call the _Compile() method, which is the main entry point into
    // compilation tasks, and record the task we are told to run next.
    Exec_CompilationTask *const nextTask = [thisTask]{
        TaskPhases taskPhases(
            thisTask, thisTask->_compilationState, thisTask->_taskPhase);
        thisTask->_Compile(thisTask->_compilationState, taskPhases);
        return taskPhases._GetNextTask();
    }();

    // Get the task sync object for running subsequent tasks.
    const Exec_CompilerTaskSync &taskSync =
        Exec_CompilationState::OutputTasksAccess::_Get(
            &thisTask->_compilationState);

    // If a pointer to a next task was returned, thisTask *did not* complete.
    // In this case there are additional phases to run, and one or more
    // sub-tasks constituting unfulfilled dependencies aren't done yet.
    if (nextTask) {
        // If the next task isn't a pointer to thisTask, we are instructed to
        // invoke a specific sub-task (c.f., TBB scheduler bypass).
        // 
        // Note, invoking the next task recursively is fast, but grows the
        // stack. Once we reach a certain stack depth, we will Run() the task to
        // prevent running out of stack space.
        if (nextTask != thisTask) {
            _RunOrInvoke(taskSync, nextTask, depth);
        }

        // Let's remove the dependency we added above to prevent re-entry.
        // 
        // After this line, the last completed dependency will immediately re-
        // run this task - so we *must* return right after. However, if we
        // happen to remove the last remaining dependency here, we are on the
        // hook to re-run this task.
        // 
        // Note, re-invoking this task recursively is fast, but grows the stack.
        // Once we reach a certain stack depth, we will Run() the task to
        // prevent running out of stack space.
        if (thisTask->RemoveDependency() == 0) {
            _RunOrInvoke(taskSync, thisTask, depth);
        }
        return;
    }

    // If the task *did* complete, and it is a sub-task, we need to remove one
    // dependency from the parent task.
    if (Exec_CompilationTask *const parent = thisTask->_parent) {
        // If we remove the last unfulfilled dependency from the parent task,
        // the parent is ready to re-run. We're responsible for making that
        // happen here.
        // 
        // Note, invoking the parent task recursively is fast, but grows the
        // stack. Once we reach a certain stack depth, we will Run() the task to
        // prevent running out of stack space.
        if (parent->RemoveDependency() == 0) {
            _RunOrInvoke(taskSync, parent, depth);
        }
    }

    // The task just completed, and tasks manage their own lifetime: We must
    // delete it now.
    delete thisTask;
}

void
Exec_CompilationTask::_MarkDone(const Exec_OutputKey::Identity &key)
{
    Exec_CompilationState::OutputTasksAccess::_Get(&_compilationState)
        .MarkDone(key);
}

PXR_NAMESPACE_CLOSE_SCOPE
