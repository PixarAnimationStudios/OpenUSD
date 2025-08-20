//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiVulkan/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
     TF_DEBUG_ENVIRONMENT_SYMBOL(HGIVULKAN_DUMP_DEVICE_MEMORY_PROPERTIES,
        "Dump the output of vkGetPhysicalDeviceMemoryProperties");
}


PXR_NAMESPACE_CLOSE_SCOPE
