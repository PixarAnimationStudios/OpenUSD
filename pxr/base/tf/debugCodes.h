//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_DEBUG_CODES_H
#define PXR_BASE_TF_DEBUG_CODES_H

#include "pxr/pxr.h"
#include "pxr/base/tf/debug.h"

PXR_NAMESPACE_OPEN_SCOPE

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

PXR_NAMESPACE_CLOSE_SCOPE

#endif
