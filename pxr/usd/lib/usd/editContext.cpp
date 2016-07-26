//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/stage.h"

UsdEditContext::UsdEditContext(const UsdStagePtr &stage)
    : _stage(stage)
    , _originalEditTarget(stage->GetEditTarget())
{
}

UsdEditContext::UsdEditContext(const UsdStagePtr &stage,
                               const UsdEditTarget &editTarget)
    : _stage(stage)
    , _originalEditTarget(stage->GetEditTarget())
{
    // Do not check validity of EditTarget: stage will do that and
    // issue an error if invalid.  We DO NOT want people authoring
    // into places they did not expect to be authoring.
    _stage->SetEditTarget(editTarget);
}

UsdEditContext::UsdEditContext(
    const std::pair<UsdStagePtr, UsdEditTarget> &stageTarget)
    : _stage(stageTarget.first)
    , _originalEditTarget(stageTarget.first->GetEditTarget())
{
    // See comment above
    _stage->SetEditTarget(stageTarget.second);
}

UsdEditContext::~UsdEditContext()
{
    // Stage should never allow an invalid EditTarget to be set...
    if (_stage and TF_VERIFY(_originalEditTarget.IsValid()))
        _stage->SetEditTarget(_originalEditTarget);
}

////////////////////////////////////////////////////////////////////////
// UsdPyEditContext

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
