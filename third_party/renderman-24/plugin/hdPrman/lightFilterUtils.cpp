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

#include "hdPrman/lightFilterUtils.h"

#include "hdPrman/renderParam.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/light.h"
#include "hdPrman/material.h"
#include "hdPrman/rixStrings.h"

#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

void HdPrmanLightFilterGenerateCoordSysAndLinks(
    riley::ShadingNode *filter,
    const SdfPath &filterPath,
    std::vector<riley::CoordinateSystemId> *coordsysIds,
    std::vector<TfToken> *filterLinks,
    HdSceneDelegate *sceneDelegate,
    HdPrman_RenderParam *renderParam,
    riley::Riley *riley)
{
    // Sample filter transform
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> xf;
    sceneDelegate->SampleTransform(filterPath, &xf);
    TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> 
        xf_rt_values(xf.count);
    for (size_t i=0; i < xf.count; ++i) {
        xf_rt_values[i] = HdPrman_GfMatrixToRtMatrix(xf.values[i]);
    }
    const riley::Transform xform = {
        unsigned(xf.count), xf_rt_values.data(), xf.times.data()};

    // To ensure the coordsys name is unique, use the full filter path.
    RtUString coordsysName = RtUString(filterPath.GetText());

    RtParamList attrs;
    attrs.SetString(RixStr.k_name, coordsysName);

    riley::CoordinateSystemId csId = riley->CreateCoordinateSystem(
        riley::UserId(stats::AddDataLocation(filterPath.GetText()).GetValue()),
        xform, attrs);
    (*coordsysIds).push_back(csId);

    // Only certain light filters require a coordsys, but we do not
    // know which, here, so we provide it in all cases.
    filter->params.SetString(RtUString("coordsys"), coordsysName);

    // Light filter linking
    VtValue val = sceneDelegate->GetLightParamValue(filterPath,
                                    HdTokens->lightFilterLink);
    TfToken lightFilterLink = TfToken();
    if (val.IsHolding<TfToken>()) {
        lightFilterLink = val.UncheckedGet<TfToken>();
    }
    
    if (!lightFilterLink.IsEmpty()) {
        renderParam->IncrementLightFilterCount(lightFilterLink);
        (*filterLinks).push_back(lightFilterLink);
        // For light filters to link geometry, the light filters must
        // be assigned a grouping membership, and the
        // geometry must subscribe to that grouping.
        filter->params.SetString(RtUString("linkingGroups"),
                            RtUString(lightFilterLink.GetText()));
        TF_DEBUG(HDPRMAN_LIGHT_LINKING)
            .Msg("HdPrman: Light filter <%s> linkingGroups \"%s\"\n",
                    filterPath.GetText(), lightFilterLink.GetText());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
