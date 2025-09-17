//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/debugCodes.h"

#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(EXEC_REQUEST_EXPIRATION,
        "Report when request indices are expired.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(EXEC_REQUEST_INVALIDATION,
        "Report when requests are notified of invalidation");
}

PXR_NAMESPACE_CLOSE_SCOPE
