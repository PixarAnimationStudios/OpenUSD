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
#ifndef WORK_ARENA_DISPATCHER_H
#define WORK_ARENA_DISPATCHER_H

/// \file work/arenaDispatcher.h

#include "pxr/pxr.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/base/work/api.h"

#include <tbb/task_arena.h>

#include <functional>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// \class WorkArenaDispatcher
///
/// This is a specialization of the WorkDispatcher that uses an isolated arena
/// to Run() all its tasks in. The WorkArenaDispatcher is useful where it must
/// be guaranteed that a specific set of tasks shall not be stolen by any
/// other dispatcher, or where stealing from other dispatchers could cause
/// lock dependencies that may lead to deadlocks. Note that a regular
/// WorkDispatcher can provide better throughput, and should thus be the
/// preferred over the WorkArenaDispatcher.
///
/// The interface of the WorkArenaDispatcher, and thread-safety notes about its
/// API are identical to those of the WorkDispatcher.
///
class WorkArenaDispatcher
{
public:
    /// Constructs a new dispatcher. The internal arena will mirror the
    /// global concurrency limit setting.
    WorkArenaDispatcher() : _arena(WorkGetConcurrencyLimit()) {}

    /// Wait() for any pending tasks to complete, then destroy the dispatcher.
    WORK_API ~WorkArenaDispatcher();

    WorkArenaDispatcher(WorkArenaDispatcher const &) = delete;
    WorkArenaDispatcher &operator=(WorkArenaDispatcher const &) = delete;

#ifdef doxygen

    /// Add work for the dispatcher to run.
    ///
    /// Before a call to Wait() is made it is safe for any client to invoke
    /// Run().  Once Wait() is invoked, it is \b only safe to invoke Run() from
    /// within the execution of tasks already added via Run().
    ///
    /// This function does not block.  The added work may be not yet started,
    /// may be started but not completed, or may be completed upon return.  No
    /// guarantee is made.
    template <class Callable, class A1, class A2, ... class AN>
    void Run(Callable &&c, A1 &&a1, A2 &&a2, ... AN &&aN);

#else // doxygen

    template <class Callable, class ... Args>
    inline void Run(Callable &&c, Args&&... args) {
        _arena.execute(
            _MakeRunner(&_dispatcher,
                        std::bind(std::forward<Callable>(c),
                                  std::forward<Args>(args)...)));
    }

#endif // doxygen

    /// Block until the work started by Run() completes.
    WORK_API void Wait();

    /// Cancel remaining work and return immediately.
    ///
    /// This call does not block.  Call Wait() after Cancel() to wait for
    /// pending tasks to complete.
    WORK_API void Cancel();

private:
    template <class Fn>
    struct _Runner {
        _Runner(WorkDispatcher *wd, Fn &&fn) : _wd(wd), _fn(std::move(fn)) {}
        _Runner(WorkDispatcher *wd, Fn const &fn) : _wd(wd), _fn(fn) {}
        void operator()() { _wd->Run(std::move(_fn)); }
        void operator()() const { _wd->Run(std::move(_fn)); }
    private:
        WorkDispatcher *_wd;
        mutable Fn _fn;
    };

    template <class Fn>
    _Runner<typename std::remove_reference<Fn>::type>
    _MakeRunner(WorkDispatcher *wd, Fn &&fn) {
        return _Runner<typename std::remove_reference<Fn>::type>(
            wd, std::forward<Fn>(fn));
    }

    // The task arena.
    tbb::task_arena _arena;

    // The dispatcher.
    WorkDispatcher _dispatcher;
};

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif // WORK_ARENA_DISPATCHER_H
