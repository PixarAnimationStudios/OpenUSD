//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_TBB_TASK_GRAPH_IMPL_H
#define PXR_BASE_WORK_TBB_TASK_GRAPH_IMPL_H

#include "pxr/pxr.h"
#include "pxr/base/work/api.h"

// The TBB version macro is located in different headers in legacy TBB
// (tbb/tbb_stddef.h) and oneTBB (tbb/version.h)
#if __has_include(<tbb/tbb_stddef.h>)
#include <tbb/tbb_stddef.h>
#elif __has_include(<tbb/version.h>)
#include <tbb/version.h>
#endif

#ifndef TBB_INTERFACE_VERSION_MAJOR
#error "TBB version macro TBB_INTERFACE_VERSION_MAJOR not found"
#endif

#if TBB_INTERFACE_VERSION_MAJOR < 12

// Under legacy TBB we implement the task graph using tbb::task.
//
// tbb::task does not exist in oneTBB, so we fall back to WorkTaskGraph's
// default implementation by not defining the WorkImpl_TaskGraph customization. 
#define WORK_IMPL_HAS_TASK_GRAPH

#include <tbb/task.h>

PXR_NAMESPACE_OPEN_SCOPE

class WorkImpl_TaskGraph
{
public:
    WORK_API WorkImpl_TaskGraph();
    WORK_API ~WorkImpl_TaskGraph() noexcept;

    WorkImpl_TaskGraph(WorkImpl_TaskGraph const &) = delete;
    WorkImpl_TaskGraph &operator=(WorkImpl_TaskGraph const &) = delete;

    class BaseTask;

    template <typename F, typename ... Args>
    F * AllocateTask(Args&&... args);

    template <typename F>
    inline void RunTask(F * task) {
        tbb::task::spawn(*task);
    }

    WORK_API void Wait();

private:
    // Task group context for initializing the root task. 
    tbb::task_group_context _context;

    // Root task that serves as an anchor to allocate additional children 
    // during execution. 
    tbb::empty_task *_rootTask;
};

class WorkImpl_TaskGraph::BaseTask : public tbb::task { 
public:
    BaseTask() = default;
    WORK_API virtual ~BaseTask();

    void AddChildReference() {
        increment_ref_count();
    }

    int RemoveChildReference() {
        return decrement_ref_count();
    }

    template <typename F, typename... Args>
    F * AllocateChild(Args &&...args) {
        return new (tbb::task::allocate_additional_child_of(*this))
            F{std::forward<Args>(args)...};
    }

protected:
    void _RecycleAsContinuation() {
        recycle_as_safe_continuation();
    }

};

template <typename F, typename ... Args>
F * WorkImpl_TaskGraph::AllocateTask(Args&&... args) {
    return new (tbb::task::allocate_additional_child_of(*_rootTask))
        F{std::forward<Args>(args)...};
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TBB_INTERFACE_VERSION_MAJOR < 12

#endif // PXR_BASE_WORK_TBB_TASK_GRAPH_IMPL_H
