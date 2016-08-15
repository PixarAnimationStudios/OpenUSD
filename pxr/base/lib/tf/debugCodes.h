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
#ifndef TF_TFDEBUGCODES_H
#define TF_TFDEBUGCODES_H

#include "pxr/base/tf/debug.h"

//
// Note that this is a private header file to lib/tf.
// If you add a new entry here, please be sure to update DebugCodes.cpp as well.
//

TF_DEBUG_CODES(

    TF_DISCOVERY_TERSE,     // these are special in that they don't have a 
    TF_DISCOVERY_DETAILED,  // corresponding entry in debugCodes.cpp; see 
    TF_DEBUG_REGISTRY,      // registryManager.cpp and debug.cpp for the reason.
    TF_DLOPEN,
    TF_DLCLOSE,

    TF_SCRIPT_MODULE_LOADER,

    TF_TYPE_REGISTRY,

    TF_ATTACH_DEBUGGER_ON_ERROR,
    TF_ATTACH_DEBUGGER_ON_FATAL_ERROR,
    TF_ATTACH_DEBUGGER_ON_WARNING

);

#endif
