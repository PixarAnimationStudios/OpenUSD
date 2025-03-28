//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

