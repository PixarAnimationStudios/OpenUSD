//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_TBB_ISOLATING_DISPATCHER_IMPL_H
#define PXR_BASE_WORK_TBB_ISOLATING_DISPATCHER_IMPL_H

#include "pxr/pxr.h"

#include "pxr/base/work/api.h"
#include "pxr/base/work/workTBB/dispatcher_impl.h"

#include <tbb/task_arena.h>

#include <utility>

// TBB implements work stealing, so the impl provides a specialization of the
// dispatcher with work stealing isolation semantics:
// WorkImpl_IsolatingDispatcher.
// 
// If this is not defined, both WorkDispatcher and WorkIsolatingDispatcher will
// use the WorkImpl_Dispatcher implementation.
#define WORK_IMPL_HAS_ISOLATING_DISPATCHER

PXR_NAMESPACE_OPEN_SCOPE

class WorkImpl_IsolatingDispatcher
{
public:
    WORK_API WorkImpl_IsolatingDispatcher();
    WORK_API ~WorkImpl_IsolatingDispatcher() noexcept;

    WorkImpl_IsolatingDispatcher(WorkImpl_IsolatingDispatcher const &) = delete;
    WorkImpl_IsolatingDispatcher &operator=(
        WorkImpl_IsolatingDispatcher const &) = delete;

    template <class Callable>
    inline void Run(Callable &&c) {
        _arena->execute([&dispatcher = _dispatcher, &c](){ 
            dispatcher.Run(std::forward<Callable>(c));
        });
    }
    
    void Reset() {
        _dispatcher.Reset();
    }

    WORK_API void Wait();

    void Cancel() {
        _dispatcher.Cancel();
    }

private:
    tbb::task_arena *_arena;
    WorkImpl_Dispatcher _dispatcher;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_TBB_ISOLATING_DISPATCHER_IMPL_H
