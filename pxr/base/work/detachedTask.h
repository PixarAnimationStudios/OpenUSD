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
    void operator()() {
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
