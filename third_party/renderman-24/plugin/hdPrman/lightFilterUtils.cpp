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

#include "hdPrman/context.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/light.h"
#include "hdPrman/material.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"

#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // tokens for RenderMan-specific light filter parameters
    ((analyticApex, "inputs:ri:lightFilter:apex"))
    ((analyticDensityExponent, "inputs:ri:lightFilter:densityPow"))
    ((analyticDensityFarDistance, "inputs:ri:lightFilter:densityFarDist"))
    ((analyticDensityFarValue, "inputs:ri:lightFilter:densityFarVal"))
    ((analyticDensityNearDistance, "inputs:ri:lightFilter:densityNearDist"))
    ((analyticDensityNearValue, "inputs:ri:lightFilter:densityNearVal"))
    ((analyticDirectional, "inputs:ri:lightFilter:directional"))
    ((analyticShearX, "inputs:ri:lightFilter:shearX"))
    ((analyticShearY, "inputs:ri:lightFilter:shearY"))
    ((analyticUseLightDirection, "inputs:ri:lightFilter:useLightDirection"))
    ((barnMode, "inputs:ri:lightFilter:barnMode"))
    ((colorRampColors, "inputs:ri:lightFilter:colorRamp_Colors"))
    ((colorRampInterpolation, "inputs:ri:lightFilter:colorRamp_Interpolation"))
    ((colorRampKnots, "inputs:ri:lightFilter:colorRamp_Knots"))
    ((colorSaturation, "inputs:ri:lightFilter:saturation"))
    ((depth, "inputs:ri:lightFilter:depth"))
    ((edgeScaleBack, "inputs:ri:lightFilter:backEdge"))
    ((edgeScaleBottom, "inputs:ri:lightFilter:bottomEdge"))
    ((edgeScaleFront, "inputs:ri:lightFilter:frontEdge"))
    ((edgeScaleLeft, "inputs:ri:lightFilter:leftEdge"))
    ((edgeScaleRight, "inputs:ri:lightFilter:rightEdge"))
    ((edgeScaleTop, "inputs:ri:lightFilter:topEdge"))
    ((edgeThickness, "inputs:ri:lightFilter:edge"))
    ((falloffFloats, "inputs:ri:lightFilter:falloff_Floats"))
    ((falloffInterpolation, "inputs:ri:lightFilter:falloff_Interpolation"))
    ((falloffKnots, "inputs:ri:lightFilter:falloff_Knots"))
    ((height, "inputs:ri:lightFilter:height"))
    ((preBarnEffect, "inputs:ri:lightFilter:preBarn"))
    ((radius, "inputs:ri:lightFilter:radius"))
    ((refineBack, "inputs:ri:lightFilter:back"))
    ((refineBottom, "inputs:ri:lightFilter:bottom"))
    ((refineFront, "inputs:ri:lightFilter:front"))
    ((refineLeft, "inputs:ri:lightFilter:left"))
    ((refineRight, "inputs:ri:lightFilter:right"))
    ((refineTop, "inputs:ri:lightFilter:top"))
    ((combineMode, "inputs:ri:lightFilter:combineMode"))
    ((density, "inputs:ri:lightFilter:density"))
    ((diffuse, "inputs:ri:lightFilter:diffuse"))
    ((exposure, "inputs:ri:lightFilter:exposure"))
    ((intensity, "inputs:ri:lightFilter:intensity"))
    ((invert, "inputs:ri:lightFilter:invert"))
    ((specular, "inputs:ri:lightFilter:specular"))
    ((scaleDepth, "inputs:ri:lightFilter:scaleDepth"))
    ((scaleHeight, "inputs:ri:lightFilter:scaleHeight"))
    ((scaleWidth, "inputs:ri:lightFilter:scaleWidth"))
    ((width, "inputs:ri:lightFilter:width"))

    (analytic)
    (cone)
    (noEffect)
    (noLight)
    (physical)

    (PxrIntMultLightFilter)
    (PxrBarnLightFilter)
    (PxrRodLightFilter)
);

