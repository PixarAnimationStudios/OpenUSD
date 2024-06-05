//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug) {
    TF_DEBUG_ENVIRONMENT_SYMBOL(TF_SCRIPT_MODULE_LOADER,
                                "show script module loading activity");
    TF_DEBUG_ENVIRONMENT_SYMBOL(TF_TYPE_REGISTRY, "show changes to the TfType registry");
    TF_DEBUG_ENVIRONMENT_SYMBOL(TF_ATTACH_DEBUGGER_ON_ERROR,
        "attach/stop in a debugger for all errors");
    TF_DEBUG_ENVIRONMENT_SYMBOL(TF_ATTACH_DEBUGGER_ON_FATAL_ERROR,
        "attach/stop in a debugger for fatal errors");
    TF_DEBUG_ENVIRONMENT_SYMBOL(TF_ATTACH_DEBUGGER_ON_WARNING,
        "attach/stop in a debugger for all warnings");
}

PXR_NAMESPACE_CLOSE_SCOPE
