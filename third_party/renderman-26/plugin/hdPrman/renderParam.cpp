//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#include "hdPrman/projectionParams.h"
#include "hdPrman/renderDelegate.h"
#if PXR_VERSION >= 2308
#include "hdPrman/renderSettings.h"
#endif
#include "hdPrman/rixStrings.h"
#include "hdPrman/tokens.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/hd/extComputationUtils.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/arch/library.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/pathUtils.h"  // Extract extension from tf token
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/stringUtils.h"

#if PXR_VERSION >= 2302
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/containerDataSourceEditor.h"
#endif

#include <Riley.h>
#include <RiTypesHelper.h>
#include <RixRiCtl.h>
#include <RixShadingUtils.h>
#include <stats/Session.h>

#include <string>
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

    // Product/driver tokens
    (deepRaster)
    (deepexr)
    (openexr)
    ((riProductType, "ri:productType"))

    // See PxrDisplayChannelAPI
    ((riDisplayChannelNamespace,    "ri:displayChannel:"))
    // See PxrDisplayDriverAPI
    ((riDisplayDriverNamespace,     "ri:displayDriver:"))
    
    ((renderTagPrefix, "rendertag_"))
    (renderCameraPath)
    ((displayfilterPrefix,  "ri:displayfilter"))
    ((samplefilterPrefix,   "ri:samplefilter"))
);

TF_DEFINE_PRIVATE_TOKENS(
    _riOptionsTokens,
    ((riRiFormatResolution,         "ri:Ri:FormatResolution"))
    ((riRiShutter,                  "ri:Ri:Shutter"))
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
TF_DEFINE_ENV_SETTING(RMAN_XPU_GPUCONFIG, "0",
                      "A comma separated list of integers for which GPU devices to use.");

// We now have two env setting related to driving hdPrman rendering using the
// render settings prim. HD_PRMAN_RENDER_SETTINGS_DRIVE_RENDER_PASS ignores the
// task's AOV bindings and creates the render view using solely the render
// settings' products; this is limited to batch (non-interactive) rendering.
// The new setting HD_PRMAN_INTERACTIVE_RENDER_WITH_RENDER_SETTINGS creates the
// render view using both the task's AOV bindings and the render settings'
// products. The Hydra framebuffer is limited to displaying only the AOVs in
// the task bindings. This will be improved in a future change.
TF_DEFINE_ENV_SETTING(HD_PRMAN_INTERACTIVE_RENDER_WITH_RENDER_SETTINGS, false,
                      "Add render settings outputs to interactive renders");

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
    const int& xpuCpuConfig,
    const std::vector<int>& xpuGpuConfig,
    const std::vector<std::string>& extraArgs) :
    frame(0),
    _rix(nullptr),
    _ri(nullptr),
    _mgr(nullptr),
    _statsSession(nullptr),
    _progressPercent(0),
    _progressMode(0),
    _riley(nullptr),
#if PXR_VERSION >= 2302
    _statsSceneIndex(nullptr),