bool HdPrmanLightFilterPopulateNodesFromLightParams(
    std::vector<riley::ShadingNode> *filterNodes,
    SdfPath &filterPath,
    HdSceneDelegate *sceneDelegate)
{
    HD_TRACE_FUNCTION();

    VtValue tval = sceneDelegate->GetLightParamValue(filterPath,
                                TfToken("lightFilterType"));
    if (!tval.IsHolding<TfToken>()) {
        TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
            .Msg("  filter type unknown\n");
        return false;
    }
    TfToken filterType = tval.UncheckedGet<TfToken>();

    riley::ShadingNode filter({
        riley::ShadingNode::Type::k_LightFilter,
        RtUString(filterType.GetText()),
        RtUString(filterPath.GetName().c_str()),
        RtParamList()});

    VtValue pval;

    pval = sceneDelegate->GetLightParamValue(filterPath, _tokens->intensity);
    if (pval.IsHolding<float>()) {
        float intensity = pval.UncheckedGet<float>();
        TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
            .Msg("      %s %f\n", _tokens->intensity.GetText(), intensity);
        filter.params.SetFloat(RtUString("intensity"), intensity);
    }
    if (filterType == _tokens->PxrIntMultLightFilter) {
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                                 _tokens->exposure);
        if (pval.IsHolding<float>()) {
            float exposure = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", _tokens->exposure.GetText(), 
                        exposure);
            filter.params.SetFloat(RtUString("exposure"), exposure);
        }
    }
    if (filterType != _tokens->PxrIntMultLightFilter) {
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                                 _tokens->density);
        if (pval.IsHolding<float>()) {
            float density = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", _tokens->density.GetText(), density);
            filter.params.SetFloat(RtUString("density"), density);
        }
    }
    pval = sceneDelegate->GetLightParamValue(filterPath, _tokens->invert);
    if (pval.IsHolding<bool>()) {
        bool invert = pval.UncheckedGet<bool>();
        TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
            .Msg("      %s %d\n", _tokens->invert.GetText(), invert);
        filter.params.SetInteger(RtUString("invert"), invert);
    }
    pval = sceneDelegate->GetLightParamValue(filterPath, _tokens->diffuse);
    if (pval.IsHolding<float>()) {
        float diffuse = pval.UncheckedGet<float>();
        TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
            .Msg("      %s %f\n", _tokens->diffuse.GetText(), diffuse);
        filter.params.SetFloat(RtUString("diffuse"), diffuse);
    }
    pval = sceneDelegate->GetLightParamValue(filterPath, _tokens->specular);
    if (pval.IsHolding<float>()) {
        float specular = pval.UncheckedGet<float>();
        TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
            .Msg("      %s %f\n", _tokens->specular.GetText(), specular);
        filter.params.SetFloat(RtUString("specular"), specular);
    }
    pval = sceneDelegate->GetLightParamValue(filterPath,
                                             _tokens->combineMode);
    if (pval.IsHolding<TfToken>()) {
        TfToken combineMode = pval.UncheckedGet<TfToken>();
        TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
            .Msg("      %s %s\n", _tokens->combineMode.GetText(), 
                    combineMode.GetText());
        // XXX -- what to do with this
        //filter.params.SetFloat(RtUString("combineMode"), combineMode);
        // XXX -- what to do with this
    }

    // XX -- not sure how to handle this param
    //filter.params.SetString(RtUString("linkingGroups"), RtUString(""));
    // XX -- not sure how to handle this param

    if (filterType == _tokens->PxrIntMultLightFilter) {
        filter.name = RtUString("PxrIntMultLightFilter");

        // XXX -- note this token is not color:saturation so be specific
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                                 _tokens->colorSaturation);
        if (pval.IsHolding<float>()) {
            float saturation = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", _tokens->colorSaturation.GetText(), 
                        saturation);
            filter.params.SetFloat(RtUString("saturation"), saturation);
        }

    } else if (filterType == _tokens->PxrBarnLightFilter) {
        filter.name = RtUString("PxrBarnLightFilter");

        TfToken barnMode = _tokens->physical;
        pval = sceneDelegate->GetLightParamValue(filterPath, _tokens->barnMode);
        if (pval.IsHolding<TfToken>()) {
            barnMode = pval.UncheckedGet<TfToken>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %s\n", _tokens->barnMode.GetText(), 
                        barnMode.GetText());
            int bm = -1;
            if (barnMode == _tokens->physical)
                bm = 0;
            else if (barnMode == _tokens->analytic)
                bm = 1;
            if (bm >= 0)
                filter.params.SetInteger(RtUString("barnMode"), bm);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath, _tokens->width);
        if (pval.IsHolding<float>()) {
            float width = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", _tokens->width.GetText(), width);
            filter.params.SetFloat(RtUString("width"), width);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath, _tokens->height);
        if (pval.IsHolding<float>()) {
            float height = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", _tokens->height.GetText(), height);
            filter.params.SetFloat(RtUString("height"), height);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath, _tokens->radius);
        if (pval.IsHolding<float>()) {
            float radius = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", _tokens->radius.GetText(), radius);
            filter.params.SetFloat(RtUString("radius"), radius);
        }
        if (barnMode == _tokens->analytic) {
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->analyticDirectional);
            if (pval.IsHolding<bool>()) {
                bool directional = pval.UncheckedGet<bool>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %d\n", 
                            _tokens->analyticDirectional.GetText(), 
                            directional);
                filter.params.SetInteger(RtUString("directional"),
                                          directional);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->analyticShearX);
            if (pval.IsHolding<float>()) {
                float shearX = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", _tokens->analyticShearX.GetText(),
                            shearX);
                filter.params.SetFloat(RtUString("shearX"), shearX);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->analyticShearY);
            if (pval.IsHolding<float>()) {
                float shearY = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", _tokens->analyticShearY.GetText(), 
                            shearY);
                filter.params.SetFloat(RtUString("shearY"), shearY);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->analyticApex);
            if (pval.IsHolding<float>()) {
                float apex = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", _tokens->analyticApex.GetText(), 
                            apex);
                filter.params.SetFloat(RtUString("apex"), apex);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->analyticUseLightDirection);
            if (pval.IsHolding<bool>()) {
                bool useLightDirection = pval.UncheckedGet<bool>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %d\n", 
                            _tokens->analyticUseLightDirection.GetText(),
                                            useLightDirection);
                filter.params.SetInteger(RtUString("useLightDirection"),
                                        useLightDirection);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->analyticDensityNearDistance);
            if (pval.IsHolding<float>()) {
                float nearDistance = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n",
                            _tokens->analyticDensityNearDistance.GetText(),
                                        nearDistance);
                filter.params.SetFloat(RtUString("densityNear"), nearDistance);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->analyticDensityFarDistance);
            if (pval.IsHolding<float>()) {
                float farDistance = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n",
                            _tokens->analyticDensityFarDistance.GetText(),
                                        farDistance);
                filter.params.SetFloat(RtUString("densityFar"), farDistance);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->analyticDensityNearValue);
            if (pval.IsHolding<float>()) {
                float nearValue = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->analyticDensityNearValue.GetText(),
                            nearValue);
                filter.params.SetFloat(RtUString("densityNearVal"), nearValue);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->analyticDensityFarValue);
            if (pval.IsHolding<float>()) {
                float farValue = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->analyticDensityFarValue.GetText(),
                            farValue);
                filter.params.SetFloat(RtUString("densityFarVal"), farValue);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->analyticDensityExponent);
            if (pval.IsHolding<float>()) {
                float exponent = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->analyticDensityExponent.GetText(),
                            exponent);
                filter.params.SetFloat(RtUString("densityPow"), exponent);
            }
        }
        float edgeThickness = 0.0;
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->edgeThickness);
        if (pval.IsHolding<float>()) {
            edgeThickness = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->edgeThickness.GetText(), 
                        edgeThickness);
            filter.params.SetFloat(RtUString("edge"), edgeThickness);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->preBarnEffect);
        if (pval.IsHolding<TfToken>()) {
            TfToken preBarnEffect = pval.UncheckedGet<TfToken>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %s\n", 
                        _tokens->preBarnEffect.GetText(),
                        preBarnEffect.GetText());
            int preBarn = -1;
            if (preBarnEffect == _tokens->noEffect)
                preBarn = 0;
            else if (preBarnEffect == _tokens->cone)
                preBarn = 1;
            else if (preBarnEffect == _tokens->noLight)
                preBarn = 1;
            if (preBarn >= 0)
                filter.params.SetInteger(RtUString("preBarn"), preBarn);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->scaleWidth);
        if (pval.IsHolding<float>()) {
            float width = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->scaleWidth.GetText(), width);
            filter.params.SetFloat(RtUString("scaleWidth"), width);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->scaleHeight);
        if (pval.IsHolding<float>()) {
            float height = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->scaleHeight.GetText(), height);
            filter.params.SetFloat(RtUString("scaleHeight"), height);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->refineTop);
        if (pval.IsHolding<float>()) {
            float top = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->refineTop.GetText(), top);
            filter.params.SetFloat(RtUString("top"), top);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->refineBottom);
        if (pval.IsHolding<float>()) {
            float bottom = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->refineBottom.GetText(), bottom);
            filter.params.SetFloat(RtUString("bottom"), bottom);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->refineLeft);
        if (pval.IsHolding<float>()) {
            float left = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->refineLeft.GetText(), left);
            filter.params.SetFloat(RtUString("left"), left);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->refineRight);
        if (pval.IsHolding<float>()) {
            float right = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->refineRight.GetText(), right);
            filter.params.SetFloat(RtUString("right"), right);
        }
        if (edgeThickness > 0.0) {
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                            _tokens->edgeScaleTop);
            if (pval.IsHolding<float>()) {
                float top = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->edgeScaleTop.GetText(), top);
                filter.params.SetFloat(RtUString("topEdge"), top);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                            _tokens->edgeScaleBottom);
            if (pval.IsHolding<float>()) {
                float bottom = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->edgeScaleBottom.GetText(), bottom);
                filter.params.SetFloat(RtUString("bottomEdge"), bottom);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                            _tokens->edgeScaleLeft);
            if (pval.IsHolding<float>()) {
                float left = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->edgeScaleLeft.GetText(), left);
                filter.params.SetFloat(RtUString("leftEdge"), left);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                            _tokens->edgeScaleRight);
            if (pval.IsHolding<float>()) {
                float right = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->edgeScaleRight.GetText(), right);
                filter.params.SetFloat(RtUString("rightEdge"), right);
            }
        }

    } else if (filterType == _tokens->PxrRodLightFilter) {
        filter.name = RtUString("PxrRodLightFilter");

        pval = sceneDelegate->GetLightParamValue(filterPath, _tokens->width);
        if (pval.IsHolding<float>()) {
            float width = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->width.GetText(), width);
            filter.params.SetFloat(RtUString("width"), width);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath, _tokens->height);
        if (pval.IsHolding<float>()) {
            float height = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->height.GetText(), height);
            filter.params.SetFloat(RtUString("height"), height);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath, _tokens->depth);
        if (pval.IsHolding<float>()) {
            float depth = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->depth.GetText(), depth);
            filter.params.SetFloat(RtUString("depth"), depth);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath, _tokens->radius);
        if (pval.IsHolding<float>()) {
            float radius = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->radius.GetText(), radius);
            filter.params.SetFloat(RtUString("radius"), radius);
        }
        float edgeThickness = 0.0;
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->edgeThickness);
        if (pval.IsHolding<float>()) {
            edgeThickness = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->edgeThickness.GetText(), edgeThickness);
            filter.params.SetFloat(RtUString("edge"), edgeThickness);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->scaleWidth);
        if (pval.IsHolding<float>()) {
            float width = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->scaleWidth.GetText(), width);
            filter.params.SetFloat(RtUString("scaleWidth"), width);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->scaleHeight);
        if (pval.IsHolding<float>()) {
            float height = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->scaleHeight.GetText(), height);
            filter.params.SetFloat(RtUString("scaleHeight"), height);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->scaleDepth);
        if (pval.IsHolding<float>()) {
            float depth = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->scaleDepth.GetText(), depth);
            filter.params.SetFloat(RtUString("scaleDepth"), depth);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->refineTop);
        if (pval.IsHolding<float>()) {
            float top = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->refineTop.GetText(), top);
            filter.params.SetFloat(RtUString("top"), top);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->refineBottom);
        if (pval.IsHolding<float>()) {
            float bottom = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->refineBottom.GetText(), bottom);
            filter.params.SetFloat(RtUString("bottom"), bottom);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->refineLeft);
        if (pval.IsHolding<float>()) {
            float left = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->refineLeft.GetText(), left);
            filter.params.SetFloat(RtUString("left"), left);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->refineRight);
        if (pval.IsHolding<float>()) {
            float right = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->refineRight.GetText(), right);
            filter.params.SetFloat(RtUString("right"), right);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->refineFront);
        if (pval.IsHolding<float>()) {
            float front = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->refineFront.GetText(), front);
            filter.params.SetFloat(RtUString("front"), front);
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->refineBack);
        if (pval.IsHolding<float>()) {
            float back = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->refineBack.GetText(), back);
            filter.params.SetFloat(RtUString("back"), back);
        }
        if (edgeThickness > 0.0) {
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                            _tokens->edgeScaleTop);
            if (pval.IsHolding<float>()) {
                float top = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->edgeScaleTop.GetText(), top);
                filter.params.SetFloat(RtUString("topEdge"), top);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                            _tokens->edgeScaleBottom);
            if (pval.IsHolding<float>()) {
                float bottom = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->edgeScaleBottom.GetText(), bottom);
                filter.params.SetFloat(RtUString("bottomEdge"), bottom);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                            _tokens->edgeScaleLeft);
            if (pval.IsHolding<float>()) {
                float left = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->edgeScaleLeft.GetText(), left);
                filter.params.SetFloat(RtUString("leftEdge"), left);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                            _tokens->edgeScaleRight);
            if (pval.IsHolding<float>()) {
                float right = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->edgeScaleRight.GetText(), right);
                filter.params.SetFloat(RtUString("rightEdge"), right);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                            _tokens->edgeScaleFront);
            if (pval.IsHolding<float>()) {
                float front = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->edgeScaleFront.GetText(), front);
                filter.params.SetFloat(RtUString("frontEdge"), front);
            }
            pval = sceneDelegate->GetLightParamValue(filterPath,
                                            _tokens->edgeScaleBack);
            if (pval.IsHolding<float>()) {
                float back = pval.UncheckedGet<float>();
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s %f\n", 
                            _tokens->edgeScaleBack.GetText(), back);
                filter.params.SetFloat(RtUString("backEdge"), back);
            }
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->colorSaturation);
        if (pval.IsHolding<float>()) {
            float saturation = pval.UncheckedGet<float>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %f\n", 
                        _tokens->colorSaturation.GetText(), saturation);
            filter.params.SetFloat(RtUString("saturation"), saturation);
        }

        // XXX -- handle the falloff spline
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->falloffKnots);
        if (pval.IsHolding<std::vector<float>>()) {
            const std::vector<float>& v =
                                pval.UncheckedGet<std::vector<float>>();
            if (TfDebug::IsEnabled(HDPRMAN_LIGHT_FILTER_LINKING)) {
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s size %d\n", 
                            _tokens->falloffKnots.GetText(), (int)(v.size()));
                for(size_t ii = 0; ii < v.size(); ii++)
                    TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                        .Msg("        %2zu: %f\n", ii, v[ii]);
            }
            filter.params.SetFloatArray(RtUString("falloff_Knots"),
                                         &v[0], v.size());
            // XXX -- extra param to hold the size of the spline
            filter.params.SetInteger(RtUString("falloff"), v.size());
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->falloffFloats);
        if (pval.IsHolding<std::vector<float>>()) {
            const std::vector<float>& v =
                                pval.UncheckedGet<std::vector<float>>();
            if (TfDebug::IsEnabled(HDPRMAN_LIGHT_FILTER_LINKING)) {
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s size %d\n", 
                            _tokens->falloffFloats.GetText(), (int)(v.size()));
                for(size_t ii = 0; ii < v.size(); ii++)
                    TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                        .Msg("        %2zu: %f\n", ii, v[ii]);
            }
            filter.params.SetFloatArray(RtUString("falloff_Floats"),
                                         &v[0], v.size());
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->falloffInterpolation);
        if (pval.IsHolding<TfToken>()) {
            TfToken interpolation = pval.UncheckedGet<TfToken>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %s\n", 
                        _tokens->falloffInterpolation.GetText(), 
                        interpolation.GetText());
            filter.params.SetString(RtUString("falloff_Interpolation"),
                                     RtUString(interpolation.GetText()));
        }
        // XXX -- handle the colorRamp spline
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->colorRampKnots);
        if (pval.IsHolding<std::vector<float>>()) {
            const std::vector<float>& v =
                                pval.UncheckedGet<std::vector<float>>();
            if (TfDebug::IsEnabled(HDPRMAN_LIGHT_FILTER_LINKING)) {
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s size %d\n", 
                            _tokens->colorRampKnots.GetText(), (int)(v.size()));
                for(size_t ii = 0; ii < v.size(); ii++)
                    TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                        .Msg("        %2zu: %f\n", ii, v[ii]);
            }
            filter.params.SetFloatArray(RtUString("colorRamp_Knots"),
                                         &v[0], v.size());
            // XXX -- extra param to hold the size of the spline
            filter.params.SetInteger(RtUString("colorRamp"), v.size());
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->colorRampColors);
        if (pval.IsHolding<std::vector<GfVec3f>>()) {
            const std::vector<GfVec3f>& v =
                                pval.UncheckedGet<std::vector<GfVec3f>>();
            if (TfDebug::IsEnabled(HDPRMAN_LIGHT_FILTER_LINKING)) {
                TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                    .Msg("      %s size %d\n", 
                            _tokens->colorRampColors.GetText(), 
                            (int)(v.size()));
                for(size_t ii = 0; ii < v.size(); ii++)
                    TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                        .Msg("      %2zu: %f %f %f\n",
                                            ii, v[ii][0], v[ii][1], v[ii][2]);
            }
            filter.params.SetColorArray(RtUString("colorRamp_Colors"), 
                    reinterpret_cast<const RtColorRGB*>(&v[0]), v.size());
        }
        pval = sceneDelegate->GetLightParamValue(filterPath,
                                        _tokens->colorRampInterpolation);
        if (pval.IsHolding<TfToken>()) {
            TfToken interpolation = pval.UncheckedGet<TfToken>();
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("      %s %s\n",
                        _tokens->colorRampInterpolation.GetText(), 
                        interpolation.GetText());
            filter.params.SetString(RtUString("colorRamp_Interpolation"),
                                     RtUString(interpolation.GetText()));
        }

    } else {
        // bail 
        TF_WARN("Light filter type %s not implemented\n", filterType.GetText());
        return false;
    }

    filterNodes->emplace_back(std::move(filter));
    return true;
}

