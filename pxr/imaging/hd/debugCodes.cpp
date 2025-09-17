//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_BPRIM_ADDED,
        "Report when bprims are added");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_BPRIM_REMOVED,
        "Report when bprims are removed")

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_BUFFER_ARRAY_INFO,
        "Report detail info of HdBufferArrays");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_BUFFER_ARRAY_RANGE_CLEANED,
        "Report when bufferArrayRange is cleaned");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_CACHE_HITS, "Report every cache hit");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_CACHE_MISSES, "Report every cache miss");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_COUNTER_CHANGED,
        "Report values when counters change");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DIRTY_ALL_COLLECTIONS,
        "Reports diagnostics when all collections are marked dirty");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DIRTY_LIST,
        "Reports dirty list state changes");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DISABLE_MULTITHREADED_RPRIM_SYNC,
        "Run RPrim sync on a single thread");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DRAWITEMS_CULLED,
        "Report the number of draw items culled in each render pass");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_ENGINE_PHASE_INFO,
        "Report the execution phase of the Hydra engine");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_EXT_COMPUTATION_ADDED,
        "Report when ExtComputations are added");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_EXT_COMPUTATION_REMOVED,
        "Report when ExtComputations are removed");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_EXT_COMPUTATION_UPDATED,
        "Report when ExtComputations are updated");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_EXT_COMPUTATION_EXECUTION,
        "Report when ExtComputations are executed");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_FREEZE_CULL_FRUSTUM,
        "Freeze the frustum used for culling at it's current value");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_INSTANCER_ADDED,
        "Report when instancers are added");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_INSTANCER_CLEANED,
        "Report when instancers are fully cleaned");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_INSTANCER_REMOVED,
        "Report when instancers are removed");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_INSTANCER_UPDATED,
        "Report when instancers are updated");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_RENDER_SETTINGS,
        "Report render settings changes");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_RENDERER_PLUGIN,
        "Report debug info on renderer plugins");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_RPRIM_ADDED,
        "Report when rprims are added");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_RPRIM_CLEANED,
        "Report when rprims are fully cleaned");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_RPRIM_REMOVED,
        "Report when rprims are removed");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_RPRIM_UPDATED,
        "Report when rprims are updated");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_SAFE_MODE,
        "Enable additional security checks");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_SELECTION_UPDATE,
        "Report when selection is updated");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_SHARED_EXT_COMPUTATION_DATA,
        "Report info related to deduplication of ext computation data buffers");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_SPRIM_ADDED,
        "Report when sprims are added");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_SPRIM_REMOVED,
        "Report when sprims are removed");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_SYNC_ALL,
        "Report debugging info for the sync all algorithm.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_TASK_ADDED,
        "Report when tasks are added");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_TASK_REMOVED,
        "Report when tasks are removed");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_VARYING_STATE,
        "Reports state tracking of varying state");
}

PXR_NAMESPACE_CLOSE_SCOPE

