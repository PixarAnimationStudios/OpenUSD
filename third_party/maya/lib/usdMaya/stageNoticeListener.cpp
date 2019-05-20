//
// Copyright 2018 Pixar
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
#include "usdMaya/stageNoticeListener.h"

#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/weakBase.h"

#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/stage.h"


PXR_NAMESPACE_OPEN_SCOPE


UsdMayaStageNoticeListener::UsdMayaStageNoticeListener() : TfWeakBase()
{
}

/* virtual */
UsdMayaStageNoticeListener::~UsdMayaStageNoticeListener()
{
    if (_stageContentsChangedKey.IsValid()) {
        TfNotice::Revoke(_stageContentsChangedKey);
    }
}

void
UsdMayaStageNoticeListener::SetStage(const UsdStageWeakPtr& stage)
{
    _stage = stage;

    _UpdateStageContentsChangedRegistration();
}

void
UsdMayaStageNoticeListener::SetStageContentsChangedCallback(
        const StageContentsChangedCallback& callback)
{
    _stageContentsChangedCallback = callback;

    _UpdateStageContentsChangedRegistration();
}

void
UsdMayaStageNoticeListener::_UpdateStageContentsChangedRegistration()
{
    if (_stage && _stageContentsChangedCallback) {
        // Register for notices if we're not already listening.
        if (!_stageContentsChangedKey.IsValid()) {
            _stageContentsChangedKey =
                TfNotice::Register(
                    TfCreateWeakPtr(this),
                    &UsdMayaStageNoticeListener::_OnStageContentsChanged);
        }
    } else {
        // Either the stage or the callback is invalid, so stop listening for
        // notices.
        if (_stageContentsChangedKey.IsValid()) {
            TfNotice::Revoke(_stageContentsChangedKey);
        }
    }
}

void
UsdMayaStageNoticeListener::_OnStageContentsChanged(
        const UsdNotice::StageContentsChanged& notice) const
{
    if (notice.GetStage() == _stage && _stageContentsChangedCallback) {
        _stageContentsChangedCallback(notice);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
