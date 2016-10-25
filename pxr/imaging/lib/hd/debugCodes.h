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
#ifndef HD_DEBUGCODES_H
#define HD_DEBUGCODES_H

#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/debug.h"

TF_DEBUG_CODES(

    HD_BUFFER_ARRAY_INFO,
    HD_BUFFER_ARRAY_RANGE_CLEANED,
    HD_CACHE_HITS,
    HD_CACHE_MISSES,
    HD_COLLECTION_CHANGED,
    HD_COUNTER_CHANGED,
    HD_DIRTY_LIST,
    HD_DISABLE_FRUSTUM_CULLING,
    HD_DISABLE_MULTITHREADED_CULLING,
    HD_DISABLE_MULTITHREADED_RPRIM_SYNC,
    HD_DRAW_BATCH,
    HD_DRAWITEM_CLEANED,
    HD_DRAWITEM_DRAWN,
    HD_DRAWITEMS_CULLED,
    HD_DUMP_GLSLFX_CONFIG,
    HD_DUMP_SHADER_SOURCE,
    HD_DUMP_SHADER_BINARY,
    HD_FREEZE_CULL_FRUSTUM,
    HD_INSTANCER_ADDED,
    HD_INSTANCER_CLEANED,
    HD_INSTANCER_REMOVED,
    HD_INSTANCER_UPDATED,
    HD_MDI,
    HD_RENDER_CONTEXT_CAPS,
    HD_RPRIM_ADDED,
    HD_RPRIM_CLEANED,
    HD_RPRIM_REMOVED,
    HD_RPRIM_UPDATED,
    HD_SAFE_MODE,
    HD_SHADER_ADDED,
    HD_SHADER_REMOVED,
    HD_SPRIM_ADDED,
    HD_SPRIM_REMOVED,
    HD_TASK_ADDED,
    HD_TASK_REMOVED,
    HD_TEXTURE_ADDED,
    HD_TEXTURE_REMOVED,
    HD_VARYING_STATE
);

#endif // HD_DEBUGCODES_H
