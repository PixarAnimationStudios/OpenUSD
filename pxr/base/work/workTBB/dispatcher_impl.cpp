//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/work/workTBB/dispatcher_impl.h"

PXR_NAMESPACE_OPEN_SCOPE

WorkImpl_Dispatcher::WorkImpl_Dispatcher()
    : _context(
        tbb::task_group_context::isolated,
        tbb::task_group_context::concurrent_wait | 
        tbb::task_group_context::default_traits)
#if TBB_INTERFACE_VERSION_MAJOR >= 12
      , _taskGroup(_context)
#endif
{
#if TBB_INTERFACE_VERSION_MAJOR < 12
    // The concurrent_wait flag used with the task_group_context ensures
    // the ref count will remain at 1 after all predecessor tasks are
    // completed, so we don't need to keep resetting it in Wait().
    _rootTask = new(tbb::task::allocate_root(_context)) tbb::empty_task;
    _rootTask->set_ref_count(1);
#endif
}

#if TBB_INTERFACE_VERSION_MAJOR >= 12
inline tbb::detail::d1::wait_context& 
WorkImpl_Dispatcher::_TaskGroup::_GetInternalWaitContext() {
#if TBB_INTERFACE_VERSION_MINOR >= 14
    return m_wait_vertex.get_context();
#else
    return m_wait_ctx;
#endif
}
#endif

WorkImpl_Dispatcher::~WorkImpl_Dispatcher() noexcept
{
    Wait();

#if TBB_INTERFACE_VERSION_MAJOR < 12
    tbb::task::destroy(*_rootTask);
#endif
}

void
WorkImpl_Dispatcher::Wait()
{
    // Wait for tasks to complete.
#if TBB_INTERFACE_VERSION_MAJOR >= 12
    // The native task_group::wait() has a comment saying its call to the
    // context reset method is not thread safe. So we do our own
    // synchronization to ensure it is called once.
    tbb::detail::d1::wait(_taskGroup._GetInternalWaitContext(), _context);
#else
    _rootTask->wait_for_all();
#endif
}

void
WorkImpl_Dispatcher::Reset() {
    if (_context.is_group_execution_cancelled()) {
        _context.reset();
    }
}

void
WorkImpl_Dispatcher::Cancel()
{
#if TBB_INTERFACE_VERSION_MAJOR >= 12
    _taskGroup.cancel();
#else
    _context.cancel_group_execution();
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
