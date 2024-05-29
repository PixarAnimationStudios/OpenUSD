//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file debugCodes.cpp

#include "pxr/imaging/hgi/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HGI_DEBUG_DEVICE_CAPABILITIES,
        "Hgi report when device capabilities are initialized and dump "
        "contents");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HGI_DEBUG_INSTANCE_CREATION,
        "Hgi report when attempting to create an Hgi instance");
}

PXR_NAMESPACE_CLOSE_SCOPE

