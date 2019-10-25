//
// Copyright 2018 Pixar
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
#include "pxr/imaging/hdSt/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
     TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DRAW_BATCH,
        "Reports diagnostics for draw batches");
     TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_FORCE_DRAW_BATCH_REBUILD,
        "Forces rebuild of draw batches.");

     TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DISABLE_FRUSTUM_CULLING,
         "Disable view frustum culling");
     TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DISABLE_MULTITHREADED_CULLING,
         "Force the use of the single threaded version of frustum culling");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DUMP_GLSLFX_CONFIG,
        "Print composed GLSLFX configuration");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DUMP_FAILING_SHADER_SOURCE,
        "Print generated shader source code for shaders that fail compilation");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DUMP_SHADER_SOURCE,
        "Print generated shader source code");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DUMP_SHADER_SOURCEFILE,
        "Write out generated shader source code to files");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_DUMP_SHADER_BINARY,
        "Write out compiled GLSL shader binary to files");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_MATERIAL_ADDED,
        "Report when a material is added");

    TF_DEBUG_ENVIRONMENT_SYMBOL(HDST_MATERIAL_REMOVED,
        "Report when a material is removed");
}

PXR_NAMESPACE_CLOSE_SCOPE

