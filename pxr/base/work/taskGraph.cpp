//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/work/taskGraph.h"

#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

WorkTaskGraph::BaseTask::~BaseTask() = default;

void
WorkTaskGraph::RunLists(const TaskLists &taskLists)
{
    WorkParallelForEach(
        taskLists.begin(), taskLists.end(),
        [this] (const TaskList &taskList) {
            for (const auto task : taskList) {
                RunTask(task);
            }
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
