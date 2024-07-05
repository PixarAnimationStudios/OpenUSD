//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdMtlx/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDMTLX_VERSION_UPGRADE,
        "Write the MaterialX documents to disk, before and after the version upgrade");
}

PXR_NAMESPACE_CLOSE_SCOPE