void HdPrmanLightFilterGenerateCoordSysAndLinks(
    riley::ShadingNode *filter,
    SdfPath &filterPath,
    std::vector<riley::CoordinateSystemId> *coordsysIds,
    std::vector<TfToken> *filterLinks,
    HdSceneDelegate *sceneDelegate,
    HdPrman_Context *context,
    riley::Riley *riley,
    const riley::ShadingNode &lightNode)
{
    static const RtUString us_PxrRodLightFilter("PxrRodLightFilter");
    static const RtUString us_PxrBarnLightFilter("PxrBarnLightFilter");

    if (filter->name == us_PxrRodLightFilter ||
        filter->name == us_PxrBarnLightFilter) {
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

        // The coordSys name is the final component of the id,
        // after stripping namespaces.
        RtParamList attrs;
        const char *csName =
                    SdfPath::StripNamespace(filterPath.GetName()).c_str();
        attrs.SetString(RixStr.k_name, RtUString(csName));

        riley::CoordinateSystemId csId;
        csId = riley->CreateCoordinateSystem(riley::UserId::DefaultId(),
                                             xform, attrs);

        (*coordsysIds).push_back(csId);

        filter->params.SetString(RtUString("coordsys"),
                                 RtUString(csName));

        if (filter->name == us_PxrBarnLightFilter) {
            filter->params.SetString(RtUString("__lightFilterParentShader"),
                                     lightNode.name);
        }
    }

    // Light filter linking
    VtValue val = sceneDelegate->GetLightParamValue(filterPath,
                                    HdTokens->lightFilterLink);
    TfToken lightFilterLink = TfToken();
    if (val.IsHolding<TfToken>()) {
        lightFilterLink = val.UncheckedGet<TfToken>();
    }
    
    if (!lightFilterLink.IsEmpty()) {
        context->IncrementLightFilterCount(lightFilterLink);
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
