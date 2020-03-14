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
#ifndef PXR_BASE_WORK_SINGULAR_TASK_H
#define PXR_BASE_WORK_SINGULAR_TASK_H

/// \file work/singularTask.h

#include "pxr/pxr.h"

#include <atomic>
#include <functional>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

class WorkDispatcher;
class WorkArenaDispatcher;

/// \class WorkSingularTask
///
/// A WorkSingularTask runs a task in a WorkDispatcher, but never concurrently
/// with itself.  That is, the function provided to the WorkSingularTask runs
/// concurrently with other tasks in the WorkDispatcher, but never with another
/// invocation of itself.
///
/// This is useful if there is single-threaded work to do that can be overlapped
/// with other parallel tasks in a dispatcher.  For example, a
/// multiple-producer, single-consumer problem can be tackled this way.  Run the
/// producer tasks as usual in a WorkDispatcher and create a WorkSingularTask
/// for the consumer.  When a producer task has generated a result to consume,
/// it invokes Wake() on the consumer task.  This ensures that the consumer runs
/// only when there are results to consume, and it lets the consumer operate
/// single-threaded.  For example, the consumer could populate stl containers
/// without locking.
///
class WorkSingularTask
{
public:

    WorkSingularTask(WorkSingularTask const &) = delete;
    WorkSingularTask &operator=(WorkSingularTask const &) = delete;

#ifdef doxygen

    /// Create a singular task to be run in \p dispatcher.  Callers must ensure
    /// that \p dispatcher lives at least as long as this WorkSingularTask.
    ///
    /// A singular task is one that will not run concurrently with itself.  See
    /// the WorkSingularTask doc for more details.
    ///
    /// After constructing a WorkSingularTask, call Wake() to ensure that the
    /// task runs at least once.
    template <class Callable, class A1, class A2, ... class AN>
    WorkSingularTask(WorkDispatcher &dispatcher,
                     Callable &&c, A1 &&a1, A2 &&a2, ... AN &&aN);

    /// \overload
    template <class Callable, class A1, class A2, ... class AN>
    WorkSingularTask(WorkArenaDispatcher &dispatcher,
                     Callable &&c, A1 &&a1, A2 &&a2, ... AN &&aN);

#else // doxygen

    template <class Callable, class... Args>
    WorkSingularTask(WorkDispatcher &d, Callable &&c, Args&&... args)
        : _waker(_MakeWaker(d, std::bind(std::forward<Callable>(c),
                                         std::forward<Args>(args)...)))
        , _count(0) {}

    template <class Callable, class... Args>
    WorkSingularTask(WorkArenaDispatcher &d, Callable &&c, Args&&... args)
        : _waker(_MakeWaker(d, std::bind(std::forward<Callable>(c),
                                         std::forward<Args>(args)...)))
        , _count(0) {}

#endif // doxygen

    /// Ensure that this task runs at least once after this call.  The task is
    /// not guaranteed to run as many times as Wake() is invoked, only that it
    /// run at least once after a call to Wake().
    inline void Wake() {
        if (++_count == 1)
            _waker(_count);
    }

private:
    template <class Dispatcher, class Fn>
    struct _Waker {
        explicit _Waker(Dispatcher &d, Fn &&fn)
            : _dispatcher(d), _fn(std::move(fn)) {}

        void operator()(std::atomic_size_t &count) const {
            _dispatcher.Run(
                [this, &count]() {
                    // We read the current refCount into oldCount, then we
                    // invoke the task function.  Finally we try to CAS the
                    // refCount to zero.  If we fail, it means some other
                    // clients have invoked Wake() in the meantime.  In that
                    // case we go again to ensure the task can do whatever it
                    // was awakened to do.  Once we successfully take the count
                    // to zero, we stop.
                    size_t old = count;
                    do { _fn(); } while (
                        !count.compare_exchange_strong(old, 0));
                });
        }
        Dispatcher &_dispatcher;
        Fn _fn;
    };

    template <class Dispatcher, class Fn>
    static std::function<void (std::atomic_size_t &)>
    _MakeWaker(Dispatcher &d, Fn &&fn) {
        return std::function<void (std::atomic_size_t &)>(
            _Waker<Dispatcher, typename std::decay<Fn>::type>(
                d, std::forward<Fn>(fn)));
    }

    std::function<void (std::atomic_size_t &)> _waker;
    std::atomic_size_t _count;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_SINGULAR_TASK_H
