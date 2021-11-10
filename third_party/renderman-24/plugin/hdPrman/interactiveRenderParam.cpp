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
#include "hdPrman/interactiveRenderParam.h"

#include "pxr/base/arch/library.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdr/registry.h"

#include "hdPrman/rixStrings.h"
#include "hdPrman/renderDelegate.h"

#include "Riley.h"
#include "RiTypesHelper.h"
#include "RixRiCtl.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HD_PRMAN_ENABLE_MOTIONBLUR, true,
                      "Enable motion blur in HdPrman");
TF_DEFINE_ENV_SETTING(HD_PRMAN_NTHREADS, 0,
                      "Override number of threads used by HdPrman");
TF_DEFINE_ENV_SETTING(HD_PRMAN_OSL_VERBOSE, 0,
                      "Override osl verbose in HdPrman");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (sourceName)
    (sourceType)
    );

void 
HdPrman_InteractiveRenderParam::_RenderThreadCallback()
{
    static RtUString const US_RENDERMODE = RtUString("renderMode");
    static RtUString const US_INTERACTIVE = RtUString("interactive");

    // Note: this is currently hard-coded because hdprman only ever 
    // create a single camera. When this changes, we will need to make sure 
    // the correct name is used here.
    // Note: why not use us_main_cam defined earlier in the same file?
    static RtUString const defaultReferenceCamera = RtUString("main_cam");

    RtParamList renderOptions;
    renderOptions.SetString(US_RENDERMODE, US_INTERACTIVE);
    renderOptions.SetString(
        RixStr.k_dice_referencecamera, 
        defaultReferenceCamera);

    bool renderComplete = false;
    while (!renderComplete) {
        while (renderThread.IsPauseRequested()) {
            if (renderThread.IsStopRequested()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (renderThread.IsStopRequested()) {
            break;
        }

        _riley->Render(
            { static_cast<uint32_t>(renderViews.size()),
              renderViews.data()},
            renderOptions);

        // If a pause was requested, we may have stopped early
        renderComplete = !renderThread.IsPauseDirty();
    }
}

HdPrman_InteractiveRenderParam::HdPrman_InteractiveRenderParam() :
    sceneLightCount(0),
    resolution{0, 0},
    _fallbackLightEnabled(false),
    _didBeginRiley(false)
{
    TfRegistryManager::GetInstance().SubscribeTo<HdPrman_RenderParam>();
    renderThread.SetRenderCallback(
        std::bind(
            &HdPrman_InteractiveRenderParam::_RenderThreadCallback, this));
    _Initialize();
}

HdPrman_InteractiveRenderParam::~HdPrman_InteractiveRenderParam()
{
    End();
}

void 
HdPrman_InteractiveRenderParam::_Initialize()
{
    _InitializePrman();

    // Register RenderMan display driver
    HdPrmanFramebuffer::Register(_rix);
}

bool 
HdPrman_InteractiveRenderParam::IsValid() const
{
    return _riley;
}

void 
HdPrman_InteractiveRenderParam::Begin(HdRenderDelegate *renderDelegate)
{
    //////////////////////////////////////////////////////////////////////// 
    //
    // Riley setup
    //
    static const RtUString us_circle("circle");
    static const RtUString us_default("default");
    static const RtUString us_lightA("lightA");
    static const RtUString us_PxrDomeLight("PxrDomeLight");

    riley::CoordinateSystemList const k_NoCoordsys = { 0, nullptr };

    // XXX Shutter settings from studio katana defaults:
    // - /root.renderSettings.shutter{Open,Close}
    float shutterInterval[2] = { 0.0f, 0.5f };
    if (!TfGetEnvSetting(HD_PRMAN_ENABLE_MOTIONBLUR)) {
        shutterInterval[1] = 0.0;
    }

    // Options
    {
        // Set thread limit for Renderman. Leave a few threads for app.
        static const unsigned appThreads = 4;
        unsigned nThreads = std::max(WorkGetConcurrencyLimit()-appThreads, 1u);
        // Check the environment
        unsigned nThreadsEnv = TfGetEnvSetting(HD_PRMAN_NTHREADS);
        if (nThreadsEnv > 0) {
            nThreads = nThreadsEnv;
        } else {
            // Otherwise check for a render setting
            VtValue vtThreads = renderDelegate->GetRenderSetting(
                HdRenderSettingsTokens->threadLimit).Cast<int>();
            if (!vtThreads.IsEmpty()) {
                nThreads = vtThreads.UncheckedGet<int>();
            }
        }
        _options.SetInteger(RixStr.k_limits_threads, nThreads);

        // Set resolution from render settings
        const VtValue resolutionVal = renderDelegate->GetRenderSetting(
            HdPrmanRenderSettingsTokens->resolution);

        if (resolutionVal.IsHolding<GfVec2i>()) {
            auto const& res = resolutionVal.UncheckedGet<GfVec2i>();
            resolution[0] = res[0];
            resolution[1] = res[1];
            _options.SetIntegerArray(RixStr.k_Ri_FormatResolution,
                                     resolution, 2);
        }

        // Read the maxSamples out of settings (if it exists).
        // Use a low value to default to a non-expensive render.
        VtValue vtMaxSamples = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedSamplesPerPixel).Cast<int>();
        int maxSamples = TF_VERIFY(!vtMaxSamples.IsEmpty()) ?
            vtMaxSamples.UncheckedGet<int>() : 16;
        _options.SetInteger(RixStr.k_hider_minsamples, 1);
        _options.SetInteger(RixStr.k_hider_maxsamples, maxSamples);

        // Read the variance threshold out of settings (if it exists). Use a
        // default of 0.001.
        VtValue vtPixelVariance = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedVariance).Cast<float>();
        float pixelVariance = TF_VERIFY(!vtPixelVariance.IsEmpty()) ?
            vtPixelVariance.UncheckedGet<float>() : 0.001f;
        _options.SetFloat(RixStr.k_Ri_PixelVariance, pixelVariance);

        HdPrman_UpdateSearchPathsFromEnvironment(_options);

        // Path tracer config.
        _options.SetInteger(RixStr.k_hider_incremental, 1);
        _options.SetInteger(RixStr.k_hider_jitter, 1);
        _options.SetInteger(RixStr.k_trace_maxdepth, 10);
        _options.SetFloat(RixStr.k_Ri_FormatPixelAspectRatio, 1.0f);
        _options.SetString(RixStr.k_bucket_order, us_circle);

        // Camera lens
        _options.SetFloatArray(RixStr.k_Ri_Shutter, shutterInterval, 2);

        // OSL verbose
        int oslVerbose = TfGetEnvSetting(HD_PRMAN_OSL_VERBOSE);
        if (oslVerbose > 0)
            _options.SetInteger(RtUString("user:osl:verbose"), oslVerbose);

        // Searchpaths (TEXTUREPATH, etc)
        HdPrman_UpdateSearchPathsFromEnvironment(_options);
        
        // Set Options from RenderSettings schema
        SetOptionsFromRenderSettings(
            static_cast<HdPrmanRenderDelegate*>(renderDelegate), _options);
        
        _riley->SetOptions(_GetDeprecatedOptionsPrunedList());
    }

    _cameraContext.Begin(_riley);

    // Integrator
    // This needs to be set before setting
    // the active render target, below.
    _integratorId = riley::IntegratorId::InvalidId();
    {
        std::string integratorName = 
            renderDelegate->GetRenderSetting<std::string>(
                HdPrmanRenderSettingsTokens->integratorName,
                HdPrmanIntegratorTokens->PxrPathTracer.GetString());
        RtParamList params;

        SetIntegratorParamsFromRenderSettings(
                            static_cast<HdPrmanRenderDelegate*>(renderDelegate),
                            integratorName,
                            params);
        RtUString rmanIntegrator(integratorName.c_str()); 
        riley::ShadingNode &integratorNode = _activeIntegratorShadingNode;
        integratorNode = {
            riley::ShadingNode::Type::k_Integrator,
            rmanIntegrator,
            rmanIntegrator,
            params
        };

        _integratorId = _riley->CreateIntegrator(riley::UserId::DefaultId(),
                                               integratorNode);
    }


    // Light
    {
        // Light shader
        riley::ShadingNode lightNode {
            riley::ShadingNode::Type::k_Light, // type
            us_PxrDomeLight, // name
            us_lightA, // handle
            RtParamList()
        };
        _fallbackLightShader = _riley->CreateLightShader(
            riley::UserId::DefaultId(), {1, &lightNode}, {0, nullptr});

        // Constant identity transform
        float const zerotime = 0.0f;
        RtMatrix4x4 matrix = RixConstants::k_IdentityMatrix;
        riley::Transform xform = { 1, &matrix, &zerotime };

        // Light instance
        SdfPath fallbackLightId("/_FallbackLight");
        _fallbackLightEnabled = true;
        // Initialize default categories.
        ConvertCategoriesToAttributes(
            fallbackLightId, VtArray<TfToken>(), _fallbackLightAttrs);
        _fallbackLightAttrs.SetString(RixStr.k_grouping_membership,
                                       us_default);
        _fallbackLightAttrs.SetString(RixStr.k_identifier_name,
                                      RtUString(fallbackLightId.GetText()));
        _fallbackLightAttrs.SetInteger(RixStr.k_visibility_camera, 0);
        _fallbackLightAttrs.SetInteger(RixStr.k_visibility_indirect, 1);
        _fallbackLightAttrs.SetInteger(RixStr.k_visibility_transmission, 1);
        _fallbackLight = _riley->CreateLightInstance(
              riley::UserId::DefaultId(),
              riley::GeometryPrototypeId::InvalidId(), // no group
              riley::GeometryPrototypeId::InvalidId(), // no geo
              riley::MaterialId::InvalidId(), // no material
              _fallbackLightShader,
              k_NoCoordsys,
              xform,
              _fallbackLightAttrs);
    }

    _CreateFallbackMaterials();
}

