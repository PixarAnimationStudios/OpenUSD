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
#include "pxr/pxr.h"
#include "usdKatana/attrMap.h"
#include "usdKatana/readLightFilter.h"
#include "usdKatana/readPrim.h"
#include "usdKatana/readXformable.h"
#include "usdKatana/utils.h"

#include "pxr/base/tf/stringUtils.h"

#include "pxr/usd/usdLux/light.h"
#include "pxr/usd/usdLux/domeLight.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdLux/geometryLight.h"
#include "pxr/usd/usdLux/sphereLight.h"
#include "pxr/usd/usdLux/diskLight.h"
#include "pxr/usd/usdLux/rectLight.h"
#include "pxr/usd/usdLux/shapingAPI.h"
#include "pxr/usd/usdLux/shadowAPI.h"
#include "pxr/usd/usdLux/linkingAPI.h"
#include "pxr/usd/usdRi/lightFilterAPI.h"
#include "pxr/usd/usdRi/pxrIntMultLightFilter.h"
#include "pxr/usd/usdRi/pxrBarnLightFilter.h"
#include "pxr/usd/usdRi/pxrRampLightFilter.h"
#include "pxr/usd/usdRi/pxrRodLightFilter.h"
#include "pxr/usd/usdRi/pxrCookieLightFilter.h"

#include <FnGeolibServices/FnAttributeFunctionUtil.h>
#include <FnLogging/FnLogging.h>
#include <pystring/pystring.h>

#include <stack>

PXR_NAMESPACE_OPEN_SCOPE

FnLogSetup("PxrUsdKatanaReadLightFilter");

using std::string;
using std::vector;
using FnKat::GroupBuilder;


// Similar to katana's group builder, but takes in usd attributes.
struct _UsdBuilder {
    GroupBuilder &_builder;
    double _time;

    _UsdBuilder& Set(std::string kat_name, UsdAttribute attr) {
        VtValue val;
        if (attr.IsValid() && attr.HasAuthoredValueOpinion()
            && attr.Get(&val, _time)) {
            FnKat::Attribute kat_attr =
                PxrUsdKatanaUtils::ConvertVtValueToKatAttr( val,
                                        /* asShaderParam */ true,
                                        /* pathAsModel */ false,
                                        /* resolvePath */ false);
            _builder.set(kat_name.c_str(), kat_attr);
        }
        return *this;
    }

    _UsdBuilder& SetSpline(std::string kat_prefix, std::string valueSuffix,
                           UsdRiSplineAPI spline) {
        // Knot count
        VtFloatArray posVec;
        if (UsdAttribute posAttr = spline.GetPositionsAttr()) {
            if (!posAttr.Get(&posVec)) {
                FnLogWarn("Invalid spline positions type: " <<
                          posAttr.GetTypeName().GetAsToken().GetString() <<
                          ": " << posAttr.GetPath().GetString());
                return *this;
            }
        }

        // Interpolation
        std::string interp = "unknown";
        {
            VtValue val;
            UsdAttribute attr = spline.GetInterpolationAttr();
            if (attr.IsValid() && attr.HasAuthoredValueOpinion()
                && attr.Get(&val, _time) && val.IsHolding<TfToken>()) {
                TfToken t = val.UncheckedGet<TfToken>();
                if (t == UsdRiTokens->linear) {
                    interp = "linear";
                } else if (t == UsdRiTokens->catmullRom) {
                    interp = "catmull-rom";
                } else if (t == UsdRiTokens->bspline) {
                    interp = "bspline";
                } else if (t == UsdRiTokens->constant) {
                    interp = "constant";
                } else {
                    FnLogWarn("Unsupported spline interpolation: " <<
                              t.GetString() << ": " <<
                              attr.GetPath().GetString());
                    return *this;
                }
            }
            else {
                FnLogWarn("Invalid spline interpolation: " <<
                          attr.GetPath().GetString());
                return *this;
            }
        }

        _builder.set(kat_prefix, FnKat::IntAttribute(posVec.size()));
        Set( kat_prefix + "_Knots", spline.GetPositionsAttr() );
        Set( kat_prefix + valueSuffix, spline.GetValuesAttr() );
        _builder.set(kat_prefix + "_Interpolation",
                     FnKat::StringAttribute(interp));
        return *this;
    }
};

