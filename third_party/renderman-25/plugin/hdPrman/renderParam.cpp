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
#include "hdPrman/renderParam.h"

#include "hdPrman/camera.h"
#include "hdPrman/cameraContext.h"
#include "hdPrman/coordSys.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/debugUtil.h"
#include "hdPrman/framebuffer.h"
#include "hdPrman/instancer.h"
#include "hdPrman/material.h"
#include "hdPrman/motionBlurSceneIndexPlugin.h"
#include "hdPrman/prmanArchDefs.h" // required for stats/Session.h
#include "hdPrman/renderDelegate.h"
#include "hdPrman/renderSettings.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/tokens.h"
#include "hdPrman/utils.h"

#include "pxr/base/arch/library.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/pathUtils.h"  // Extract extension from tf token
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/imaging/hd/extComputationUtils.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/usd/sdf/path.h"

#include "Riley.h"
#include "RiTypesHelper.h"
#include "RixRiCtl.h"
#include "stats/Session.h"

#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (percentDone)
    (PrimvarPass)
    (name)
    (sourceName)
    (sourceType)
    (lpe)

    // See PxrDisplayChannelAPI
    ((riDisplayChannelNamespace,    "ri:displayChannel:"))
    // See PxrDisplayDriverAPI
    ((riDisplayDriverNamespace,     "ri:displayDriver:"))
);

TF_DEFINE_PRIVATE_TOKENS(
    _riOptionsTokens,
    ((riRiFormatResolution,         "ri:Ri:FormatResolution"))
    ((riHiderMinSamples,            "ri:hider:minsammples"))
    ((riHiderMaxSamples,            "ri:hider:maxsamples"))
    ((riRiPixelVariance,            "ri:Ri:PixelVariance"))
    ((riRiFormatPixelAspectRatio,   "ri:Ri:FormatPixelAspectRatio"))
    ((riLimitsThreads,              "ri:limits:threads"))
);


TF_DEFINE_ENV_SETTING(HD_PRMAN_ENABLE_MOTIONBLUR, true,
                      "Enable motion blur in HdPrman");
TF_DEFINE_ENV_SETTING(HD_PRMAN_NTHREADS, 0,
                      "Override number of threads used by HdPrman");
TF_DEFINE_ENV_SETTING(HD_PRMAN_OSL_VERBOSE, 0,
                      "Override osl verbose in HdPrman");
TF_DEFINE_ENV_SETTING(HD_PRMAN_DISABLE_HIDER_JITTER, false,
                      "Disable hider jitter");
TF_DEFINE_ENV_SETTING(HD_PRMAN_DEFER_SET_OPTIONS, true,
                      "Defer first SetOptions call to render settings prim sync.");

extern TfEnvSetting<bool> HD_PRMAN_ENABLE_QUICKINTEGRATE;

static bool _enableQuickIntegrate =
    TfGetEnvSetting(HD_PRMAN_ENABLE_QUICKINTEGRATE);

// Used when Creating Riley RenderView from the RenderSettings or RenderSpec
static GfVec2i _fallbackResolution = GfVec2i(512,512);

TF_MAKE_STATIC_DATA(std::vector<HdPrman_RenderParam::IntegratorCameraCallback>,
                    _integratorCameraCallbacks)
{
    _integratorCameraCallbacks->clear();
}

HdPrman_RenderParam::HdPrman_RenderParam(
        HdPrmanRenderDelegate* renderDelegate,
        const std::string &rileyVariant,
        const std::string &xpuVariant,
        const std::vector<std::string>& extraArgs) :
    _rix(nullptr),
    _ri(nullptr),
    _mgr(nullptr),
    _statsSession(nullptr),
    _progressPercent(0),
    _riley(nullptr),
    _sceneLightCount(0),
    _shutterInterval(HDPRMAN_SHUTTEROPEN_DEFAULT, HDPRMAN_SHUTTERCLOSE_DEFAULT),
    _initRileyOptions(false),
    _sampleFiltersId(riley::SampleFilterId::InvalidId()),
    _displayFiltersId(riley::DisplayFilterId::InvalidId()),
    _lastLegacySettingsVersion(0),
    _resolution(0),
    _renderDelegate(renderDelegate)
{
    // Create the stats session
    _CreateStatsSession();

    // Setup to use the default GPU
    _xpuGpuConfig.push_back(0);

    TfRegistryManager::GetInstance().SubscribeTo<HdPrman_RenderParam>();
    _CreateRiley(rileyVariant, xpuVariant, extraArgs);
    
    // Register RenderMan display driver
    HdPrmanFramebuffer::Register(_rix);
}

HdPrman_RenderParam::~HdPrman_RenderParam()
{
    DeleteRenderThread();

    _DeleteInternalPrims();

    _DestroyRiley();

    _DestroyStatsSession();
}

void
HdPrman_RenderParam::IncrementLightLinkCount(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightLinkMutex);
    ++_lightLinkRefs[name];
}

void 
HdPrman_RenderParam::DecrementLightLinkCount(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightLinkMutex);
    if (--_lightLinkRefs[name] == 0) {
        _lightLinkRefs.erase(name);
    }
}

bool 
HdPrman_RenderParam::IsLightLinkUsed(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightLinkMutex);
    return _lightLinkRefs.find(name) != _lightLinkRefs.end();
}

void
HdPrman_RenderParam::IncrementLightFilterCount(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightFilterMutex);
    ++_lightFilterRefs[name];
}

void 
HdPrman_RenderParam::DecrementLightFilterCount(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightFilterMutex);
    if (--_lightFilterRefs[name] == 0) {
        _lightFilterRefs.erase(name);
    }
}

bool 
HdPrman_RenderParam::IsLightFilterUsed(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightFilterMutex);
    return _lightFilterRefs.find(name) != _lightFilterRefs.end();
}

static size_t
_ConvertPointsPrimvar(
    HdSceneDelegate *sceneDelegate,
    SdfPath const &id,
    GfVec2f const &shutterInterval,
    RtPrimVarList& primvars,
    const size_t * npointsHint)
{
    HdExtComputationPrimvarDescriptorVector compPrimvar;
#if PXR_VERSION < 2402
    for (auto const& pv : sceneDelegate->GetExtComputationPrimvarDescriptors(
        id, HdInterpolationVertex)) {
        if (pv.name == HdTokens->points) {
            compPrimvar.emplace_back(pv);
        }
    }
#endif

    // Get points time samples
    HdTimeSampleArray<VtVec3fArray, HDPRMAN_MAX_TIME_SAMPLES> points;
    {
        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedPoints;
        if (compPrimvar.empty()) {
            sceneDelegate->SamplePrimvar(id, HdTokens->points, &boxedPoints);
        } else {
            HdExtComputationUtils::SampledValueStore<HDPRMAN_MAX_TIME_SAMPLES>
                compSamples;
            HdExtComputationUtils::SampleComputedPrimvarValues<
                HDPRMAN_MAX_TIME_SAMPLES>(
                    compPrimvar, sceneDelegate, HDPRMAN_MAX_TIME_SAMPLES,
                    &compSamples);
            boxedPoints = compSamples[HdTokens->points];
        }
        if (!points.UnboxFrom(boxedPoints)) {
            TF_WARN("<%s> points did not have expected type vec3f[]",
                    id.GetText());
        }
    }

    size_t npoints = 0;
    if (npointsHint) {
        npoints = *npointsHint;
    } else {
        if (points.count > 0) {
            npoints = points.values[0].size();
        }
        primvars.SetDetail(
            1,        /* uniform */
            npoints,  /* vertex */
            npoints,  /* varying */
            npoints   /* faceVarying */);
    }

    primvars.SetTimes(points.count, &points.times[0]);
    for (size_t i=0; i < points.count; ++i) {
        if (points.values[i].size() == npoints) {
            primvars.SetPointDetail(
                RixStr.k_P, 
                (RtPoint3*) points.values[i].cdata(),
                RtDetailType::k_vertex, 
                i);
        } else {
            TF_WARN("<%s> primvar 'points' size (%zu) did not match "
                    "expected (%zu)",
                    id.GetText(), 
                    points.values[i].size(),
                    npoints);
        }
    }

    return npoints;
}

void
HdPrman_ConvertPointsPrimvar(
    HdSceneDelegate *sceneDelegate,
    SdfPath const &id,
    GfVec2f const &shutterInterval,
    RtPrimVarList& primvars,
    const size_t npoints)

{
    _ConvertPointsPrimvar(
        sceneDelegate, id, shutterInterval, primvars, &npoints);
}

size_t
HdPrman_ConvertPointsPrimvarForPoints(
    HdSceneDelegate *sceneDelegate,
    SdfPath const &id,
    GfVec2f const &shutterInterval,
    RtPrimVarList& primvars)
{
    return _ConvertPointsPrimvar(
        sceneDelegate, id, shutterInterval, primvars, nullptr);
}

inline static RtDetailType
_RixDetailForHdInterpolation(HdInterpolation interp)
{
    switch (interp) {
    case HdInterpolationInstance:
        // Instance-level primvars, aka attributes, must be constant.
        return RtDetailType::k_constant;
    case HdInterpolationConstant:
        return RtDetailType::k_constant;
    case HdInterpolationUniform:
        return RtDetailType::k_uniform;
    case HdInterpolationVertex:
        return RtDetailType::k_vertex;
    case HdInterpolationVarying:
        return RtDetailType::k_varying;
    case HdInterpolationFaceVarying:
        return RtDetailType::k_facevarying;
    default:
        TF_CODING_ERROR("Unknown HdInterpolation value");
        return RtDetailType::k_constant;
    }
}

static RtUString
_GetPrmanPrimvarName(TfToken const& hdPrimvarName,
                     RtDetailType const& /*detail*/)
{
    // Handle cases where Hydra built-in primvars map to Renderman
    // built-in primvars.
    if (hdPrimvarName == HdTokens->points) {
        return RixStr.k_P;
    } else if (hdPrimvarName == HdTokens->normals) {
        // Hydra "normals" becomes Renderman "N"
        return RixStr.k_N;
    } else if (hdPrimvarName == HdTokens->widths) {
        return RixStr.k_width;
    }

    return RtUString(hdPrimvarName.GetText());
}

static HdExtComputationPrimvarDescriptorVector
_GetComputedPrimvars(HdSceneDelegate* sceneDelegate,
                     SdfPath const& id,
                     HdInterpolation interp,
                     HdDirtyBits dirtyBits)
{
    HdExtComputationPrimvarDescriptorVector dirtyCompPrimvars;

    // Get all the dirty computed primvars
    HdExtComputationPrimvarDescriptorVector compPrimvars;
    compPrimvars = sceneDelegate->GetExtComputationPrimvarDescriptors
                                    (id,interp);
    for (auto const& pv : compPrimvars) {
        if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name)
            && pv.name != HdTokens->points) {
            dirtyCompPrimvars.emplace_back(pv);
        }
    }

    return dirtyCompPrimvars;
}

static bool
_IsPrototypeAttribute(TfToken const& primvarName)
{
    // This is a list of names for uniform primvars/attributes that
    // affect the prototype geometry in Renderman. They need to be
    // emitted on the prototype as primvars to take effect, instead of
    // on geometry instances.
    //
    // This list was created based on this doc page:
    //   https://rmanwiki.pixar.com/display/REN23/Primitive+Variables
    typedef std::unordered_set<TfToken, TfToken::HashFunctor> TfTokenSet;
    static const TfTokenSet prototypeAttributes = {
        // Common
        TfToken("ri:attributes:identifier:object"),
        // Shading
        TfToken("ri:attributes:derivatives:extrapolate"),
        TfToken("ri:attributes:displacement:ignorereferenceinstance"),
        TfToken("ri:attributes:displacementbound:CoordinateSystem"),
        TfToken("ri:attributes:displacementbound:offscreen"),
        TfToken("ri:attributes:displacementbound:sphere"),
        TfToken("ri:attributes:Ri:Orientation"),
        TfToken("ri:attributes:trace:autobias"),
        TfToken("ri:attributes:trace:bias"),
        TfToken("ri:attributes:trace:sssautobias"),
        TfToken("ri:attributes:trace:sssbias"),
        TfToken("ri:attributes:trace:displacements"),
        // Dicing
        TfToken("ri:attributes:dice:micropolygonlength"),
        TfToken("ri:attributes:dice:offscreenstrategy"),
        TfToken("ri:attributes:dice:rasterorient"),
        TfToken("ri:attributes:dice:referencecamera"),
        TfToken("ri:attributes:dice:referenceinstance"),
        TfToken("ri:attributes:dice:strategy"),
        TfToken("ri:attributes:dice:worlddistancelength"),
        TfToken("ri:attributes:Ri:GeometricApproximationFocusFactor"),
        TfToken("ri:attributes:Ri:GeometricApproximationMotionFactor"),
        // Points
        TfToken("ri:attributes:falloffpower"),
        // Volume
        TfToken("ri:attributes:dice:minlength"),
        TfToken("ri:attributes:dice:minlengthspace"),
        TfToken("ri:attributes:Ri:Bound"),
        TfToken("ri:attributes:volume:dsominmax"),
        TfToken("ri:attributes:volume:aggregate"),
        // SubdivisionMesh
        TfToken("ri:attributes:dice:pretessellate"),
        TfToken("ri:attributes:dice:watertight"),
        TfToken("ri:attributes:shade:faceset"),
        TfToken("ri:attributes:stitchbound:CoordinateSystem"),
        TfToken("ri:attributes:stitchbound:sphere"),
        // NuPatch
        TfToken("ri:attributes:trimcurve:sense"),
        // PolygonMesh
        TfToken("ri:attributes:polygon:concave"),
        TfToken("ri:attributes:polygon:smoothdisplacement"),
        TfToken("ri:attributes:polygon:smoothnormals"),
        // Procedural
        TfToken("ri:attributes:procedural:immediatesubdivide"),
        TfToken("ri:attributes:procedural:reentrant")
    };

    return prototypeAttributes.count(primvarName) > 0;
}

