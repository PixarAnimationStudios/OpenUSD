//
// Unlicensed 2022 benmalartre
//
#include "pxr/pxr.h"
#include "pxr/usd/exec/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(EXEC_TYPE_CONFORMANCE, "Diagnostcs from parsing "
            "and conforming default values for Exec and Sdf type conformance");
}

PXR_NAMESPACE_CLOSE_SCOPE