void
PxrUsdKatanaReadLightFilter(
        const UsdLuxLightFilter& lightFilter,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    const UsdPrim filterPrim = lightFilter.GetPrim();
    const SdfPath primPath = filterPrim.GetPath();
    const double currentTime = data.GetCurrentTime();

    GroupBuilder materialBuilder;
    GroupBuilder filterBuilder;
    _UsdBuilder usdBuilder = {filterBuilder, currentTime};
    
    if (UsdRiLightFilterAPI f = UsdRiLightFilterAPI(filterPrim)) {
        usdBuilder
            .Set("density", f.GetRiDensityAttr())
            .Set("intensity", f.GetRiIntensityAttr())
            .Set("exposure", f.GetRiExposureAttr())
            .Set("invert", f.GetRiInvertAttr())
            .Set("diffuse", f.GetRiDiffuseAttr())
            .Set("specular", f.GetRiSpecularAttr())
            ;

        // combineMode
        {
            VtValue val;
            UsdAttribute attr = f.GetRiCombineModeAttr();
            if (attr.IsValid() && attr.HasAuthoredValueOpinion()
                && attr.Get(&val, currentTime) && val.IsHolding<TfToken>()) {
                TfToken t = val.Get<TfToken>();
                if (t == UsdRiTokens->multiply) {
                    filterBuilder.set("combineMode",
                                      FnKat::StringAttribute("mult"));
                } else if (t == UsdRiTokens->max) {
                    filterBuilder.set("combineMode",
                                      FnKat::StringAttribute("max"));
                } else if (t == UsdRiTokens->min) {
                    filterBuilder.set("combineMode",
                                      FnKat::StringAttribute("min"));
                } else if (t == UsdRiTokens->screen) {
                    filterBuilder.set("combineMode",
                                      FnKat::StringAttribute("screen"));
                }
            }
        }
    }
    if (UsdRiPxrIntMultLightFilter f = UsdRiPxrIntMultLightFilter(filterPrim)) {
        materialBuilder.set("prmanLightfilterShader",
                            FnKat::StringAttribute("PxrIntMultLightFilter"));
    }
    if (UsdRiPxrBarnLightFilter f = UsdRiPxrBarnLightFilter(filterPrim)) {
        materialBuilder.set("prmanLightfilterShader",
                            FnKat::StringAttribute("PxrBarnLightFilter"));
        usdBuilder
            .Set("directional", f.GetAnalyticDirectionalAttr())
            .Set("shearX", f.GetAnalyticShearXAttr())
            .Set("shearY", f.GetAnalyticShearYAttr())
            .Set("apex", f.GetAnalyticApexAttr())
            .Set("useLightDirection", f.GetAnalyticDirectionalAttr())
            .Set("width", f.GetWidthAttr())
            .Set("height", f.GetHeightAttr())
            .Set("radius", f.GetRadiusAttr())
            .Set("edge", f.GetEdgeThicknessAttr())
            .Set("scaleWidth", f.GetScaleWidthAttr())
            .Set("scaleHeight", f.GetScaleHeightAttr())
            .Set("left", f.GetRefineLeftAttr())
            .Set("right", f.GetRefineRightAttr())
            .Set("top", f.GetRefineTopAttr())
            .Set("bottom", f.GetRefineBottomAttr())
            .Set("leftEdge", f.GetEdgeLeftAttr())
            .Set("rightEdge", f.GetEdgeRightAttr())
            .Set("topEdge", f.GetEdgeTopAttr())
            .Set("bottomEdge", f.GetEdgeBottomAttr())
            .Set("densityNear", f.GetAnalyticDensityNearDistanceAttr())
            .Set("densityNearVal", f.GetAnalyticDensityNearValueAttr())
            .Set("densityFar", f.GetAnalyticDensityFarDistanceAttr())
            .Set("densityFarVal", f.GetAnalyticDensityFarValueAttr())
            .Set("densityPow", f.GetAnalyticDensityExponentAttr())
            ;

        // barnMode 
        {
            VtValue val;
            UsdAttribute attr = f.GetBarnModeAttr();
            if (attr.IsValid() && attr.HasAuthoredValueOpinion()
                && attr.Get(&val, currentTime) && val.IsHolding<TfToken>()) {
                TfToken t = val.Get<TfToken>();
                if (t == UsdRiTokens->analytic) {
                    filterBuilder.set("barnMode", FnKat::IntAttribute(1));
                } else if (t == UsdRiTokens->physical) {
                    filterBuilder.set("barnMode", FnKat::IntAttribute(0));
                }
            }
        }
        // preBarn
        {
            VtValue val;
            UsdAttribute attr = f.GetPreBarnEffectAttr();
            if (attr.IsValid() && attr.HasAuthoredValueOpinion()
                && attr.Get(&val, currentTime) && val.IsHolding<TfToken>()) {
                TfToken t = val.Get<TfToken>();
                if (t == UsdRiTokens->noEffect) {
                    filterBuilder.set("preBarn", FnKat::IntAttribute(0));
                } else if (t == UsdRiTokens->cone) {
                    filterBuilder.set("preBarn", FnKat::IntAttribute(1));
                } else if (t == UsdRiTokens->noLight) {
                    filterBuilder.set("preBarn", FnKat::IntAttribute(2));
                }
            }
        }
    }
    if (UsdRiPxrCookieLightFilter f = UsdRiPxrCookieLightFilter(filterPrim)) {
        materialBuilder.set("prmanLightfilterShader",
                            FnKat::StringAttribute("PxrCookieLightFilter"));
        // cookieMode 
        {
            VtValue val;
            UsdAttribute attr = f.GetCookieModeAttr();
            if (attr.IsValid() && attr.HasAuthoredValueOpinion()
                && attr.Get(&val, currentTime) && val.IsHolding<TfToken>()) {
                TfToken t = val.Get<TfToken>();
                if (t == UsdRiTokens->analytic) {
                    filterBuilder.set("cookieMode", FnKat::IntAttribute(1));
                } else if (t == UsdRiTokens->physical) {
                    filterBuilder.set("cookieMode", FnKat::IntAttribute(0));
                }
            }
        }
        // tileMode
        {
            VtValue val;
            UsdAttribute attr = f.GetTextureWrapModeAttr();
            if (attr.IsValid() && attr.HasAuthoredValueOpinion()
                && attr.Get(&val, currentTime) && val.IsHolding<TfToken>()) {
                TfToken t = val.Get<TfToken>();
                if (t == UsdRiTokens->off) {
                    filterBuilder.set("tileMode", FnKat::IntAttribute(0));
                } else if (t == UsdRiTokens->repeat) {
                    filterBuilder.set("tileMode", FnKat::IntAttribute(2));
                } else if (t == UsdRiTokens->clamp) {
                    filterBuilder.set("tileMode", FnKat::IntAttribute(1));
                }
            }
        }
        usdBuilder
            .Set("map", f.GetTextureMapAttr())
            .Set("fillColor", f.GetTextureFillColorAttr())
            .Set("width", f.GetWidthAttr())
            .Set("height", f.GetHeightAttr())
            .Set("directional", f.GetAnalyticDirectionalAttr())
            .Set("shearX", f.GetAnalyticShearXAttr())
            .Set("shearY", f.GetAnalyticShearYAttr())
            .Set("apex", f.GetAnalyticApexAttr())
            .Set("useLightDirection", f.GetAnalyticDirectionalAttr())
            .Set("invertU", f.GetTextureInvertUAttr())
            .Set("invertV", f.GetTextureInvertVAttr())
            .Set("scaleU", f.GetTextureScaleUAttr())
            .Set("scaleV", f.GetTextureScaleVAttr())
            .Set("offsetU", f.GetTextureOffsetUAttr())
            .Set("offsetV", f.GetTextureOffsetVAttr())
            .Set("blur", f.GetAnalyticBlurAmountAttr())
            .Set("sBlurMult", f.GetAnalyticBlurSMultAttr())
            .Set("tBlurMult", f.GetAnalyticBlurTMultAttr())
            .Set("blurNearDist", f.GetAnalyticBlurNearDistanceAttr())
            .Set("blurNearVal", f.GetAnalyticBlurNearValueAttr())
            .Set("blurFarDist", f.GetAnalyticBlurFarDistanceAttr())
            .Set("blurFarVal", f.GetAnalyticBlurFarValueAttr())
            .Set("blurMidpoint", f.GetAnalyticBlurMidpointAttr())
            .Set("blurMidVal", f.GetAnalyticBlurMidValueAttr())
            .Set("blurPow", f.GetAnalyticBlurExponentAttr())
            .Set("densityNearDist", f.GetAnalyticDensityNearDistanceAttr())
            .Set("densityNearVal", f.GetAnalyticDensityNearValueAttr())
            .Set("densityFarDist", f.GetAnalyticDensityFarDistanceAttr())
            .Set("densityFarVal", f.GetAnalyticDensityFarValueAttr())
            .Set("densityMidpoint", f.GetAnalyticDensityMidpointAttr())
            .Set("densityMidVal", f.GetAnalyticDensityMidValueAttr())
            .Set("densityPow", f.GetAnalyticDensityExponentAttr())
            .Set("saturation", f.GetColorSaturationAttr())
            .Set("midpoint", f.GetColorMidpointAttr())
            .Set("contrast", f.GetColorContrastAttr())
            .Set("whitepoint", f.GetColorWhitepointAttr())
            .Set("tint", f.GetColorTintAttr())
            ;
    }
    if (UsdRiPxrRampLightFilter f = UsdRiPxrRampLightFilter(filterPrim)) {
        materialBuilder.set("prmanLightfilterShader",
                            FnKat::StringAttribute("PxrRampLightFilter"));
        usdBuilder
            .Set("beginDist", f.GetFalloffRampBeginDistanceAttr())
            .Set("endDist", f.GetFalloffRampEndDistanceAttr())
            .SetSpline("colorRamp", "_Colors", f.GetColorRampAPI())
            .SetSpline("ramp", "_Floats", f.GetFalloffRampAPI())
            ;
        // rampMode
        {
            VtValue val;
            UsdAttribute attr = f.GetRampModeAttr();
            if (attr.IsValid() && attr.HasAuthoredValueOpinion()
                && attr.Get(&val, currentTime) && val.IsHolding<TfToken>()) {
                TfToken t = val.Get<TfToken>();
                if (t == UsdRiTokens->distanceToLight) {
                    filterBuilder.set("rampMode", FnKat::IntAttribute(0));
                } else if (t == UsdRiTokens->linear) {
                    filterBuilder.set("rampMode", FnKat::IntAttribute(1));
                } else if (t == UsdRiTokens->spherical) {
                    filterBuilder.set("rampMode", FnKat::IntAttribute(2));
                } else if (t == UsdRiTokens->radial) {
                    filterBuilder.set("rampMode", FnKat::IntAttribute(3));
                }
            }
        }
    }
    if (UsdRiPxrRodLightFilter f = UsdRiPxrRodLightFilter(filterPrim)) {
        materialBuilder.set("prmanLightfilterShader",
                            FnKat::StringAttribute("PxrRodLightFilter"));
        usdBuilder
            .Set("width", f.GetWidthAttr())
            .Set("height", f.GetHeightAttr())
            .Set("depth", f.GetDepthAttr())
            .Set("radius", f.GetRadiusAttr())
            .Set("edge", f.GetEdgeThicknessAttr())
            .Set("scaleWidth", f.GetScaleWidthAttr())
            .Set("scaleHeight", f.GetScaleHeightAttr())
            .Set("scaleDepth", f.GetScaleDepthAttr())
            .Set("left", f.GetRefineLeftAttr())
            .Set("right", f.GetRefineRightAttr())
            .Set("top", f.GetRefineTopAttr())
            .Set("bottom", f.GetRefineBottomAttr())
            .Set("front", f.GetRefineFrontAttr())
            .Set("back", f.GetRefineBackAttr())
            .Set("leftEdge", f.GetEdgeLeftAttr())
            .Set("rightEdge", f.GetEdgeRightAttr())
            .Set("topEdge", f.GetEdgeTopAttr())
            .Set("bottomEdge", f.GetEdgeBottomAttr())
            .Set("frontEdge", f.GetEdgeFrontAttr())
            .Set("backEdge", f.GetEdgeBackAttr())
            .Set("saturation", f.GetColorSaturationAttr())
            .SetSpline("colorRamp", "_Colors", f.GetColorRampAPI())
            .SetSpline("falloff", "_Floats", f.GetFalloffRampAPI())
            ;
    }

    // Gather prman statements
    FnKat::GroupBuilder prmanBuilder;
    PxrUsdKatanaReadPrimPrmanStatements(filterPrim, currentTime, prmanBuilder);
    attrs.set("prmanStatements", prmanBuilder.build());
    materialBuilder.set("prmanLightfilterParams", filterBuilder.build());
    attrs.set("material", materialBuilder.build());
    PxrUsdKatanaReadXformable(lightFilter, data, attrs);
    attrs.set("type", FnKat::StringAttribute("light filter"));
}

PXR_NAMESPACE_CLOSE_SCOPE
