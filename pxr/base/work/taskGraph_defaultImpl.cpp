//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/work/taskGraph_defaultImpl.h"

#include "pxr/base/arch/hints.h"

PXR_NAMESPACE_OPEN_SCOPE

// Invoking the parent task recursively is fast at the cost of growing the 
// stack space. We mitigate this tradeoff by recursing until an empirically 
// determined cutoff depth, at which point we submit subsequent tasks on the 
// owning task graph's work dispatcher to truncate stack growth. 
static inline void
_RunOrInvoke(
    WorkTaskGraph_DefaultImpl::BaseTask *const task, 
    const int depth,
    WorkTaskGraph_DefaultImpl * const taskGraph)
{
    // This cutoff depth was empirically determined to maximize speed gains 
    // from recursively invoking tasks while limiting growth of the stack 
    // space. 
    if (ARCH_LIKELY(depth < 50)) {
        task->operator()(depth + 1, taskGraph);
    } else {
        taskGraph->RunTask(task);
    }
}

WorkTaskGraph_DefaultImpl::BaseTask::~BaseTask() = default;

void
WorkTaskGraph_DefaultImpl::BaseTask::operator()(
    const int depth,
    WorkTaskGraph_DefaultImpl * taskGraph) const 
{
    // Top-level tasks (i.e. tasks submitted via RunTask()) maintain a pointer 
    // to their owning task graph. Capture it to pass to descendents of this 
    // task. 
    if (!taskGraph) {
        taskGraph = _taskGraph;
        TF_VERIFY(taskGraph);
    }

    // Since oneTBB requires a const call operator, we must const_cast here. 
    WorkTaskGraph_DefaultImpl::BaseTask * const thisTask = 
        const_cast<WorkTaskGraph_DefaultImpl::BaseTask *>(this);

    // When running a task, it is initially not recycled. The execute() method
    // will recycle the task if needed.
    thisTask->_recycle = false;

    // Perform the work defined by the derived task class.
    WorkTaskGraph_DefaultImpl::BaseTask * const nextTask = thisTask->execute();

    // The task has been recycled.
    if (_recycle) {
        // If this task is recycled, we expect the reference count was increment
        // within the execute() method. Remove this extra reference now.
        // 
        // If the reference count reaches zero, there are no more child tasks
        // running, and we are responsible for re-executing the task here.
        if (thisTask->RemoveChildReference() == 0) {
            _RunOrInvoke(thisTask, depth, taskGraph);
        }
    }
        
    // The task has not been recycled.
    else {
        // Retain a pointer to the parent task, if any, before deleting this
        // task.
        WorkTaskGraph_DefaultImpl::BaseTask * const parentTask = _parent;

        // If this task is completed, destroy it and reclaim its resources. 
        delete this;

        // If this is the last child of a recycled parent task, execute the
        // parent task. 
        if (parentTask && parentTask->RemoveChildReference() == 0 && 
                parentTask->_recycle) { 
            _RunOrInvoke(parentTask, depth, taskGraph);
        }
    }

    // Run the next task, if one has been provided.
    if (nextTask) { 
        _RunOrInvoke(nextTask, depth, taskGraph);
    }
}

void
WorkTaskGraph_DefaultImpl::Wait() {
    _dispatcher.Wait();
}

PXR_NAMESPACE_CLOSE_SCOPE
