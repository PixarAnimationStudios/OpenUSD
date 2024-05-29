//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
//
// Shared library with a registry function.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/registryManager.h"
#include <cstdio>

PXR_NAMESPACE_USING_DIRECTIVE

class Tf_TestRegistryFunctionPlugin;

TF_REGISTRY_FUNCTION(Tf_TestRegistryFunctionPlugin)
{
    printf("* Running Tf_TestRegistryFunctionPlugin registry function from "
           __FILE__ "\n");
}
