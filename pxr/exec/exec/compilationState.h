//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_COMPILATION_STATE_H
#define PXR_EXEC_EXEC_COMPILATION_STATE_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/compilerTaskSync.h"
#include "pxr/exec/exec/taskCycleDetector.h"

#include "pxr/base/work/dispatcher.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class EsfStage;
class Exec_CompilationTask;
class Exec_Program;
class WorkDispatcher;

/// Data shared between all compilation tasks.
/// 
/// We construct an instance of this class at the beginning of a round of
/// compilation and then pass along a reference to this instance to all
/// compilation tasks. This prevents bloating the size of every task with this
/// commonly used data.
/// 
class Exec_CompilationState
{
public:
    Exec_CompilationState(
        WorkDispatcher &dispatcher,
        const EsfStage &stage,
        Exec_Program *program)
        : _dispatcher(dispatcher)
        , _stage(stage)
        , _outputTasks(dispatcher)
        , _inputRecompilationTasks(dispatcher)
        , _cycleDetectingTasks(dispatcher)
        , _program(program) {
        TF_VERIFY(_program);
    }

    /// The dispatcher for running tasks.
    WorkDispatcher &GetDispatcher() {
        return _dispatcher;
    }

    /// The scene adapter stage.
    const EsfStage &GetStage() const {
        return _stage;
    }

    /// The program being compiled.
    Exec_Program *GetProgram() {
        return _program;
    }

    /// Gets object for tracking potential task cycles.
    Exec_TaskCycleDetector &GetTaskCycleDetector() {
        return _taskCycleDetector;
    }

    /// Extends access to the various Exec_CompilerTaskSync<T> members.
    class TaskSyncAccess {
        friend class Exec_CompilationTask;

        static Exec_OutputProvidingTaskSync &
        _GetOutputProvidingTaskSync(Exec_CompilationState *state) {
            return state->_outputTasks;
        }

        static Exec_InputRecompilationTaskSync &
        _GetInputRecompilationTaskSync(Exec_CompilationState *state) {
            return state->_inputRecompilationTasks;
        }

        static Exec_CycleDetectingTaskSync &
        _GetCycleDetectingTaskSync(Exec_CompilationState *state) {
            return state->_cycleDetectingTasks;
        }
    };

    /// Constructs and runs a new top-level compilation task.
    template<class TaskType, class ... Args>
    static void NewTask(Exec_CompilationState &state, Args&&... args);

private:
    WorkDispatcher &_dispatcher;
    const EsfStage &_stage;
    Exec_TaskCycleDetector _taskCycleDetector;
    Exec_OutputProvidingTaskSync _outputTasks;
    Exec_InputRecompilationTaskSync _inputRecompilationTasks;
    Exec_CycleDetectingTaskSync _cycleDetectingTasks;
    Exec_Program *_program;
};

template<class TaskType, class ... Args>
void
Exec_CompilationState::NewTask(Exec_CompilationState &state, Args&&... args)
{
    // TODO: We need a small-object task allocator.
    // Tasks manage their own lifetime, and delete themselves after completion.
    TaskType *const task = new TaskType(state, std::forward<Args>(args)...);
    state._dispatcher.Run(std::ref(*task));
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
