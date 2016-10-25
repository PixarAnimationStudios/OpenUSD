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
#include "pxr/imaging/hd/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_BUFFER_ARRAY_INFO, "Report detail info of HdBufferArrays");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_BUFFER_ARRAY_RANGE_CLEANED, "Report when bufferArrayRange is cleaned");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_CACHE_HITS, "Report every cache hit");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_CACHE_MISSES, "Report every cache miss");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_COLLECTION_CHANGED, "Report when cached collections change");
    
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_COUNTER_CHANGED, "Report values when counters change");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DIRTY_LIST, "Reports dirty list state changes");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DISABLE_FRUSTUM_CULLING, "Disable view frustum culling");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DISABLE_MULTITHREADED_CULLING,
                                "Force the use of the single threaded version of frustum culling");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DISABLE_MULTITHREADED_RPRIM_SYNC,
                                "Run RPrim sync on a single thread");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DRAW_BATCH, "Reports diagnostics for draw batches");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DRAWITEM_DRAWN, "Report each draw item as it is drawn");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DRAWITEM_CLEANED, "Report when draw items are cleaned");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DRAWITEMS_CULLED, "Report the number of draw items culled in each render pass");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DUMP_GLSLFX_CONFIG, "Print composed GLSLFX configuration");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DUMP_SHADER_SOURCE, "Print generated shader code");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_DUMP_SHADER_BINARY, "Write out compiled GLSL shader binary");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_FREEZE_CULL_FRUSTUM,
                                "Freeze the frustum used for culling at it's current value");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_INSTANCER_ADDED, "Report when instancers are added");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_INSTANCER_CLEANED, "Report when instancers are fully cleaned");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_INSTANCER_REMOVED, "Report when instancers are removed");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_INSTANCER_UPDATED, "Report when instancers are updated");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_MDI, "Report info related to multi-draw-indirect batches");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_RENDER_CONTEXT_CAPS,
                                "Report when render context caps are initialized");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_RPRIM_ADDED, "Report when rprims are added");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_RPRIM_CLEANED, "Report when rprims are fully cleaned");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_RPRIM_REMOVED, "Report when rprims are removed");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_RPRIM_UPDATED, "Report when rprims are updated");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_SAFE_MODE, "Enable additional security checks");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_SPRIM_ADDED, "Report when sprims are added");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_SPRIM_REMOVED, "Report when sprims are removed")
;
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_SHADER_ADDED, "Report when shaders are added");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_SHADER_REMOVED, "Report when shaders are removed");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_TASK_ADDED, "Report when tasks are added");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_TASK_REMOVED, "Report when tasks are removed");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_TEXTURE_ADDED, "Report when textures are added");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_TEXTURE_REMOVED, "Report when textures are removed");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_VARYING_STATE, "Reports state tracking of varying state");
}
