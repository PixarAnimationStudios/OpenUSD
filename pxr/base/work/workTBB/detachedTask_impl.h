//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_TBB_DETACHED_TASK_IMPL_H
#define PXR_BASE_WORK_TBB_DETACHED_TASK_IMPL_H

#include <tbb/blocked_range.h>
#include "pxr/base/work/workTBB/dispatcher_impl.h"
#include "pxr/base/work/api.h"

#if TBB_INTERFACE_VERSION_MAJOR >= 12
#include <tbb/task_group.h>
#else
#include <tbb/task.h>
#endif

#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class WorkImpl_Dispatcher;

WORK_API 
WorkImpl_Dispatcher & WorkTBB_GetDetachedDispatcher();

WORK_API
void WorkTBB_EnsureDetachedTaskProgress();

/// Invoke \p fn asynchronously, discard any errors it produces, and provide
/// no way to wait for it to complete.
template <class Fn>
inline void WorkImpl_RunDetachedTask(Fn &&fn){
    WorkTBB_GetDetachedDispatcher().Run(std::forward<Fn>(fn));
    WorkTBB_EnsureDetachedTaskProgress();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_TBB_DETACHED_TASK_IMPL_H
