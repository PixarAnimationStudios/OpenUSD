//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usdImaging/usdExecImaging/debugCodes.h"

#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug) {
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDEXECIMAGING_REQUEST,
        "show UsdExecImaging_Request API calls");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDEXECIMAGING_GRAPH_AFTER_REBUILD,
        "write dot graph to file after rebuilding the exec request");
}

PXR_NAMESPACE_CLOSE_SCOPE