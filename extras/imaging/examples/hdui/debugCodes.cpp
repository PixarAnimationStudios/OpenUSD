//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdui/debugCodes.h"

#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDUI_HSD_FILTER,
        "Report details about the HduiSceneIndexDebuggerWidget's filter "
        "operations");
    
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDUI_HSD_TREE_WIDGET,
        "Report details about the HduiSceneIndexTreeWidget's operations");
}

PXR_NAMESPACE_CLOSE_SCOPE
