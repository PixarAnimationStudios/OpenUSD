//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_AUTO_APPLY_API_SCHEMAS, 
        "USD API schema auto application details");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_CHANGES, 
        "USD change processing");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_CLIPS, 
        "USD clip details");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_COMPOSITION, 
        "USD composition details");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_INSTANCING, 
        "USD instancing diagnostics");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_PATH_RESOLUTION, 
        "USD path resolution diagnostics");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_PAYLOADS, 
        "USD payload load/unload messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_PRIM_LIFETIMES, 
        "USD prim ctor/dtor messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_SCHEMA_REGISTRATION, 
        "USD schema registration details.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_STAGE_CACHE, 
        "USD stage cache details");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_STAGE_LIFETIMES, 
        "USD stage ctor/dtor messages");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_STAGE_OPEN, 
        "USD stage opening details");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_STAGE_INSTANTIATION_TIME, 
        "USD stage instantiation timing");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_VALIDATE_VARIABILITY, 
        "USD attribute variability validation");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USD_VALUE_RESOLUTION, 
        "USD trace of layers inspected as values are resolved");
}

PXR_NAMESPACE_CLOSE_SCOPE

