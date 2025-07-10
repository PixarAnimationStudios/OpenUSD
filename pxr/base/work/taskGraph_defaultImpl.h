//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_TASK_GRAPH_DEFAULT_IMPL_H
#define PXR_BASE_WORK_TASK_GRAPH_DEFAULT_IMPL_H

#include "pxr/pxr.h"

#include "pxr/base/work/api.h"
#include "pxr/base/work/dispatcher.h"

#include <atomic>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// Default implementation for WorkTaskGraph.
// See documentation on WorkTaskGraph and WorkTaskGraph::BaskTask for API docs.

class WorkTaskGraph_DefaultImpl
{
public:
    WorkTaskGraph_DefaultImpl() = default;
    ~WorkTaskGraph_DefaultImpl() noexcept = default;

    WorkTaskGraph_DefaultImpl(WorkTaskGraph_DefaultImpl const &) = delete;
    WorkTaskGraph_DefaultImpl &
    operator=(WorkTaskGraph_DefaultImpl const &) = delete;

    class BaseTask;

    template <typename F, typename ... Args>
    F * AllocateTask(Args&&... args);

    template <typename F>
    inline void RunTask(F * task) {
        task->_SetTaskGraph(this);
        _dispatcher.Run(std::ref(*task));
    }

    WORK_API void Wait();

private:
    // Work dispatcher for running and synchronizing on tasks.
    WorkDispatcher _dispatcher;
};

class WorkTaskGraph_DefaultImpl::BaseTask {
public:
    BaseTask() = default;
    WORK_API virtual ~BaseTask();

    /// Callable operator that implements continuation passing, recycling, and 
    /// scheduler bypass. 
    WORK_API void operator()(const int depth = 0, 
        WorkTaskGraph_DefaultImpl * const taskGraph = nullptr) const;

    virtual BaseTask * execute() = 0;

    void AddChildReference() {
        _childCount.fetch_add(1, std::memory_order_acquire);
    }

    int RemoveChildReference() {
        return _childCount.fetch_sub(1, std::memory_order_release) - 1;
    }
    
    template <typename F, typename ... Args>
    F * AllocateChild(Args&&... args) {
        AddChildReference();
        F* obj = new F{std::forward<Args>(args)...};
        obj->_parent = this;
        return obj;
    }

protected:
    void _RecycleAsContinuation() {
        _recycle = true;
    }

private:
    // Befriend the task graph that owns instances of this class. 
    friend class WorkTaskGraph_DefaultImpl;

    // Set the back-pointer to this task's owning task graph. 
    void _SetTaskGraph(WorkTaskGraph_DefaultImpl * const taskGraph) {
        _taskGraph = taskGraph;
    }

private:
    // The task graph that owns this task. 
    WorkTaskGraph_DefaultImpl * _taskGraph = nullptr;

    // The parent/successor of this task. 
    BaseTask * _parent = nullptr;

    // A flag that indicates whether this task is awaiting re-execution.
    bool _recycle = false;

    // An atomic reference count to track this task's pending children. 
    std::atomic<int> _childCount = 0;
};

template <typename F, typename ... Args>
F * WorkTaskGraph_DefaultImpl::AllocateTask(Args&&... args) {
    return new F{std::forward<Args>(args)...};
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif 
