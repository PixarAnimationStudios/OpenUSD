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
#ifndef USDMAYA_NOTICE_H
#define USDMAYA_NOTICE_H

/// \file usdMaya/notice.h

#include "pxr/base/tf/notice.h"

#include "usdMaya/api.h"

#include <maya/MMessage.h>
#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Notice sent when the Maya scene resets, either by opening a new scene or
/// switching to a new scene.
/// It is *very important* that you call InstallListener() during plugin
/// initialization and removeListener() during plugin uninitialization.
class UsdMayaSceneResetNotice : public TfNotice
{
public:
    PXRUSDMAYA_API
    UsdMayaSceneResetNotice();

    /// Registers the proper Maya callbacks for recognizing stage resets.
    PXRUSDMAYA_API
    static void InstallListener();

    /// Removes any Maya callbacks for recognizing stage resets.
    PXRUSDMAYA_API
    static void RemoveListener();

private:
    static MCallbackId _afterNewCallbackId;
    static MCallbackId _beforeFileReadCallbackId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