#endif
    _sceneLightCount(0),
    _fallbackLightEnabled(false),
    _shutterInterval(HDPRMAN_SHUTTEROPEN_DEFAULT, HDPRMAN_SHUTTERCLOSE_DEFAULT),
    _initRileyOptions(false),
    _sampleFiltersId(riley::SampleFilterId::InvalidId()),
    _displayFiltersId(riley::DisplayFilterId::InvalidId()),
    _lastLegacySettingsVersion(0),
    _resolution(0),
    _displayFiltersDirty(false),
    _sampleFiltersDirty(false),
    _sampleFilterId(riley::SampleFilterId::InvalidId()),
    _displayFilterId(riley::DisplayFilterId::InvalidId()),
    _renderDelegate(renderDelegate),
    _huskFrameStart(1),
    _huskFrameIncrement(1),
    _usingHusk(false),
    _useQN(false),
    _qnCheapPass(false),
    _qnMinSamples(2),
    _qnInterval(4)
{
#if _PRMANAPI_VERSION_MAJOR_ < 26
    // Create the stats session
    _CreateStatsSession();
#endif

    TfRegistryManager::GetInstance().SubscribeTo<HdPrman_RenderParam>();
    _CreateRiley(rileyVariant, xpuCpuConfig, xpuGpuConfig, extraArgs);
    
    // Register RenderMan display driver
    HdPrmanFramebuffer::Register(_rix);

    // Calling these before
    // RixSymbolResolver::ResolvePredefinedStrings (which is in _CreateRiley)
    // causes a crash.
    _envOptions = HdPrman_Utils::GetRileyOptionsFromEnvironment();
    _fallbackOptions = HdPrman_Utils::GetDefaultRileyOptions();
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
    HdSceneDelegate* sceneDelegate,
    const SdfPath& id,
    const GfVec2f& shutterInterval,
    RtPrimVarList& primvars,
    const size_t* npointsHint)
{
    HdExtComputationPrimvarDescriptorVector compPrimvar;
#if PXR_VERSION <= 2311
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
            sceneDelegate->SamplePrimvar(id, HdTokens->points,
#if HD_API_VERSION >= 68
                                         shutterInterval[0],
                                         shutterInterval[1],
#endif                                         
                                         &boxedPoints);
        } else {
            HdExtComputationUtils::SampledValueStore<HDPRMAN_MAX_TIME_SAMPLES>
                compSamples;
            HdExtComputationUtils::SampleComputedPrimvarValues<
                HDPRMAN_MAX_TIME_SAMPLES>(
                    compPrimvar, sceneDelegate, HDPRMAN_MAX_TIME_SAMPLES,
#if HD_API_VERSION >= 73
                    shutterInterval[0], shutterInterval[1],
#endif
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

    // Ignore any incorrectly sized points
    std::vector<float> shutterTimes;
    std::vector<size_t> pointsIndex;
    for (size_t i = 0; i < points.count; ++i) {
        if (points.values[i].empty()) {
            TF_WARN("<%s> primvar 'points' was empty", id.GetText());
            continue;
        }
        if (points.values[i].size() != npoints) {
            TF_WARN(
                "<%s> primvar 'points' size (%zu) did not match "
                "expected (%zu)",
                id.GetText(), points.values[i].size(), npoints);
            if(points.values[i].size() < npoints) {
                // Only skip if there aren't enough points available,
                // otherwise only warn.
                continue;
            }
        }
        shutterTimes.push_back(points.times[i]);
        pointsIndex.push_back(i);
    }

    // Set points primvars
    primvars.SetTimes(shutterTimes.size(), shutterTimes.data());
    for (size_t i = 0; i < pointsIndex.size(); ++i) {
        primvars.SetPointDetail(
            RixStr.k_P, (RtPoint3*)points.values[pointsIndex[i]].cdata(),
            RtDetailType::k_vertex, i);
    }

    return npoints;
}

void
HdPrman_ConvertPointsPrimvar(
    HdSceneDelegate* sceneDelegate,
    const SdfPath& id,
    const GfVec2f& shutterInterval,
    RtPrimVarList& primvars,
    const size_t npoints)

{
    _ConvertPointsPrimvar(
        sceneDelegate, id, shutterInterval, primvars, &npoints);
}

size_t
HdPrman_ConvertPointsPrimvarForPoints(
    HdSceneDelegate* sceneDelegate,
    const SdfPath&id,
    const GfVec2f& shutterInterval,
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
    // TODO: Should these be generated automatically from PRManPrimVars.args?
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
        TfToken("ri:attributes:volume:aggregate"),
        TfToken("ri:attributes:volume:aggregaterespectvisibility"),
        TfToken("ri:attributes:volume:dsominmax"),
        TfToken("ri:attributes:volume:dsovelocity"),
        TfToken("ri:attributes:volume:fps"),
        TfToken("ri:attributes:volume:shutteroffset"),
        TfToken("ri:attributes:volume:velocityshuttercorrection"),
        // SubdivisionMesh
        TfToken("ri:attributes:dice:pretessellate"),
        TfToken("ri:attributes:dice:watertight"),
        TfToken("ri:attributes:shade:faceset"),
        TfToken("ri:attributes:stitchbound:CoordinateSystem"),
        TfToken("ri:attributes:stitchbound:sphere"),
        // NuPatch
        TfToken("ri:attributes:trimcurve:sense"),
        // Curves
        TfToken("ri:attributes:curve:opacitysamples"),
        TfToken("ri:attributes:curve:widthaffectscurvature"),
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
         const GfVec2d &shutterInterval,
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
            static const char *riPrefix = "ri:";
            static const char *riAttrPrefix = "ri:attributes:";
            static const char *primvarsPrefix = "primvars:";
            const bool hasUserPrefix =
                TfStringStartsWith(primvar.name.GetString(), userAttrPrefix);
            const bool hasRiPrefix =
                TfStringStartsWith(primvar.name.GetString(), riPrefix);
            bool hasRiAttributesPrefix =
                TfStringStartsWith(primvar.name.GetString(), riAttrPrefix);
            const bool hasPrimvarsPrefix =
                    TfStringStartsWith(primvar.name.GetString(),primvarsPrefix);

            // Strip "primvars:" from the name
            TfToken primvarName = primvar.name;
            if (hasPrimvarsPrefix) {
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
            } else if (hasRiPrefix) {
                // For example, coming from USD:
                // "primvars:ri:dice:micropolygonlength".
                // See the USD PxrPrimvarsAPI schema for more examples.
                const char *strippedName = primvarName.GetText();
                strippedName += strlen(riPrefix);
                name = _GetPrmanPrimvarName(TfToken(strippedName), detail);
            } else {
                name = _GetPrmanPrimvarName(primvarName, detail);
            }

            // As HdPrman and USD have evolved over time, there have been
            // multiple representations allowed for RenderMan primvars:
            //
            //   1. "ri:FOO"
            //   2. "primvars:ri:attributes:FOO"
            //   3. "ri:atrtibutes:FOO"
            //
            // Warn if we encounter the same primvar multiple times:
            if (params.HasParam(name)) {
                TF_WARN("<%s> provided multiple representations of the primvar "
                        "'%s'", id.GetText(), name.CStr());
            }
            // When both ri:attributes and primvar:ri:attributes versions of 
            // the same primvars exist, the primvar:ri:attributes version should
            // win out.
            if (hasRiAttributesPrefix && !hasPrimvarsPrefix &&
                params.HasParam(name)) {
                continue;
            }
        } else {
            name = _GetPrmanPrimvarName(primvar.name, detail);
        }

        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> samples;
        sceneDelegate->SamplePrimvar(id, primvar.name,
#if HD_API_VERSION >= 68
                                     shutterInterval[0],
                                     shutterInterval[1],
#endif
                                     &samples);
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

        // For non-constant primvars, check array size to make sure it
        // matches the expected topology size.
        if (hdInterp != HdInterpolationConstant &&
            val.IsArrayValued() && 
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
                        int numVarying, int numFaceVarying,
                        const GfVec2d &shutterInterval,
                        float time)
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
                 primvarSizes[i],
                 shutterInterval,
                 time);
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
                                       SdfPath const& id, bool isGeometry)
{
    RtParamList attrs;

    // Convert Hydra instance-rate primvars, and "user:" prefixed
    // constant  primvars, to Riley attributes.
    const HdInterpolation hdInterpValues[] = {
        HdInterpolationConstant,
    };
    for (HdInterpolation hdInterp: hdInterpValues) {
        _Convert(sceneDelegate, id, hdInterp, attrs, 1, GetShutterInterval());
    }

    // Hydra id -> Riley Rix::k_identifier_name
    attrs.SetString(RixStr.k_identifier_name, RtUString(id.GetText()));

    // Hydra visibility -> Riley Rix::k_visibility
    if (!sceneDelegate->GetVisible(id)) {
        attrs.SetInteger(RixStr.k_visibility_camera, 0);
        attrs.SetInteger(RixStr.k_visibility_indirect, 0);
        attrs.SetInteger(RixStr.k_visibility_transmission, 0);
    }

    if (isGeometry) {
        // Hydra categories -> Riley k_grouping_membership
        // Note that lights and light filters also have a grouping membership,
        // but that comes from the light (linking) params.
        VtArray<TfToken> categories = sceneDelegate->GetCategories(id);

        if(id.IsPrimPropertyPath() && categories.size() == 0)
        {
            // Id of point instanced object comes in looking like a property,
            // eg. /instances.proto0_mesh_0_id0
            // The light linking may be at the parent level, so look there
            // for categories.
            SdfPath pid = id.GetParentPath();
            categories = sceneDelegate->GetCategories(pid);
        }

        ConvertCategoriesToAttributes(id, categories, attrs);

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
        if (!attrs.HasParam( RixStr.k_grouping_membership)) {
            // Clear any categories that may have been previously tacked on by
            // explicitly adding an empty string valued attribute.
            attrs.SetString( RixStr.k_grouping_membership, RtUString("") );
        }
        attrs.SetString( RixStr.k_lightfilter_subset, RtUString("") );
        attrs.SetString( RixStr.k_lighting_subset, RtUString("default") );
        TF_DEBUG(HDPRMAN_LIGHT_LINKING)
            .Msg("HdPrman: <%s> no categories; grouping:membership = \"\"; "
                 "lighting:subset = \"default\"; lightFilter:subset = \"\"\n",
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
#if PXR_VERSION >= 2311
                // Houdini 20 (with 2308) crashes sometimes with deferred sync
                // so always sync in HdPrmanMaterial::Sync like we used to.
                material->SyncToRiley(sceneDelegate, riley);
#endif
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
            if (token == _riOptionsTokens->riRiShutter) {
                // Shutter comes from the camera,
                // ignore if specified in render settings
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
            TfToken role;
            HdPrman_ProjectionParams::GetIntegratorParamRole(entry.first, role);
            HdPrman_Utils::SetParamFromVtValue(riName, entry.second,
                role, &params);
        }
    }
}

void
HdPrman_RenderParam::SetProjectionParamsFromRenderSettings(
    HdPrmanRenderDelegate *renderDelegate,
    std::string& projectionName,
    RtParamList& params)
{
    HdRenderSettingsMap renderSettings = renderDelegate->GetRenderSettingsMap();

    TfToken preFix(std::string("ri:projection:") + projectionName);
    for (auto const& entry : renderSettings) {
        TfToken token = entry.first;
        VtValue val = entry.second;

        bool hasRiPrefix = TfStringStartsWith(token.GetText(),
                                              preFix.GetText());
        if (hasRiPrefix) {
            // Strip namespace from USD.
            RtUString riName;
            riName = RtUString(token.GetText()+preFix.size()+1);
            TfToken role;
            HdPrman_ProjectionParams::GetProjectionParamRole(token, role);
            HdPrman_Utils::SetParamFromVtValue(riName, val, role, &params);
        }
    }
}

static std::string
_ExpandVarsInString(
    const std::string& input,
    const std::string& source,
    const int numberF,
    const int numberN)
{
    std::string output = input;
    static const char* formatStrings[]
        = { "%01d", "%02d", "%03d", "%04d", "%05d", "%d" };
    const bool hasAngleVars = (output.find('<') != std::string::npos);
    const bool hasDollarVars = (output.find('$') != std::string::npos);
    if (hasAngleVars || hasDollarVars) {

        // Expand number
        static const char* angleVarStringsF[]
            = { "<F1>", "<F2>", "<F3>", "<F4>", "<F5>", "<F>" };
        static const char* angleVarStringsN[]
            = { "<N1>", "<N2>", "<N3>", "<N4>", "<N5>", "<N>" };
        static const char* dollarVarStringsF[]
            = { "$F1", "$F2", "$F3", "$F4", "$F5", "$F" };
        static const char* dollarVarStringsN[]
            = { "$N1", "$N2", "$N3", "$N4", "$N5", "$N" };
        static const char* dollarBraceVarStringsF[]
            = { "${F1}", "${F2}", "${F3}", "${F4}", "${F5}", "${F}" };
        static const char* dollarBraceVarStringsN[]
            = { "${N1}", "${N2}", "${N3}", "${N4}", "${N5}", "${N}" };
        for (size_t i = 0; i < TfArraySize(formatStrings); ++i) {
            const std::string strF = TfStringPrintf(formatStrings[i], numberF);
            const std::string strN = TfStringPrintf(formatStrings[i], numberN);
            if (hasAngleVars) {
                output = TfStringReplace(output, angleVarStringsF[i], strF);
                output = TfStringReplace(output, angleVarStringsN[i], strN);
            }
            if (hasDollarVars) {
                output = TfStringReplace(output, dollarVarStringsF[i], strF);
                output = TfStringReplace(output, dollarVarStringsN[i], strN);
                output
                    = TfStringReplace(output, dollarBraceVarStringsF[i], strF);
                output
                    = TfStringReplace(output, dollarBraceVarStringsN[i], strN);
            }
        }

        // Expand source string
        if (hasAngleVars) {
            output = TfStringReplace(output, "<OS>", source);
        }
        if (hasDollarVars) {
            output = TfStringReplace(output, "$OS", source);
            output = TfStringReplace(output, "${OS}", source);
        }
    }

    // Support printf style formating in file name, like %04d
    if (output.find('%') != std::string::npos) {
        output = TfStringPrintf(output.c_str(), numberF);
    }

    return output;
}

static std::string
_AddFileSuffix(const std::string& filename, const std::string& suffix)
{
    std::string extension = TfGetExtension(filename);
    if (!extension.empty()) {
        extension = "." + extension;
    }
    const std::string base
        = filename.substr(0, filename.length() - extension.length());
    return base + suffix + extension;
}

void
HdPrman_RenderParam::SetBatchCommandLineArgs(
    VtValue const &cmdLine,
    RtParamList * options)
{
    if (!cmdLine.IsHolding<VtArray<std::string>>()) {
        _usingHusk = false;
        return;
    }
    _usingHusk = true;
    int huskTileIndex = 0;
    _huskTileSuffix = "";
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
            // Also disable if argument is -1.
            if (checkpointinterval.Empty() && *i != "-1" && *i != "0") {
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
        else if (*i == "--threads" || *i == "-j") {
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
            RtUString checkpointexitat;
            options->GetString(RixStr.k_checkpoint_exitat, checkpointexitat);
            // Checkpoint exitat from render settings wins
            if (checkpointexitat.Empty()) {
                try {
                    const int n = stoi(*i);
                    if (n > 0) {
                        options->SetString(RixStr.k_checkpoint_exitat,
                                    RtUString(i->c_str()));
                    }
                }
                catch (const std::invalid_argument &e) {
                    TF_WARN("Invalid argument to --timelimit\n");
                }
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
        else if (*i == "--frame" || *i == "-f") {
            ++i;
            if (i == end) {
                TF_WARN("No value found for --frame argument\n");
                break;
            }
            try {
                _huskFrameStart = stoi(*i);
            } catch (const std::invalid_argument& e) {
                TF_WARN("Invalid argument to --frame\n");
            } catch (const std::out_of_range& e) {
                TF_WARN("Invalid argument to --frame\n");
            }
        }
        else if (*i == "--frame-inc" || *i == "-i") {
            ++i;
            if (i == end) {
                TF_WARN("No value found for --frame-inc argument\n");
                break;
            }
            try {
                _huskFrameIncrement = stoi(*i);
            } catch (const std::invalid_argument& e) {
                TF_WARN("Invalid argument to --frame-inc\n");
            } catch (const std::out_of_range& e) {
                TF_WARN("Invalid argument to --frame-inc\n");
            }
        }
        else if (*i == "--tile-index") {
            ++i;
            if (i == end) {
                TF_WARN("No value found for --tile-index argument\n");
                break;
            }
            try {
                huskTileIndex = stoi(*i);
            } catch (const std::invalid_argument& e) {
                TF_WARN("Invalid argument to --tile-index\n");
            } catch (const std::out_of_range& e) {
                TF_WARN("Invalid argument to --tile-index\n");
            }
        }
        else if (*i == "--tile-suffix") {
            ++i;
            if (i == end) {
                TF_WARN("No value found for --tile-suffix argument\n");
                break;
            }
            _huskTileSuffix = i->c_str();
        }
    }

    // If we are rendering a tile expand the tile suffix.
    if (!_huskTileSuffix.empty()) {
        _huskTileSuffix = _ExpandVarsInString(
            _huskTileSuffix, "", huskTileIndex, huskTileIndex + 1);

        // Also rename stats files so they don't overwrite each other.
        RtUString legacyStatsFilename;
        if (options->GetString(
                RixStr.k_statistics_filename, legacyStatsFilename)) {
            const std::string newLegacyStatsFilename = _AddFileSuffix(
                std::string(legacyStatsFilename.CStr()), _huskTileSuffix);
            options->SetString(
                RixStr.k_statistics_filename,
                RtUString(newLegacyStatsFilename.c_str()));
        }
        RtUString legacyStatsXMLFilename;
        if (options->GetString(
                RixStr.k_statistics_xmlfilename, legacyStatsXMLFilename)) {
            const std::string newLegacyStatsXMLFilename = _AddFileSuffix(
                std::string(legacyStatsXMLFilename.CStr()), _huskTileSuffix);
            options->SetString(
                RixStr.k_statistics_xmlfilename,
                RtUString(newLegacyStatsXMLFilename.c_str()));
        }
        RtUString legacyStatsShaderProfile;
        if (options->GetString(
                RixStr.k_statistics_shaderprofile, legacyStatsShaderProfile)) {
            const std::string newLegacyStatsShaderProfile = _AddFileSuffix(
                std::string(legacyStatsShaderProfile.CStr()), _huskTileSuffix);
            options->SetString(
                RixStr.k_statistics_shaderprofile,
                RtUString(newLegacyStatsShaderProfile.c_str()));
        }

        // Roz stats: JSON Report listener output filename
        // This takes care of incorporating the tile suffix into the name
        RtUString statsJsonFilename;
        static const RtUString us_statistics_jsonFilename("statistics:jsonFilename");
        if (options->GetString(
                us_statistics_jsonFilename, statsJsonFilename)) {
            const std::string newStatsJsonFilename = _AddFileSuffix(
                std::string(statsJsonFilename.CStr()), _huskTileSuffix);
            options->SetString(
                us_statistics_jsonFilename,
                RtUString(newStatsJsonFilename.c_str()));
        }

        // Roz stats: JSON Report listener metric matching regexp
        // This takes care of incorporating the tile suffix into the name
        RtUString statsJsonMetricsRegexp;
        static const RtUString us_statistics_jsonMetricsRegexp("statistics:jsonMetricsRegexp");
        if (options->GetString(
                us_statistics_jsonMetricsRegexp, statsJsonMetricsRegexp)) {
            const std::string newStatsJsonMetricsRegexp = _AddFileSuffix(
                std::string(statsJsonMetricsRegexp.CStr()), _huskTileSuffix);
            options->SetString(
                us_statistics_jsonMetricsRegexp,
                RtUString(newStatsJsonMetricsRegexp.c_str()));
        }
    }

    // Force incremental to be enabled when checkpointing
    RtUString checkpointinterval;
    options->GetString(RixStr.k_checkpoint_interval,
                      checkpointinterval);
    RtUString checkpointexitat;
    options->GetString(RixStr.k_checkpoint_exitat, checkpointexitat);
    if(!checkpointinterval.Empty() || !checkpointexitat.Empty() || doSnapshot) {
        options->SetInteger(RixStr.k_hider_incremental, 1);
    }
}

static void
insertCombinerFilter(std::vector<riley::ShadingNode> &nodes)
{
    static const RtUString us_PxrSampleFilterCombiner("PxrSampleFilterCombiner");
    static const RtUString us_PxrDisplayFilterCombiner("PxrDisplayFilterCombiner");
    static const RtUString us_filter("filter");

    if(nodes.size() <= 1)
        return;

    bool isSample = (nodes[0].type == riley::ShadingNode::Type::k_SampleFilter);

    // Insert a combiner node that references the list of filters
    std::vector<RtUString> refVals;
    for(auto it = nodes.begin();
        it != nodes.end(); ++it)
    {
        refVals.push_back(it->handle);
    }

    riley::ShadingNode combiner;
    combiner.handle = isSample ?
            us_PxrSampleFilterCombiner :
            us_PxrDisplayFilterCombiner;
    combiner.type = isSample ?
        riley::ShadingNode::Type::k_SampleFilter :
        riley::ShadingNode::Type::k_DisplayFilter;
    combiner.name = combiner.handle;
    if(isSample)
        combiner.params.SetSampleFilterReferenceArray(us_filter,
                                                      refVals.data(),
                                                      refVals.size());
    else
        combiner.params.SetDisplayFilterReferenceArray(us_filter,
                                                       refVals.data(),
                                                       refVals.size());

    nodes.push_back(combiner);
}

void
HdPrman_RenderParam::_AddCryptomatteFixes(const RtUString& riName, VtValue& val)
{
    if (riName == RtUString("attribute")) {
        if (!val.IsEmpty()) {
            // translate primvars: to user:
            // for people who don't realize they need to
            // refer to rman attributes with user:
            std::string v = val.UncheckedGet<std::string>();
            v = TfStringReplace(v, "primvars:", "user:");
            val = VtValue(v);
        }
    }
    else if (riName == RtUString("filename") && !_huskTileSuffix.empty()) {
        if (!val.IsEmpty()) {
            // add the husk tile suffix (if one exists) so that cryptomattes
            // files do not overwrite each other
            std::string v = val.UncheckedGet<std::string>();
            v = _AddFileSuffix(v, _huskTileSuffix);
            val = VtValue(v);
        }
    }
}

void
HdPrman_RenderParam::SetFiltersFromRenderSettings(
    HdPrmanRenderDelegate *renderDelegate)
{
    HdRenderSettingsMap renderSettings = renderDelegate->GetRenderSettingsMap();

    std::vector<TfToken> prefixes;
    prefixes.push_back(_tokens->displayfilterPrefix);
    prefixes.push_back(_tokens->samplefilterPrefix);

    // Stop render and crease sceneVersion to trigger restart.
    riley::Riley * riley = AcquireRiley();
    if(!riley) {
        return;
    }

    for(auto const& prefix : prefixes)
    {

        // Create shading nodes for each sample filter
        // They're numbered starting with 1
        std::vector<riley::ShadingNode> nodes;
        int nodeIdx = 0;
        bool isSample = (prefix == _tokens->samplefilterPrefix);
        while(true)
        {
            std::string defaultFilterName("None");
            std::string nmStr = prefix.GetText();
            nmStr += std::to_string(nodeIdx);
            nmStr += ":name";
            std::string filterName =
                    renderDelegate->GetRenderSetting<std::string>(
                        TfToken(nmStr.c_str()),
                        defaultFilterName);
            if(!filterName.empty() && filterName != "None")
            {
                riley::ShadingNode sn;
                sn.name = RtUString(filterName.c_str());
                std::string handle = filterName += std::to_string(nodeIdx);
                sn.handle = RtUString(handle.c_str());
                sn.type = isSample ?
                    riley::ShadingNode::Type::k_SampleFilter :
                    riley::ShadingNode::Type::k_DisplayFilter;
                nodes.push_back(sn);
            }
            else
            {
                break;
            }
            nodeIdx++;
        }

        // Append filters collected from shading nodes,
        // which is not currently the primary workflow,
        // but they may be present
        if(isSample)
        {
            for(auto it = _sampleFilters.begin();
                it != _sampleFilters.end(); ++it)
            {
                nodes.push_back(it->second);
            }
        }
        else
        {
            for(auto it = _displayFilters.begin();
                it != _displayFilters.end(); ++it)
            {
                nodes.push_back(it->second);
            }
        }
        _sampleFiltersDirty = false;
        _displayFiltersDirty = false;

        if(!nodes.empty())
        {
            for (auto const& entry : renderSettings) {
                TfToken token = entry.first;
                VtValue val = entry.second;
                bool hasRiPrefix = TfStringStartsWith(token.GetText(),
                                                      prefix.GetText());
                if (hasRiPrefix) {
                    std::vector<std::string> toks = TfStringSplit(token, ":");
                    unsigned long idx = 0;
                    // strip off the index
                    // eg. ri:samplefilter0:PxrBackgroundSampleFilter:name
                    // eg. ri:displayfilter0:PxrBackgroundSampleFilter:name
                    RtUString riName;;
                    if(toks.size() == 4)
                    {
                        int offset = isSample ? 12 : 13;
                        std::string idxStr(toks[1].begin()+offset, toks[1].end());
                        if(!idxStr.empty())
                            idx = std::stoi(idxStr);
                    }
                    if(toks.size() != 4 ||
                       idx >= nodes.size())
                        continue;
                    std::vector<std::string> toks2;
                    toks2.push_back(toks[2]);
                    toks2.push_back(toks[3]);
                    riName = RtUString(toks2[1].c_str());
                    token = TfToken(TfStringJoin(toks2, ":"));

                    // Some specific fixes for cryptomatte sample filter
                    if (isSample && toks[2] == "PxrCryptomatte") {
                        _AddCryptomatteFixes(riName, val);
                    }

                    TfToken role;
                    HdPrman_ProjectionParams::GetFilterParamRole(token, role);

                    HdPrman_Utils::SetParamFromVtValue(riName, val,
                                   role, &nodes[idx].params);
                }
            }

            insertCombinerFilter(nodes);
        }

        riley::ShadingNetwork const filterNetwork =
            { static_cast<uint32_t>(nodes.size()), nodes.data() };

         if(isSample) {
             if (_sampleFilterId != riley::SampleFilterId::InvalidId()) {
                 riley->DeleteSampleFilter(_sampleFilterId);
                 _sampleFilterId = riley::SampleFilterId::InvalidId();
             }

           _sampleFilterId = riley->CreateSampleFilter(
                riley::UserId(stats::AddDataLocation("/sampleFilters").
                              GetValue()),
                filterNetwork,
                RtParamList());
        } else {
            if (_displayFilterId != riley::DisplayFilterId::InvalidId()) {
                riley->DeleteDisplayFilter(_displayFilterId);
                _displayFilterId = riley::DisplayFilterId::InvalidId();
             }

            _displayFilterId = riley->CreateDisplayFilter(
                riley::UserId(stats::AddDataLocation("/displayFilters").
                              GetValue()),
                filterNetwork,
                RtParamList());
        }
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

#if _PRMANAPI_VERSION_MAJOR_ > 25
    // Build unique server ID for live stats
    // TODO - report this to statsmgr_houdini via scene index
#ifdef WIN32
    auto pid =  _getpid();
#else
    auto pid =  getpid();
#endif
    const std::string serverId = "hdprman_statsserver_" + std::to_string(pid);

    // Update session config so this render's stats server is correctly registered
    sessionConfig.SetServerId(serverId);
#endif

    // Instantiate a stats Session from config object.
    _statsSession = &stats::AddSession(sessionConfig);

    // Validate and inform
    _statsSession->LogInfo("HDPRMan", "Created Roz stats session '" +
                                      _statsSession->GetName() + "'.");

#if _PRMANAPI_VERSION_MAJOR_ > 25
#if PXR_VERSION >= 2302
    // Session is created, now we want to propagate information about the
    // session into a hydra scene index where it can be extracted by
    // other modules interacting with the stats (e.g. live stats UI)

    // This name must match the string in client UI panels
    const std::string rmanStatsSceneIndexName = "RenderMan Stats";

    // Get pointer to a new scene index (ref-counted)
    _statsSceneIndex = HdRetainedSceneIndex::New();
    HdSceneIndexNameRegistry::GetInstance().RegisterNamedSceneIndex(
        rmanStatsSceneIndexName, _statsSceneIndex);

    // Editor for entering data into the scene index
    HdContainerDataSourceEditor editor;

    // Add serverID to the stats hydra scene index, to be picked up by UI code
    editor.Set(HdDataSourceLocator(TfToken("liveStatsServerId")),
        HdRetainedTypedSampledDataSource<std::string>::New(serverId));

    // Finalize addition of scene index information
    _statsSceneIndex->AddPrims({{SdfPath("/globals"), TfToken("globals"), 
        editor.Finish()}});
#endif
#endif
}

void HdPrman_RenderParam::_PRManSystemBegin(
    const std::vector<std::string>& extraArgs)
{
#if _PRMANAPI_VERSION_MAJOR_ >= 26
    // Must invoke PRManSystemBegin() and PRManRenderBegin()
    // before we start using Riley.
    // Turning off unwanted statistics warnings
    // TODO: Fix incorrect tear-down handling of these statistics in
    // interactive contexts as described in PRMAN-2353

    std::vector<std::string> sArgs;
    sArgs.reserve(3+extraArgs.size());
    sArgs.push_back(""); // Empty argv[0]: hdPrman will do Xcpt/signal handling
    sArgs.push_back("-woff");
    sArgs.push_back("R56008,R56009");
    sArgs.insert(std::end(sArgs), std::begin(extraArgs), std::end(extraArgs));

    // PRManSystemBegin expects array of char* rather than std::string
    std::vector<const char*> cArgs;
    cArgs.reserve(sArgs.size());
    std::transform(sArgs.begin(), sArgs.end(), std::back_inserter(cArgs),
                   [](const std::string& str) { return str.c_str();} );

    _ri->PRManSystemBegin(cArgs.size(),
        const_cast<const char **>(cArgs.data()));
#endif
}

int HdPrman_RenderParam::_PRManRenderBegin(
    const std::vector<std::string>& extraArgs)
{
    // Must invoke PRManSystemBegin() and PRManRenderBegin()
    // before we start using Riley.
    std::vector<std::string> sArgs;
#if _PRMANAPI_VERSION_MAJOR_ >= 26
    sArgs.reserve(2+extraArgs.size());
#else
    sArgs.reserve(5+extraArgs.size());
    sArgs.push_back("hdPrman");  // Empty argv[0]: hdPrman will do Xcpt/signal handling
    sArgs.push_back("-woff");
    sArgs.push_back("R56008,R56009");
#endif
    sArgs.push_back("-statssession");
    sArgs.push_back(_statsSession->GetName());
    sArgs.insert(std::end(sArgs), std::begin(extraArgs), std::end(extraArgs));

    // PRManRenderBegin expects array of char* rather than std::string
    std::vector<const char*> cArgs;
    cArgs.reserve(sArgs.size());
    std::transform(sArgs.begin(), sArgs.end(), std::back_inserter(cArgs),
                   [](const std::string& str) { return str.c_str();} );
#if _PRMANAPI_VERSION_MAJOR_ >= 26
    return _ri->PRManRenderBegin(cArgs.size(),
        const_cast<const char **>(cArgs.data()));
#else
    return _ri->PRManBegin(cArgs.size(), const_cast<char **>(cArgs.data()));
#endif
}

void
HdPrman_RenderParam::_CreateRiley(const std::string &rileyVariant,
                                  const int& xpuCpuConfig,
                                  const std::vector<int>& xpuGpuConfig,
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

    // mode is 1 for -Progress and 2 for -progress
    _progressMode =
        std::find(extraArgs.begin(),
                  extraArgs.end(),
                  "-Progress") != extraArgs.end() ? 1 :
        (std::find(extraArgs.begin(),
                   extraArgs.end(),
                   "-progress") != extraArgs.end() ? 2 : 0);

#if _PRMANAPI_VERSION_MAJOR_ >= 26
    // Initialize internals of PRMan system
    _PRManSystemBegin(extraArgs);

    // Create the RenderMan stats session
    _CreateStatsSession();
#endif

    // Instantiate PRMan renderer ahead of CreateRiley
    int err = _PRManRenderBegin(extraArgs);
    if (err)
    {
        TF_RUNTIME_ERROR("Could not initialize Renderer.");
        return;
    }

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

    _xpu = (!rileyVariant.empty() ||
            (rileyVariant.find("xpu") != std::string::npos));

    // Acquire Riley instance.
    _mgr = (RixRileyManager*)_rix->GetRixInterface(k_RixRileyManager);
    RtParamList renderConfigParams;
    if(IsXpu())
    {
        // Allow xpuGpuConfig to be overridden with RMAN_XPU_GPUCONFIG env var
        std::vector<int> xpuGpuConfigOverride;
        if(!xpuGpuConfig.empty()) {
            const std::string envXpuConfig =
                TfGetenv("RMAN_XPU_GPUCONFIG", "");
            if(!envXpuConfig.empty()) {
                std::vector<std::string> toks = TfStringSplit(envXpuConfig, ",");
                for(auto tok=toks.begin(); tok != toks.end(); ++tok) {
                    if(!tok->empty()) {
                        xpuGpuConfigOverride.push_back(atoi(tok->c_str()));
                    }
                }
            }
        }

        static const RtUString us_cpuConfig("xpu:cpuconfig");
        static const RtUString us_gpuConfig("xpu:gpuconfig");
        renderConfigParams.SetInteger(us_cpuConfig, xpuCpuConfig);
        renderConfigParams.SetIntegerArray(us_gpuConfig,
                                            xpuGpuConfigOverride.empty() ?
                                                xpuGpuConfig.data() :
                                                xpuGpuConfigOverride.data(),
                                            xpuGpuConfigOverride.empty() ?
                                                xpuGpuConfig.size() :
                                                xpuGpuConfigOverride.size());
    }

    static const RtUString us_statsSessionName("statsSessionName");
    renderConfigParams.SetString(us_statsSessionName,
                                 RtUString(_statsSession->GetName().c_str()));

    _riley = _mgr->CreateRiley(RtUString(rileyVariant.c_str()),
                               renderConfigParams);

    if(!_riley) {
        TF_RUNTIME_ERROR("Could not initialize riley API.");
        return;
    }
}

struct RenderOutputDataTypeDesc 
{
    riley::RenderOutputType rileyType;
    RtUString fileDataType;
};

static const std::unordered_map<std::string, RenderOutputDataTypeDesc> _RenderOutputDataTypeMap
{
    // Integer
    { "i8", { riley::RenderOutputType::k_Integer, US_NULL } },
    { "int8", { riley::RenderOutputType::k_Integer, US_NULL } },
    { "int", { riley::RenderOutputType::k_Integer, US_NULL } },
    { "int2", { riley::RenderOutputType::k_Vector, US_NULL } },
    { "int3", { riley::RenderOutputType::k_Vector, US_NULL } },
    { "int4", { riley::RenderOutputType::k_Vector, US_NULL } },
    { "int64", { riley::RenderOutputType::k_Integer, US_NULL } },

    // Unsigned Integer
    { "u8", { riley::RenderOutputType::k_Integer, RtUString("uint") } },
    { "uint8", { riley::RenderOutputType::k_Integer, RtUString("uint") } },
    { "uint", { riley::RenderOutputType::k_Integer, RtUString("uint") } },
    { "uint2", { riley::RenderOutputType::k_Vector, RtUString("uint") } },
    { "uint3", { riley::RenderOutputType::k_Vector, RtUString("uint") } },
    { "uint4", { riley::RenderOutputType::k_Vector, RtUString("uint") } },
    { "uint64", { riley::RenderOutputType::k_Integer, RtUString("uint") } },

    // Floating Point
    { "half", { riley::RenderOutputType::k_Float, RtUString("half") } },
    { "float16", { riley::RenderOutputType::k_Float, RtUString("half") } },
    { "float", { riley::RenderOutputType::k_Float, RtUString("float") } },
    { "double", { riley::RenderOutputType::k_Float, RtUString("float") } },

    // Vectors
    { "half2", { riley::RenderOutputType::k_Vector, RtUString("half") } },
    { "half3", { riley::RenderOutputType::k_Vector, RtUString("half") } },
    { "half4", { riley::RenderOutputType::k_Vector, RtUString("half") } },
    { "float2", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "float3", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "float4", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "double2", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "double3", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "double4", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "point3h", { riley::RenderOutputType::k_Vector, RtUString("half") } },
    { "point3f", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "point3d", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "vector3h", { riley::RenderOutputType::k_Vector, RtUString("half") } },
    { "vector3f", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "vector3d", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "normal3h", { riley::RenderOutputType::k_Vector, RtUString("half") } },
    { "normal3f", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "normal3d", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "texCoord2f", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "texCoord2d", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "texCoord2h", { riley::RenderOutputType::k_Vector, RtUString("half") } },
    { "texCoord3f", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "texCoord3d", { riley::RenderOutputType::k_Vector, RtUString("float") } },
    { "texCoord3h", { riley::RenderOutputType::k_Vector, RtUString("half") } },

    // Colors
    { "color2h", { riley::RenderOutputType::k_Color, RtUString("half") } },
    { "color2f", { riley::RenderOutputType::k_Color, RtUString("half") } },
    { "color2d", { riley::RenderOutputType::k_Color, RtUString("half") } },
    { "color3h", { riley::RenderOutputType::k_Color, RtUString("half") } },
    { "color3f", { riley::RenderOutputType::k_Color, RtUString("float") } },
    { "color3d", { riley::RenderOutputType::k_Color, RtUString("float") } },
    { "color4h", { riley::RenderOutputType::k_Color, RtUString("half") } },
    { "color4f", { riley::RenderOutputType::k_Color, RtUString("float") } },
    { "color4d", { riley::RenderOutputType::k_Color, RtUString("float") } },
    { "color2i8", { riley::RenderOutputType::k_Color, US_NULL } },
    { "color3i8", { riley::RenderOutputType::k_Color, US_NULL } },
    { "color4i8", { riley::RenderOutputType::k_Color, US_NULL } },
    { "color2u8", { riley::RenderOutputType::k_Color, RtUString("uint") } },
    { "color3u8", { riley::RenderOutputType::k_Color, RtUString("uint") } },
    { "color4u8", { riley::RenderOutputType::k_Color, RtUString("uint") } },
};

static riley::RenderOutputType
_ToRenderOutputType(const TfToken& t)
{
    auto type = _RenderOutputDataTypeMap.find(t);
    if (type == _RenderOutputDataTypeMap.end()) {
        TF_RUNTIME_ERROR(
            "Unimplemented renderVar dataType '%s'; "
            "skipping",
            t.GetText());
        return riley::RenderOutputType::k_Color;
    }

    return type->second.rileyType;
}

static RtUString
_ToFileDataType(const std::string& format)
{
    auto type = _RenderOutputDataTypeMap.find(format);
    if (type == _RenderOutputDataTypeMap.end()) {
        return US_NULL;
    }

    return type->second.fileDataType;
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

static
RtUString
_GetOutputDisplayDriverType(const std::string &extension)
{
    static const std::map<std::string,TfToken> extToDisplayDriver{
        { std::string("exr"),  TfToken("openexr") },
        { std::string("tif"),  TfToken("tiff") },
        { std::string("tiff"), TfToken("tiff") },
        { std::string("png"),  TfToken("png") }
    };
    
    const auto it = extToDisplayDriver.find(extension);
    if (it != extToDisplayDriver.end()) {
        return RtUString(it->second.GetText());
    }

    TF_WARN(
        "Could not determine display driver for product filename extension %s."
        "Falling back to openexr.", extension.c_str());

    return RtUString(_tokens->openexr.GetText());
}

// Overload used when creating the render view from a renderSpec dict.
static
RtUString
_GetOutputDisplayDriverType(const TfToken &name)
{
    const std::string outputExt = TfGetExtension(name.GetString());
    return _GetOutputDisplayDriverType(outputExt);
}

// Overload used when creating the render view from a render settings' product.
static
RtUString
_GetOutputDisplayDriverType(
    const VtDictionary &productSettings,
    const TfToken &productName,
    const TfToken &productType)
{
    // Use "ri:productType" from the product's namespaced settings if
    // available.
    const TfToken driverName =
        VtDictionaryGet<TfToken>(
            productSettings,
            _tokens->riProductType.GetText(),
            VtDefault = TfToken());

    if (!driverName.IsEmpty()) {
        return RtUString(driverName.GetText());
    }

    // Otherwise, use the extension from the product name and product type
    // to determine the driver.
    //
    const std::string outputExt = TfGetExtension(productName.GetString());

    if (productType == _tokens->deepRaster && outputExt == std::string("exr")) {
        return RtUString(_tokens->deepexr.GetText());
    }

    return _GetOutputDisplayDriverType(outputExt);
}

// Temporary workaround for RMAN-21883:
//
// The args file for d_openexr says the default for asrgba is 1.
// The code for d_openexr uses a default of 0.
//
// The args default is reflected into the USD Ri schema; consequently,
// USD app integrations may assume they can skip exporting values that
// match this value.  The result is that there is no way for users to
// request that value.
//
// Here, we update the default parameters to match the args file.
// If no value is present, we explicitly set it to 1.
static void
_ApplyOpenexrDriverWorkaround(HdPrman_RenderViewDesc::DisplayDesc *display)
{
    static const RtUString openexr("openexr");
    static const RtUString asrgba("asrgba");
    if (display->driver == openexr) {
        uint32_t paramId;
        if (!display->params.GetParamId(asrgba, paramId)) {
            display->params.SetInteger(asrgba, 1);
        }
    }
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

        // XXX Temporary; see RMAN-21883
        _ApplyOpenexrDriverWorkaround(&displayDesc);

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
    const RtUString& filter, const GfVec2f& filterWidth,
    std::vector<HdPrman_RenderViewDesc::RenderOutputDesc> *renderOutputDescs,
    std::vector<size_t> *renderOutputIndices);

#if PXR_VERSION >= 2308
static
HdPrman_RenderViewDesc
_ComputeRenderViewDesc(
    HdRenderSettings::RenderProducts const &products,
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
    if (!products.empty()) {
        renderViewDesc.resolution = products[0].resolution;
    } else {
        renderViewDesc.resolution = {1024, 768};
    }

    // TODO: Get filter and filterWidth from renderSettings prim
    // See comments in _UpdatePixelFilter method
    static const RtUString defaultPixelFilter = RixStr.k_box;
    static const GfVec2f defaultPixelFilterWidth(1.f, 1.f);

    /* RenderProduct */
    int renderVarIndex = 0;
    std::map<SdfPath, int> seenRenderVars;


    for (const HdRenderSettings::RenderProduct &product : products) {
        // Create a DisplayDesc for this RenderProduct
        HdPrman_RenderViewDesc::DisplayDesc displayDesc;
        displayDesc.name = RtUString(product.name.GetText());
        displayDesc.params = _ToRtParamList(product.namespacedSettings,
            _tokens->riDisplayDriverNamespace);
        displayDesc.driver = _GetOutputDisplayDriverType(
            product.namespacedSettings, product.name, product.type);

        // XXX Temporary; see RMAN-21883
        _ApplyOpenexrDriverWorkaround(&displayDesc);

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
                             defaultPixelFilter, defaultPixelFilterWidth,
                             &renderViewDesc.renderOutputDescs,
                             &renderOutputIndices);
        }
        renderViewDesc.displayDescs.push_back(displayDesc);
    }

    return renderViewDesc;
}
#endif

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

#if PXR_VERSION >= 2308
/// XXX This should eventually replace the above use of the RenderSpec
void 
HdPrman_RenderParam::CreateRenderViewFromRenderSettingsProducts(
    HdRenderSettings::RenderProducts const &products,
    HdPrman_RenderViewContext *renderViewContext)
{
    // XXX Ideally, the render terminals and camera context are provided as
    //     arguments. They are currently managed by render param.
    const HdPrman_RenderViewDesc renderViewDesc =
        _ComputeRenderViewDesc(
            products,
            GetCameraContext().GetCameraId(), 
            GetActiveIntegratorId(), 
            GetSampleFilterList(),
            GetDisplayFilterList());

    renderViewContext->CreateRenderView(renderViewDesc, AcquireRiley());

}
#endif

void
HdPrman_RenderParam::FatalError(const char* msg)
{
    _DestroyRiley();
    throw std::runtime_error(msg);
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
        if (_ri) {
            // Tear down renderer
#if _PRMANAPI_VERSION_MAJOR_ >= 26
            _ri->PRManRenderEnd();
#endif
        }
        _mgr = nullptr;
    }

    _riley = nullptr;

    if (_rix) {
        // Remove our exception handler
        RixXcpt* rix_xcpt = (RixXcpt*)_rix->GetRixInterface(k_RixXcpt);
        rix_xcpt->Unregister(&_xcpt);
    }

#if _PRMANAPI_VERSION_MAJOR_ >= 26
    if (_statsSession)
    {
        // We own the session, it's our responsibility to tell Roz to remove
        // its reference and free the memory
        stats::RemoveSession(*_statsSession);
        _statsSession = nullptr;
    }
#endif

    if (_ri) {
        // Final prman shutdown
#if _PRMANAPI_VERSION_MAJOR_ >= 26
        _ri->PRManSystemEnd();
#else
        _ri->PRManEnd();
#endif
        _ri = nullptr;
    }
}

void
HdPrman_RenderParam::UpdateRenderStats(VtDictionary &stats)
{
    // The GetRenderStats method owned by the hdPrman renderDelegate
    // is a callback that returns stats to hydra.  This method adds to
    // the dictionary the progress value that comes from
    // the rix progress callback.
    stats[_tokens->percentDone.GetString()] = _progressPercent;
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
            riley::UserId(stats::AddDataLocation("/_FallbackMaterial").
                          GetValue()),
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
            riley::UserId(stats::AddDataLocation("/_FallbackVolumeMaterial").
                              GetValue()),
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
    // Stop render and increase sceneVersion to trigger restart.
    riley::Riley *riley = AcquireRiley();
    if(!riley) {
        return;
    }
    riley->InvalidateTexture(RtUString(path.c_str()));
}

static
std::string
_GetIntegratorName(HdRenderDelegate * const renderDelegate)
{
    const std::string &integratorNameFromRS =
        renderDelegate->GetRenderSetting<std::string>(
            HdPrmanRenderSettingsTokens->integratorName,
            HdPrmanIntegratorTokens->PxrPathTracer.GetString());

    // Avoid potentially empty integrator
    return integratorNameFromRS.empty() ?
            HdPrmanIntegratorTokens->PxrPathTracer.GetString() :
            integratorNameFromRS;
}

riley::ShadingNode
HdPrman_RenderParam::_ComputeIntegratorNode(
    HdRenderDelegate * const renderDelegate,
    const HdPrmanCamera * const cam)
{
#if PXR_VERSION >= 2308
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
        
        // TODO: Adjust when PxrPathTracer adds support for excludeSubset
        if (integratorNodeType == HdPrmanIntegratorTokens->PbsPathTracer ||
            integratorNodeType == HdPrmanIntegratorTokens->PxrUnified) {
            _SetExcludeSubset(_lastExcludedRenderTags,
                rileyIntegratorNode.params);
        }
        return rileyIntegratorNode;
    }
#endif

    const std::string &integratorName(_GetIntegratorName(renderDelegate));

    const RtUString rtIntegratorName(integratorName.c_str());

    _integratorParams.Clear();

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
    
    // TODO: Adjust when PxrPathTracer adds support for excludeSubset
    if (integratorName == HdPrmanIntegratorTokens->PbsPathTracer.GetString() ||
        integratorName == HdPrmanIntegratorTokens->PxrUnified.GetString()) {
        _SetExcludeSubset(_lastExcludedRenderTags,
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
HdPrman_RenderParam::SetActiveRenderTags(
    const TfTokenVector& activeRenderTags,
    HdRenderIndex* renderIndex)
{
    // sort the active tags for set_difference
    TfTokenVector sortedTags(activeRenderTags);
    std::sort(sortedTags.begin(), sortedTags.end());
    
    // set for uniqueness, ordered for set_difference
    std::set<TfToken> rprimTags; 
    for (const SdfPath& id : renderIndex->GetRprimIds()) {
        const HdRprim* rprim = renderIndex->GetRprim(id);
        rprimTags.insert(rprim->GetRenderTag());
    }
    
    // fast set for comparison with cached
    TfToken::Set excludedTags;
    
    // All rprim tags not in activeTags should be excluded (rprim - active)
    std::set_difference(
        rprimTags.begin(), rprimTags.end(),
        sortedTags.begin(), sortedTags.end(),
        std::inserter(excludedTags, excludedTags.begin()));
    if (excludedTags != _lastExcludedRenderTags) {
        _lastExcludedRenderTags = excludedTags;
        UpdateIntegrator(renderIndex);
    }
}

void
HdPrman_RenderParam::AddRenderTagToGroupingMembership(
    const TfToken& renderTag,
    RtParamList& params)
{
    // XXX: UStrings cannot be concatenated, and the only way to initialize a
    // UString is with a char*. So things can get a little baroque here. The
    // temporary variables hopefully help make it readable.
    if (!renderTag.IsEmpty()) {
        const std::string renderTagString = TfStringPrintf("%s%s",
            _tokens->renderTagPrefix.GetText(), renderTag.GetText());
            
        RtUString membership;
        params.GetString(RixStr.k_grouping_membership, membership);
        
        if (membership.Empty()) {
            membership = RtUString(renderTagString.c_str());
        } else {
            const std::string membershipString = TfStringPrintf("%s %s",
                renderTagString.c_str(), membership.CStr());
            membership = RtUString(membershipString.c_str());
        }
        params.SetString(RixStr.k_grouping_membership, membership);
    }
}

/* static */
void
HdPrman_RenderParam::_SetExcludeSubset(
    const TfToken::Set& excludedTags,
    RtParamList& params) {
    // XXX: excludeSubset is not in RixStr.
    // (excludesubset is, but we need a capital S.)
    static const RtUString k_excludeSubset("excludeSubset");
    std::string exclude;
    for (const TfToken& tag : excludedTags) {
        if (tag.IsEmpty()) {
            continue;
        }
        if (!exclude.empty()) {
            exclude += " ";
        }
        exclude += _tokens->renderTagPrefix.GetString() + tag.GetString();
    }
    // XXX: This should be the only place anyone sets excludeSubset
    params.SetString(k_excludeSubset, RtUString(exclude.c_str()));
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
    static RtUString const US_PROGRESSMODE = RtUString("progressMode");

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
    renderOptions.SetInteger(US_PROGRESSMODE, _progressMode);

    HdPrman_RenderViewContext &ctx = GetRenderViewContext();
    const riley::RenderViewId renderViewIds[] = { ctx.GetRenderViewId() };

    _riley->Render(
        { static_cast<uint32_t>(TfArraySize(renderViewIds)),
          renderViewIds },
        renderOptions);
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
    // Initialize legacy options from the render settings map.
    UpdateLegacyOptions();
    
    // Force initialization of Riley scene options.
    // (see related comments in SetRileyOptions)
#if PXR_VERSION >= 2311 // avoid deferring for now because can cause crash
    if (!HdRenderIndex::IsSceneIndexEmulationEnabled() ||
        !TfGetEnvSetting(HD_PRMAN_DEFER_SET_OPTIONS))
#endif
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

        if(cameraPath.IsEmpty())
        {
            // When running in husk, the above query fails and
            // we expect to find renderCameraPath.
            const HdRenderSettingsMap renderSettings =
                renderDelegate->GetRenderSettingsMap();
            auto it = renderSettings.find(_tokens->renderCameraPath);
            if(it != renderSettings.end()) {
                std::string renderCameraPath =
                    it->second.UncheckedGet<std::string>();
                GetCameraContext().SetCameraPath(SdfPath(renderCameraPath));
            }
        }
    }

    // If the error handler gets a severe termination, including having no valid
    // license, terminate the render.
    if (_xcpt.handleExit) {
        End();
    }
}

// See comment in SetRileyOptions on when this function needs to be called.
void
HdPrman_RenderParam::_CreateInternalPrims()
{
    GetCameraContext().CreateRileyCamera(
        AcquireRiley(), HdPrman_CameraContext::GetDefaultReferenceCameraName());

#ifdef DO_FALLBACK_LIGHTS
    _CreateFallbackLight();
#endif
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
    if(!riley) {
        return;
    }

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
HdPrman_RenderParam::SetRileySceneIndexObserverOptions(
    RtParamList const &params)
{
    _rileySceneIndexObserverOptions = params;
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
        //   env > scene index observer > render settings prim >
        //                                    legacy settings map > fallback
        //
        // XXX: Some riley clients require certain options to be present
        // on every SetOptions call (e.g. XPU currently needs
        // ri:searchpath:texture). As a conservative measure, compose
        // all sources of options for initialization and subsequent updates.
        // Ideally, the latter would require just the legacy and prim options.

        RtParamList composedParams = HdPrman_Utils::Compose(
            _envOptions,
            _rileySceneIndexObserverOptions,
#if PXR_VERSION >= 2311 // causes issues for houdini 20, eg. bad shutter interval
            _renderSettingsPrimOptions, 
#endif
            GetLegacyOptions(),
            _fallbackOptions);

        RtParamList prunedOptions = HdPrman_Utils::PruneDeprecatedOptions(
                    composedParams);

        if (_renderDelegate->IsInteractive() && !_usingHusk) {
            prunedOptions = HdPrman_Utils::PruneBatchOnlyOptions(prunedOptions);
        }

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
HdPrman_RenderParam::End()
{
    DeleteRenderThread();
    _framebuffer.reset();
    _DestroyRiley();
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

    // Only let one thread try to stop things at once.
    std::lock_guard<std::mutex> lock(_stopMutex);

    while (_renderThread->IsRendering()) {
        {
            TRACE_SCOPE("riley::Stop");
            _riley->Stop();
        }
        using namespace std::chrono_literals;
        if (_renderThread->IsRendering()) {
            std::this_thread::sleep_for(1ms);
        }
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
    return false;
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
HdPrman_RenderParam::_UpdatePixelFilter()
{
    // Currently we still use the legacy Riley API where each AOV sets it's own
    // filter and filter width. This is impossible now that we only support
    // importance sampling and all AOVs must have the same value. Until the
    // Riley API is modified to set filter and filter width as an option, we
    // need to get the render setting ourselves and set it for each AOV.

    // WARNING: Defaults are hardcoded. 
    // Make sure they match the values in PRManOptions.args.
#if _PRMANAPI_VERSION_MAJOR_ >= 26
    static const std::string defaultPixelFilter("gaussian");
    static const GfVec2f defaultPixelFilterWidth(2.f, 2.f);
#else
    static const std::string defaultPixelFilter("box");
    static const GfVec2f defaultPixelFilterWidth(1.f, 1.f);
#endif

    const RtUString pixelFilter(
        _renderDelegate
            ->GetRenderSetting<std::string>(
                HdPrmanRenderSettingsTokens->pixelFilter, defaultPixelFilter)
            .c_str());
    const GfVec2f pixelFilterWidth = _renderDelegate->GetRenderSetting<GfVec2f>(
        HdPrmanRenderSettingsTokens->pixelFilterWidth, defaultPixelFilterWidth);

    if (pixelFilter != _pixelFilter || pixelFilterWidth != _pixelFilterWidth) {
        _pixelFilter = pixelFilter;
        _pixelFilterWidth = pixelFilterWidth;
        return true;
    }

    return false;
}

bool
HdPrman_RenderParam::_UpdateQNSettings()
{ 
    // look for QN settings
    const HdRenderSettingsMap renderSettingsMap =
        _renderDelegate->GetRenderSettingsMap();
    const bool useQN = _renderDelegate->GetRenderSetting<bool>(TfToken("rmanEnableQNDenoise"), false);        
    const bool qnCheapPass = _renderDelegate->GetRenderSetting<bool>(TfToken("rmanQNCheapPass"), false);        
    const int qnMinSamples = _renderDelegate->GetRenderSetting<int>(TfToken("rmanQNMinSamples"), 2);        
    const int qnInterval = _renderDelegate->GetRenderSetting<int>(TfToken("rmanQNInterval"), 4);        
    if (useQN != _useQN || qnCheapPass != _qnCheapPass ||
        qnMinSamples != _qnMinSamples || qnInterval != _qnInterval)
    {
        _useQN = useQN;
        _qnCheapPass = qnCheapPass;
        _qnMinSamples = qnMinSamples;
        _qnInterval = qnInterval;
        return true;
    }
    return false;
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
        return riley::RenderOutputType::k_Color;
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
    const TfToken &hdAovName,
    const std::string& sourceType,
    RtUString *rmanAovName,
    RtUString *rmanSourceName)
{
    static const RtUString us_st("__st");
    static const RtUString us_primvars_st("primvars:st");

    // Initialize rmanAovName with the HdAovName
    if (!hdAovName.GetString().empty()) {
        *rmanAovName = RtUString(hdAovName.GetText());
    }

    // If the sourceType hints that the source is an lpe or primvar, make sure
    // it starts with "lpe:" or "primvars:" as required by prman.
    if(sourceType == "lpe")
    {
        std::string sn = rmanSourceName->CStr();
        if(sn.rfind("lpe:", 0) == std::string::npos)
            sn = "lpe:" + sn;
        *rmanSourceName = RtUString(sn.c_str());
    }
    else if(sourceType == "primvar")
    {
        std::string sn = rmanSourceName->CStr();
        if(sn.rfind("primvars:", 0) == std::string::npos)
            sn = "primvars:" + sn;
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
}

// Return a RtParamList of the driver settings in the given aovSettings
// and update the Rman Aov and Source Names based on the aovSettings
static RtParamList
_GetOutputParamsAndUpdateRmanNames(
    const HdAovSettingsMap &aovSettings,
    RtUString *rmanAovName,
    RtUString *rmanSourceName)
{
    RtParamList params;
    std::string sourceType;
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
            sourceType =
                settingVal.GetWithDefault<TfToken>().GetString();
        }

        // Gather all properties with the 'driver:parameters:aov' prefix 
        // into the RtParamList, updating the hdAovName if needed. 
        else if (TfStringStartsWith(
                 settingName.GetText(), "driver:parameters:aov:") ||
                 TfStringStartsWith(
                 settingName.GetText(), "ri:driver:parameters:aov:")) {
            RtUString name(TfStringGetSuffix(settingName, ':').c_str());
            if (name == RixStr.k_name) {
                hdAovName = settingVal.IsHolding<std::string>() ?
                    TfToken(settingVal.Get<std::string>().c_str()) :
                    settingVal.Get<TfToken>();
            } else {
                HdPrman_Utils::SetParamFromVtValue(name, settingVal,
                    TfToken(), &params);
            }
        }
    }

    _UpdateRmanAovAndSourceName(
        hdAovName, sourceType, rmanAovName, rmanSourceName);

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
        if ((productName == RixStr.k_framebuffer) && !isXpu && _useQN)
        {
            // interactive denoiser is turned on
            std::string hdPrmanPath;
            if (PlugPluginPtr const plugin =
                PlugRegistry::GetInstance().GetPluginWithName("hdPrman")) {
                const std::string path = TfGetPathName(plugin->GetPath());
                if (!path.empty()) {
                    hdPrmanPath = 
                        TfStringCatPaths(path, "hdPrman" ARCH_LIBRARY_SUFFIX);
                }
                driver = RtUString("quicklyNoiseless");
                displayParams.SetString(RtUString("dspyDSOPath"), RtUString(hdPrmanPath.c_str()));
                displayParams.SetInteger(RtUString("cheaPass"), (int) _qnCheapPass); 
                displayParams.SetInteger(RtUString("minSamples"), _qnMinSamples);
                displayParams.SetInteger(RtUString("interval"), _qnInterval);
                displayParams.SetInteger(RtUString("normalAsColor"), 1);
                displayParams.SetInteger(RtUString("immediateClose"), 1);
            }
            else
            {
                TF_WARN("Failed to load display plugin\n");
            }
        }
        displayDesc.driver = driver;
        displayDesc.params = displayParams;
        displayDesc.renderOutputIndices = renderOutputIndices;

        renderViewDesc.displayDescs.push_back(std::move(displayDesc));
    }
}

RtUString
_AddRenderOutput(
    RtUString aovName,
    const TfToken &dataType,
    HdFormat aovFormat,
    RtUString sourceName,
    const RtParamList& params,
    const RtUString& filter,
    const GfVec2f& filterWidth,
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

    // Get the rule from the given RtParamList
    RtUString rule = RixStr.k_filter;
    if (!params.GetString(RixStr.k_rule, rule)) {
        params.GetString(RixStr.k_filter, rule);
    }
    if (rule != RixStr.k_min && rule != RixStr.k_max
        && rule != RixStr.k_zmin && rule != RixStr.k_zmax
        && rule != RixStr.k_sum && rule != RixStr.k_average) {
        rule = RixStr.k_filter;
    }

    // Adjust the rule/filter/filterSize as needed
    RtUString value;
    static const RtUString k_depth("depth");
    // "cpuTime" and "sampleCount" should use rule "sum"
    if (aovName == k_cpuTime || aovName == k_sampleCount) {
        rule = RixStr.k_sum;
    // "id", "id2", "z" and "depth" should use rule "zmin"
    } else if (aovName == RixStr.k_id || aovName == RixStr.k_id2 ||
              aovName == RixStr.k_z || aovName == k_depth ||
              rType == riley::RenderOutputType::k_Integer) {
        rule = RixStr.k_zmin;
    // If statistics are set, use that as the rule
    } else if (params.GetString(RixStr.k_statistics, value) &&
              !value.Empty() && value != k_none) {
        rule = value;
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

    // Should correspond to driver:parameters:aov:format parameter
    RtUString format = US_NULL;
    params.GetString(RtUString("format"), format);
    const RtUString fileDataType
        = (format) ? _ToFileDataType(format.CStr()) : US_NULL;
    if (fileDataType) {
        // Can't seem to use RixStr.k_filedatatype until ritokens.db has been
        // incremented
        static const RtUString k_filedatatype("filedatatype");
        extraParams.SetString(k_filedatatype, fileDataType);
    }

    // Create the RenderOutputDesc for this AOV
    {
        HdPrman_RenderViewDesc::RenderOutputDesc renderOutputDesc;
        renderOutputDesc.name = aovName;
        renderOutputDesc.type = rType;
        renderOutputDesc.sourceName = sourceName;
        renderOutputDesc.rule = rule;
        renderOutputDesc.filter = filter;
        renderOutputDesc.filterWidth = filterWidth;
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
        renderOutputDesc.rule = rule;
        renderOutputDesc.filter = filter;
        renderOutputDesc.filterWidth = filterWidth;
        renderOutputDesc.relativePixelVariance = relativePixelVariance;
        renderOutputDesc.params = extraParams;

        renderOutputDescs->push_back(std::move(renderOutputDesc));
        renderOutputIndices->push_back(renderOutputDescs->size()-1);
    }
    return rule;
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
#if PXR_VERSION >= 2308
    const HdRenderPassAovBindingVector& aovBindings,
    HdPrman_RenderSettings* renderSettings)
#else
    const HdRenderPassAovBindingVector& aovBindings)
#endif
{
    if (!_framebuffer) {
        _framebuffer = std::make_unique<HdPrmanFramebuffer>();
    }

    static bool useRenderSettingsProductsForInteractiveRenderView =
        TfGetEnvSetting(HD_PRMAN_INTERACTIVE_RENDER_WITH_RENDER_SETTINGS);

    const bool dirtyProductsOnRenderSettingsPrim =
#if PXR_VERSION >= 2411
           useRenderSettingsProductsForInteractiveRenderView
        && renderSettings
        && renderSettings->GetAndResetHasDirtyProducts();
#else
    false;
#endif

    // Update the Pixel Filter and Pixel Filter Width
    const bool pixelFilterChanged = _UpdatePixelFilter();
    const bool qnChanged = _UpdateQNSettings();

    // Early exit if the render output is unchanged
    if (!dirtyProductsOnRenderSettingsPrim && _lastBindings == aovBindings && !pixelFilterChanged && !qnChanged) {
        return;
    }

    // Proceed with creating displays if the number has changed
    // or the display names don't match what we have.

    // Stop render and crease sceneVersion to trigger restart.
    riley::Riley * riley = AcquireRiley();

    std::lock_guard<std::mutex> lock(_framebuffer->mutex);

    _framebuffer->pendingClear = true;

    _lastBindings = aovBindings;

    // Displays & Display Channels
    HdPrman_RenderViewDesc renderViewDesc;

    // Process AOV bindings.
    {
        std::vector<size_t> renderOutputIndices;
        HdPrmanFramebuffer::AovDescVector aovDescs;

        std::unordered_map<TfToken, RtUString, TfToken::HashFunctor> sourceNames;
        for (const HdRenderPassAovBinding &aovBinding : aovBindings) {
            TfToken dataType;
            std::string sourceType;
            RtUString rmanAovName(aovBinding.aovName.GetText());
            RtUString rmanSourceName;
            HdFormat aovFormat = aovBinding.renderBuffer->GetFormat();
            _AdjustColorFormat(&aovFormat);

            RtParamList renderOutputParams =
                _GetOutputParamsAndUpdateRmanNames(aovBinding.aovSettings,
                                 &rmanAovName,
                                 &rmanSourceName);

            if(!rmanSourceName.Empty()) {
                // This is a workaround for an issue where we get an
                // unexpected duplicate in the aovBindings sometimes,
                // where the second entry lacks a sourceName.
                // Can't just skip it because the caller expects
                // a result in the buffer.
                sourceNames[aovBinding.aovName] = rmanSourceName;
            } else {
                auto it = sourceNames.find(aovBinding.aovName);
                if(it != sourceNames.end())
                {
                    rmanSourceName = it->second;
                }
            }

            RtUString rule = _AddRenderOutput(rmanAovName,
                                              dataType,
                                              aovFormat,
                                              rmanSourceName,
                                              renderOutputParams,
                                              _pixelFilter,
                                              _pixelFilterWidth,
                                              &renderViewDesc.renderOutputDescs,
                                              &renderOutputIndices);

            {
                HdPrmanFramebuffer::AovDesc aovDesc;
                aovDesc.name = aovBinding.aovName;
                aovDesc.format = aovFormat;
                aovDesc.clearValue = aovBinding.clearValue;
                aovDesc.rule = HdPrmanFramebuffer::ToAccumulationRule(rule);

                aovDescs.push_back(std::move(aovDesc));
            }
        }

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
    }

#if PXR_VERSION >=2308
    if (useRenderSettingsProductsForInteractiveRenderView && renderSettings) {

        // Get the descriptors for the render settings products.
        // N.B. this overrides the camera opinion on the product.  That
        // isn't the intent in case it becomes a problem.
        auto rsrvd =
            _ComputeRenderViewDesc(renderSettings->GetRenderProducts(),
                                   renderViewDesc.cameraId, 
                                   renderViewDesc.integratorId, 
                                   renderViewDesc.sampleFilterList,
                                   renderViewDesc.displayFilterList);

        // Adjust indices to account for the ones we already have.  The
        // entries in rsrvd.renderOutputIndices index into
        // rsrvd.renderOutputDescs.  Since we're moving the latter's
        // entries to the end of renderViewDesc.renderOutputDescs we must
        // adjust the indices to reflect their new positions.
        const auto base = renderViewDesc.renderOutputDescs.size();
        for (auto& displayDesc: rsrvd.displayDescs) {
            for (auto& index: displayDesc.renderOutputIndices) {
                index += base;
            }
        }

        // Add to final lists.
        renderViewDesc.renderOutputDescs.insert(
            renderViewDesc.renderOutputDescs.end(),
            std::make_move_iterator(rsrvd.renderOutputDescs.begin()),
            std::make_move_iterator(rsrvd.renderOutputDescs.end()));
        renderViewDesc.displayDescs.insert(
            renderViewDesc.displayDescs.end(),
            std::make_move_iterator(rsrvd.displayDescs.begin()),
            std::make_move_iterator(rsrvd.displayDescs.end()));
    }
#endif

    TF_DEBUG(HDPRMAN_RENDER_PASS)
        .Msg("Create Riley RenderView from AOV bindings: #renderOutputs = %zu"
             " ,#displays = %zu.\n", renderViewDesc.renderOutputDescs.size(),
             renderViewDesc.displayDescs.size());

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

    // Update the Pixel Filter and Pixel Filter Width
    _UpdatePixelFilter();

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
                                          "driver:parameters:") ||
                       TfStringStartsWith(settingName.GetText(),
                                          "ri:driver:parameters:")) {
                driverParameters.push_back(settingName);
            }
        }

        // If an --output or -o has been specified on command line, override the
        // product's name and expand variables:
        // <OS> : source prim (render  product node name) 
        // <F>, <F1>, <F2>, <F3>, <F4>, <F5> : frame number with padding 
        // <N> : the ordinial frame number
        // Vars can also use dollar style (braces optional) eg. $F4
        // ${F4} $OS or printf style formatting: %04d
        std::string outputName;
        const int ordinalFrame = std::max(
            1, ((frame - _huskFrameStart) / _huskFrameIncrement) + 1);
        if (idx < _outputNames.size()) {
            outputName = _ExpandVarsInString(
                _outputNames[idx], sourcePrimName, frame, ordinalFrame);
        }
        // If there are less outputNames than products, use the first
        // outputName only if it contains variables (so we don't overwrite the
        // first image).
        else if (!_outputNames.empty()) {
            outputName = _ExpandVarsInString(
                _outputNames[0], sourcePrimName, frame, ordinalFrame);
            if (_outputNames[0] == outputName) {
                outputName = "";
            }
        }
        if (!outputName.empty()) {
            // If we have a tile suffix make sure we add it to our outputName
            if (!_huskTileSuffix.empty()) {
                outputName = _AddFileSuffix(outputName, _huskTileSuffix);
            }
            productName = TfToken(outputName);
        }

        // Build Display Settings ParamList using the driverParameters gathered 
        // above from the Render Product Settings
        RtParamList displayParams;
        for (const TfToken& paramName : driverParameters) {
            std::string suffix = TfStringGetSuffix(paramName, ':');

            // Support solaris stlye exr settings
            if (TfStringStartsWith(paramName, "driver:parameters:OpenEXR:")) {
                if (suffix == "dwa_compression") {
                    suffix = "compressionlevel";
                }
                else if (suffix != "compression") {
                    suffix = "exrheader_" + suffix;
                }
            }
            else if (
                suffix == "artist" || suffix == "comment"
                || suffix == "hostname") {
                suffix = "exrheader_" + suffix;
            }

            auto val = renderProduct.find(paramName);
            if (val != renderProduct.end()) {
                const RtUString name(suffix.c_str());
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
                    &rmanAovName,
                    &rmanSourceName);

            // Create the RenderOutputDesc for this AOV/RenderVar
            _AddRenderOutput(rmanAovName,
                             dataType,
                             aovFormat,
                             rmanSourceName,
                             renderOutputParams,
                             _pixelFilter,
                             _pixelFilterWidth,
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
    renderViewDesc.sampleFilterList = GetSampleFilterList();
    renderViewDesc.displayFilterList = GetDisplayFilterList();

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

static
std::string
_GetQuickIntegratorName(HdRenderDelegate * const renderDelegate)
{
    const std::string &integratorNameFromRS =
        renderDelegate->GetRenderSetting<std::string>(
            HdPrmanRenderSettingsTokens->interactiveIntegrator,
            HdPrmanIntegratorTokens->PxrDirectLighting.GetString());

    // Avoid potentially empty integrator
    return integratorNameFromRS.empty() ?
            HdPrmanIntegratorTokens->PxrDirectLighting.GetString() :
            integratorNameFromRS;
}

riley::ShadingNode
HdPrman_RenderParam::_ComputeQuickIntegratorNode(
    HdRenderDelegate * const renderDelegate,
    const HdPrmanCamera * const cam)
{
    const std::string &integratorName(_GetQuickIntegratorName(renderDelegate));

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
        const std::string &integratorName(
            _GetQuickIntegratorName(renderDelegate));

        _quickIntegratorId = _riley->CreateIntegrator(
            riley::UserId(
                stats::AddDataLocation(integratorName.c_str()).GetValue()),
            _ComputeQuickIntegratorNode(renderDelegate, camera));

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

#if PXR_VERSION >= 2308
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
#endif

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

void
HdPrman_RenderParam::_CreateFallbackLight()
{
    static const RtUString us_PxrDomeLight("PxrDomeLight");
    static const RtUString us_lightA("lightA");    
    static const RtUString us_traceLightPaths("traceLightPaths");
    static const RtUString us_lightGroup("lightGroup");
    static const RtUString us_A("A");

    RtParamList nodeParams;
    nodeParams.SetFloat(RixStr.k_intensity, 1.0f);
    nodeParams.SetInteger(us_traceLightPaths, 1);
    nodeParams.SetString(us_lightGroup, us_A);

    // Light shader
    const riley::ShadingNode lightNode {
        riley::ShadingNode::Type::k_Light, // type
            us_PxrDomeLight, // name
            us_lightA, // handle
            nodeParams
            };
    _fallbackLightShader = _riley->CreateLightShader(
        riley::UserId::DefaultId(), {1, &lightNode}, {0, nullptr});
    
    riley::CoordinateSystemList const k_NoCoordsys = { 0, nullptr };
    
    // Constant identity transform
    const float zerotime[1] = { 0.0f };
    const RtMatrix4x4 matrix[1] = { RixConstants::k_IdentityMatrix };
    const riley::Transform xform = { 1, matrix, zerotime };

    // Light instance
    const SdfPath fallbackLightId("/_FallbackLight");

    // Initialize default categories.
    ConvertCategoriesToAttributes(
        fallbackLightId,
        VtArray<TfToken>(),
        _fallbackLightAttrs);

    static const RtUString us_default("default");

    _fallbackLightAttrs.SetString(
        RixStr.k_grouping_membership,
        us_default);
    _fallbackLightAttrs.SetString(
        RixStr.k_identifier_name,
        RtUString(fallbackLightId.GetText()));
    _fallbackLightAttrs.SetInteger(
        RixStr.k_visibility_camera,
        0);
    _fallbackLightAttrs.SetInteger(
        RixStr.k_visibility_indirect,
        1);
    _fallbackLightAttrs.SetInteger(
        RixStr.k_visibility_transmission,
        1);
    _fallbackLightAttrs.SetInteger(
        RixStr.k_lighting_mute,
        !_fallbackLightEnabled);

    _fallbackLight = _riley->CreateLightInstance(
        riley::UserId(
            stats::AddDataLocation(fallbackLightId.GetText()).GetValue()),
        riley::GeometryPrototypeId::InvalidId(), // no group
        riley::GeometryPrototypeId::InvalidId(), // no geo
        riley::MaterialId::InvalidId(), // no material
        _fallbackLightShader,
        k_NoCoordsys,
        xform,
        _fallbackLightAttrs);
}

void
HdPrman_RenderParam::SetFallbackLightsEnabled(bool enabled)
{
    if (_fallbackLightEnabled == enabled) {
        return;
    }
    _fallbackLightEnabled = enabled;

    // Stop render and crease sceneVersion to trigger restart.
    riley::Riley * riley = AcquireRiley();
    if(!riley) {
        return;
    }

    _fallbackLightAttrs.SetInteger(RixStr.k_lighting_mute, !enabled);

    riley->ModifyLightInstance(
          riley::GeometryPrototypeId::InvalidId(), // no group
          _fallbackLight,
          nullptr, // no material change
          nullptr, // no shader change
          nullptr, // no coordsys change
          nullptr, // no xform change
          &_fallbackLightAttrs);
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

static const float*
_GetShutterParam(const RtParamList &params)
{
    return params.GetFloatArray(RixStr.k_Ri_Shutter, 2);
}

void
HdPrman_RenderParam::_UpdateShutterInterval(const RtParamList& composedParams)
{
    if (const float* val = _GetShutterParam(composedParams)) {
        _shutterInterval = GfVec2f(val[0], val[1]);
    }

    // When there's only one sample available the motion blur plug-in
    // doesn't have access to the correct shutter interval, so this is
    // a workaround to provide it.
    HdPrman_MotionBlurSceneIndexPlugin::SetShutterInterval(
        _shutterInterval[0], _shutterInterval[1]);
}

PXR_NAMESPACE_CLOSE_SCOPE
