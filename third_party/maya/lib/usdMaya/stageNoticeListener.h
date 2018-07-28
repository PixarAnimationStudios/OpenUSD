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
#ifndef PXRUSDMAYA_STAGE_NOTICE_LISTENER_H
#define PXRUSDMAYA_STAGE_NOTICE_LISTENER_H

/// \file usdMaya/stageNoticeListener.h

#include "usdMaya/api.h"

#include "pxr/pxr.h"

#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/weakBase.h"

#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/stage.h"

#include <functional>


PXR_NAMESPACE_OPEN_SCOPE


/// A notice listener that can invoke callbacks in response to notices about a
/// specific USD stage.
///
/// For callbacks for a particular notice type to be invoked, the listener must
/// have been populated with a callback for notices of that type *and* a USD
/// stage.
class UsdMayaStageNoticeListener : public TfWeakBase
{
    public:
        PXRUSDMAYA_API
        UsdMayaStageNoticeListener();

        PXRUSDMAYA_API
        virtual ~UsdMayaStageNoticeListener();

        /// Set the USD stage for which this instance will listen for notices.
        PXRUSDMAYA_API
        void SetStage(const UsdStageWeakPtr& stage);

        /// Callback type for StageContentsChanged notices.
        typedef std::function<void (const UsdNotice::StageContentsChanged& notice)>
            StageContentsChangedCallback;

        /// Sets the callback to be invoked when the listener receives a
        /// StageContentsChanged notice.
        PXRUSDMAYA_API
        void SetStageContentsChangedCallback(
                const StageContentsChangedCallback& callback);

    private:
        UsdMayaStageNoticeListener(const UsdMayaStageNoticeListener&);
        UsdMayaStageNoticeListener& operator=(
                const UsdMayaStageNoticeListener&);

        UsdStageWeakPtr _stage;

        /// Handling for UsdNotice::StageContentsChanged.

        TfNotice::Key _stageContentsChangedKey;
        StageContentsChangedCallback _stageContentsChangedCallback;

        void _UpdateStageContentsChangedRegistration();
        void _OnStageContentsChanged(
                const UsdNotice::StageContentsChanged& notice) const;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
