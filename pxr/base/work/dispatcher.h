//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_DISPATCHER_H
#define PXR_BASE_WORK_DISPATCHER_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/work/api.h"
#include "pxr/base/work/impl.h"
#include "pxr/base/work/threadLimits.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/errorTransport.h"

#include <functional>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// The Work_Dispatcher interface, specialized with a dispatcher impl template
// argument.
// 
// Clients expected to use the WorkDispatcher type instead.
template <class Impl>
class Work_Dispatcher
{
protected:
    // Prevent construction of the work dispatcher base class.
    WORK_API Work_Dispatcher();

public:
    /// Wait() for any pending tasks to complete, then destroy the dispatcher.
    WORK_API ~Work_Dispatcher() noexcept;

    Work_Dispatcher(Work_Dispatcher const &) = delete;
    Work_Dispatcher &operator=(Work_Dispatcher const &) = delete;

#ifdef doxygen

    /// Add work for the dispatcher to run.
    ///
    /// Before a call to Wait() is made it is safe for any client to invoke
    /// Run().  Once Wait() is invoked, it is \b only safe to invoke Run() from
    /// within the execution of tasks already added via Run().
    ///
    /// This function does not block, in general.  It may block if concurrency
    /// is limited to 1.  The added work may be not yet started, may be started
    /// but not completed, or may be completed upon return.  No guarantee is
    /// made.
    template <class Callable, class A1, class A2, ... class AN>
    void Run(Callable &&c, A1 &&a1, A2 &&a2, ... AN &&aN);

#else // doxygen

    template <class Callable>
    inline void Run(Callable &&c) {
        _dispatcher.Run(
            _InvokerTask<typename std::remove_reference<Callable>::type>(
                std::forward<Callable>(c), &_errors));
    }

    template <class Callable, class A0, class ... Args>
    inline void Run(Callable &&c, A0 &&a0, Args&&... args) {
        Run(std::bind(std::forward<Callable>(c),
                      std::forward<A0>(a0),
                      std::forward<Args>(args)...));
    }
    
#endif // doxygen

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

    /// Returns true if Cancel() has been called.  Calling Wait() will reset the
    /// cancel state.
    WORK_API bool IsCancelled() const;

private:
    typedef tbb::concurrent_vector<TfErrorTransport> _ErrorTransports;

    // Function invoker helper that wraps the invocation with an ErrorMark so we
    // can transmit errors that occur back to the thread that Wait() s for tasks
    // to complete.
    template <class Fn>
    struct _InvokerTask {
        explicit _InvokerTask(Fn &&fn, _ErrorTransports *err) 
            : _fn(std::move(fn)), _errors(err) {}

        explicit _InvokerTask(Fn const &fn, _ErrorTransports *err) 
            : _fn(fn), _errors(err) {}

        // Ensure only moves happen, no copies.
        _InvokerTask(_InvokerTask &&other) = default;
        _InvokerTask(const _InvokerTask &other) = delete;
        _InvokerTask &operator=(const _InvokerTask &other) = delete;

        void operator()() const {
            TfErrorMark m;
            _fn();
            if (!m.IsClean())
                Work_Dispatcher::_TransportErrors(m, _errors);
        }
    private:
        Fn _fn;
        _ErrorTransports *_errors;
    };

    // Helper function that removes errors from \p m and stores them in a new
    // entry in \p errors.
    WORK_API static void
    _TransportErrors(const TfErrorMark &m, _ErrorTransports *errors);

    // WorkDispatcher implementation
    Impl _dispatcher;
    std::atomic<bool> _isCancelled;

    // The error transports we use to transmit errors in other threads back to
    // this thread.
    _ErrorTransports _errors;

    // Concurrent calls to Wait() have to serialize certain cleanup operations.
    std::atomic_flag _waitCleanupFlag;
};

/// \class WorkDispatcher
/// \extends Work_Dispatcher
///
/// A work dispatcher runs concurrent tasks.  The dispatcher supports adding
/// new tasks from within running tasks.  This suits problems that exhibit
/// hierarchical structured parallelism: tasks that discover additional tasks
/// during their execution.
///
/// Typical use is to create a dispatcher and invoke Run() to begin doing
/// work, then Wait() for the work to complete.  Tasks may invoke Run() during
/// their execution as they discover additional tasks to perform.
///
/// For example,
///
/// \code
/// WorkDispatcher dispatcher;
/// for (i = 0; i != N; ++i) {
///     dispatcher.Run(DoSomeWork, workItem[i]);
/// }
/// dispatcher.Wait();
/// \endcode
///
/// Calls to Run() and Cancel() may be made concurrently. Calls to Wait() may
/// also be made concurrently.  However, once any calls to Wait() are in-flight,
/// calls to Run() and Cancel() must only be made by tasks already added by
/// Run().  This means that users of this class are responsible to synchronize
/// concurrent calls to Wait() to ensure this requirement is met.
///
/// Additionally, Wait() must never be called by a task added by Run(), since
/// that task could never complete.
///
class WorkDispatcher 
    : public Work_Dispatcher<PXR_WORK_IMPL_NS::WorkImpl_Dispatcher>
{};

// Wrapper class for non-const tasks.
template <class Fn>
struct Work_DeprecatedMutableTask {
    explicit Work_DeprecatedMutableTask(Fn &&fn) 
        : _fn(std::move(fn)) {}

    explicit Work_DeprecatedMutableTask(Fn const &fn) 
        : _fn(fn) {}

    // Ensure only moves happen, no copies.
    Work_DeprecatedMutableTask
        (Work_DeprecatedMutableTask &&other) = default;
    Work_DeprecatedMutableTask
        (const Work_DeprecatedMutableTask &other) = delete;
    Work_DeprecatedMutableTask
        &operator= (const Work_DeprecatedMutableTask &other) = delete;

    void operator()() const {
        _fn();
    }
private:
    mutable Fn _fn;
};

// Wrapper function to convert non-const tasks to a Work_DeprecatedMutableTask. 
// When adding new tasks refrain from using this wrapper, instead ensure the 
// call operator of the task is const such that it is compatible with oneTBB.
template <typename Fn>
Work_DeprecatedMutableTask<typename std::remove_reference_t<Fn>> 
WorkMakeDeprecatedMutableTask(Fn &&fn) {
    return Work_DeprecatedMutableTask<typename std::remove_reference_t<Fn>>
            (std::forward<Fn>(fn));
}

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_DISPATCHER_H