template <typename T>
static void
_Convert(HdSceneDelegate *sceneDelegate, SdfPath const& id,
         HdInterpolation hdInterp, T& params, int expectedSize,
         float time = 0.f)
{
    static_assert(std::disjunction<
        std::is_same<RtParamList, T>,
        std::is_same<RtPrimVarList, T>>::value,
        "params must be RtParamList or RtPrimVarList");
        
    // XXX:TODO: To support array-valued types, we need more
    // shaping information.  Currently we assume arrays are
    // simply N scalar values, according to the detail.
    
    std::string label;
    if constexpr (std::is_same<RtPrimVarList, T>()) {
        label = "primvar";
    } else {
        label = "attribute";
    }

    const RtDetailType detail = _RixDetailForHdInterpolation(hdInterp);

    TF_DEBUG(HDPRMAN_PRIMVARS)
        .Msg("HdPrman: _Convert called -- <%s> %s %s\n",
             id.GetText(),
             TfEnum::GetName(hdInterp).c_str(),
             label.c_str());

    // Computed primvars
    if constexpr (std::is_same<RtPrimVarList, T>()) {
        // XXX: Prman doesn't seem to check dirtyness before pulling a value.
        // Passing AllDirty until we plumb/respect change tracking.
        HdExtComputationPrimvarDescriptorVector  computedPrimvars =
            _GetComputedPrimvars(sceneDelegate, id, hdInterp, 
                                 HdChangeTracker::AllDirty);
        if (!computedPrimvars.empty()) {
            // Execute the computations
            HdExtComputationUtils::ValueStore valueStore
                = HdExtComputationUtils::GetComputedPrimvarValues(
                    computedPrimvars, sceneDelegate);
        
            for (auto const& compPrimvar : computedPrimvars) {
                auto const it = valueStore.find(compPrimvar.name);
                if (!TF_VERIFY(it != valueStore.end())) {
                    continue;
                }
                VtValue val = it->second;
                if (val.IsEmpty() ||
                    (val.IsArrayValued() && val.GetArraySize() == 0)) {
                    continue;
                }

                RtUString name = _GetPrmanPrimvarName(compPrimvar.name, detail);

                TF_DEBUG(HDPRMAN_PRIMVARS)
                    .Msg("HdPrman: <%s> %s primvar "
                         "Computed Primvar \"%s\" (%s) = \"%s\"\n",
                         id.GetText(),
                         TfEnum::GetName(hdInterp).c_str(),
                         compPrimvar.name.GetText(),
                         name.CStr(),
                         TfStringify(val).c_str());

                if (val.IsArrayValued() && 
                    val.GetArraySize() != static_cast<size_t>(expectedSize)) {
                    TF_WARN("<%s> primvar '%s' size (%zu) did not match "
                            "expected (%d)", id.GetText(),
                            compPrimvar.name.GetText(), val.GetArraySize(),
                            expectedSize);
                    continue;
                }

                if (!HdPrman_Utils::SetPrimVarFromVtValue(name, val, detail,
                                    compPrimvar.role, &params)) {
                    TF_WARN("Ignoring unhandled primvar of type %s for %s.%s\n",
                        val.GetTypeName().c_str(), id.GetText(),
                        compPrimvar.name.GetText());
                }
            }
        }
    }

    // Authored primvars
    for (HdPrimvarDescriptor const& primvar:
         sceneDelegate->GetPrimvarDescriptors(id, hdInterp))
    {
        TF_DEBUG(HDPRMAN_PRIMVARS)
            .Msg("HdPrman: authored id <%s> hdInterp %s label %s "
                 "primvar \"%s\"\n",
                 id.GetText(),
                 TfEnum::GetName(hdInterp).c_str(),
                 label.c_str(),
                 primvar.name.GetText());

        // Skip params with special handling.
        if (primvar.name == HdTokens->points) {
            continue;
        }

        // Constant Hydra primvars become either Riley primvars or attributes,
        // depending on prefix and the name.
        // 1.) Constant primvars with the "ri:attributes:" or
        //     "primvars:ri:attributes:" prefixes have that
        //     prefix stripped and become primvars for geometry prototype
        //     "attributes" or attributes for geometry instances.
        // 2.) Constant primvars with the "user:" prefix become attributes.
        // 3.) Other constant primvars get set on prototype geometry as
        //     primvars.
        RtUString name;
        if (hdInterp == HdInterpolationConstant) {
            static const char *userAttrPrefix = "user:";
            static const char *riAttrPrefix = "ri:attributes:";
            static const char *primvarsPrefix = "primvars:";
            bool hasUserPrefix =
                TfStringStartsWith(primvar.name.GetString(), userAttrPrefix);
            bool hasRiAttributesPrefix =
                TfStringStartsWith(primvar.name.GetString(), riAttrPrefix);
            const bool hasPrimvarRiAttributesPrefix =
                    TfStringStartsWith(primvar.name.GetString(),primvarsPrefix);

            // Strip "primvars:" from the name
            TfToken primvarName = primvar.name;
            if (hasPrimvarRiAttributesPrefix) {
                const char *strippedName = primvar.name.GetText();
                strippedName += strlen(primvarsPrefix);
                primvarName = TfToken(strippedName);
                hasRiAttributesPrefix =
                      TfStringStartsWith(primvarName.GetString(), riAttrPrefix);
            }

            bool skipPrimvar = false;
            if constexpr (std::is_same<RtParamList, T>()) {
                // When we're looking for attributes on geometry instances,
                // they need to have either 'user:' or 'ri:attributes:' as a
                // prefix.
                if (!hasUserPrefix && !hasRiAttributesPrefix) {
                    skipPrimvar = true;
                } else if (hasRiAttributesPrefix) {
                    // For 'ri:attributes' we check if the attribute is a
                    // prototype attribute and if so omit it, since it
                    // was included with the primvars.
                    if (_IsPrototypeAttribute(primvarName)) {
                        skipPrimvar = true;
                    }
                }
            } else {
                // When we're looking for actual primvars, we skip the ones with
                // the 'user:' or 'ri:attributes:' prefix. Except for a specific
                // set of attributes that affect tessellation and dicing of the
                // prototype geometry and so it becomes part of the primvars.
                if (hasUserPrefix) {
                    skipPrimvar = true;
                } else if (hasRiAttributesPrefix) {
                    // If this ri attribute does not affect the prototype
                    // we skip
                    if (!_IsPrototypeAttribute(primvarName)) {
                        skipPrimvar = true;
                    }
                }
            }

            if (skipPrimvar) {
                continue;
            }

            if (hasRiAttributesPrefix) {
                const char *strippedName = primvarName.GetText();
                strippedName += strlen(riAttrPrefix);
                name = _GetPrmanPrimvarName(TfToken(strippedName), detail);
            } else {
                name = _GetPrmanPrimvarName(primvarName, detail);
            }

            // ri:attributes and primvars:ri:attributes primvars end up having
            // the same name, potentially causing collisions in the primvar list.
            // When both ri:attributes and primvar:ri:attributes versions of 
            // the same primvars exist, the primvar:ri:attributes version should
            // win out.
            if (hasRiAttributesPrefix && !hasPrimvarRiAttributesPrefix &&
                params.HasParam(name)) {
                continue;
            }
        } else {
            name = _GetPrmanPrimvarName(primvar.name, detail);
        }

        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> samples;
        sceneDelegate->SamplePrimvar(id, primvar.name, &samples);
        // XXX: The motion blur scene index plugin ensures that only a single
        // sample at offset 0 is returned for any primvar on which Prman does
        // not support motion samples. Currently, that's all primvars except P.
        // We call Resample() here because HdPrman also does not yet support
        // time-sampled primvars other than P.
        // HdPrman_Utils::SetPrimVarFromVtValue expects a single VtValue and no
        // mechanism exists to ensure all primvars are sampled at the same set
        // of times, which would be a Prman requirement since times are a
        // property of the whole RtPrimVarList.
        VtValue val = samples.Resample(time);

        TF_DEBUG(HDPRMAN_PRIMVARS)
            .Msg("HdPrman: <%s> %s %s \"%s\" (%s) = \"%s\"\n",
                 id.GetText(),
                 TfEnum::GetName(hdInterp).c_str(),
                 label.c_str(),
                 primvar.name.GetText(),
                 name.CStr(),
                 TfStringify(val).c_str());

        if (val.IsEmpty() ||
            (val.IsArrayValued() && val.GetArraySize() == 0)) {
            continue;
        }

        if (val.IsArrayValued() && 
            val.GetArraySize() != static_cast<size_t>(expectedSize)) {
            TF_WARN("<%s> %s '%s' size (%zu) did not match "
                    "expected (%d)", id.GetText(), label.c_str(),
                    primvar.name.GetText(), val.GetArraySize(), expectedSize);
            continue;
        }
        if constexpr(std::is_same<RtPrimVarList, T>()) {
            if (!HdPrman_Utils::SetPrimVarFromVtValue(name, val, detail,
                primvar.role, &params)) {
                TF_WARN("Ignoring unhandled primvar of type %s for %s.%s\n",
                    val.GetTypeName().c_str(), id.GetText(),
                    primvar.name.GetText());
            }
        } else {
            if (!HdPrman_Utils::SetParamFromVtValue(name, val, primvar.role,
                &params)) {
                TF_WARN("Ignoring unhandled attribute of type %s for %s.%s\n",
                    val.GetTypeName().c_str(), id.GetText(),
                    primvar.name.GetText());
            }
        }
    }
}

void
HdPrman_ConvertPrimvars(HdSceneDelegate *sceneDelegate, SdfPath const& id,
                        RtPrimVarList& primvars, int numUniform, int numVertex,
                        int numVarying, int numFaceVarying, float time)
{
    const HdInterpolation hdInterpValues[] = {
        HdInterpolationConstant,
        HdInterpolationUniform,
        HdInterpolationVertex,
        HdInterpolationVarying,
        HdInterpolationFaceVarying,
    };
    // The expected size of each interpolation mode. -1 means any size is
    // acceptable.
    const int primvarSizes[] = {
        1,
        numUniform,
        numVertex,
        numVarying,
        numFaceVarying
    };
    const int modeCount = 5;
    for (size_t i = 0; i < modeCount; ++i) {
        _Convert(sceneDelegate, id, hdInterpValues[i], primvars,
                 primvarSizes[i], time);
    }
}

void
HdPrman_TransferMaterialPrimvarOpinions(HdSceneDelegate *sceneDelegate,
                                        SdfPath const& materialId,
                                        RtPrimVarList& primvars)
{
    if (!materialId.IsEmpty()) {
        if (const HdSprim *sprim = sceneDelegate->GetRenderIndex().GetSprim(
            HdPrimTypeTokens->material, materialId)) {
            const HdPrmanMaterial *material =
                dynamic_cast<const HdPrmanMaterial*>(sprim);
            if (material && material->IsValid()) {
                const HdMaterialNetwork2 & matNetwork = 
                    material->GetMaterialNetwork();
                for (const auto & nodeIt : matNetwork.nodes) {
                    const HdMaterialNode2 & node = nodeIt.second;
                    if (node.nodeTypeId == _tokens->PrimvarPass) {
                        for (const auto &param : node.parameters) {
                            uint32_t paramId;
                            RtUString paramName = 
                                RtUString(param.first.GetText());
                            if (!primvars.GetParamId(paramName, paramId)) {
                                HdPrman_Utils::SetPrimVarFromVtValue(paramName,
                                    param.second, RtDetailType::k_constant,
                                    /*role*/TfToken(), &primvars);
                            }
                        }
                    }
                }
            }
        }
    }
}

RtParamList
HdPrman_RenderParam::ConvertAttributes(HdSceneDelegate *sceneDelegate,
                                   SdfPath const& id, bool isGeometry,
                                   bool *visible)
{
    RtParamList attrs;

    // Convert Hydra instance-rate primvars, and "user:" prefixed
    // constant  primvars, to Riley attributes.
    const HdInterpolation hdInterpValues[] = {
        HdInterpolationConstant,
    };
    for (HdInterpolation hdInterp: hdInterpValues) {
        _Convert(sceneDelegate, id, hdInterp, attrs, 1);
    }

    // Hydra id -> Riley Rix::k_identifier_name
    attrs.SetString(RixStr.k_identifier_name, RtUString(id.GetText()));

    // Hydra visibility -> Riley Rix::k_visibility
    bool vis = sceneDelegate->GetVisible(id);
    if (visible) {
        *visible = vis;
    }
    if (!vis) {
        attrs.SetInteger(RixStr.k_visibility_camera, 0);
        attrs.SetInteger(RixStr.k_visibility_indirect, 0);
        attrs.SetInteger(RixStr.k_visibility_transmission, 0);
    }

    // Hydra categories -> Riley k_grouping_membership
    VtArray<TfToken> categories = sceneDelegate->GetCategories(id);
    ConvertCategoriesToAttributes(id, categories, attrs);

    if (isGeometry) { 
        // Hydra cullStyle & doubleSided -> Riley k_Ri_Sides
        // Ri:Sides is most analogous to GL culling style. When Ri:Sides = 1,
        // prman will skip intersections on the back, with "back" determined by
        // winding order (Ri:Orientation). Prman's default value for Ri:Sides
        // is 2. By considering both cullStyle and doubleSided, we can accurately
        // reproduce all the Hydra cull styles. While usd does not surface cullStyle,
        // some Hydra constructs rely on cullStyle to achieve their intended looks,
        // e.g., the cards drawmode adapter.

        // TODO: (tgvarik) Check how Ri:ReverseOrientation interacts with
        //       displacement. What is intended when front-face culling is applied 
        //       to a surface with displacement? Should be vanishingly rare.

        const HdCullStyle cullStyle = sceneDelegate->GetCullStyle(id);
        switch (cullStyle) {
            case HdCullStyleNothing:
                attrs.SetInteger(RixStr.k_Ri_Sides, 2);
                break;
            case HdCullStyleFront:
                attrs.SetInteger(RixStr.k_Ri_ReverseOrientation, 1);
                // fallthrough
            case HdCullStyleBack:
                attrs.SetInteger(RixStr.k_Ri_Sides, 1);
                break;
            case HdCullStyleFrontUnlessDoubleSided:
                attrs.SetInteger(RixStr.k_Ri_ReverseOrientation, 
                    sceneDelegate->GetDoubleSided(id) ? 0 : 1);
                // fallthrough
            case HdCullStyleBackUnlessDoubleSided:
                attrs.SetInteger(RixStr.k_Ri_Sides, 
                    sceneDelegate->GetDoubleSided(id) ? 2 : 1);
                break;
            case HdCullStyleDontCare:
                // Noop. If the prim has no opinion on the matter,
                // defer to Prman default by not setting Ri:Sides.
                break;
        }
    

        // Double-sidedness in usd is a property of the gprim for legacy reasons.
        // Double-sidedness in prman is a property of the material. To achieve
        // consistency, we need to communicate the gprim's double-sidedness to
        // the material via an attribute, which allows the material to determine
        // whether it should shade both sides or just the front.

        // Integer primvars do not exist in prman, which is why we do this on
        // the attributes instead. Furthermore, all custom attributes like this
        // must be in the "user:" namespace to be accessible from the shader.
        attrs.SetInteger(
            RtUString("user:hydra:doubleSided"),
            sceneDelegate->GetDoubleSided(id) ? 1 : 0
        );
    }
    
    return attrs;
}

void
HdPrman_RenderParam::ConvertCategoriesToAttributes(
    SdfPath const& id,
    VtArray<TfToken> const& categories,
    RtParamList& attrs)
{
    if (categories.empty()) {
        attrs.SetString( RixStr.k_lightfilter_subset, RtUString("") );
        attrs.SetString( RixStr.k_lighting_subset, RtUString("default") );
        TF_DEBUG(HDPRMAN_LIGHT_LINKING)
            .Msg("HdPrman: <%s> no categories; lighting:subset = \"default\"\n",
                 id.GetText());
        return;
    }

    std::string membership;
    for (TfToken const& category : categories) {
        if (!membership.empty()) {
            membership += " ";
        }
        membership += category;
    }
    // Fetch incoming grouping:membership and tack it onto categories
    RtUString inputGrouping = RtUString("");
    attrs.GetString(RixStr.k_grouping_membership, inputGrouping);
    if (inputGrouping != RtUString("")) {
        std::string input = inputGrouping.CStr();
        membership += " " + input;
    }
    attrs.SetString( RixStr.k_grouping_membership,
                     RtUString(membership.c_str()) );
    TF_DEBUG(HDPRMAN_LIGHT_LINKING)
        .Msg("HdPrman: <%s> grouping:membership = \"%s\"\n",
             id.GetText(), membership.c_str());

    // Light linking:
    // Geometry subscribes to categories of lights illuminating it.
    // Take any categories used by a light as a lightLink param
    // and list as k_lighting_subset.
    std::string lightingSubset = "default";
    for (TfToken const& category: categories) {
        if (IsLightLinkUsed(category)) {
            if (!lightingSubset.empty()) {
                lightingSubset += " ";
            }
            lightingSubset += category;
        }
    }
    attrs.SetString( RixStr.k_lighting_subset,
                     RtUString(lightingSubset.c_str()) );
    TF_DEBUG(HDPRMAN_LIGHT_LINKING)
        .Msg("HdPrman: <%s> lighting:subset = \"%s\"\n",
             id.GetText(), lightingSubset.c_str());

    // Light filter linking:
    // Geometry subscribes to categories of light filters applied to it.
    // Take any categories used by a light filter as a lightFilterLink param
    // and list as k_lightfilter_subset.
    std::string lightFilterSubset = "default";
    for (TfToken const& category: categories) {
        if (IsLightFilterUsed(category)) {
            if (!lightFilterSubset.empty()) {
                lightFilterSubset += " ";
            }
            lightFilterSubset += category;
        }
    }
    attrs.SetString( RixStr.k_lightfilter_subset,
                      RtUString(lightFilterSubset.c_str()) );
    TF_DEBUG(HDPRMAN_LIGHT_LINKING)
        .Msg("HdPrman: <%s> lightFilter:subset = \"%s\"\n",
             id.GetText(), lightFilterSubset.c_str());
}

