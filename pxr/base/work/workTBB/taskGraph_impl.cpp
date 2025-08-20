//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/work/workTBB/taskGraph_impl.h"

#if TBB_INTERFACE_VERSION_MAJOR < 12

PXR_NAMESPACE_OPEN_SCOPE

WorkImpl_TaskGraph::WorkImpl_TaskGraph()
    : _context(
        tbb::task_group_context::isolated,
        tbb::task_group_context::concurrent_wait | 
        tbb::task_group_context::default_traits)
    , _rootTask(new (tbb::task::allocate_root(_context)) tbb::empty_task()) 
{
    _rootTask->set_ref_count(1);
}

WorkImpl_TaskGraph::~WorkImpl_TaskGraph() noexcept
{
    _rootTask->wait_for_all();
    tbb::task::destroy(*_rootTask);
}

WorkImpl_TaskGraph::BaseTask::~BaseTask() = default;

void
WorkImpl_TaskGraph::Wait()
{
    _rootTask->wait_for_all();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TBB_INTERFACE_VERSION_MAJOR < 12
