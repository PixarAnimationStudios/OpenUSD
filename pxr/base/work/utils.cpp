//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/work/utils.h"
#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(WORK_SYNCHRONIZE_ASYNC_DESTROY_CALLS, false,
                      "Make WorkSwapDestroyAsync and WorkMoveDestroyAsync "
                      "wait for destruction completion rather than destroying "
                      "asynchronously");

bool
Work_ShouldSynchronizeAsyncDestroyCalls()
{
    return TfGetEnvSetting(WORK_SYNCHRONIZE_ASYNC_DESTROY_CALLS);
}

PXR_NAMESPACE_CLOSE_SCOPE
