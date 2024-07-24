//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usd/pyEditContext.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdPyEditContext::UsdPyEditContext(
    const std::pair<UsdStagePtr, UsdEditTarget> &stageTarget)
    : _stage(stageTarget.first)
    , _editTarget(stageTarget.second)
{
}

UsdPyEditContext::UsdPyEditContext(
    const UsdStagePtr &stage, const UsdEditTarget &editTarget)
    : _stage(stage)
    , _editTarget(editTarget)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