void 
HdPrman_InteractiveRenderParam::SetIntegrator(riley::IntegratorId iid)
{
    _integratorId = iid;
    for (auto const& id : renderViews) {
        _riley->ModifyRenderView(id, nullptr, nullptr, &_integratorId,
                                nullptr, nullptr, nullptr);
    }
}

void 
HdPrman_InteractiveRenderParam::StartRender()
{
    // Last chance to set Ri options before starting riley!
    // Called from HdPrman_RenderPass::_Execute

    // Prepare Riley state for rendering.
    // Pass a valid riley callback pointer during IPR
    if (!_didBeginRiley) {
        renderThread.StartThread();
        _didBeginRiley = true;
    }

    renderThread.StartRender();
}

void 
HdPrman_InteractiveRenderParam::End()
{
    if (renderThread.IsThreadRunning()) {
        renderThread.StopThread();
    }

    // Reset to initial state.
    if (_mgr) {
        if(_riley) {
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
HdPrman_InteractiveRenderParam::SetFallbackLightsEnabled(bool enabled)
{
    if (_fallbackLightEnabled == enabled) {
        return;
    }
    _fallbackLightEnabled = enabled;

    // Stop render and crease sceneVersion to trigger restart.
    riley::Riley * riley = AcquireRiley();

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

void
HdPrman_InteractiveRenderParam::StopRender()
{
    if (renderThread.IsRendering()) {
        // It is necessary to call riley->Stop() until it succeeds
        // because it's possible for it to be skipped if called too early,
        // before the render has gotten underway.
        // Also keep checking if render thread is still active,
        // in case it has somehow managed to stop already.
        while((_riley->Stop() == riley::StopResult::k_NotRendering) &&
              renderThread.IsRendering())
        {
        }
        renderThread.StopRender();
    }
}

bool
HdPrman_InteractiveRenderParam::IsRenderStopped()
{
    return !renderThread.IsThreadRunning();
}

void
HdPrman_InteractiveRenderParam::CreateDisplays(
    const HdRenderPassAovBindingVector& aovBindings)
{
    // Proceed with creating displays if the number has changed
    // or the display names don't match what we have.
    bool needCreate = false;
    if(framebuffer.aovs.size() != aovBindings.size())
    {
        needCreate = true;
    }
    else
    {
        for( size_t aov = 0; aov < aovBindings.size(); ++aov )
        {
            if(aovBindings[aov].aovName != framebuffer.aovs[aov].name)
            {
                needCreate = true;
                break;
            }
            else if((aovBindings[aov].aovName == HdAovTokens->color ||
                    aovBindings[aov].aovName == HdAovTokens->depth) &&
                    (aovBindings[aov].clearValue !=
                     framebuffer.aovs[aov].clearValue))
            {
                // Request a framebuffer clear if the clear value in the aov
                // has changed from the framebuffer clear value.
                // We do this before StartRender() to avoid race conditions
                // where some buckets may get discarded or cleared with
                // the wrong value.

                // Stops render and increases sceneVersion to trigger restart.
                AcquireRiley();

                framebuffer.pendingClear = true;
                framebuffer.aovs[aov].clearValue = aovBindings[aov].clearValue;
            }
        }
    }

    if(!needCreate)
    {
        return;
    }

    // Stop render and crease sceneVersion to trigger restart.
    riley::Riley * riley = AcquireRiley();

    std::lock_guard<std::mutex> lock(framebuffer.mutex);

    static const RtUString us_bufferID("bufferID");
    static const RtUString us_hydra("hydra");
    static const RtUString us_ci("ci");
    static const RtUString us_st("__st");
    static const RtUString us_primvars_st("primvars:st");

    if(framebuffer.aovs.size())
    {
        framebuffer.aovs.clear();
        framebuffer.w = 0;
        framebuffer.h = 0;
        riley->DeleteRenderTarget(framebuffer.rtId);
        riley->DeleteDisplay(framebuffer.dspyId);
    }
    // Displays & Display Channels
    riley::RenderTargetId rtid = riley::RenderTargetId::InvalidId();

    riley::FilterSize const filterwidth = { 1.0f, 1.0f };

    std::vector<riley::RenderOutputId> renderOutputs;

    RtParamList renderOutputParams;

    std::unordered_map<RtUString, RtUString> sourceNames;
    for( size_t aov = 0; aov < aovBindings.size(); ++aov )
    {
        std::string dataType;
        std::string sourceType;
        RtUString aovName(aovBindings[aov].aovName.GetText());
        RtUString sourceName;
        riley::RenderOutputType rt = riley::RenderOutputType::k_Float;
        RtUString filterName = RixStr.k_filter;

        HdFormat aovFormat = aovBindings[aov].renderBuffer->GetFormat();

        // Prman always renders colors as float, so for types with 3 or 4
        // components, always set the format in our framebuffer to float.
        // Conversion will take place in the Blit method of renderBuffer.cpp
        // when it notices that the aovBinding's buffer format doesn't match
        // our framebuffer's format.
        int componentCount = HdGetComponentCount(aovFormat);
        if(componentCount == 3)
        {
            aovFormat = HdFormatFloat32Vec3;
        }
        else if(componentCount == 4)
        {
            aovFormat = HdFormatFloat32Vec4;
        }

        // Prman only supports float, color, and integer
        if(aovFormat == HdFormatFloat32)
        {
            rt = riley::RenderOutputType::k_Float;
        }
        else if(aovFormat == HdFormatFloat32Vec4 ||
                aovFormat == HdFormatFloat32Vec3)
        {
            rt = riley::RenderOutputType::k_Color;
        }
        else if(aovFormat == HdFormatInt32)
        {
            rt = riley::RenderOutputType::k_Integer;
        }

        // Look at the aovSettings to see if there is
        // information about the source.  In prman
        // an aov can have an arbitrary name, while its source
        // might be an lpe or a standard aov name.
        // When no source is specified, we'll assume the aov name
        // is standard and also use that as the source.
        for(auto it = aovBindings[aov].aovSettings.begin();
            it != aovBindings[aov].aovSettings.end(); it++)
        {
            if(it->first == _tokens->sourceName)
            {
                VtValue val = it->second;
                sourceName =
                    RtUString(
                            val.UncheckedGet<TfToken>().GetString().c_str());
            }
            else if(it->first == _tokens->sourceType)
            {
                VtValue val = it->second;
                sourceType = val.UncheckedGet<TfToken>().GetString();
            }
        }

        // If the sourceType hints that the source is an lpe, make sure
        // it starts with "lpe:" as required by prman.
        if(sourceType == RixStr.k_lpe.CStr())
        {
            std::string sn = sourceName.CStr();
            if(sn.find(RixStr.k_lpe.CStr()) == std::string::npos)
                sn = "lpe:" + sn;
            sourceName = RtUString(sn.c_str());
        }

        // Map some standard hydra aov names to their equivalent prman names
        if(aovBindings[aov].aovName == HdAovTokens->color ||
           aovBindings[aov].aovName.GetString() == us_ci.CStr())
        {
            aovName = RixStr.k_Ci;
            sourceName = RixStr.k_Ci;
        }
        else if(aovBindings[aov].aovName == HdAovTokens->depth)
        {
            sourceName = RixStr.k_z;
        }
        else if(aovBindings[aov].aovName == HdAovTokens->normal)
        {
            sourceName= RixStr.k_Nn;
        }
        else if(aovBindings[aov].aovName == HdAovTokens->primId)
        {
            aovName = RixStr.k_id;
            sourceName = RixStr.k_id;
        }
        else if(aovBindings[aov].aovName == HdAovTokens->instanceId)
        {
            aovName = RixStr.k_id2;
            sourceName = RixStr.k_id2;
        }
        else if(aovBindings[aov].aovName == HdAovTokens->elementId)
        {
            aovName = RixStr.k_faceindex;
            sourceName = RixStr.k_faceindex;
        }
        else if(aovName == us_primvars_st)
        {
            sourceName = us_st;
        }

        // If no sourceName is specified, assume name is a standard prman aov
        if(sourceName.Empty())
        {
            sourceName = aovName;
        }

        // XPU is picky about AOV names, it wants only standard names
        if(IsXpu())
        {
            aovName = sourceName;
        }

        // z and integer types require zmin filter
        if(sourceName == RixStr.k_z || rt == riley::RenderOutputType::k_Integer)
        {
            filterName = RixStr.k_zmin;
        }

        if(!sourceName.Empty())
        {
            // This is a workaround for an issue where we get an
            // unexpected duplicate in the aovBindings sometimes,
            // where the second entry lacks a sourceName.
            // Can't just skip it because the caller expects
            // a result in the buffer
            sourceNames[RtUString(aovBindings[aov].aovName.GetText())] =
                sourceName;
        }
        else
        {
            auto it =
                sourceNames.find(RtUString(aovBindings[aov].aovName.GetText()));
            if(it != sourceNames.end())
            {
                sourceName = it->second;
            }
        }

        renderOutputs.push_back(
            riley->CreateRenderOutput(riley::UserId::DefaultId(),
                                      aovName,
                                      rt,
                                      sourceName,
                                      filterName,
                                      RixStr.k_box,
                                      filterwidth,
                                      1.0f,
                                      renderOutputParams));
        framebuffer.AddAov(aovBindings[aov].aovName,
                           aovFormat,
                           aovBindings[aov].clearValue);

        // When a float4 color is requested, assume we require alpha as well.
        // This assumption is reflected in framebuffer.cpp HydraDspyData
        if(rt == riley::RenderOutputType::k_Color && componentCount == 4)
        {
            renderOutputs.push_back(
                riley->CreateRenderOutput(riley::UserId::DefaultId(),
                                          RixStr.k_a,
                                          riley::RenderOutputType::k_Float,
                                          RixStr.k_a,
                                          RixStr.k_filter,
                                          RixStr.k_box,
                                          filterwidth,
                                          1.0f,
                                          renderOutputParams));
        }
    }

    riley::Extent const renderTargetFormat = {
        static_cast<uint32_t>(resolution[0]),
        static_cast<uint32_t>(resolution[1]), 1};
    RtParamList renderTargetParams;
    rtid = riley->CreateRenderTarget(
        riley::UserId::DefaultId(),
        {(uint32_t)renderOutputs.size(), renderOutputs.data()},
        renderTargetFormat, RtUString("weighted"), 1.0f, renderTargetParams);
    framebuffer.rtId = rtid;

    if(IsXpu())
    {
        // XPU loads hdPrman as the display plug-in
        PlugPluginPtr plugin =
            PlugRegistry::GetInstance().GetPluginWithName("hdPrman");
        assert(plugin);
        std::string hdPrmanPath("");
        if (plugin) {
            std::string path = TfGetPathName(plugin->GetPath());
            if (!path.empty()) {
                hdPrmanPath =
                    TfStringCatPaths(path, "hdPrman");
            }
        }

        RtParamList displayParams;
        displayParams.SetString(RixStr.k_Ri_name, RixStr.k_framebuffer);
        displayParams.SetString(RixStr.k_Ri_type,
                                RtUString(hdPrmanPath.c_str()));
        displayParams.SetInteger(us_bufferID, framebuffer.id);
        framebuffer.dspyId = riley->CreateDisplay(
            riley::UserId::DefaultId(), framebuffer.rtId, RixStr.k_framebuffer,
            RtUString(hdPrmanPath.c_str()),
            {(uint32_t)renderOutputs.size(), renderOutputs.data()},
            displayParams);
    }
    else
    {
        RtParamList displayParams;
        framebuffer.dspyId = riley->CreateDisplay(
            riley::UserId::DefaultId(),
            framebuffer.rtId,
            RixStr.k_framebuffer,
            us_hydra,
            {(uint32_t)renderOutputs.size(), renderOutputs.data()},
            displayParams);
    }

    // For now, we always recreate RenderViews
    for (auto id : renderViews)
    {
        riley->DeleteRenderView(id);
    }
    renderViews.clear();

    riley::RenderViewId const renderView = riley->CreateRenderView(
        riley::UserId::DefaultId(),
        framebuffer.rtId,
        _cameraContext.GetCameraId(),
        _integratorId,
        {0, nullptr},
        {0, nullptr},
        RtParamList());
    renderViews.push_back(renderView);
    renderTargets[renderView] = framebuffer.rtId;
}

RtParamList&
HdPrman_InteractiveRenderParam::GetOptions() 
{
    return _options;
}

riley::IntegratorId
HdPrman_InteractiveRenderParam::GetActiveIntegratorId()
{
    return _integratorId;
}

riley::ShadingNode &
HdPrman_InteractiveRenderParam::GetActiveIntegratorShadingNode()
{
    return _activeIntegratorShadingNode;
}

HdPrmanCameraContext &
HdPrman_InteractiveRenderParam::GetCameraContext()
{
    return _cameraContext;
}

RtParamList 
HdPrman_InteractiveRenderParam::_GetDeprecatedOptionsPrunedList()
{
    // The following should not be given to Riley::SetOptions() anymore.
    static std::vector<RtUString> const _deprecatedRileyOptions = {
        RixStr.k_Ri_PixelFilterName, 
        RixStr.k_hider_pixelfiltermode, 
        RixStr.k_Ri_PixelFilterWidth,
        RixStr.k_Ri_ScreenWindow};

    RtParamList prunedOptions = _options;
    uint32_t paramId;
    for (auto name : _deprecatedRileyOptions) {
        if (prunedOptions.GetParamId(name, paramId)) {
            prunedOptions.Remove(paramId);
        }
    }

    return prunedOptions;
}

void
HdPrman_InteractiveRenderParam::InvalidateTexture(const std::string &path)
{
    _ri->InvalidateTexture(RtUString(path.c_str()));

    StopRender();
    sceneVersion.fetch_add(1);
}

riley::Riley *
HdPrman_InteractiveRenderParam::AcquireRiley()
{
    StopRender();
    sceneVersion++;

    return _riley;
}

PXR_NAMESPACE_CLOSE_SCOPE
