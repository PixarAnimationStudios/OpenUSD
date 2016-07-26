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
#include "pxr/usd/usd/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_CHANGES, "Usd change processing");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_CLIPS, "Usd clip details");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_COMPOSITION, "Usd composition details");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_DATA_BD, "Usd BD file format traces");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_DATA_BD_TRY, "Usd BD call traces. Prints names, errors and results.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_INSTANCING, "Usd instancing diagnostics");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_PATH_RESOLUTION, "Usd path resolution diagnostics");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_PAYLOADS, "Usd payload load/unload messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_PRIM_LIFETIMES, "Usd prim ctor/dtor messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_STAGE_CACHE, "Usd stage cache details");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_STAGE_LIFETIMES, "Usd stage ctor/dtor messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_STAGE_OPEN, "Usd stage opening details");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_STAGE_INSTANTIATION_TIME, "Usd stage instantiation timing");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_VALIDATE_VARIABILITY, "Usd attribute variability validation");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_VALUE_RESOLUTION, "Usd trace of layers inspected as values are resolved");
}
