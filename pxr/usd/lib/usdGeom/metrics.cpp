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
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

// XXX Backwards-compatibility code for transitioning from customData["zUp"]
// XXX encoding to newer, permanent upAxis metadata.

TF_DEFINE_ENV_SETTING(
    USD_READ_ZUP_FOR_UP_AXIS, true, 
    "Whether UsdGeomGetStageUpAxis() will consult old-style customData[\"zUp\"]"
    " if upAxis metadata is unauthored.  Will eventually be retired - provided"
    " for backwards compatibility while transitioning.");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (zUp)
    (UsdGeomMetrics)
);

static
TfToken
_GetUpAxisFromZUp(const UsdStageWeakPtr &stage)
{
    TF_FOR_ALL(prim, stage->GetPseudoRoot().
                            GetFilteredChildren(UsdPrimIsDefined and
                                                not UsdPrimIsAbstract)){
        VtValue isZup = prim->GetCustomDataByKey(_tokens->zUp);
        if (isZup.IsEmpty()){
            continue;
        }
        else if (isZup.IsHolding<bool>()){
            return isZup.Get<bool>() ? UsdGeomTokens->z : UsdGeomTokens->y;
        }
        else {
            TF_WARN("Found non-boolean 'zUp' customData in UsdStage root at "
                    "layer '%s', primPath <%s>.",
                    stage->GetRootLayer()->GetIdentifier().c_str(),
                    prim->GetPath().GetText());
            return TfToken();
        }
    }

    return TfToken();
}

TfToken 
UsdGeomGetStageUpAxis(const UsdStageWeakPtr &stage)
{
    if (not TF_VERIFY(stage)){
        return TfToken();
    }

    // Even after we remove backwards compatibility support, we will still 
    // need to know if upAxis has been authored, since we want to provide
    // a potentially different fallback than that of the SdfSchema.
    if (stage->HasAuthoredMetadata(UsdGeomTokens->upAxis)){
        TfToken axis;
        stage->GetMetadata(UsdGeomTokens->upAxis, &axis);
        return axis;
    }

    if (TfGetEnvSetting(USD_READ_ZUP_FOR_UP_AXIS)){
        TfToken upAxis = _GetUpAxisFromZUp(stage);
        if (not upAxis.IsEmpty())
            return upAxis;
    }
    
    return UsdGeomGetFallbackUpAxis();
}

bool 
UsdGeomSetStageUpAxis(const UsdStageWeakPtr &stage, const TfToken &axis)
{
    if (not TF_VERIFY(stage)){
        return false;
    }
    if (axis != UsdGeomTokens->y and axis != UsdGeomTokens->z){
        TF_CODING_ERROR("UsdStage upAxis can only be set to \"Y\" or \"Z\", "
                        "not attempted \"%s\" on stage %s.",
                        axis.GetText(),
                        stage->GetRootLayer()->GetIdentifier().c_str());
        return false;
    }

    return stage->SetMetadata(UsdGeomTokens->upAxis, VtValue(axis));
}

TF_MAKE_STATIC_DATA(TfToken, _fallbackUpAxis)
{
    TfToken upAxis;
    std::string definingPluginName;
    TfToken schemaFallback = 
        SdfSchema::GetInstance().GetFallback(UsdGeomTokens->upAxis)
        .Get<TfToken>();
    
    PlugPluginPtrVector plugs = PlugRegistry::GetInstance().GetAllPlugins();
    TF_FOR_ALL(plugIter, plugs) {
        PlugPluginPtr plug = *plugIter;
        JsObject metadata = plug->GetMetadata();
        JsValue metricsDictValue;
        if (TfMapLookup(metadata, _tokens->UsdGeomMetrics, &metricsDictValue)){
            if (not metricsDictValue.Is<JsObject>()) {
                TF_CODING_ERROR(
                        "%s[%s] was not a dictionary in plugInfo.json file.",
                        plug->GetName().c_str(), 
                        _tokens->UsdGeomMetrics.GetText());
                continue;
            }

            JsObject metricsDict =
                metricsDictValue.Get<JsObject>();
            JsValue upAxisValue;
            if (TfMapLookup(metricsDict, 
                            UsdGeomTokens->upAxis,
                            &upAxisValue)) {
                if (not upAxisValue.Is<std::string>()) {
                    TF_CODING_ERROR(
                        "%s[%s][%s] was not a string.",
                        plug->GetName().c_str(),
                        _tokens->UsdGeomMetrics.GetText(),
                        UsdGeomTokens->upAxis.GetText());
                    continue;
                }
                
                std::string axisStr = upAxisValue.Get<std::string>();
                TfToken axisToken;
                if (axisStr == "Y"){
                    axisToken = UsdGeomTokens->y;
                } else if (axisStr == "Z") {
                    axisToken = UsdGeomTokens->z;
                } else {
                    TF_CODING_ERROR(
                        "%s[%s][%s] had value \"%s\", but only \"Y\" and"
                        " \"Z\" are allowed.",
                        plug->GetName().c_str(),
                        _tokens->UsdGeomMetrics.GetText(),
                        UsdGeomTokens->upAxis.GetText(),
                        axisStr.c_str());
                    continue;
                }
                
                if (not upAxis.IsEmpty() and upAxis != axisToken) {
                    TF_CODING_ERROR("Plugins %s and %s provided different"
                                    " fallback values for %s.  Ignoring all"
                                    " plugins and using schema fallback of"
                                    " \"%s\"",
                                    definingPluginName.c_str(),
                                    plug->GetName().c_str(),
                                    UsdGeomTokens->upAxis.GetText(),
                                    schemaFallback.GetText());
                    *_fallbackUpAxis = schemaFallback;
                    return;
                } else if (upAxis.IsEmpty()) {
                    definingPluginName = plug->GetName();
                    upAxis = axisToken;
                }
            }
            
        }
    }

    *_fallbackUpAxis = upAxis.IsEmpty() ? schemaFallback : upAxis;
}

TfToken
UsdGeomGetFallbackUpAxis()
{
    return *_fallbackUpAxis;
}