bool
HdPrman_ResolveMaterial(HdSceneDelegate *sceneDelegate,
                        SdfPath const& hdMaterialId,
                        riley::Riley *riley,
                        riley::MaterialId *materialId,
                        riley::DisplacementId *dispId)
{
    if (hdMaterialId != SdfPath()) {
        if (HdSprim *sprim = sceneDelegate->GetRenderIndex().GetSprim(
            HdPrimTypeTokens->material, hdMaterialId)) {
            if (HdPrmanMaterial *material =
                dynamic_cast<HdPrmanMaterial*>(sprim)) {
                // Resolving the material indicates that it is
                // actually in use, so we sync to Riley.
                material->SyncToRiley(sceneDelegate, riley);
                if (material->IsValid()) {
                    *materialId = material->GetMaterialId();
                    *dispId = material->GetDisplacementId();
                    return true;
                }
            }
        }
    }
    return false;
}

HdPrman_RenderParam::RileyCoordSysIdVecRefPtr
HdPrman_RenderParam::ConvertAndRetainCoordSysBindings(
    HdSceneDelegate *sceneDelegate,
    SdfPath const& id)
{
    // Query Hydra coordinate system bindings.
    HdIdVectorSharedPtr hdIdVecPtr = sceneDelegate->GetCoordSysBindings(id);
    if (!hdIdVecPtr) {
        return nullptr;
    }

    // We have bindings to convert.
    std::lock_guard<std::mutex> lock(_coordSysMutex);
    // Check for an existing converted binding vector.
    _HdToRileyCoordSysMap::const_iterator it =
        _hdToRileyCoordSysMap.find(hdIdVecPtr);
    if (it != _hdToRileyCoordSysMap.end()) {
        // Found an existing conversion.
        // Record an additional use on this geometry.
        _geomToHdCoordSysMap[id] = hdIdVecPtr;
        return it->second;
    }
    // Convert Hd ids to Riley id's.
    RileyCoordSysIdVec rileyIdVec;
    rileyIdVec.reserve(hdIdVecPtr->size());
    for (SdfPath const& hdId: *hdIdVecPtr) {
        // Look up sprim for binding.
        const HdSprim *sprim = sceneDelegate->GetRenderIndex()
            .GetSprim(HdPrimTypeTokens->coordSys, hdId);
        // Expect there to be an sprim with this id.
        if (TF_VERIFY(sprim)) {
            // Expect it to be an HdPrmanCoordSys.
            const HdPrmanCoordSys *prmanSprim =
                dynamic_cast<const HdPrmanCoordSys*>(sprim);
            if (TF_VERIFY(prmanSprim) && prmanSprim->IsValid()) {
                // Use the assigned Riley ID.
                rileyIdVec.push_back(prmanSprim->GetCoordSysId());
            }
        }
    }

    // Establish a cache entry.
    RileyCoordSysIdVecRefPtr rileyIdVecPtr =
        std::make_shared<RileyCoordSysIdVec>(rileyIdVec);
    _hdToRileyCoordSysMap[hdIdVecPtr] = rileyIdVecPtr;
    _geomToHdCoordSysMap[id] = hdIdVecPtr;
    return rileyIdVecPtr;
}

void
HdPrman_RenderParam::ReleaseCoordSysBindings(SdfPath const& id)
{
    std::lock_guard<std::mutex> lock(_coordSysMutex);
    _GeomToHdCoordSysMap::iterator geomIt = _geomToHdCoordSysMap.find(id);
    if (geomIt == _geomToHdCoordSysMap.end()) {
        // No cached bindings to release.
        return;
    }
    if (TF_VERIFY(geomIt->second) && geomIt->second.use_count() == 1) {
        // If this is the last geometry using this Riley vector,
        // we can release the cache entry.  This will free the vector.
        // (Note that the Riley coordinate system object lifetime
        // is managed by the HdPrmanCoordSys sprim.)
        _hdToRileyCoordSysMap.erase(geomIt->second);
    }
    _geomToHdCoordSysMap.erase(geomIt);
}

static bool
_Contains(
    HdRenderSettingsMap const &settings,
    TfToken const &key)
{
    return settings.find(key) != settings.end();
}

void
HdPrman_RenderParam::UpdateLegacyOptions()
{
    const HdRenderSettingsMap renderSettingsMap =
        _renderDelegate->GetRenderSettingsMap();
    RtParamList &options = GetLegacyOptions();

    VtValue batchCommandLine;

    for (auto const& entry : renderSettingsMap) {
        TfToken token = entry.first;
        VtValue val = entry.second;

        if (TfStringStartsWith(token.GetText(), "ri:")) {
            // Skip integrator settings.
            if (TfStringStartsWith(token.GetText(), "ri:integrator")) {
                continue;
            }

            // Strip "ri:" namespace from USD.
            RtUString riName;
            riName = RtUString(token.GetText()+3);

            // XXX there is currently no way to distinguish the type of a 
            // float3 setting (color, point, vector).  All float3 settings are
            // treated as float[3] until we have a way to determine the type. 
            HdPrman_Utils::SetParamFromVtValue(riName, val, TfToken(), &options);
        } else {
            
            // ri: namespaced settings win over custom settings tokens when
            // present.
            if (token == HdRenderSettingsTokens->convergedSamplesPerPixel) {
                if (!_Contains(renderSettingsMap,
                        _riOptionsTokens->riHiderMaxSamples)) {

                    VtValue vtInt = val.Cast<int>();
                    int maxSamples = TF_VERIFY(!vtInt.IsEmpty()) ?
                        vtInt.UncheckedGet<int>() : 64; // RenderMan default
                    options.SetInteger(RixStr.k_hider_maxsamples, maxSamples);
                }

            } else if (token == HdRenderSettingsTokens->convergedVariance) {
                if (!_Contains(renderSettingsMap,
                        _riOptionsTokens->riRiPixelVariance)) {
                    VtValue vtFloat = val.Cast<float>();
                    float pixelVariance = TF_VERIFY(!vtFloat.IsEmpty()) ?
                        vtFloat.UncheckedGet<float>() : 0.001f;
                    options.SetFloat(RixStr.k_Ri_PixelVariance, pixelVariance);
                }

            } else if (token == HdPrmanRenderSettingsTokens->pixelAspectRatio) {
                if (!_Contains(renderSettingsMap,
                        _riOptionsTokens->riRiFormatPixelAspectRatio)) {
                    options.SetFloat(RixStr.k_Ri_FormatPixelAspectRatio, 
                                    val.UncheckedGet<float>());
                }

            } else if (token == HdPrmanRenderSettingsTokens->resolution ) {
                if (!_Contains(renderSettingsMap,
                        _riOptionsTokens->riRiFormatResolution)) {
                    const GfVec2i& res = val.UncheckedGet<GfVec2i>();
                    options.SetIntegerArray(RixStr.k_Ri_FormatResolution, 
                                            res.data(), 2);
                }
            } else if (token == HdRenderSettingsTokens->threadLimit) {
                if (!_Contains(renderSettingsMap,
                        _riOptionsTokens->riLimitsThreads)) {
                    VtValue vtInt = val.Cast<int>();
                    if (!vtInt.IsEmpty()) {
                        options.SetInteger(RixStr.k_limits_threads,
                            vtInt.UncheckedGet<int>());
                    }
                }

            } else if (token == HdPrmanRenderSettingsTokens->batchCommandLine) {
                batchCommandLine = val;
            }
            // Note: HdPrmanRenderSettingsTokens->disableMotionBlur is handled in
            //       SetRileyShutterIntervalFromCameraContextCameraPath.  
        }
    }
    // Apply the batch command line settings last, so that they can
    // either intentionally override render settings, or sometimes be skipped
    // if the equivalent render setting exists, like for checkpointinterval.
    // Otherwise, since settings are in a hash map, it would be random
    // whether the command line settings or render settings win.
    SetBatchCommandLineArgs(batchCommandLine, &options);
}

void
HdPrman_RenderParam::SetIntegratorParamsFromRenderSettingsMap(
    HdPrmanRenderDelegate *renderDelegate,
    const std::string& integratorName,
    RtParamList& params)
{
    HdRenderSettingsMap renderSettings = renderDelegate->GetRenderSettingsMap();

    TfToken prefix(std::string("ri:integrator:") + integratorName + ":");
    for (auto const& entry : renderSettings) {
        if (TfStringStartsWith(entry.first.GetText(), prefix.GetText())) {
            // Strip namespace prefix from USD.
            RtUString riName(entry.first.GetText() + prefix.size());
            HdPrman_Utils::SetParamFromVtValue(riName, entry.second,
                TfToken(), &params);
        }
    }        
}

void
HdPrman_RenderParam::SetBatchCommandLineArgs(
    VtValue const &cmdLine,
    RtParamList * options)
{
    if (!cmdLine.IsHolding<VtArray<std::string>>()) {
        return;
    }
    bool doSnapshot = false;
    const VtArray<std::string>& v =
        cmdLine.UncheckedGet<VtArray<std::string>>();
    for (auto i = v.cbegin(),
             end = v.cend();
         i != end; ++i) {
        if (*i == "--snapshot") {
            ++i;
            if(i == end) {
                TF_WARN("No value found for --snapshot argument\n");
                break;
            }
            RtUString checkpointinterval;
            options->GetString(RixStr.k_checkpoint_interval,
                              checkpointinterval);
            // Checkpoint interval from render settings wins
            // because normally it's not set, so if it's set the user
            // chose that, and it accepts more expressive values
            // than the --snapshot arg. Also, Solaris always puts
            // the --snapshot arg on the commandline, so even though
            // it seems like it would make sense for command line to win,
            // users should simply not set the checkpoint render settings
            // if they want to use --snapshot.
            if (checkpointinterval.Empty()) {
                doSnapshot = true;
                const std::vector<std::string> toks =
                    TfStringTokenize(*i, ",");
                std::vector<RtUString> us;
                us.reserve(toks.size());
                std::transform(toks.begin(), toks.end(),
                               std::back_inserter(us),
                               [](const std::string& str) -> RtUString
                               { return RtUString(str.c_str()); } );
                options->SetStringArray(RixStr.k_checkpoint_interval,
                                       us.data(), us.size());
            }
        }
        else if (*i == "--threads") {
            ++i;
            if(i == end) {
                TF_WARN("No value found for --threads argument\n");
                break;
            }
            try {
                const int n = stoi(*i);
                options->SetInteger(RixStr.k_limits_threads, n);
            }
            catch (const std::invalid_argument &e) {
                TF_WARN("Invalid argument to --threads\n");
            }
            catch (const std::out_of_range &e) {
                TF_WARN("Invalid argument to --threads\n");
            }
        } else if (*i == "--timelimit") {
            ++i;
            if(i == end) {
                TF_WARN("No value found for --timelimit argument\n");
                break;
            }
            RtUString exitat;
            options->GetString(RixStr.k_checkpoint_exitat, exitat);
            // Checkpoint exitat from render settings wins
            if (exitat.Empty()) {
                options->SetString(RixStr.k_checkpoint_exitat,
                                  RtUString(i->c_str()));
            }
        } else if (*i == "--output" || *i == "-o") {
            ++i;
            if(i == end) {
                TF_WARN("No value found for --output argument\n");
                break;
            }
            // Husk accepts comma separated list for multiple outputs
            _outputNames = TfStringTokenize(i->c_str(), ",");
        }
    }

    // Force incremental to be enabled when checkpointing
    RtUString checkpointinterval;
    options->GetString(RixStr.k_checkpoint_interval,
                      checkpointinterval);
    if(!checkpointinterval.Empty() || doSnapshot) {
        options->SetInteger(RixStr.k_hider_incremental, 1);
    }
}

void
HdPrman_RenderParam::SetIntegratorParamsFromCamera(
    HdPrmanRenderDelegate *renderDelegate,
    const HdPrmanCamera *camera,
    std::string const& integratorName,
    RtParamList &integratorParams)
{
    for(auto const& cb: *_integratorCameraCallbacks) {
        cb(renderDelegate, camera, integratorName, integratorParams);
    }
}

void 
HdPrman_RenderParam::RegisterIntegratorCallbackForCamera(
    IntegratorCameraCallback const& callback)
{
   _integratorCameraCallbacks->push_back(callback);
}

void
HdPrman_RenderParam::_CreateStatsSession(void)
{
    // Set log level for diagnostics relating to initialization. If we succeed in loading a
    // config file then the log level specified in the config file will take precedence.
    stats::Logger::LogLevel statsDebugLevel = stats::GlobalLogger()->DefaultLogLevel();
    stats::SetGlobalLogLevel(statsDebugLevel);
    stats::SetGlobalLogLevel(stats::Logger::k_debug);

    // Build default listener plugin search path
    std::string listenerPath(".");
    char* rmanTreePath = getenv("RMANTREE");
    if (rmanTreePath)
    {
        listenerPath += ":";
        listenerPath += rmanTreePath;
        listenerPath += "/lib/plugins/listeners";
    }

    stats::SetListenerPluginSearchPath(listenerPath);

    // Create our stats Session config.
    std::string configFilename("stats.ini");
    std::string configSearchPathStr;
    char* configSearchPathOverride = getenv("RMAN_STATS_CONFIG_PATH");
    if (configSearchPathOverride)
    {
        configSearchPathStr = std::string(configSearchPathOverride);
    }

    // This could eventually come from a GUI so we go through
    // the motion of checking to see if we have a filename.
    stats::SessionConfig sessionConfig("HDPRman Stats Session");
    if (!configFilename.empty() && !configSearchPathStr.empty())
    {
        // Try to resolve the file in the given path and load the
        // configuration data. If it fails to find the config
        // file we'll just fall back onto the defaults.
        sessionConfig.LoadConfigFile(configSearchPathStr, configFilename);
    }

    // Instantiate a stats Session from config object.
    _statsSession = &stats::AddSession(sessionConfig);

    // Validate and inform
    _statsSession->LogInfo("HDPRMan", "Created Roz stats session '" +
                                      _statsSession->GetName() + "'.");
}

