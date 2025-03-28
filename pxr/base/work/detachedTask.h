//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_DETACHED_TASK_H
#define PXR_BASE_WORK_DETACHED_TASK_H

/// \file work/detachedTask.h

#include "pxr/pxr.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/work/api.h"
#include "pxr/base/work/dispatcher.h"

#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

template <class Fn>
struct Work_DetachedTask
{
    explicit Work_DetachedTask(Fn &&fn) : _fn(std::move(fn)) {}
    explicit Work_DetachedTask(Fn const &fn) : _fn(fn) {}
    void operator()() const {
        TfErrorMark m;
        _fn();
        m.Clear();
    }
private:
    Fn _fn;
};

WORK_API
WorkDispatcher &Work_GetDetachedDispatcher();

WORK_API
void Work_EnsureDetachedTaskProgress();

/// Invoke \p fn asynchronously, discard any errors it produces, and provide
/// no way to wait for it to complete.
template <class Fn>
void WorkRunDetachedTask(Fn &&fn)
{
    using FnType = typename std::remove_reference<Fn>::type;
    Work_DetachedTask<FnType> task(std::forward<Fn>(fn));
    if (WorkHasConcurrency()) {
        Work_GetDetachedDispatcher().Run(std::move(task));
        Work_EnsureDetachedTaskProgress();
    }
    else {
        task();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_DETACHED_TASK_H
