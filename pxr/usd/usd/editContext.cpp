//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/stage.h"

PXR_NAMESPACE_OPEN_SCOPE


UsdEditContext::UsdEditContext(const UsdStagePtr &stage)
    : _stage(stage)
    , _originalEditTarget(stage ? stage->GetEditTarget() : UsdEditTarget())
{
    if (!_stage) {
        TF_CODING_ERROR("Cannot construct EditContext with invalid stage");
    }
}

UsdEditContext::UsdEditContext(const UsdStagePtr &stage,
                               const UsdEditTarget &editTarget)
    : _stage(stage)
    , _originalEditTarget(stage ? stage->GetEditTarget() : UsdEditTarget())
{
    if (!_stage) {       
        TF_CODING_ERROR("Cannot construct EditContext with invalid stage"); 
    } else {
        // Do not check validity of EditTarget: stage will do that and
        // issue an error if invalid.  We DO NOT want people authoring
        // into places they did not expect to be authoring.
        _stage->SetEditTarget(editTarget);
    }
}

UsdEditContext::UsdEditContext(
    const std::pair<UsdStagePtr, UsdEditTarget> &stageTarget)
    : _stage(stageTarget.first)
    , _originalEditTarget(stageTarget.first ? 
        stageTarget.first->GetEditTarget(): UsdEditTarget())
{
    if (!_stage) {
        TF_CODING_ERROR("Cannot construct EditContext with invalid stage");
    } else {
        // See comment above
        _stage->SetEditTarget(stageTarget.second);
    }
}

UsdEditContext::~UsdEditContext()
{
    // Stage should never allow an invalid EditTarget to be set...
    if (_stage && TF_VERIFY(_originalEditTarget.IsValid()))
        _stage->SetEditTarget(_originalEditTarget);
}

PXR_NAMESPACE_CLOSE_SCOPE

