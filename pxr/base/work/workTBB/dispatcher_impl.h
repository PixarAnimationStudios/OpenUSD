//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_TBB_DISPATCHER_IMPL_H
#define PXR_BASE_WORK_TBB_DISPATCHER_IMPL_H

#include "pxr/pxr.h"
#include "pxr/base/work/api.h"

// Blocked range is not used in this file, but this header happens to pull in
// the TBB version header in a way that works in all TBB versions.
#include <tbb/blocked_range.h>
#include <tbb/concurrent_vector.h>
#if TBB_INTERFACE_VERSION_MAJOR >= 12
#include <tbb/task_group.h>
#else
#include <tbb/task.h>
#endif

#include <functional>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class WorkImpl_Dispatcher
{
public:
    /// Construct a new dispatcher.
    WORK_API WorkImpl_Dispatcher();

    /// Wait() for any pending tasks to complete, then destroy the dispatcher.
    WORK_API ~WorkImpl_Dispatcher() noexcept;

    WorkImpl_Dispatcher(WorkImpl_Dispatcher const &) = delete;
    WorkImpl_Dispatcher &operator=(WorkImpl_Dispatcher const &) = delete;

    template <class Callable>
    inline void Run(Callable &&c) {
#if TBB_INTERFACE_VERSION_MAJOR >= 12
        _taskGroup.run(std::forward<Callable>(c));
#else
        _rootTask->spawn(
            *new(_rootTask->allocate_additional_child_of(*_rootTask))
                _InvokerTaskWrapper<typename std::remove_reference<Callable>::type>(
                std::forward<Callable>(c)));
#endif
    }
    
    /// Reinitialize context for WorkImpl_Dispatcher if it has one.
    WORK_API void Reset();

    /// Block until the work started by Run() completes.
    WORK_API void Wait();

    /// Cancel remaining work and return immediately.
    ///
    /// Calling this function affects task that are being run directly
    /// by this dispatcher. If any of these tasks are using their own
    /// dispatchers to run tasks, these dispatchers will not be affected
    /// and these tasks will run to completion, unless they are also
    /// explicitly cancelled.
    ///
    /// This call does not block.  Call Wait() after Cancel() to wait for
    /// pending tasks to complete.
    WORK_API void Cancel();

#if TBB_INTERFACE_VERSION_MAJOR < 12
    template <class Fn>
    struct _InvokerTaskWrapper : public tbb::task {
        explicit _InvokerTaskWrapper(Fn &&fn)
            : _fn(std::move(fn)) {}

        explicit _InvokerTaskWrapper(Fn const &fn)
            : _fn(fn) {}

        virtual tbb::task* execute() {
            // In anticipation of OneTBB, ensure that _fn meets OneTBB's
            // requirement that a task's call operator must be const.
            const_cast<_InvokerTaskWrapper const *>(this)->_fn();
            return NULL;
        }
    private:
        Fn _fn;
    };
#endif
    // Task group context to run tasks in.
    tbb::task_group_context _context;
#if TBB_INTERFACE_VERSION_MAJOR >= 12
    // Custom task group that lets us implement thread safe concurrent wait.
    class _TaskGroup : public tbb::task_group {
    public:
        _TaskGroup(tbb::task_group_context& ctx) : tbb::task_group(ctx) {}
         inline tbb::detail::d1::wait_context& _GetInternalWaitContext();
    };

    _TaskGroup _taskGroup;
#else
    // Root task that allows us to cancel tasks invoked directly by this
    // dispatcher.
    tbb::empty_task* _rootTask;
#endif

};

PXR_NAMESPACE_CLOSE_SCOPE

///////////////////////////////////////////////////////////////////////////////

#endif // PXR_BASE_WORK_TBB_DISPATCHER_IMPL_H
