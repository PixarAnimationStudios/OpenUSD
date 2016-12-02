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
#include "pxr/usdImaging/usdImaging/debugCodes.h"

#include "pxr/base/tf/registryManager.h"

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_COLLECTIONS, "Report collection queries");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_CHANGES, "Report change processing events");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_UPDATES, "Report non-authored, time-varying data changes");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_PLUGINS, "Report plugin status messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_SHADERS, "Report shader status messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_TEXTURES, "Report texture status messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_SELECTION, "Report selection messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_INSTANCER, "Report instancer messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_POINT_INSTANCER_PROTO_CREATED,
                                "Report PI prototype stats as they are created");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDIMAGING_POINT_INSTANCER_PROTO_CULLING,
                                "Report PI culling debug info");
}