void
HdPrman_RenderParam::_CreateRiley(const std::string &rileyVariant,
    const std::string &xpuDevices,
    const std::vector<std::string>& extraArgs)
{
    _rix = RixGetContext();
    if (!_rix) {
        TF_RUNTIME_ERROR("Could not initialize Rix API.");
        return;
    }
    _ri = (RixRiCtl*)_rix->GetRixInterface(k_RixRiCtl);
    if (!_ri) {
        TF_RUNTIME_ERROR("Could not initialize Ri API.");
        return;
    }

    // Must invoke PRManBegin() before we start using Riley.
    // Turning off unwanted statistics warnings
    // TODO: Fix incorrect tear-down handling of these statistics in 
    // interactive contexts as described in PRMAN-2353

    std::vector<std::string> sArgs;
    sArgs.push_back("hdPrman");
    sArgs.push_back("-woff");
    sArgs.push_back("R56008,R56009");
    sArgs.push_back("-statssession");
    sArgs.push_back(_statsSession->GetName());
    sArgs.insert(std::end(sArgs), std::begin(extraArgs), std::end(extraArgs));

    std::vector<const char*> cArgs;

    // PRManBegin expects array of char* rather than std::string
    cArgs.reserve(sArgs.size());
    std::transform(sArgs.cbegin(), sArgs.cend(), std::back_inserter(cArgs),
                   [](const std::string& str) { return str.c_str();} );

    _ri->PRManBegin(cArgs.size(), const_cast<char **>(cArgs.data()));

    // Register an Xcpt handler
    RixXcpt* rix_xcpt = (RixXcpt*)_rix->GetRixInterface(k_RixXcpt);
    rix_xcpt->Register(&_xcpt);

    // Register progress callback
     RixEventCallbacks* rix_event_callbacks =
        (RixEventCallbacks*)_rix->GetRixInterface(k_RixEventCallbacks);
     rix_event_callbacks->RegisterCallback(RixEventCallbacks::k_Progress,
                                           _ProgressCallback, this);

    // Populate RixStr struct
    RixSymbolResolver* sym = (RixSymbolResolver*)_rix->GetRixInterface(
        k_RixSymbolResolver);
    sym->ResolvePredefinedStrings(RixStr);

    // Sanity check symbol resolution with a canary symbol, shutterTime.
    // This can catch accidental linking with incompatible versions.
    TF_VERIFY(RixStr.k_shutterOpenTime == RtUString("shutterOpenTime"),
              "Renderman API tokens do not match expected values.  "
              "There may be a compile/link version mismatch.");

    // Acquire Riley instance.
    _mgr = (RixRileyManager*)_rix->GetRixInterface(k_RixRileyManager);

    _xpu = (!rileyVariant.empty() ||
            (rileyVariant.find("xpu") != std::string::npos));

    // Decide whether to use the CPU, GPU, or both
    RtParamList paramList;
    if (_xpu && !xpuDevices.empty()) {
        static const RtUString cpuConfig("xpu:cpuconfig");
        static const RtUString gpuConfig("xpu:gpuconfig");

        const bool useCpu = xpuDevices.find("cpu") != std::string::npos;
        paramList.SetInteger(cpuConfig, useCpu ? 1 : 0);

        const bool useGpu = xpuDevices.find("gpu") != std::string::npos;
        if (useGpu) {
            paramList.SetIntegerArray(gpuConfig, 
                                _xpuGpuConfig.data(), _xpuGpuConfig.size());
        }
    }

    _riley = _mgr->CreateRiley(RtUString(rileyVariant.c_str()), paramList);

    if(!_riley) {
        TF_RUNTIME_ERROR("Could not initialize riley API.");
        return;
    }
}


static
riley::RenderOutputType
_ToRenderOutputType(const TfToken &t)
{
    if (t == TfToken("color3f")) {
        return riley::RenderOutputType::k_Color;
    } else if (t == TfToken("float3") ||
               t == TfToken("normal3f") ||
               t == TfToken("point3f") ||
               t == TfToken("vector3f")) {
        return riley::RenderOutputType::k_Vector;
    } else if (t == TfToken("float")) {
        return riley::RenderOutputType::k_Float;
    } else if (t == TfToken("int")) {
        return riley::RenderOutputType::k_Integer;
    } else {
        TF_RUNTIME_ERROR("Unimplemented renderVar dataType '%s'; "
                         "skipping", t.GetText());
        return riley::RenderOutputType::k_Integer;
    }
}

// Helper to convert a dictionary of Hydra settings to Riley params,
// stripping the namespace prefix if provided.
static
RtParamList
_ToRtParamList(VtDictionary const& dict, TfToken prefix=TfToken())
{
    RtParamList params;
    for (auto const& entry: dict) {
        std::string key = entry.first;

        // EXR metadata transformation:
        // Keys of the format "ri:exrheader:A:B:C"
        // will be changed to "exrheader_A/B/C"
        // for use with the d_openexr display driver conventions.
        if (TfStringStartsWith(key, "ri:exrheader:")) {
            key = TfStringReplace(key, "ri:exrheader:", "exrheader_");
            for (char &c: key) {
                if (c == ':') {
                    c = '/';
                }
            }
        }

        // Remove namespace prefix
        if (TfStringStartsWith(key, prefix.GetString())) {
            key = key.substr(prefix.size());
        }

        RtUString riName(key.c_str());
        HdPrman_Utils::SetParamFromVtValue(riName, entry.second,
                                           /* role = */ TfToken(), &params);
    }
    return params;
}

// TODO this is not a robust solution
static
RtUString
_GetOutputDisplayDriverType(const TfToken &name)
{
    // get output display driver type
    // TODO this is not a robust solution
    static const std::map<std::string,TfToken> extToDisplayDriver{
        { std::string("exr"),  TfToken("openexr") },
        { std::string("tif"),  TfToken("tiff") },
        { std::string("tiff"), TfToken("tiff") },
        { std::string("png"),  TfToken("png") }
    };
    
    const std::string outputExt = TfGetExtension(name.GetString());
    const TfToken displayFormat = extToDisplayDriver.at(outputExt);
    return RtUString(displayFormat.GetText());
}

static
HdPrman_RenderViewDesc
_ComputeRenderViewDesc(
    const VtDictionary &renderSpec,
    const riley::CameraId cameraId,
    const riley::IntegratorId integratorId,
    const riley::SampleFilterList &sampleFilterList,
    const riley::DisplayFilterList &displayFilterList)
{
    HdPrman_RenderViewDesc renderViewDesc;

    renderViewDesc.cameraId = cameraId;
    renderViewDesc.integratorId = integratorId;
    renderViewDesc.resolution = _fallbackResolution;
    renderViewDesc.sampleFilterList = sampleFilterList;
    renderViewDesc.displayFilterList = displayFilterList;

    const std::vector<VtValue> &renderVars =
        VtDictionaryGet<std::vector<VtValue>>(
            renderSpec,
            HdPrmanExperimentalRenderSpecTokens->renderVars);
    
    for (const VtValue &renderVarVal : renderVars) {
        const VtDictionary renderVar = renderVarVal.Get<VtDictionary>();

        const std::string &nameStr =
            VtDictionaryGet<std::string>(
                renderVar,
                HdPrmanExperimentalRenderSpecTokens->name);
        const std::string &sourceNameStr =
            VtDictionaryGet<std::string>(
                renderVar,
                HdPrmanExperimentalRenderSpecTokens->sourceName,
                VtDefault = nameStr);
        const TfToken sourceType =
            VtDictionaryGet<TfToken>(
                renderVar,
                HdPrmanExperimentalRenderSpecTokens->sourceType,
                VtDefault = TfToken());

        // Map renderVar to RenderMan AOV name and source.
        // For LPE's, we use the name of the prim rather than the LPE,
        // and include an "lpe:" prefix on the source.
        const RtUString aovName( (sourceType == _tokens->lpe)
            ? nameStr.c_str()
            : sourceNameStr.c_str());
        const RtUString sourceName( (sourceType == _tokens->lpe)
            ? ("lpe:" + sourceNameStr).c_str()
            : sourceNameStr.c_str());

        HdPrman_RenderViewDesc::RenderOutputDesc renderOutputDesc;
        renderOutputDesc.name = aovName;
        renderOutputDesc.type = _ToRenderOutputType(
            TfToken(
                VtDictionaryGet<std::string>(
                    renderVar,
                    HdPrmanExperimentalRenderSpecTokens->type)));
        renderOutputDesc.sourceName = sourceName;
        renderOutputDesc.rule = RixStr.k_filter;
        renderOutputDesc.params = _ToRtParamList(
            VtDictionaryGet<VtDictionary>(
                renderVar,
                HdPrmanExperimentalRenderSpecTokens->params,
                VtDefault = VtDictionary()),
            _tokens->riDisplayChannelNamespace);
        renderViewDesc.renderOutputDescs.push_back(renderOutputDesc);
    }
    
    const std::vector<VtValue> & renderProducts =
        VtDictionaryGet<std::vector<VtValue>>(
            renderSpec,
            HdPrmanExperimentalRenderSpecTokens->renderProducts);

    for (const VtValue &renderProductVal : renderProducts) {

        const VtDictionary &renderProduct = renderProductVal.Get<VtDictionary>();

        HdPrman_RenderViewDesc::DisplayDesc displayDesc;

        const TfToken name(
            VtDictionaryGet<std::string>(
                renderProduct,
                HdPrmanExperimentalRenderSpecTokens->name));

        displayDesc.name = RtUString(name.GetText());
        displayDesc.driver = _GetOutputDisplayDriverType(name);

        displayDesc.params = _ToRtParamList(
            VtDictionaryGet<VtDictionary>(
                renderProduct,
                HdPrmanExperimentalRenderSpecTokens->params,
                VtDefault = VtDictionary()),
            _tokens->riDisplayDriverNamespace);

        const VtIntArray &renderVarIndices =
            VtDictionaryGet<VtIntArray>(
                renderProduct,
                HdPrmanExperimentalRenderSpecTokens->renderVarIndices);
        for (const int renderVarIndex : renderVarIndices) {
            displayDesc.renderOutputIndices.push_back(renderVarIndex);
        }
        renderViewDesc.displayDescs.push_back(displayDesc);
    }

    return renderViewDesc;
}

// Forward declaration of helper to create Render Output in the RenderViewDesc
static RtUString
_AddRenderOutput(RtUString aovName, 
    const TfToken &dataType, HdFormat aovFormat, 
    RtUString sourceName, const RtParamList &params,
    std::vector<HdPrman_RenderViewDesc::RenderOutputDesc> *renderOutputDescs,
    std::vector<size_t> *renderOutputIndices);

static
HdPrman_RenderViewDesc
_ComputeRenderViewDesc(
    HdRenderSettings::RenderProduct const &product,
    const riley::CameraId cameraId,
    const riley::IntegratorId integratorId,
    const riley::SampleFilterList &sampleFilterList,
    const riley::DisplayFilterList &displayFilterList)
{
    HdPrman_RenderViewDesc renderViewDesc;
    renderViewDesc.cameraId = cameraId;
    renderViewDesc.integratorId = integratorId;
    renderViewDesc.sampleFilterList = sampleFilterList;
    renderViewDesc.displayFilterList = displayFilterList;
    renderViewDesc.resolution = product.resolution;

    /* RenderProduct */
    int renderVarIndex = 0;
    std::map<SdfPath, int> seenRenderVars;

    // Create a DisplayDesc for this RenderProduct
    HdPrman_RenderViewDesc::DisplayDesc displayDesc;
    displayDesc.name = RtUString(product.name.GetText());
    displayDesc.params = _ToRtParamList(product.namespacedSettings,
        _tokens->riDisplayDriverNamespace);
    displayDesc.driver = _GetOutputDisplayDriverType(product.name);

    /* RenderVar */
    for (const HdRenderSettings::RenderProduct::RenderVar &renderVar :
            product.renderVars) {
        // Store the index to this RenderVar from all the renderOutputDesc's 
        // saved on this renderViewDesc
        auto renderVarIt = seenRenderVars.find(renderVar.varPath);
        if (renderVarIt != seenRenderVars.end()) {
            displayDesc.renderOutputIndices.push_back(renderVarIt->second);
            continue;
        } 
        seenRenderVars.insert(
            std::pair<SdfPath, int>(renderVar.varPath, renderVarIndex));
        displayDesc.renderOutputIndices.push_back(renderVarIndex);
        renderVarIndex++;

        // Map renderVar to RenderMan AOV name and source.
        // For LPE's, we use the name of the prim rather than the LPE,
        // and include an "lpe:" prefix on the source.
        std::string aovNameStr = (renderVar.sourceType == _tokens->lpe)
            ? renderVar.varPath.GetName()
            : renderVar.sourceName;
        std::string sourceNameStr = (renderVar.sourceType == _tokens->lpe) 
            ? "lpe:" + renderVar.sourceName
            : renderVar.sourceName;
        const RtUString aovName(aovNameStr.c_str());
        const RtUString sourceName(sourceNameStr.c_str());

        // Create a RenderOutputDesc for this RenderVar and add it to the 
        // renderViewDesc.
        // Note that we are not using the renderOutputIndices passed into 
        // this function, we are instead relying on the indices stored above
        std::vector<size_t> renderOutputIndices;
        _AddRenderOutput(aovName, 
                        renderVar.dataType, 
                        HdFormatInvalid, // using renderVar.dataType
                        sourceName, 
                        _ToRtParamList(renderVar.namespacedSettings,
                                       _tokens->riDisplayChannelNamespace),
                        &renderViewDesc.renderOutputDescs,
                        &renderOutputIndices);
    }
    renderViewDesc.displayDescs.push_back(displayDesc);

    return renderViewDesc;
}

void
HdPrman_RenderParam::CreateRenderViewFromRenderSpec(
    const VtDictionary &renderSpec)
{
    const HdPrman_RenderViewDesc renderViewDesc =
        _ComputeRenderViewDesc(
            renderSpec,
            GetCameraContext().GetCameraId(),
            GetActiveIntegratorId(),
            GetSampleFilterList(),
            GetDisplayFilterList());

    TF_DEBUG(HDPRMAN_RENDER_PASS)
        .Msg("Create Riley RenderView from the RenderSpec.\n");
                
    GetRenderViewContext().CreateRenderView(renderViewDesc, AcquireRiley());
}

/// XXX This should eventually replace the above use of the RenderSpec
void 
HdPrman_RenderParam::CreateRenderViewFromRenderSettingsProduct(
    HdRenderSettings::RenderProduct const &product,
    HdPrman_RenderViewContext *renderViewContext)
{
    // XXX Ideally, the render terminals and camera context are provided as
    //     arguments. They are currently managed by render param.
    const HdPrman_RenderViewDesc renderViewDesc =
        _ComputeRenderViewDesc(
            product,
            GetCameraContext().GetCameraId(), 
            GetActiveIntegratorId(), 
            GetSampleFilterList(),
            GetDisplayFilterList());

    renderViewContext->CreateRenderView(renderViewDesc, AcquireRiley());

}

void
HdPrman_RenderParam::_DestroyRiley()
{
     RixEventCallbacks* rix_event_callbacks =
        (RixEventCallbacks*)_rix->GetRixInterface(k_RixEventCallbacks);
     rix_event_callbacks->UnregisterCallback(RixEventCallbacks::k_Progress,
                                             _ProgressCallback, this);

    if (_mgr) {
        if (_riley) {
            // Riley/RIS crashes if SetOptions hasn't been called prior to
            // destroying the riley instance.
            if (!_initRileyOptions) {
                TF_DEBUG(HDPRMAN_RENDER_SETTINGS).Msg(
                    "[DestroyRiley] Calling SetOptions to workaround crash.\n");
                riley::Riley * const riley = AcquireRiley();
                riley->SetOptions(RtParamList());
            }
            _mgr->DestroyRiley(_riley);
        }
        _mgr = nullptr;
    }

    _riley = nullptr;

    if (_rix) {
        RixXcpt* rix_xcpt = (RixXcpt*)_rix->GetRixInterface(k_RixXcpt);
        rix_xcpt->Unregister(&_xcpt);
    }

    if (_ri) {
        _ri->PRManEnd();
        _ri = nullptr;
    }
}

void
HdPrman_RenderParam::_DestroyStatsSession(void)
{
    if (_statsSession)
    {
        stats::RemoveSession(*_statsSession);
        _statsSession = nullptr;
    }
}

static
RtParamList
_ComputeVolumeNodeParams()
{
    static const RtUString us_densityFloatPrimVar("densityFloatPrimVar");
    static const RtUString us_density("density");
    static const RtUString us_diffuseColor("diffuseColor");

    RtParamList result;
    result.SetString(
        us_densityFloatPrimVar, us_density);
    // 18% albedo chosen to match Storm's fallback volume shader.
    result.SetColor(
        us_diffuseColor, RtColorRGB(0.18, 0.18, 0.18));
    return result;
}

