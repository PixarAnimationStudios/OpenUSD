//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Diagnostic.cpp
//


#include <Metal/Metal.h>

#include "pxr/imaging/hgiMetal/diagnostic.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIMETAL_DEBUG, 0,
                      "Enable Metal debugging for HgiMetal");

TF_DEBUG_CODES(
    HGIMETAL_DEBUG_ERROR_STACKTRACE
);

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HGIMETAL_DEBUG_ERROR_STACKTRACE,
        "HgiMetal dump stack trace on Metal error");
}

bool
HgiMetalDebugEnabled()
{
    static bool _v = TfGetEnvSetting(HGIMETAL_DEBUG) == 1;
    return _v;
}

void
HgiMetalPostPendingMetalErrors(std::string const & where)
{
}

void
HgiMetalSetupMetalDebug()
{
    if (!HgiMetalDebugEnabled()) return;
}

PXR_NAMESPACE_CLOSE_SCOPE

