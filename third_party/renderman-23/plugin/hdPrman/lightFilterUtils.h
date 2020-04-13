//
// Copyright 2019 Pixar
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
#ifndef EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_CONVERSIONS_H
#define EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_CONVERSIONS_H

#include "hdPrman/light.h"
#include "hdPrman/context.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hf/diagnostic.h"

#include "RixShadingUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

bool HdPrmanLightFilterPopulateParams(
    riley::ShadingNode *filter,
    SdfPath &filterPath,
    TfToken filterType,
    std::vector<riley::CoordinateSystemId> *coordsysIds,
    std::vector<TfToken> *filterLinks,
    HdSceneDelegate *sceneDelegate,
    HdPrman_Context *context,
    riley::Riley *riley,
    RtUString lightTypeName);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