void
HdPrman_RenderParam::_CreateFallbackMaterials()
{
    // Default material
    {
        std::vector<riley::ShadingNode> materialNodes;
        HdPrman_ConvertHdMaterialNetwork2ToRmanNodes(
            HdPrmanMaterial_GetFallbackSurfaceMaterialNetwork(),
            SdfPath("/PxrSurface"), // We assume this terminal name here
            &materialNodes
        );
        _fallbackMaterialId = _riley->CreateMaterial(
            riley::UserId(stats::AddDataLocation(materialNodes[0].name.CStr()).GetValue()),
            {static_cast<uint32_t>(materialNodes.size()), materialNodes.data()},
            RtParamList());
    }

    // Volume default material
    {
        static const RtUString us_PxrVolume("PxrVolume");
        static const RtUString us_simpleVolume("simpleVolume");

        const std::vector<riley::ShadingNode> materialNodes{
            riley::ShadingNode{
                riley::ShadingNode::Type::k_Bxdf,
                us_PxrVolume,
                us_simpleVolume,
                _ComputeVolumeNodeParams()}};
        _fallbackVolumeMaterialId = _riley->CreateMaterial(
            riley::UserId(stats::AddDataLocation(materialNodes[0].name.CStr()).GetValue()),
            {static_cast<uint32_t>(materialNodes.size()), materialNodes.data()},
            RtParamList());
    }
}

void
HdPrman_RenderParam::SetLastLegacySettingsVersion(const int version)
{
    _lastLegacySettingsVersion = version;
}

void
HdPrman_RenderParam::SetResolution(GfVec2i const & resolution)
{
    _resolution = resolution;
}

void
HdPrman_RenderParam::InvalidateTexture(const std::string &path)
{
    AcquireRiley();

    _ri->InvalidateTexture(RtUString(path.c_str()));
}

riley::ShadingNode
HdPrman_RenderParam::_ComputeIntegratorNode(
    HdRenderDelegate * const renderDelegate,
    const HdPrmanCamera * const cam)
{
    // Use the integrator node from a terminal connection on the 
    // renderSettingsPrim if we can
    if (!GetRenderSettingsIntegratorPath().IsEmpty()) {

        // Create Integrator Riley Node
        const TfToken integratorNodeType =
            GetRenderSettingsIntegratorNode().nodeTypeId;
        riley::ShadingNode rileyIntegratorNode;
        rileyIntegratorNode.type = riley::ShadingNode::Type::k_Integrator;
        rileyIntegratorNode.name = RtUString(integratorNodeType.GetText());
        rileyIntegratorNode.handle = RtUString(integratorNodeType.GetText());

        // Initialize the Integrator parameters 
        const TfToken prefix("ri:");
        for (const auto &param : GetRenderSettingsIntegratorNode().parameters) {
            // Strip the 'ri' namespace before setting the param
            if (TfStringStartsWith(param.first.GetText(), prefix.GetText())) {
                RtUString riName(param.first.GetText() + prefix.size());
                HdPrman_Utils::SetParamFromVtValue(riName, param.second,
                    TfToken(), &rileyIntegratorNode.params);
            }
        }

        if (cam) {
            SetIntegratorParamsFromCamera(
                static_cast<HdPrmanRenderDelegate*>(renderDelegate),
                cam,
                integratorNodeType.GetString(),
                rileyIntegratorNode.params);
        }
        return rileyIntegratorNode;
    }

    const std::string &integratorName =
        renderDelegate->GetRenderSetting<std::string>(
            HdPrmanRenderSettingsTokens->integratorName,
            HdPrmanIntegratorTokens->PxrPathTracer.GetString());

    const RtUString rtIntegratorName(integratorName.c_str());

    // If the settings map / env var say to use PbsPathTracer,
    // we'll turn on volume aggregate rendering.
    if (integratorName == HdPrmanIntegratorTokens->PbsPathTracer.GetString()) {
        HdPrman_Utils::SetParamFromVtValue(RtUString("volumeAggregate"),
            VtValue(4), TfToken(), &_integratorParams);
    }

    SetIntegratorParamsFromRenderSettingsMap(
        static_cast<HdPrmanRenderDelegate*>(renderDelegate),
        integratorName,
        _integratorParams);

    if (cam) {
        SetIntegratorParamsFromCamera(
            static_cast<HdPrmanRenderDelegate*>(renderDelegate),
            cam,
            integratorName,
            _integratorParams);
    }

    return riley::ShadingNode{
        riley::ShadingNode::Type::k_Integrator,
        rtIntegratorName,
        rtIntegratorName,
        _integratorParams};
}

void
HdPrman_RenderParam::_CreateIntegrator(HdRenderDelegate * const renderDelegate)
{
    // Called before we have access to the camera Sprim, so we ignore
    // integrator opinions coming from the camera here. They will be
    // consumed in UpdateIntegrator.
    static const HdPrmanCamera * const camera = nullptr;

    riley::ShadingNode integratorNode(
        _ComputeIntegratorNode(renderDelegate, camera));
    _integratorId = _riley->CreateIntegrator(
        riley::UserId(
            stats::AddDataLocation(integratorNode.name.CStr()).GetValue()),
        integratorNode);
    
    TF_VERIFY(_integratorId != riley::IntegratorId::InvalidId());

    _activeIntegratorId = _integratorId;
}

void
HdPrman_RenderParam::UpdateIntegrator(const HdRenderIndex * const renderIndex)
{
    if (!TF_VERIFY(_integratorId != riley::IntegratorId::InvalidId())) {
        return;
    }

    const riley::ShadingNode node = _ComputeIntegratorNode(
        renderIndex->GetRenderDelegate(),
        _cameraContext.GetCamera(renderIndex));

    AcquireRiley()->ModifyIntegrator(_integratorId, &node);
}

