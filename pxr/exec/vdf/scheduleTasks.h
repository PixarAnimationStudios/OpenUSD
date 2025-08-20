//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SCHEDULE_TASKS_H
#define PXR_EXEC_VDF_SCHEDULE_TASKS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/mask.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfOutput;

/// Type describing a task id. An id is comprised of an index, as well as
/// one bit of additional information.
/// 
typedef uint32_t VdfScheduleTaskId;

/// Type describing a task index.
///
typedef uint32_t VdfScheduleTaskIndex;

/// Type describing a number of task indices or ids.
///
typedef uint32_t VdfScheduleTaskNum;

/// A sentinel value used to describe an invalid task index or id.
///
static const uint32_t VdfScheduleTaskInvalid = uint32_t(-1);

/// Returns true if the given task index or id is invalid.
///
inline bool VdfScheduleTaskIsInvalid(uint32_t task) {
    return task == VdfScheduleTaskInvalid;
}

/// A structure describing a range of task ids, beginning at taskId and
/// ending at taskId + taskNum.
///
struct VdfScheduleNodeTasks {
    VdfScheduleTaskId taskId;
    VdfScheduleTaskNum taskNum;
};

/// A bit field with additional flags to further describe a
/// VdfScheduleComputeTask.
///
struct VdfScheduleComputeTaskFlags {
    bool isAffective : 1;   // Is this compute task affective?
    bool hasKeep : 1;       // Does this compute task keep any data?
};

/// A VdfScheduleComputeTask represents a unit of computation for the parallel
/// evaluation engine. Every scheduled node has at least one of these tasks.
///
struct VdfScheduleComputeTask {
    // The index into the VdfScheduleNodeInvocation array. This is an invalid
    // index for nodes with only one compute task.
    VdfScheduleTaskIndex invocationIndex;

    // The corresponding inputs task. This is an invalid task index for nodes
    // with only one compute task, and without prereqs and reads, which could
    // be run in parallel with read/writes.
    VdfScheduleTaskIndex inputsTaskIndex;

    // The corresponding node preparation task. Every scheduled node has at
    // most one of these. The task is shared amongst all invocations of that
    // node. An invalid index denotes that node preparation need not be
    // synchronized.
    VdfScheduleTaskIndex prepTaskIndex;

    // Index into the input dependencies array. The first required input
    // dependency, i.e. read/write or read not dependent on prereqs.
    VdfScheduleTaskIndex requiredsIndex;

    // The number of required input dependencies.
    VdfScheduleTaskNum requiredsNum;

    // Additional bit flags.
    VdfScheduleComputeTaskFlags flags;
};

/// Further describes every invocation of a single node. Note, that nodes with
/// invocations always have exactly one output, and the data in this structure
/// is relevant to that single output.
///
struct VdfScheduleNodeInvocation {
    VdfMask requestMask;
    VdfMask affectsMask;
    VdfMask keepMask;
};

/// Structure describing an additional task used to run prereqs and reads
/// concurrently with read/write input dependencies.
///
struct VdfScheduleInputsTask {
    VdfScheduleTaskIndex inputDepIndex;
    VdfScheduleTaskNum prereqsNum;
    VdfScheduleTaskNum optionalsNum;
};

/// A sequential index assigned to the unique output and mask combination of a
/// VdfScheduleInputDependency instance. This index aids in effectively
/// de-duplicating individual input dependencies referring to the same output
/// and mask.
///
typedef uint32_t VdfScheduleInputDependencyUniqueIndex;

/// Describes a single input dependency, i.e. the output and mask to be used
/// to check for a cache hit, as well as the compute task id and number of
/// compute tasks (or a single keep task) to invoke on cache misses.
///
struct VdfScheduleInputDependency {
    // A unique index for the output and mask combination of this dependency.
    VdfScheduleInputDependencyUniqueIndex uniqueIndex;

    // The requested output at the source end of the input dependency.
    const VdfOutput &output;

    // The requested mask at the source end of the input dependency.
    VdfMask mask; 

    // The compute task id of the first task to be invoked to fulfill this
    // input dependency. Note, this can be a compute task id, or a keep task
    // index. If this is a keep task index, computeTaskNum shall be set to 0.
    VdfScheduleTaskId computeOrKeepTaskId;

    // The number of compute task ids to be invoked to fulfill this input
    // dependency. If this is 0, the input dependency is for a keep task.
    VdfScheduleTaskNum computeTaskNum;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif