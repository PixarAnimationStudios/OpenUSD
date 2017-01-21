//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef WORK_DISPATCHER_H
#define WORK_DISPATCHER_H

/// \file work/dispatcher.h

#include "pxr/pxr.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/errorTransport.h"

#include <tbb/concurrent_vector.h>
#include <tbb/task.h>

#include <functional>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// \class WorkDispatcher
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
/// Calls to Run() and Cancel() may be made concurrently.  However, once Wait()
/// is called, calls to Run() and Cancel() must only be made by tasks already
/// added by Run().  Additionaly, Wait() must never be called by a task added by
/// Run(), since that task could never complete.
///
class WorkDispatcher
{
public:
    /// Construct a new dispatcher.
    WorkDispatcher();

    /// Wait() for any pending tasks to complete, then destroy the dispatcher.
    ~WorkDispatcher();

    WorkDispatcher(WorkDispatcher const &) = delete;
    WorkDispatcher &operator=(WorkDispatcher const &) = delete;

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
        _rootTask->spawn(_MakeInvokerTask(std::forward<Callable>(c)));
    }

    template <class Callable, class A0, class ... Args>
    inline void Run(Callable &&c, A0 &&a0, Args&&... args) {
        Run(std::bind(std::forward<Callable>(c),
                      std::forward<A0>(a0),
                      std::forward<Args>(args)...));
    }
    
#endif // doxygen

    /// Block until the work started by Run() completes.
    void Wait();

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
    void Cancel();

private:
    typedef tbb::concurrent_vector<TfErrorTransport> _ErrorTransports;

    // Function invoker helper that wraps the invocation with an ErrorMark so we
    // can transmit errors that occur back to the thread that Wait() s for tasks
    // to complete.
    template <class Fn>
    struct _InvokerTask : public tbb::task {
        explicit _InvokerTask(Fn &&fn, _ErrorTransports *err) 
            : _fn(std::move(fn)), _errors(err) {}

        explicit _InvokerTask(Fn const &fn, _ErrorTransports *err) 
            : _fn(fn), _errors(err) {}

        virtual tbb::task* execute() {
            TfErrorMark m;
            _fn();
            if (!m.IsClean())
                WorkDispatcher::_TransportErrors(m, _errors);
            return NULL;
        }
    private:
        Fn _fn;
        _ErrorTransports *_errors;
    };

    // Make an _InvokerTask instance, letting the function template deduce Fn.
    template <class Fn>
    _InvokerTask<typename std::remove_reference<Fn>::type>&
    _MakeInvokerTask(Fn &&fn) { 
        return *new( _rootTask->allocate_additional_child_of(*_rootTask) )
            _InvokerTask<typename std::remove_reference<Fn>::type>(
                std::forward<Fn>(fn), &_errors);
    }

    // Helper function that removes errors from \p m and stores them in a new
    // entry in \p errors.
    static void
    _TransportErrors(const TfErrorMark &m, _ErrorTransports *errors);

    // Task group context and associated root task that allows us to cancel
    // tasks invoked directly by this dispatcher.
    tbb::task_group_context _context;
    tbb::empty_task* _rootTask;

    // The error transports we use to transmit errors in other threads back to
    // this thread.
    _ErrorTransports _errors;
};

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif // WORK_DISPATCHER_H
