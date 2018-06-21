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
#include "pxr/pxr.h"
#include "pxr/usd/ndr/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(NDR_DISCOVERY, "Diagnostics from discovering nodes for Node Definition Registry");
    TF_DEBUG_ENVIRONMENT_SYMBOL(NDR_PARSING, "Diagnostics from parsing nodes for Node Definition Registry");
    TF_DEBUG_ENVIRONMENT_SYMBOL(NDR_INFO, "Advisory information for Node Definition Registry");
    TF_DEBUG_ENVIRONMENT_SYMBOL(NDR_STATS, "Statistics for registries derived from NdrRegistry");
    TF_DEBUG_ENVIRONMENT_SYMBOL(NDR_DEBUG, "Advanced debugging for Node Definition Registry");
}

PXR_NAMESPACE_CLOSE_SCOPE