void 
HdPrman_RenderParam::_RenderThreadCallback()
{
    static RtUString const US_RENDERMODE = RtUString("renderMode");
    static RtUString const US_INTERACTIVE = RtUString("interactive");

    // Note: this is currently hard-coded because hdprman currently 
    // creates only one single camera (via the camera context).
    // When this changes, we will need to make sure 
    // the correct name is used here.
    RtUString const &defaultReferenceCamera =
        GetCameraContext().GetCameraName();

    RtParamList renderOptions;
    renderOptions.SetString(US_RENDERMODE, US_INTERACTIVE);
    renderOptions.SetString(
        RixStr.k_dice_referencecamera, 
        defaultReferenceCamera);

    bool renderComplete = false;
    while (!renderComplete) {
        while (_renderThread->IsPauseRequested()) {
            if (_renderThread->IsStopRequested()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (_renderThread->IsStopRequested()) {
            break;
        }

        HdPrman_RenderViewContext &ctx = GetRenderViewContext();

        const riley::RenderViewId renderViewIds[] = { ctx.GetRenderViewId() };

        _riley->Render(
            { static_cast<uint32_t>(TfArraySize(renderViewIds)),
              renderViewIds },
            renderOptions);

        // If a pause was requested, we may have stopped early
        renderComplete = !_renderThread->IsPauseDirty();
    }
}

void 
HdPrman_RenderParam::_ProgressCallback(RixEventCallbacks::Event,
                                       RtConstPointer data, RtPointer clientData)
{
    int const* pp = static_cast<int const*>(data);
    HdPrman_RenderParam *param = static_cast<HdPrman_RenderParam*>(clientData);
    param->_progressPercent = *pp;

    if (!param->IsInteractive()) {
        // XXX Placeholder to simulate RenderMan's built-in writeProgress
        // option, until iether HdPrman can pass that in, and/or it gets
        // replaced with Roz-based client-side progress reporting
        printf("R90000  %3i%%\n", param->_progressPercent);
    }
}

bool 
HdPrman_RenderParam::IsValid() const
{
    return _riley;
}

void 
HdPrman_RenderParam::Begin(HdPrmanRenderDelegate *renderDelegate)
{
    _envOptions = HdPrman_Utils::GetRileyOptionsFromEnvironment();
    _fallbackOptions = HdPrman_Utils::GetDefaultRileyOptions();
    // Initialize legacy options from the render settings map.
    UpdateLegacyOptions();
    
    // Force initialization of Riley scene options.
    // (see related comments in SetRileyOptions)
    if (!HdRenderIndex::IsSceneIndexEmulationEnabled() ||
        !TfGetEnvSetting(HD_PRMAN_DEFER_SET_OPTIONS))
    {
        SetRileyOptions();
    }
        
    // Set the camera path before the first sync so that
    // HdPrmanCamera::Sync can detect whether it is syncing the
    // current camera and needs to set the riley shutter interval
    // which needs to be set before any time-sampled primvars are
    // synced. This is a workaround that is necessary only when a well-formed
    // render settings prim isn't available.
    //
    {
        const VtDictionary &renderSpec =
            renderDelegate->GetRenderSetting<VtDictionary>(
                HdPrmanRenderSettingsTokens->experimentalRenderSpec,
                VtDictionary());
        SdfPath cameraPath = VtDictionaryGet<SdfPath>(
            renderSpec,
            HdPrmanExperimentalRenderSpecTokens->camera,
            VtDefault = SdfPath());
        GetCameraContext().SetCameraPath(cameraPath);
    }
}

// See comment in SetRileyOptions on when this function needs to be called.
void
HdPrman_RenderParam::_CreateInternalPrims()
{
    GetCameraContext().CreateRileyCamera(
        AcquireRiley(), HdPrman_CameraContext::GetDefaultReferenceCameraName());

    _CreateFallbackMaterials();

    _CreateIntegrator(_renderDelegate);
    _CreateQuickIntegrator(_renderDelegate);
    _activeIntegratorId = GetIntegratorId();
}

static void
_DeleteAndResetMaterial(
    riley::Riley * const riley,
    riley::MaterialId *id)
{
    if (*id != riley::MaterialId::InvalidId()) {
        riley->DeleteMaterial(*id);
        *id = riley::MaterialId::InvalidId();
    }
}

static void
_DeleteAndResetIntegrator(
    riley::Riley * const riley,
    riley::IntegratorId *id)
{
    if (*id != riley::IntegratorId::InvalidId()) {
        riley->DeleteIntegrator(*id);
        *id = riley::IntegratorId::InvalidId();
    }
}

static void
_DeleteAndResetSampleFilter(
    riley::Riley * const riley,
    riley::SampleFilterId *id)
{
    if (*id != riley::SampleFilterId::InvalidId()) {
        riley->DeleteSampleFilter(*id);
        *id = riley::SampleFilterId::InvalidId();
    }
}

static void
_DeleteAndResetDisplayFilter(
    riley::Riley * const riley,
    riley::DisplayFilterId *id)
{
    if (*id != riley::DisplayFilterId::InvalidId()) {
        riley->DeleteDisplayFilter(*id);
        *id = riley::DisplayFilterId::InvalidId();
    }
}

void
HdPrman_RenderParam::_DeleteInternalPrims()
{
    riley::Riley * const riley = AcquireRiley();

    // Renderview has a handle to the camera, so delete it first.
    GetRenderViewContext().DeleteRenderView(riley);
    GetCameraContext().DeleteRileyCameraAndClipPlanes(riley);

    _DeleteAndResetMaterial(riley, &_fallbackMaterialId);
    _DeleteAndResetMaterial(riley, &_fallbackVolumeMaterialId);
    _DeleteAndResetIntegrator(riley, &_integratorId);
    _DeleteAndResetIntegrator(riley, &_quickIntegratorId);
    _DeleteAndResetSampleFilter(riley, &_sampleFiltersId);
    _DeleteAndResetDisplayFilter(riley, &_displayFiltersId);
}

void
HdPrman_RenderParam::SetRenderSettingsPrimOptions(
    RtParamList const &params)
{
    _renderSettingsPrimOptions = params;

    TF_DEBUG(HDPRMAN_RENDER_SETTINGS).Msg(
        "Updating render settings param list \n %s\n",
        HdPrmanDebugUtil::RtParamListToString(params).c_str()
    );
}

void
HdPrman_RenderParam::SetDrivingRenderSettingsPrimPath(
    SdfPath const &path)
{
    if (path != _drivingRenderSettingsPrimPath) {
        _drivingRenderSettingsPrimPath = path;
        TF_DEBUG(HDPRMAN_RENDER_SETTINGS).Msg(
            "Driving render settings prim is %s\n", path.GetText());
    }
}

SdfPath const&
HdPrman_RenderParam::GetDrivingRenderSettingsPrimPath() const
{
    return _drivingRenderSettingsPrimPath;
}

void
HdPrman_RenderParam::SetRileyOptions()
{
    // There are a couple of RIS/Riley limitations to call out:
    // 1. Current Riley implementations require `SetOptions()` to be the first
    //    call made before any scene manipulation (which includes the creation
    //    of Riley scene objects).
    // 2. Several riley settings are immutable and need to be set on the
    //    first SetOptions call.
    //
    // When scene index emulation is enabled, the first SetOptions call is
    // deferred until HdPrman_RenderSettings::Sync. A fallback render settings
    // prim is added via HdPrman_RenderSettingsFilteringSceneIndexPlugin to
    // allow this strategy to work for scenes without one.
    //
    // When scene index emulation is disabled, we have no way to know or
    // guarantee that a render settings prim is present. The first SetOptions
    // call is called after constructing the Riley instance in
    // HdPrman_RenderParam::Begin.
    //
    {
        // Compose scene options with the precedence:
        //     env > render settings prim > legacy settings map > fallback
        //
        // XXX: Some riley clients require certain options to be present
        // on every SetOptions call (e.g. XPU currently needs
        // ri:searchpath:texture). As a conservative measure, compose
        // all sources of options for initialization and subsequent updates.
        // Ideally, the latter would require just the legacy and prim options.

        RtParamList composedParams = HdPrman_Utils::Compose(
            _envOptions,
            _renderSettingsPrimOptions, 
            GetLegacyOptions(),
            _fallbackOptions);

        RtParamList prunedOptions = HdPrman_Utils::PruneDeprecatedOptions(
                    composedParams);

        riley::Riley * const riley = AcquireRiley();
        riley->SetOptions(prunedOptions);

        TF_DEBUG(HDPRMAN_RENDER_SETTINGS).Msg(
            "SetOptions called on the composed param list:\n  %s\n",
            HdPrmanDebugUtil::RtParamListToString(
                prunedOptions, /*indent = */2).c_str());
        
        // If we've updated the riley shutter interval in SetOptions above,
        // make sure to update the cached value.
        _UpdateShutterInterval(prunedOptions);
    }

    if (!_initRileyOptions) {
        _initRileyOptions = true;

        // Safe to create riley objects that aren't backed by the scene.
        // See limitation (1) above.
        _CreateInternalPrims();
    }
}

void 
HdPrman_RenderParam::SetActiveIntegratorId(const riley::IntegratorId id)
{
    _activeIntegratorId = id;

    riley::Riley * riley = AcquireRiley();

    GetRenderViewContext().SetIntegratorId(id, riley);
}

void 
HdPrman_RenderParam::StartRender()
{
    // Last chance to set Ri options before starting riley!
    // Called from HdPrman_RenderPass::_Execute for *interactive* rendering.
    // NOTE: We don't use a render thread for offline ("batch") rendering. See
    //       HdPrman_RenderPass::_RenderInMainThread().

    // Prepare Riley state for rendering.
    // Pass a valid riley callback pointer during IPR

    if (!_renderThread) {
        _renderThread = std::make_unique<HdRenderThread>();
        _renderThread->SetRenderCallback(
            std::bind(
                &HdPrman_RenderParam::_RenderThreadCallback, this));
        _renderThread->StartThread();
    }

    // Clear out old stats values
    if (_statsSession)
    {
        _statsSession->RemoveOldMetricData();
    }

    _renderThread->StartRender();
}

void
HdPrman_RenderParam::StopRender(bool blocking)
{
    TRACE_FUNCTION();

    if (!_renderThread || !_renderThread->IsRendering()) {
        return;
    }

    TF_DESCRIBE_SCOPE("Waiting for RenderMan to stop");

    if (!blocking) {
        {
            TRACE_SCOPE("riley::RequestUpdate");
            _riley->RequestUpdate();
        }
        return;
    }

    // Note: if we were rendering, when the flag goes low we'll be back in
    // render thread idle until another StartRender comes in, so we don't need
    // to manually call renderThread->StopRender. Theoretically
    // riley->Stop() is blocking, but we need the loop here because:
    // 1. It's possible that IsRendering() is true because we're in the preamble
    //    of the render loop, before calling into riley. In that case, Stop()
    //    is a no-op and we need to call it again after we call into Riley.
    // 2. We've occassionally seen cases where Stop() returns successfully,
    //    but the riley threadpools don't shut down right away.
    while (_renderThread->IsRendering()) {
        {
            TRACE_SCOPE("riley::Stop");
            _riley->Stop();
        }
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100us);
    }

    // Clear out old stats values. TODO: should we be calling this here? 
    if (_statsSession)
    {
        _statsSession->RemoveOldMetricData();
    }
}

bool
HdPrman_RenderParam::IsRendering()
{
    return _renderThread && _renderThread->IsRendering();
}

bool
HdPrman_RenderParam::IsPauseRequested()
{
    return _renderThread && _renderThread->IsPauseRequested();
}

void
HdPrman_RenderParam::DeleteRenderThread()
{
    if (_renderThread) {
        _renderThread->StopThread();
        _renderThread.reset();
    }
}

bool
HdPrman_RenderParam::_UpdateFramebufferClearValues(
    const HdRenderPassAovBindingVector& aovBindings)
{
    if(_framebuffer->aovBuffers.size() != aovBindings.size()) {
        // Number of AOVs changed, can't update framebuffer clear values.
        return false;
    }

    for(size_t aov = 0; aov < aovBindings.size(); ++aov) {
        const HdRenderPassAovBinding &aovBinding =
            aovBindings[aov];
        HdPrmanFramebuffer::AovDesc &aovDesc =
            _framebuffer->aovBuffers[aov].desc;
        if (aovBinding.aovName != aovDesc.name) {
            // Different AOV, can't update framebuffer clear value.
            return false;
        }
        
        if(aovBinding.aovName == HdAovTokens->color ||
           aovBinding.aovName == HdAovTokens->depth) {
            if (aovDesc.clearValue != aovBinding.clearValue) {
                // Request a framebuffer clear if the clear value in the aov
                // has changed from the framebuffer clear value.
                // We do this before StartRender() to avoid race conditions
                // where some buckets may get discarded or cleared with
                // the wrong value.
                
                // Stops render and increases sceneVersion to trigger restart.
                AcquireRiley();
                
                _framebuffer->pendingClear = true;
                aovDesc.clearValue = aovBinding.clearValue;
            }
        }
    }

    return true;
} 

static riley::RenderOutputType
_ToRenderOutputTypeFromHdFormat(const HdFormat aovFormat) 
{
    // Prman only supports float, color, and integer
    if(aovFormat == HdFormatFloat32) {
        return riley::RenderOutputType::k_Float;
    } else if(aovFormat == HdFormatFloat32Vec4 ||
              aovFormat == HdFormatFloat32Vec3) {
        return riley::RenderOutputType::k_Color;
    } else if(aovFormat == HdFormatInt32) {
        return riley::RenderOutputType::k_Integer;
    } else {
        return riley::RenderOutputType::k_Float;
    }
}

// If the aovFormat has 3 or 4 channels, make format Float32
static void
_AdjustColorFormat(HdFormat* aovFormat) 
{
    // Prman always renders colors as float, so for types with 3 or 4
    // components, always set the format in our framebuffer to float.
    // Conversion will take place in the Blit method of renderBuffer.cpp
    // when it notices that the aovBinding's buffer format doesn't match
    // our framebuffer's format.
    const int componentCount = HdGetComponentCount(*aovFormat);
    if(componentCount == 3) {
        *aovFormat = HdFormatFloat32Vec3;
    } else if(componentCount == 4) {
        *aovFormat = HdFormatFloat32Vec4;
    }
}

// Update the given Rman AOV and Source names 
//  - aovName: Map the given hdAovName to the Prman equivalent
//  - SourceName: Add 'lpe:' prefix as needed 
static void
_UpdateRmanAovAndSourceName(
    bool isXPU, bool isLPE,
    const TfToken &hdAovName,
    RtUString *rmanAovName,
    RtUString *rmanSourceName)
{
    static const RtUString us_st("__st");
    static const RtUString us_primvars_st("primvars:st");

    // Initialize rmanAovName with the HdAovName
    if (!hdAovName.GetString().empty()) {
        *rmanAovName = RtUString(hdAovName.GetText());
    }

    // If the sourceType hints that the source is an lpe, make sure
    // it starts with "lpe:" as required by prman.
    if (isLPE) {
        std::string sn = rmanSourceName->CStr();
        sn = (sn.find(RixStr.k_lpe.CStr()) == std::string::npos)
            ? "lpe:" + std::string(rmanSourceName->CStr())
            : rmanSourceName->CStr();
        *rmanSourceName = RtUString(sn.c_str());
    }

    // Update the Aov and Source names by mapping the HdAovName to an 
    // equivalent Prman name
    if (hdAovName == HdAovTokens->color || hdAovName.GetString() == "ci") {
        *rmanAovName = RixStr.k_Ci;
        *rmanSourceName = RixStr.k_Ci;
    } else if (hdAovName == HdAovTokens->depth) {
        *rmanSourceName = RixStr.k_z;
    } else if (hdAovName == HdAovTokens->normal) {
        *rmanSourceName= RixStr.k_Nn;
    } else if (hdAovName == HdAovTokens->primId) {
        *rmanAovName = RixStr.k_id;
        *rmanSourceName = RixStr.k_id;
    } else if (hdAovName == HdAovTokens->instanceId) {
        *rmanAovName = RixStr.k_id2;
        *rmanSourceName = RixStr.k_id2;
    } else if (hdAovName == HdAovTokens->elementId) {
        *rmanAovName = RixStr.k_faceindex;
        *rmanSourceName = RixStr.k_faceindex;
    } else if (*rmanAovName == us_primvars_st) {
        *rmanSourceName = us_st;
    }

    // If no sourceName is specified, assume name is a standard prman aov
    if (rmanSourceName->Empty()) {
        *rmanSourceName = *rmanAovName;
    }

    // XPU is picky about AOV names, it wants only standard names
    if (isXPU) {
        *rmanAovName = *rmanSourceName;
    }
}

// Return a RtParamList of the driver settings in the given aovSettings
// and update the Rman Aov and Source Names based on the aovSettings
static RtParamList
_GetOutputParamsAndUpdateRmanNames(
    const HdAovSettingsMap &aovSettings,
    bool isXPU,
    RtUString *rmanAovName,
    RtUString *rmanSourceName)
{
    RtParamList params;
    bool isLPE = false;
    TfToken hdAovName(rmanAovName->CStr());
    for (auto const& aovSetting : aovSettings) {
        const TfToken & settingName = aovSetting.first;
        const VtValue & settingVal = aovSetting.second;

        // Update hdAovName and rmanSourceName if authored in the aovSettingsMap
        if (settingName == _tokens->sourceName) {
            *rmanSourceName =
                RtUString(settingVal.GetWithDefault<std::string>().c_str());
        }
        else if (settingName == _tokens->name) {
            hdAovName = settingVal.UncheckedGet<TfToken>();
        }

        // Determine if the output is of type LPE or not
        else if (settingName == _tokens->sourceType) {
            const std::string sourceType =
                settingVal.GetWithDefault<TfToken>().GetString();
            isLPE = (sourceType == RixStr.k_lpe.CStr());
        }

        // Gather all properties with the 'driver:parameters:aov' prefix 
        // into the RtParamList, updating the hdAovName if needed. 
        else if (TfStringStartsWith(
                 settingName.GetText(), "driver:parameters:aov:")) {
            RtUString name(TfStringGetSuffix(settingName, ':').c_str());
            if (name == RixStr.k_name) {
                hdAovName = settingVal.UncheckedGet<TfToken>();
            } else {
                HdPrman_Utils::SetParamFromVtValue(name, settingVal,
                    TfToken(), &params);
            }
        }
    }

    _UpdateRmanAovAndSourceName(
        isXPU, isLPE, hdAovName, rmanAovName, rmanSourceName);

    return params;
}

void
HdPrman_RenderParam::_CreateRileyDisplay(
    const RtUString& productName, const RtUString& productType,
    HdPrman_RenderViewDesc& renderViewDesc,
    const std::vector<size_t>& renderOutputIndices,
    RtParamList& displayParams, bool isXpu)
{
    RtUString driver = productType;
    if(isXpu) {
        // XPU loads hdPrman as the display plug-in
        if (productName == RixStr.k_framebuffer) {
            std::string hdPrmanPath;
            if (PlugPluginPtr const plugin =
                PlugRegistry::GetInstance().GetPluginWithName("hdPrman")) {
                const std::string path = TfGetPathName(plugin->GetPath());
                if (!path.empty()) {
                    hdPrmanPath =
                        TfStringCatPaths(path, "hdPrman" ARCH_LIBRARY_SUFFIX);
                }
                driver = RtUString(hdPrmanPath.c_str());
            } else {
                TF_WARN("Failed to load xpu display plugin\n");
            }
        }

        displayParams.SetString(RixStr.k_Ri_name, productName);
        displayParams.SetString(RixStr.k_Ri_type, productType);
        if(_framebuffer) {
            static const RtUString us_bufferID("bufferID");
            displayParams.SetInteger(us_bufferID, _framebuffer->id);
        }
    }

    {
        HdPrman_RenderViewDesc::DisplayDesc displayDesc;
        displayDesc.name = productName;
        displayDesc.driver = driver;
        displayDesc.params = displayParams;
        displayDesc.renderOutputIndices = renderOutputIndices;

        renderViewDesc.displayDescs.push_back(std::move(displayDesc));
    }
}

static
std::string
_ExpandVarsInProductName(const std::string & productName,
                         const std::string & sourcePrimName, int frame)
{
    std::string expandedName = productName;
    static const char* formatStrings[] = {
        "%01d", "%02d", "%03d", "%04d", "%05d", "%d" };
    const bool hasAngleVars = (expandedName.find('<') != std::string::npos);
    const bool hasDollarVars = (expandedName.find('$') != std::string::npos);
    if(hasAngleVars || hasDollarVars) {
        static const char* frameAngleVarStrings[] = {
            "<F1>", "<F2>", "<F3>", "<F4>", "<F5>", "<F>" };
        static const char* frameDollarVarStrings[] = {
            "$F1", "$F2", "$F3", "$F4", "$F5", "$F" };
        static const char* frameDollarBraceVarStrings[] = {
            "${F1}", "${F2}", "${F3}", "${F4}", "${F5}", "${F}" };

        std::string frame_str;
        for( size_t i=0; i < TfArraySize(formatStrings); ++i) {
            frame_str = TfStringPrintf(formatStrings[i], frame);
            if(hasAngleVars) {
                expandedName =
                    TfStringReplace(expandedName,
                                    frameAngleVarStrings[i],
                                    frame_str);
            }
            if(hasDollarVars) {
                expandedName =
                    TfStringReplace(expandedName,
                                    frameDollarVarStrings[i],
                                    frame_str);
                expandedName =
                    TfStringReplace(expandedName,
                                    frameDollarBraceVarStrings[i],
                                    frame_str);
            }
        }
        if(hasAngleVars) {
            expandedName =
                TfStringReplace(expandedName, "<OS>",
                                sourcePrimName);
        }
        if(hasDollarVars) {
            expandedName =
                TfStringReplace(expandedName, "$OS",
                                sourcePrimName);
            expandedName =
                TfStringReplace(expandedName, "${OS}",
                                sourcePrimName);
        }
    }
    // Support printf style formating in file name, like %04d
    if(expandedName.find('%') != std::string::npos) {
        expandedName = TfStringPrintf( expandedName.c_str(), frame);
    }
    return expandedName;
}

static
RtUString
_AddRenderOutput(
    RtUString aovName,
    const TfToken &dataType,
    HdFormat aovFormat,
    RtUString sourceName,
    const RtParamList& params,
    std::vector<HdPrman_RenderViewDesc::RenderOutputDesc> * renderOutputDescs,
    std::vector<size_t> * renderOutputIndices)
{
    static RtUString const k_cpuTime("cpuTime");
    static RtUString const k_sampleCount("sampleCount");
    static RtUString const k_none("none");

    // Get the Render Type from the given dataType, or aovFormat
    riley::RenderOutputType rType = (dataType.IsEmpty())
        ? _ToRenderOutputTypeFromHdFormat(aovFormat)
        : _ToRenderOutputType(dataType);
    // Make sure 'Ci' sources use the Color Output type
    if (sourceName == RixStr.k_Ci) {
        rType = riley::RenderOutputType::k_Color;
    }

    // Get the rule, filter, and filterSize from the given RtParamList
    RtUString rule = RixStr.k_filter;
    params.GetString(RixStr.k_rule, rule);

    RtUString filter = RixStr.k_box;
    params.GetString(RixStr.k_filter, filter);

    float filterSize[2] = {1.0f, 1.0f};
    if(float const* filterwidth =
       params.GetFloatArray(RixStr.k_filterwidth, 2)) {
        filterSize[0] = filterwidth[0];
        filterSize[1] = filterwidth[1];
    }

    // Adjust the rule/filter/filterSize as needed
    RtUString value;
    static const RtUString k_depth("depth");
    // "cpuTime" and "sampleCount" should use rule "sum"
    if (aovName == k_cpuTime || aovName == k_sampleCount) {
        rule = RixStr.k_sum;
        filter = RixStr.k_box;
        filterSize[0] = 1;
        filterSize[1] = 1;
    // "id", "id2", "z" and "depth" should use rule "zmin"
    } else if (aovName == RixStr.k_id || aovName == RixStr.k_id2 ||
              aovName == RixStr.k_z || aovName == k_depth ||
              rType == riley::RenderOutputType::k_Integer) {
        rule = RixStr.k_zmin;
        filter = RixStr.k_box;
        filterSize[0] = 1;
        filterSize[1] = 1;
    // If statistics are set, use that as the rule
    } else if (params.GetString(RixStr.k_statistics, value) &&
              !value.Empty() && value != k_none) {
        rule = value;
    // Certain filter types need to be converted to rules
    } else if (filter == RixStr.k_min  || filter == RixStr.k_max  ||
               filter == RixStr.k_zmin || filter == RixStr.k_zmax ||
               filter == RixStr.k_sum  || filter == RixStr.k_average) {
        rule = filter;
        filter = RixStr.k_box;
        filterSize[0] = 1;
        filterSize[1] = 1;
    }

    // Get the relativePixelVariance and remap from the given RtParamList
    float relativePixelVariance = 1.0f;
    params.GetFloat(RixStr.k_relativepixelvariance, relativePixelVariance);

    RtParamList extraParams;
    float remap[3] = {0.0f, 0.0f, 0.0f};
    if (float const* remapValue = params.GetFloatArray(RixStr.k_remap, 3)) {
        remap[0] = remapValue[0];
        remap[1] = remapValue[1];
        remap[2] = remapValue[2];
        extraParams.SetFloatArray(RixStr.k_remap, remap, 3);
    }

    // Create the RenderOutputDesc for this AOV
    {
        HdPrman_RenderViewDesc::RenderOutputDesc renderOutputDesc;
        renderOutputDesc.name = aovName;
        renderOutputDesc.type = rType;
        renderOutputDesc.sourceName = sourceName;
        renderOutputDesc.rule = rule;
        renderOutputDesc.filter = filter;
        renderOutputDesc.filterWidth.Set( filterSize[0], filterSize[1] );
        renderOutputDesc.relativePixelVariance = relativePixelVariance;
        renderOutputDesc.params = extraParams;

        TF_DEBUG(HDPRMAN_RENDER_PASS)
            .Msg("Add RenderOutputDesc: \n - name: '%s'\n - type: '%d'\n"
                 " - sourceName: '%s'\n - rule: '%s'\n - filter: '%s'\n\n",
                 aovName.CStr(), int(rType), sourceName.CStr(), 
                 rule.CStr(), filter.CStr());

        renderOutputDescs->push_back(std::move(renderOutputDesc));
        renderOutputIndices->push_back(renderOutputDescs->size()-1);
    }

    // When a float4 color is requested, assume we require alpha as well.
    // This assumption is reflected in framebuffer.cpp HydraDspyData
    const int componentCount = HdGetComponentCount(aovFormat);
    if (rType == riley::RenderOutputType::k_Color && componentCount == 4) {
        HdPrman_RenderViewDesc::RenderOutputDesc renderOutputDesc;
        renderOutputDesc.name = RixStr.k_a;
        renderOutputDesc.type = riley::RenderOutputType::k_Float;
        renderOutputDesc.sourceName = RixStr.k_a;
        renderOutputDesc.rule = RixStr.k_filter;
        renderOutputDesc.filter = RixStr.k_box;

        renderOutputDescs->push_back(std::move(renderOutputDesc));
        renderOutputIndices->push_back(renderOutputDescs->size()-1);
    }
    return rule;
}

static
void
_ComputeRenderOutputAndAovDescs(
    const HdRenderPassAovBindingVector& aovBindings,
    bool isXpu,
    std::vector<HdPrman_RenderViewDesc::RenderOutputDesc> * renderOutputDescs,
    std::vector<size_t> * renderOutputIndices,
    HdPrmanFramebuffer::AovDescVector * aovDescs)
{
    std::unordered_map<TfToken, RtUString, TfToken::HashFunctor> sourceNames;

    for (const HdRenderPassAovBinding &aovBinding : aovBindings) {

        // RmanAovName
        RtUString rmanAovName(aovBinding.aovName.GetText());

        // AovFormat
        HdFormat aovFormat = aovBinding.renderBuffer->GetFormat();
        _AdjustColorFormat(&aovFormat);

        // Rman Aov and Source Names, and RenderOutputParams
        RtUString rmanSourceName;
        RtParamList renderOutputParams =
            _GetOutputParamsAndUpdateRmanNames(
                aovBinding.aovSettings,
                isXpu,
                &rmanAovName,
                &rmanSourceName);
        if (!rmanSourceName.Empty()) {
            // This is a workaround for an issue where we get an
            // unexpected duplicate in the aovBindings sometimes,
            // where the second entry lacks a sourceName.
            // Can't just skip it because the caller expects
            // a result in the buffer.
            sourceNames[aovBinding.aovName] = rmanSourceName;
        } else {
            auto it = sourceNames.find(aovBinding.aovName);
            if (it != sourceNames.end()) {
                rmanSourceName = it->second;
            }
        }

        // Create a RenderOutputDesc from the aovBinding
        RtUString rule = _AddRenderOutput(rmanAovName,
                                          TfToken(),
                                          aovFormat,
                                          rmanSourceName,
                                          renderOutputParams, 
                                          renderOutputDescs,
                                          renderOutputIndices);

        // Create a AovDesc from the aovBinding
        {
            HdPrmanFramebuffer::AovDesc aovDesc;
            aovDesc.name = aovBinding.aovName;
            aovDesc.format = aovFormat;
            aovDesc.clearValue = aovBinding.clearValue;
            aovDesc.rule = HdPrmanFramebuffer::ToAccumulationRule(rule);
            
            aovDescs->push_back(std::move(aovDesc));
        }
    }
}

template <typename T>
static T _Get(const HdAovSettingsMap & m, const TfToken & key, const T default_val=T())
{
    auto v = m.find(key);
    if(v != m.end() && v->second.IsHolding<T>()) {
        return v->second.UncheckedGet<T>();
    }
    return default_val;
}

static RtUString
_GetAsRtUString(const HdAovSettingsMap & m, const TfToken & key)
{
    TfToken v = _Get<TfToken>(m, key);
    return RtUString(v.GetString().c_str());
}

void
HdPrman_RenderParam::CreateFramebufferAndRenderViewFromAovs(
    const HdRenderPassAovBindingVector& aovBindings)
{
    if (!_framebuffer) {
        _framebuffer = std::make_unique<HdPrmanFramebuffer>();
    }

    if(_UpdateFramebufferClearValues(aovBindings)) {
        // AOVs are the same and updating the clear values succeeded,
        // nothing more to do.
        return;
    }

    // Proceed with creating displays if the number has changed
    // or the display names don't match what we have.

    // Stop render and crease sceneVersion to trigger restart.
    riley::Riley * riley = AcquireRiley();

    std::lock_guard<std::mutex> lock(_framebuffer->mutex);

    // Displays & Display Channels
    HdPrman_RenderViewDesc renderViewDesc;
    std::vector<size_t> renderOutputIndices;
    HdPrmanFramebuffer::AovDescVector aovDescs;

    _ComputeRenderOutputAndAovDescs(
        aovBindings,
        IsXpu(),
        &renderViewDesc.renderOutputDescs,
        &renderOutputIndices,
        &aovDescs);

    _framebuffer->CreateAovBuffers(aovDescs);

    RtParamList displayParams;
    static const RtUString us_hydra("hydra");
    _CreateRileyDisplay(RixStr.k_framebuffer,
                        us_hydra,
                        renderViewDesc,
                        renderOutputIndices,
                        displayParams,
                        IsXpu());

    renderViewDesc.cameraId = GetCameraContext().GetCameraId();
    renderViewDesc.integratorId = GetActiveIntegratorId();
    renderViewDesc.sampleFilterList = GetSampleFilterList();
    renderViewDesc.displayFilterList = GetDisplayFilterList();
    renderViewDesc.resolution = GetResolution();

    TF_DEBUG(HDPRMAN_RENDER_PASS)
        .Msg("Create Riley RenderView from AOV bindings.\n");
    GetRenderViewContext().CreateRenderView(renderViewDesc, riley);
}

void
HdPrman_RenderParam::CreateRenderViewFromLegacyProducts(
    const VtArray<HdRenderSettingsMap>& renderProducts, int frame)
{
    // Display edits are not currently supported in HdPrman
    // RenderMan Display drivers are inteded for use in batch rendering, 
    // so bail here if Riley has already been started, since this means that
    // the Displays already exist.
    if (renderProducts.empty() ||
        GetRenderViewContext().GetRenderViewId() !=
            riley::RenderViewId::InvalidId()) {
        return;
    }

    // Currently XPU only supports having one Riley Target and View.
    // Here we loop over the Render Products (a USD concept which corresponds
    // to a Riley Display), make a list of Riley Displays, and collect a list
    // of all the outputs (AOVs) used by the Displays.
    // One Target will be used for all Displays, it needs to be created 
    // before the Displays, and takes a list of all possible outputs (AOVs).
    // 
    // The View and Displays are created, each referencing the Target's id.
    // 
    // XXX In the future, when xpu supports it, we may want to change this to 
    // allow for a different Target/View for each Display.

    HdPrman_RenderViewDesc renderViewDesc;

    unsigned idx = 0;
    for (const HdRenderSettingsMap& renderProduct : renderProducts) {
        TfToken productName;
        TfToken productType;
        std::string sourcePrimName;
        VtArray<HdAovSettingsMap> aovs;

        // Note:
        //  - productType or productName are not guaranteed to exist
        //  - order of settings is not guaranteed so we save relevant settings
        //    to the driverParameters
        std::vector<TfToken> driverParameters;
        for (auto const& productSetting : renderProduct) {
            const TfToken& settingName = productSetting.first;
            VtValue settingVal = productSetting.second;
            if (settingName == HdPrmanRenderProductTokens->productType) {
                productType = settingVal.UncheckedGet<TfToken>();
            } else if (settingName == HdPrmanRenderProductTokens->productName) {
                productName = settingVal.UncheckedGet<TfToken>();
            } else if (settingName == HdPrmanRenderProductTokens->orderedVars) {
                VtArray<HdAovSettingsMap> orderedVars =
                    settingVal.UncheckedGet<VtArray<HdAovSettingsMap>>();
                
                // Find Ci and a Outputs in the RenderVar list
                int Ci_idx = -1;
                int a_idx = -1;
                for (size_t i = 0; i < orderedVars.size(); ++i) {
                    std::string srcName;
                    const HdAovSettingsMap &orderedVar = orderedVars[i];
                    auto it =
                        orderedVar.find(HdPrmanAovSettingsTokens->sourceName);
                    if (it != orderedVar.end()) {
                        srcName = it->second.UncheckedGet<std::string>();
                    }
                    if (Ci_idx < 0 && srcName == RixStr.k_Ci.CStr()) {
                        if (Ci_idx != -1) {
                            TF_WARN("Multiple Ci outputs found\n");
                        }
                        Ci_idx = i;
                    } else if (a_idx < 0 && srcName == RixStr.k_a.CStr()) {
                        a_idx = i;
                    }
                    if (Ci_idx >= 0 && a_idx >= 0) {
                        break;
                    }
                }

                // Populate the AOVs Array from the RenderVar list making sure
                // that the Ci and a RenderVars are first. 
                aovs.reserve(orderedVars.size());
                if (Ci_idx >= 0 && Ci_idx < static_cast<int>(orderedVars.size())) {
                    aovs.push_back(orderedVars[Ci_idx]);
                }
                if (a_idx >= 0 && a_idx < static_cast<int>(orderedVars.size())) {
                    aovs.push_back(orderedVars[a_idx]);
                }
                for (size_t i = 0; i < orderedVars.size(); ++i) {
                    const int varIdx = static_cast<int>(i);
                    if (varIdx != Ci_idx && varIdx != a_idx) {
                        aovs.push_back(orderedVars[i]);
                    }
                }
            } else if (settingName == HdPrmanRenderProductTokens->sourcePrim) {
                const SdfPath sourcePrim = settingVal.UncheckedGet<SdfPath>();
                sourcePrimName = sourcePrim.GetName().c_str();
            } else if (TfStringStartsWith(settingName.GetText(),
                                          "driver:parameters:")) {
                driverParameters.push_back(settingName);
            }
        }

        // If an outputName has been specified on command line,
        // override the product's name.
        // But if there are multiple products, and only one outputName
        // has been specified, only use it for products beyond the first
        // if it contains variables, so we don't just overwrite the first image.
        std::string outputName;
        if (idx < _outputNames.size()) {
            outputName = _outputNames[idx];
        } else if (!_outputNames.empty() &&
                    _outputNames[0].find('<') != std::string::npos) {
            outputName = _outputNames[0];
        }

        // Expand a few possible variables.
        // <OS> : source prim (render product node name)
        // <F>, <F1>, <F2>, <F3>, <F4>, <F5> : frame number, with padding
        // vars can also be dollar style, braces optional, eg. $F4 ${F4} $OS
        // or printf style formatting: %04d
        if (!outputName.empty()) {
            productName = TfToken(
                _ExpandVarsInProductName(outputName, sourcePrimName, frame));
        }

        // Build Display Settings ParamList using the driverParameters gathered 
        // above from the Render Product Settings
        RtParamList displayParams;
        for (const TfToken& paramName : driverParameters) {
            auto val = renderProduct.find(paramName);
            if (val != renderProduct.end()) {
                const RtUString name(TfStringGetSuffix(paramName, ':').c_str());
                HdPrman_Utils::SetParamFromVtValue(name, val->second,
                    TfToken(), &displayParams);
            }
        }

        // Keep a list of the indices for the Render Outputs (AOVs/RenderVars) 
        // of this Display (RenderProduct)
        // renderViewDesc.renderOutputDescs is a list of all Render Outputs
        // across all Displays, these renderOutputIndices index into that list.
        std::vector<size_t> renderOutputIndices;
        for (const HdAovSettingsMap &aov : aovs) {

            // DataType
            const TfToken dataType =
                _Get<TfToken>(aov, HdPrmanAovSettingsTokens->dataType);

            // Format
            HdFormat aovFormat =
                _Get<HdFormat>(aov,
                               HdPrmanAovSettingsTokens->format,
                               HdFormatFloat32);
            _AdjustColorFormat(&aovFormat);

            // RmanSourceName 
            RtUString rmanSourceName =
                _GetAsRtUString(aov, HdPrmanAovSettingsTokens->sourceName);

            // RenderOutputParams and update the Rman Aov and Source Names
            RtUString rmanAovName = rmanSourceName;
            const HdAovSettingsMap aovSettings =
                _Get<HdAovSettingsMap>(aov,
                                       HdPrmanAovSettingsTokens->aovSettings);
            RtParamList renderOutputParams =
                _GetOutputParamsAndUpdateRmanNames(
                    aovSettings,
                    IsXpu(),
                    &rmanAovName,
                    &rmanSourceName);

            // Create the RenderOutputDesc for this AOV/RenderVar
            _AddRenderOutput(rmanAovName,
                             dataType,
                             aovFormat,
                             rmanSourceName,
                             renderOutputParams,
                             &renderViewDesc.renderOutputDescs,
                             &renderOutputIndices);

        }

        _CreateRileyDisplay(RtUString(productName.GetText()),
                            RtUString(productType.GetText()),
                            renderViewDesc,
                            renderOutputIndices,
                            displayParams,
                            IsXpu());
        ++idx;
    }

    renderViewDesc.cameraId = GetCameraContext().GetCameraId();
    renderViewDesc.integratorId = GetActiveIntegratorId();
    renderViewDesc.resolution = GetResolution();

    TF_DEBUG(HDPRMAN_RENDER_PASS)
        .Msg("Create Riley RenderView from the legacy products.\n");
    GetRenderViewContext().CreateRenderView(renderViewDesc, _riley);
}

bool
HdPrman_RenderParam::DeleteFramebuffer()
{
    if (_framebuffer) {
        _framebuffer.reset();
        return true;
    }
    return false;
}

riley::IntegratorId
HdPrman_RenderParam::GetActiveIntegratorId()
{
    return _activeIntegratorId;
}

riley::Riley *
HdPrman_RenderParam::AcquireRiley()
{
    // Scene manipulation API can only be called during the "editing" phase
    // (when Render() is not running).
    StopRender(/*blocking = true*/);
    sceneVersion++;

    return _riley;
}

static const float*
_GetShutterParam(const RtParamList &params)
{
    return params.GetFloatArray(RixStr.k_Ri_Shutter, 2);
}

void
HdPrman_RenderParam::_UpdateShutterInterval(const RtParamList &composedParams)
{
    if (const float *val = _GetShutterParam(composedParams)) {
        _shutterInterval = GfVec2f(val[0], val[1]);
    }

    // When there's only one sample available the motion blur plug-in 
    // doesn't have access to the correct shutter interval, so this is 
    // a workaround to provide it.
    HdPrman_MotionBlurSceneIndexPlugin::SetShutterInterval(
        _shutterInterval[0], _shutterInterval[1]);
}

riley::ShadingNode
HdPrman_RenderParam::_ComputeQuickIntegratorNode(
    HdRenderDelegate * const renderDelegate,
    const HdPrmanCamera * const cam)
{
    const std::string &integratorName =
        renderDelegate->GetRenderSetting<std::string>(
            HdPrmanRenderSettingsTokens->interactiveIntegrator,
            HdPrmanIntegratorTokens->PxrDirectLighting.GetString());

    const RtUString rtIntegratorName(integratorName.c_str());

    SetIntegratorParamsFromRenderSettingsMap(
        static_cast<HdPrmanRenderDelegate*>(renderDelegate),
        integratorName,
        _quickIntegratorParams);

    if (cam) {
        SetIntegratorParamsFromCamera(
            static_cast<HdPrmanRenderDelegate*>(renderDelegate),
            cam,
            integratorName,
            _quickIntegratorParams);
    }

    static const RtUString numLightSamples("numLightSamples");
    static const RtUString numBxdfSamples("numBxdfSamples");

    _quickIntegratorParams.SetInteger(numLightSamples, 1);
    _quickIntegratorParams.SetInteger(numBxdfSamples, 1);        

    return riley::ShadingNode{
        riley::ShadingNode::Type::k_Integrator,
        rtIntegratorName,
        rtIntegratorName,
        _quickIntegratorParams};
}

void
HdPrman_RenderParam::_CreateQuickIntegrator(
    HdRenderDelegate * const renderDelegate)
{
    // See comment in _CreateIntegrator.
    static const HdPrmanCamera * const camera = nullptr;

    if (_enableQuickIntegrate) {
        riley::ShadingNode integratorNode(
          _ComputeQuickIntegratorNode(renderDelegate, camera));
        _quickIntegratorId = _riley->CreateIntegrator(
            riley::UserId(
                stats::AddDataLocation(integratorNode.name.CStr()).GetValue()),
            integratorNode);
    
        TF_VERIFY(_quickIntegratorId != riley::IntegratorId::InvalidId());
    }
}

void
HdPrman_RenderParam::UpdateQuickIntegrator(
    const HdRenderIndex * const renderIndex)
{
    if (_enableQuickIntegrate) {
        if (!TF_VERIFY(_quickIntegratorId != riley::IntegratorId::InvalidId())) {
            return;
        }

        const riley::ShadingNode node =
            _ComputeQuickIntegratorNode(
                renderIndex->GetRenderDelegate(),
                _cameraContext.GetCamera(renderIndex));
        
        AcquireRiley()->ModifyIntegrator(
            _quickIntegratorId,
            &node);
    }
}

// tl;dr: Motion blur is currently supported only if the camera path and/or
//        disableMotionBlur are set on the legacy render settings map BEFORE
//        syncing prims.
//        When using a well-formed render settings prim, the computed unioned
//        shutter interval may be available (23.11 onwards) which circumvents
//        the above limitation.
//
// Here's the longform story:
//
// Riley has a limitation in that the shutter interval scene option param
// has to be set before any time sampled primvars or transforms are
// given to Riley.
//
// The shutter interval is specified on the camera. In the legacy task based
// data flow, the camera used to render is known only during render pass
// execution which happens AFTER prim sync. To circumvent this, we use
// the legacy render settings map to provide the camera path during render
// delegate construction. See HdPrmanExperimentalRenderSpecTokens->camera
// and _tokens->renderCameraPath (latter is used by Solaris).
//
// When the said camera is sync'd, we commit its shutter interval IFF it is
// the one to use for rendering. See HdPrman_Camera::Sync.
//
// This "shutter interval discovery" issue may not be relevant when using the
// render settings prim. If using 23.11 and later, the shutter interval
// is computed from on the cameras used by the render products. See
// HdPrman_RenderSettings::Sync.
//
// HOWEVER:
// Changing the camera shutter (either on the camera or changing the camera
// used) AFTER syncing prims with motion samples (e.g., lights & geometry)
// requires the prims to be resync'd. This scenario isn't supported currently.
// XXX Note that updating the render setting _tokens->renderCameraPath currently
//     results in marking all rprims dirty.
//     See HdPrmanRenderDelegate::SetRenderSetting. This handling is rather
//     adhoc and should be cleaned up.
//
void
HdPrman_RenderParam::SetRileyShutterIntervalFromCameraContextCameraPath(
    const HdRenderIndex * const renderIndex)
{
    // Fallback shutter interval.
    float shutterInterval[2] = {
        HDPRMAN_SHUTTEROPEN_DEFAULT,
        HDPRMAN_SHUTTERCLOSE_DEFAULT
    };

    // Handle legacy render setting.
    const bool disableMotionBlur =
        renderIndex->GetRenderDelegate()->GetRenderSetting<bool>(
            HdPrmanRenderSettingsTokens->disableMotionBlur, false);
    if (disableMotionBlur) {
        // Disable motion blur by sampling at current frame only.
        shutterInterval[0] = 0.0f;
        shutterInterval[1] = 0.0f;

    } else {
        // Try to get shutter interval from camera.
        // Note that shutter open and close times are frame relative and refer 
        // to the times the shutter begins to open and fully closes
        // respectively.
        if (const HdCamera * const camera =
                _cameraContext.GetCamera(renderIndex)) {
            shutterInterval[0] = camera->GetShutterOpen();
            shutterInterval[1] = camera->GetShutterClose();
        }

        // Deprecated.
        const bool instantaneousShutter =
            renderIndex->GetRenderDelegate()->GetRenderSetting<bool>(
                HdPrmanRenderSettingsTokens->instantaneousShutter, false);
        if (instantaneousShutter) {
            // Disable motion blur by making the interval a single point.
            shutterInterval[1] = shutterInterval[0];
        }
    }

    // Update the shutter interval on the *legacy* options param list and
    // commit the scene options. Note that the legacy options has a weaker
    // opinion that the env var HD_PRMAN_ENABLE_MOTIONBLUR and the render
    // settings prim.
    RtParamList &options = GetLegacyOptions();
    options.SetFloatArray(RixStr.k_Ri_Shutter, shutterInterval, 2);

    SetRileyOptions();
}

void
HdPrman_RenderParam::SetRenderSettingsIntegratorPath(
    HdSceneDelegate *sceneDelegate,
    SdfPath const &renderSettingsIntegratorPath)
{
    if (_renderSettingsIntegratorPath != renderSettingsIntegratorPath) {
        if (! HdRenderIndex::IsSceneIndexEmulationEnabled()) {
            // Mark the Integrator Prim Dirty
            sceneDelegate->GetRenderIndex().GetChangeTracker()
                .MarkSprimDirty(renderSettingsIntegratorPath, 
                                HdChangeTracker::DirtyParams);
        }
        _renderSettingsIntegratorPath = renderSettingsIntegratorPath;

        // Update the Integrator back to the default when the path is empty
        if (_renderSettingsIntegratorPath.IsEmpty()) {
            UpdateIntegrator(&sceneDelegate->GetRenderIndex());
        }
    }
}

void 
HdPrman_RenderParam::SetRenderSettingsIntegratorNode(
    HdRenderIndex *renderIndex, HdMaterialNode2 const &integratorNode)
{
    if (_renderSettingsIntegratorNode != integratorNode) {
        // Save the HdMaterialNode2, the riley integrator is created
        // inside UpdateIntegrator based on this node.
        _renderSettingsIntegratorNode = integratorNode;
        UpdateIntegrator(renderIndex);
    }
}

void
HdPrman_RenderParam::SetConnectedSampleFilterPaths(
    HdSceneDelegate *sceneDelegate,
    SdfPathVector const &connectedSampleFilterPaths)
{
    if (_connectedSampleFilterPaths != connectedSampleFilterPaths) {
        // Reset the Filter Shading Nodes and update the Connected Paths
        _sampleFilterNodes.clear();
        _connectedSampleFilterPaths = connectedSampleFilterPaths;

        if (! HdRenderIndex::IsSceneIndexEmulationEnabled()) {
            // Mark the SampleFilter Prims Dirty
            for (const SdfPath &path : connectedSampleFilterPaths) {
                sceneDelegate->GetRenderIndex().GetChangeTracker()
                    .MarkSprimDirty(path, HdChangeTracker::DirtyParams);
            }
        }
    }

    // If there are no connected SampleFilters, delete the riley SampleFilter
    if (_connectedSampleFilterPaths.size() == 0) {
        if (_sampleFiltersId != riley::SampleFilterId::InvalidId()) {
            AcquireRiley()->DeleteSampleFilter(_sampleFiltersId);
            _sampleFiltersId = riley::SampleFilterId::InvalidId();
        }
    }
}

void
HdPrman_RenderParam::SetConnectedDisplayFilterPaths(
    HdSceneDelegate *sceneDelegate,
    SdfPathVector const &connectedDisplayFilterPaths)
{
    if (_connectedDisplayFilterPaths != connectedDisplayFilterPaths) {
        // Reset the Filter Shading Nodes and update the Connected Paths
        _displayFilterNodes.clear();
        _connectedDisplayFilterPaths = connectedDisplayFilterPaths;

        if (! HdRenderIndex::IsSceneIndexEmulationEnabled()) {
            // Mark the DisplayFilter prims Dirty
            for (const SdfPath &path : connectedDisplayFilterPaths) {
                sceneDelegate->GetRenderIndex().GetChangeTracker()
                    .MarkSprimDirty(path, HdChangeTracker::DirtyParams);
            }
        }
    }

    // If there are no connected DisplayFilters, delete the riley DisplayFilter
    if (_connectedDisplayFilterPaths.size() == 0) {
        if (_displayFiltersId != riley::DisplayFilterId::InvalidId()) {
            AcquireRiley()->DeleteDisplayFilter(_displayFiltersId);
            _displayFiltersId = riley::DisplayFilterId::InvalidId();
        }
    }
}

void
HdPrman_RenderParam::CreateSampleFilterNetwork(HdSceneDelegate *sceneDelegate)
{
    std::vector<riley::ShadingNode> shadingNodes;
    std::vector<RtUString> filterRefs;

    // Gather shading nodes and reference paths (for combiner) for all connected
    // and visible SampleFilters. The filterRefs order needs to match the order
    // of SampleFilters specified in the RenderSettings connection.
    for (const auto& path : _connectedSampleFilterPaths) {
        if (sceneDelegate->GetVisible(path)) {
            const auto it = _sampleFilterNodes.find(path);
            if (!TF_VERIFY(it != _sampleFilterNodes.end())) {
                continue;
            }
            if (it->second.name) {
                shadingNodes.push_back(it->second);
                filterRefs.push_back(RtUString(path.GetText()));
            }
        }
    }

    // If we have multiple SampleFilters, create a SampleFilter Combiner Node
    if (shadingNodes.size() > 1) {
        static RtUString filterArrayName("filter");
        static RtUString pxrSampleFilterCombiner("PxrSampleFilterCombiner");

        riley::ShadingNode combinerNode;
        combinerNode.type = riley::ShadingNode::Type::k_SampleFilter;
        combinerNode.handle = pxrSampleFilterCombiner;
        combinerNode.name = pxrSampleFilterCombiner;
        combinerNode.params.SetSampleFilterReferenceArray(
            filterArrayName, filterRefs.data(), filterRefs.size());
        shadingNodes.push_back(combinerNode);
    }
    
    // Create or update the Riley SampleFilters
    riley::ShadingNetwork const sampleFilterNetwork = {
        static_cast<uint32_t>(shadingNodes.size()), &shadingNodes[0] };
    
    if (_sampleFiltersId == riley::SampleFilterId::InvalidId()) {
        _sampleFiltersId = AcquireRiley()->CreateSampleFilter(
            riley::UserId(stats::AddDataLocation("/sampleFilters").
                              GetValue()),
            sampleFilterNetwork, 
            RtParamList());
    }
    else {
        AcquireRiley()->ModifySampleFilter(
            _sampleFiltersId, &sampleFilterNetwork, nullptr);
    }

    if (_sampleFiltersId == riley::SampleFilterId::InvalidId()) {
        TF_WARN("Failed to create the Sample Filter(s)\n");
    }
}

void
HdPrman_RenderParam::CreateDisplayFilterNetwork(HdSceneDelegate *sceneDelegate)
{
    std::vector<riley::ShadingNode> shadingNodes;
    std::vector<RtUString> filterRefs;

    // Gather shading nodes and reference paths (for combiner) for all connected
    // and visible DisplayFilters. The filterRefs order needs to match the order
    // of DisplayFilters specified in the RenderSettings connection.
    for (const auto& path : _connectedDisplayFilterPaths) {
        if (sceneDelegate->GetVisible(path)) {
            const auto it = _displayFilterNodes.find(path);
            if (!TF_VERIFY(it != _displayFilterNodes.end())) {
                continue;
            }
            if (it->second.name) {
                shadingNodes.push_back(it->second);
                filterRefs.push_back(RtUString(path.GetText()));
            }
        }
    }

    // If we have multiple DisplayFilters, create a DisplayFilter Combiner Node
    if (shadingNodes.size() > 1) {
        static RtUString filterArrayName("filter");
        static RtUString pxrDisplayFilterCombiner("PxrDisplayFilterCombiner");

        riley::ShadingNode combinerNode;
        combinerNode.type = riley::ShadingNode::Type::k_DisplayFilter;
        combinerNode.handle = pxrDisplayFilterCombiner;
        combinerNode.name = pxrDisplayFilterCombiner;
        combinerNode.params.SetDisplayFilterReferenceArray(
            filterArrayName, filterRefs.data(), filterRefs.size());
        shadingNodes.push_back(combinerNode);
    }
    
    // Create or update the Riley DisplayFilters
    riley::ShadingNetwork const displayFilterNetwork = {
        static_cast<uint32_t>(shadingNodes.size()), &shadingNodes[0] };
    
    if (_displayFiltersId == riley::DisplayFilterId::InvalidId()) {
        _displayFiltersId = AcquireRiley()->CreateDisplayFilter(
            riley::UserId(stats::AddDataLocation("/displayFilters").
                              GetValue()),
            displayFilterNetwork, 
            RtParamList());
    }
    else {
        AcquireRiley()->ModifyDisplayFilter(
            _displayFiltersId, &displayFilterNetwork, nullptr);
    }

    if (_displayFiltersId == riley::DisplayFilterId::InvalidId()) {
        TF_WARN("Failed to create the Display Filter(s)\n");
    }
}

void
HdPrman_RenderParam::AddSampleFilter(
    HdSceneDelegate *sceneDelegate,
    SdfPath const& path,
    riley::ShadingNode const& node)
{
    // Update or Add the SampleFilter Shading node
    const auto filterIt = _sampleFilterNodes.insert( {path, node} );
    if (!filterIt.second) {
        filterIt.first->second = node;
    }

    // If we have all the Shading Nodes, create the SampleFilters in Riley
    if (_sampleFilterNodes.size() == _connectedSampleFilterPaths.size()) {
        CreateSampleFilterNetwork(sceneDelegate);
    }
}

void
HdPrman_RenderParam::AddDisplayFilter(
    HdSceneDelegate *sceneDelegate,
    SdfPath const& path,
    riley::ShadingNode const& node)
{
    // Update or Add the DisplayFilter Shading Node
    const auto filterIt = _displayFilterNodes.insert({ path, node });
    if (!filterIt.second) {
        filterIt.first->second = node;
    }

    // If we have all the Shading Nodes, creat the DisplayFilters in Riley 
    if (_displayFilterNodes.size() == _connectedDisplayFilterPaths.size()) {
        CreateDisplayFilterNetwork(sceneDelegate);
    }
}

riley::SampleFilterList
HdPrman_RenderParam::GetSampleFilterList()
{
    return (_sampleFiltersId == riley::SampleFilterId::InvalidId()) 
        ? riley::SampleFilterList({ 0, nullptr })
        : riley::SampleFilterList({ 1, &_sampleFiltersId });
}

riley::DisplayFilterList
HdPrman_RenderParam::GetDisplayFilterList()
{
    return (_displayFiltersId == riley::DisplayFilterId::InvalidId())
        ? riley::DisplayFilterList({ 0, nullptr })
        : riley::DisplayFilterList({ 1, &_displayFiltersId });
}

HdPrmanInstancer*
HdPrman_RenderParam::GetInstancer(const SdfPath& id)
{
    if (id.IsEmpty()) { return nullptr; }
    if (HdRenderIndex* index = _renderDelegate->GetRenderIndex()) {
        return static_cast<HdPrmanInstancer*>(index->GetInstancer(id));
    }
    return nullptr;
}

bool
HdPrman_RenderParam::IsInteractive() const
{
    return _renderDelegate->IsInteractive();
}

PXR_NAMESPACE_CLOSE_SCOPE
