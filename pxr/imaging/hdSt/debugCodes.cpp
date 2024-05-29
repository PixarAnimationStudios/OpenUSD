//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
     TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DRAW,
        "Reports diagnostics for drawing");
     TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DRAW_BATCH,
        "Reports diagnostics for draw batches");
     TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_FORCE_DRAW_BATCH_REBUILD,
        "Forces rebuild of draw batches.");
     TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DRAW_ITEM_GATHER,
        "Reports when draw items are fetched for a render pass.");
     TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DRAWITEMS_CACHE,
        "Reports lookups from the draw items cache.");

     TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DISABLE_FRUSTUM_CULLING,
         "Disable view frustum culling");
     TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DISABLE_MULTITHREADED_CULLING,
         "Force the use of the single threaded version of frustum culling");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DUMP_GLSLFX_CONFIG,
        "Print composed GLSLFX configuration");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DUMP_FAILING_SHADER_SOURCE,
        "Print generated shader source code for shaders that fail compilation");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DUMP_FAILING_SHADER_SOURCEFILE,
        "Write out generated shader source code to files for shaders that "
        "fail compilation");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DUMP_SHADER_SOURCE,
        "Print generated shader source code");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DUMP_SHADER_SOURCEFILE,
        "Write out generated shader source code to files");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_LOG_COMPUTE_SHADER_PROGRAM_HITS,
        "Log compute shader program hits in the resource registry.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_LOG_COMPUTE_SHADER_PROGRAM_MISSES,
        "Log compute shader program misses in the resource registry.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_LOG_DRAWING_SHADER_PROGRAM_HITS,
        "Log drawing shader program hits in the resource registry. "
        "Use env var 'HDST_DEBUG_SHADER_PROGRAM_FOR_PRIM' to limit logging to "
        "a subset of prims.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_LOG_DRAWING_SHADER_PROGRAM_MISSES,
        "Log drawing shader program misses in the resource registry."
        "Use env var 'HDST_DEBUG_SHADER_PROGRAM_FOR_PRIM' to limit logging to "
        "a subset of prims.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_MATERIAL_ADDED,
        "Report when a material is added");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_MATERIAL_REMOVED,
        "Report when a material is removed");
}

PXR_NAMESPACE_CLOSE_SCOPE

