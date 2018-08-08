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
#include "pxrUsdMayaGL/debugCodes.h"

#include "pxr/pxr.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"


PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYAGL_BATCHED_DRAWING,
        "Prints out batched drawing event info.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYAGL_BATCHED_SELECTION,
        "Prints out batched selection event info.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING,
        "Reports on changes in the sets of shape adapters registered with the "
        "batch renderer.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE,
        "Report Maya Hydra shape adapter lifecycle events.");
}


PXR_NAMESPACE_CLOSE_SCOPE
