//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_DETACHED_TASK_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_DETACHED_TASK_H

#include "workTaskflowExample/api.h"
#include "workTaskflowExample/dispatcher.h"

WORK_TASKFLOW_EXAMPLE_API
WorkImpl_Dispatcher &
WorkTaskflow_GetDetachedDispatcher();

WORK_TASKFLOW_EXAMPLE_API
void
WorkTaskflow_EnsureDetachedTaskProgress();

template <typename Fn>
void
WorkImpl_RunDetachedTask(Fn &&fn)
{
    WorkImpl_Dispatcher &dispatcher = WorkTaskflow_GetDetachedDispatcher();
    dispatcher.RunWithGlobal(std::forward<Fn>(fn));
    WorkTaskflow_EnsureDetachedTaskProgress();
};

#endif // PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_DETACHED_TASK_H
