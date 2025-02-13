//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdr/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(SDR_DISCOVERY, "Diagnostics from discovering "
        "nodes for Shader Node Definition Registry");
    TF_DEBUG_ENVIRONMENT_SYMBOL(SDR_PARSING, "Diagnostics from parsing nodes "
        "for Shader Node Definition Registry");
    TF_DEBUG_ENVIRONMENT_SYMBOL(SDR_INFO, "Advisory information for Shader "
        "Node Definition Registry");
    TF_DEBUG_ENVIRONMENT_SYMBOL(SDR_STATS, "Statistics for registries derived "
        "from SdrRegistry");
    TF_DEBUG_ENVIRONMENT_SYMBOL(SDR_DEBUG, "Advanced debugging for Shader "
        "Node Definition Registry");
    TF_DEBUG_ENVIRONMENT_SYMBOL(SDR_TYPE_CONFORMANCE, "Diagnostics from "
        "parsing and conforming default values for Sdr and Sdf type "
        "conformance");
}

PXR_NAMESPACE_CLOSE_SCOPE

