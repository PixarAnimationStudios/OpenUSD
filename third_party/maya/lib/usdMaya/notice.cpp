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
#include "usdMaya/notice.h"

#include "pxr/base/tf/instantiateType.h"

#include <maya/MFileIO.h>
#include <maya/MSceneMessage.h>

PXR_NAMESPACE_OPEN_SCOPE


namespace {

static
void
_OnMayaNewOrOpenSceneCallback(void* /*clientData*/)
{
    // kBeforeFileRead messages are emitted when importing/referencing files,
    // which we don't consider a "scene reset".
    if (MFileIO::isImportingFile() || MFileIO::isReferencingFile()) {
        return;
    }

    UsdMayaSceneResetNotice().Send();
}

} // anonymous namespace

TF_INSTANTIATE_TYPE(UsdMayaSceneResetNotice,
                    TfType::CONCRETE, TF_1_PARENT(TfNotice));

MCallbackId UsdMayaSceneResetNotice::_afterNewCallbackId = 0;
MCallbackId UsdMayaSceneResetNotice::_beforeFileReadCallbackId = 0;

UsdMayaSceneResetNotice::UsdMayaSceneResetNotice()
{
}

/* static */
void
UsdMayaSceneResetNotice::InstallListener()
{
    // Send scene reset notices when changing scenes (either by switching
    // to a new empty scene or by opening a different scene). We do not listen
    // for kSceneUpdate messages since those are also emitted after a SaveAs
    // operation, which we don't consider a "scene reset".
    // Note also that we listen for kBeforeFileRead messages because those fire
    // at the right time (after any existing scene has been closed but before
    // the new scene has been opened). However, they are also emitted when a
    // file is imported or referenced, so we check for that and do *not* send
    // a scene reset notice.
    if (_afterNewCallbackId == 0) {
        _afterNewCallbackId =
            MSceneMessage::addCallback(MSceneMessage::kAfterNew,
                                       _OnMayaNewOrOpenSceneCallback);
    }

    if (_beforeFileReadCallbackId == 0) {
        _beforeFileReadCallbackId =
            MSceneMessage::addCallback(MSceneMessage::kBeforeFileRead,
                                       _OnMayaNewOrOpenSceneCallback);
    }
}

/* static */
void
UsdMayaSceneResetNotice::RemoveListener()
{
    if (_afterNewCallbackId != 0) {
        MMessage::removeCallback(_afterNewCallbackId);
    }

    if (_beforeFileReadCallbackId == 0) {
        MMessage::removeCallback(_beforeFileReadCallbackId);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
