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

#include "context.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/renderDelegate.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/usd/sdr/registry.h"

#include "Riley.h"
#include "RtParamList.h"
#include "RixRiCtl.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

void HdxPrman_RenderThreadCallback(HdxPrman_InteractiveContext *context)
{
    riley::RenderSettings settings;
    settings.mode = riley::RenderMode::k_Interactive;
    bool renderComplete = false;
    while (!renderComplete) {
        while (context->renderThread.IsPauseRequested()) {
            if (context->renderThread.IsStopRequested()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (context->renderThread.IsStopRequested()) {
            break;
        }
        context->riley->Render(context->renderViews.size(),
                           context->renderViews.data(), settings);        
        // If a pause was requested, we may have stopped early
        renderComplete = !context->renderThread.IsPauseDirty();
    }
}

HdxPrman_InteractiveContext::HdxPrman_InteractiveContext() :
    sceneLightCount(0),
    resolution{0, 0},
    _fallbackLightEnabled(false),
    _didBeginRiley(false)
{
    TfRegistryManager::GetInstance().SubscribeTo<HdPrman_Context>();
    renderThread.SetRenderCallback(std::bind(
        HdxPrman_RenderThreadCallback, this));
    _Initialize();
}

HdxPrman_InteractiveContext::~HdxPrman_InteractiveContext()
{
    End();    
}

TF_DEFINE_ENV_SETTING(HDX_PRMAN_ENABLE_MOTIONBLUR, true,
                      "bool env setting to control hdPrman motion blur");
TF_DEFINE_ENV_SETTING(HDX_PRMAN_NTHREADS, 0,
                      "override number of threads used by hdPrman");
TF_DEFINE_ENV_SETTING(HDX_PRMAN_OSL_VERBOSE, 0,
                      "override osl verbose in hdPrman");

void HdxPrman_InteractiveContext::_Initialize()
{
    rix = RixGetContext();
    if (!rix) {
        TF_RUNTIME_ERROR("Could not initialize Rix API.");
        return;
    }
    ri = (RixRiCtl*)rix->GetRixInterface(k_RixRiCtl);
    if (!ri) {
        TF_RUNTIME_ERROR("Could not initialize Ri API.");
        return;
    }

    // Must invoke PRManBegin() before we start using Riley.
    char arg0[] = "hdxPrman";
    char* argv[] = { arg0 };
    ri->PRManBegin(1, argv);

    // Register an Xcpt handler
    RixXcpt* rix_xcpt = (RixXcpt*)rix->GetRixInterface(k_RixXcpt);
    rix_xcpt->Register(&xcpt);

    // Populate RixStr struct
    RixSymbolResolver* sym = (RixSymbolResolver*)rix->GetRixInterface(
        k_RixSymbolResolver);
    sym->ResolvePredefinedStrings(RixStr);

    // Sanity check symbol resolution with a canary symbol, shutterTime.
    // This can catch accidental linking with incompatible versions.
    TF_VERIFY(RixStr.k_shutterOpenTime == RtUString("shutterOpenTime"),
              "Renderman API tokens do not match expected values.  "
              "There may be a compile/link version mismatch.");

    // Acquire Riley instance.
    mgr = (RixRileyManager*)rix->GetRixInterface(k_RixRileyManager);
    riley = mgr->CreateRiley(nullptr);

    if(!riley)
    {
        return;
    }

    // Register RenderMan display driver
    HdxPrmanFramebuffer::Register(rix);
}

bool HdxPrman_InteractiveContext::IsValid() const
{
    return (riley != nullptr);
}

void HdxPrman_InteractiveContext::Begin(HdRenderDelegate *renderDelegate)
{
    //////////////////////////////////////////////////////////////////////// 
    //
    // Riley setup
    //
    static const RtUString us_bufferID("bufferID");
    static const RtUString us_circle("circle");
    static const RtUString us_defaultColor("defaultColor");
    static const RtUString us_default("default");
    static const RtUString us_density("density");
    static const RtUString us_densityFloatPrimVar("densityFloatPrimVar");
    static const RtUString us_diffuseColor("diffuseColor");
    static const RtUString us_diffuseDoubleSided("diffuseDoubleSided");
    static const RtUString us_displayColor("displayColor");
    static const RtUString us_hydra("hydra");
    static const RtUString us_lightA("lightA");
    static const RtUString us_main_cam("main_cam");
    static const RtUString us_main_cam_projection("main_cam_projection");
    static const RtUString us_pv_color("pv_color");
    static const RtUString us_pv_color_resultRGB("pv_color:resultRGB");
    static const RtUString us_PxrDomeLight("PxrDomeLight");
    static const RtUString us_PxrPerspective("PxrPerspective");
    static const RtUString us_PxrPrimvar("PxrPrimvar");
    static const RtUString us_PxrSurface("PxrSurface");
    static const RtUString us_PxrVolume("PxrVolume");
    static const RtUString us_shadowDistance("shadowDistance");
    static const RtUString us_shadowFalloff("shadowFalloff");
    static const RtUString us_simpleTestSurface("simpleTestSurface");
    static const RtUString us_simpleVolume("simpleVolume");
    static const RtUString us_specularDoubleSided("specularDoubleSided");
    static const RtUString us_specularEdgeColor("specularEdgeColor");
    static const RtUString us_specularFaceColor("specularFaceColor");
    static const RtUString us_specularModelType("specularModelType");
    static const RtUString us_varname("varname");

    riley::ScopedCoordinateSystem const k_NoCoordsys = { 0, nullptr };

    // XXX Shutter settings from studio katana defaults:
    // - /root.renderSettings.shutter{Open,Close}
    float shutterInterval[2] = { 0.0f, 0.5f };
    // - /root.prmanGlobalStatements.camera.shutterOpening.shutteropening
    const float shutterCurve[10] = {0, 0.05, 0, 0, 0, 0, 0.05, 1.0, 0.35, 0.0};

    if (!TfGetEnvSetting(HDX_PRMAN_ENABLE_MOTIONBLUR))
        shutterInterval[1] = 0.0;

    // Options
    {
        // Set thread limit for Renderman. Leave a few threads for app.
        static const unsigned appThreads = 4;
        unsigned nThreads = std::max(WorkGetConcurrencyLimit()-appThreads, 1u);
        // Check the environment
        unsigned nThreadsEnv = TfGetEnvSetting(HDX_PRMAN_NTHREADS);
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

        // Read the maxSamples out of settings (if it exists). Use a default
        // of 1024, so we don't cut the progressive render off early.
        // Setting a lower value here would be useful for unit tests.
        VtValue vtMaxSamples = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedSamplesPerPixel).Cast<int>();
        int maxSamples = TF_VERIFY(!vtMaxSamples.IsEmpty()) ?
            vtMaxSamples.UncheckedGet<int>() : 1024;
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
        int oslVerbose = TfGetEnvSetting(HDX_PRMAN_OSL_VERBOSE);
        if (oslVerbose > 0)
            _options.SetInteger(RtUString("user:osl:verbose"), oslVerbose);

        // Searchpaths (TEXTUREPATH, etc)
        HdPrman_UpdateSearchPathsFromEnvironment(_options);
        
        // Set Options from RenderSettings schema
        SetOptionsFromRenderSettings(
            static_cast<HdPrmanRenderDelegate*>(renderDelegate), _options);
        
        riley->SetOptions(_options);
    }

    // Integrator
    // This needs to be set before setting
    // the active render target, below.
    integratorId = riley::IntegratorId::k_InvalidId;
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
        riley::ShadingNode  integratorNode {
            riley::ShadingNode::k_Integrator,
            rmanIntegrator,
            rmanIntegrator,
            params
        };    
        integratorId = riley->CreateIntegrator(integratorNode);
    }

    // Camera
    {
        RtUString camName("main_cam");

        // Camera params
        RtParamList camParams;
        // Shutter curve (normalized over shutter interval)
        // XXX Riley decomposes the original float[10] style shutter curve
        // as 3 separtae parameters
        camParams.SetFloat(RixStr.k_shutterOpenTime, shutterCurve[0]);
        camParams.SetFloat(RixStr.k_shutterCloseTime, shutterCurve[1]);
        camParams.SetFloatArray(RixStr.k_shutteropening, shutterCurve+2, 8);
        
        // Projection
        riley::ShadingNode cameraNode = riley::ShadingNode {
            riley::ShadingNode::k_Projection,
            us_PxrPerspective,
            us_main_cam_projection,
            RtParamList()
        };
        cameraNode.params.SetFloat(RixStr.k_fov, 60.0f);

        // Transform
        float const zerotime = 0.0f;
        RtMatrix4x4 matrix = RixConstants::k_IdentityMatrix;
        matrix.Translate(0.f, 0.f, -5.0f);
        riley::Transform xform = { 1, &matrix, &zerotime };

        cameraId = riley->CreateCamera(camName, cameraNode, xform, camParams);
    }

    // Displays & Display Channels
    riley::RenderTargetId rtid = riley::RenderTargetId::k_InvalidId;
    {
        float const filterwidth[] = { 1.0f, 1.0f };

        std::vector<riley::RenderOutputId> renderOutputs;

        RtParamList renderOutputParams;
        renderOutputs.push_back(
            riley->CreateRenderOutput(RixStr.k_Ci, 
                                      riley::RenderOutputType::k_Color,
                                      RixStr.k_Ci, 
                                      RixStr.k_filter, 
                                      RixStr.k_box, 
                                      filterwidth, 
                                      1.0f, 
                                      renderOutputParams));
        
        renderOutputs.push_back(
            riley->CreateRenderOutput(RixStr.k_a, 
                                      riley::RenderOutputType::k_Float,
                                      RixStr.k_a, 
                                      RixStr.k_filter, 
                                      RixStr.k_box, 
                                      filterwidth, 
                                      1.0f, 
                                      renderOutputParams));

        renderOutputParams.SetString(RixStr.k_rule, RixStr.k_zmin);
        renderOutputs.push_back(
            riley->CreateRenderOutput(RixStr.k_z, 
                                      riley::RenderOutputType::k_Integer,
                                      RixStr.k_z, 
                                      RixStr.k_zmin, 
                                      RixStr.k_box, 
                                      filterwidth, 
                                      1.0f, 
                                      renderOutputParams));

        renderOutputs.push_back(
            riley->CreateRenderOutput(RixStr.k_id, 
                                      riley::RenderOutputType::k_Integer,
                                      RixStr.k_id, 
                                      RixStr.k_zmin, 
                                      RixStr.k_box, 
                                      filterwidth, 
                                      1.0f, 
                                      renderOutputParams));

        renderOutputs.push_back(
            riley->CreateRenderOutput(RixStr.k_id2, 
                                      riley::RenderOutputType::k_Integer,
                                      RixStr.k_id2, 
                                      RixStr.k_zmin, 
                                      RixStr.k_box, 
                                      filterwidth, 
                                      1.0f, 
                                      renderOutputParams));

        renderOutputs.push_back(
            riley->CreateRenderOutput(RixStr.k_faceindex, 
                                      riley::RenderOutputType::k_Integer,
                                      RixStr.k_faceindex, RixStr.k_zmin, 
                                      RixStr.k_box, 
                                      filterwidth, 
                                      1.0f, 
                                      renderOutputParams));
        
        uint32_t renderTargetFormat[3] = 
            {static_cast<uint32_t>(resolution[0]),
             static_cast<uint32_t>(resolution[1]), 1};
        RtParamList renderTargetParams;
        rtid =riley->CreateRenderTarget(
                    (uint32_t)renderOutputs.size(), 
                    renderOutputs.data(), 
                    renderTargetFormat,
                    RtUString("weighted"), 
                    1.0f, 
                    renderTargetParams);

        RtParamList displayParams;
        riley->CreateDisplay(rtid, 
                             RixStr.k_framebuffer, 
                             us_hydra, 
                             (uint32_t)renderOutputs.size(),
                             renderOutputs.data(),
                             displayParams);

    }

    // Clear values...
    framebuffer.Resize(resolution[0], resolution[1]);
    framebuffer.clearColor[0] = framebuffer.clearColor[1] =
        framebuffer.clearColor[2] = (uint8_t)(0.0707f * 255);
    framebuffer.clearColor[3] = 255;
    framebuffer.clearDepth = 1.0f;
    framebuffer.clearId = -1;

    // Set camera & display
    renderViews.clear();
    renderViews.push_back(riley::RenderView{rtid, 
                                            integratorId, 
                                            cameraId});
    riley->SetActiveCamera(cameraId);

    // Light
    {
        // Light shader
        riley::ShadingNode lightNode {
            riley::ShadingNode::k_Light, // type
            us_PxrDomeLight, // name
            us_lightA, // handle
            RtParamList()
        };
        riley::LightShaderId _fallbackLightShader =
            riley->CreateLightShader(&lightNode, 1, nullptr, 0);

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
        _fallbackLight = riley->CreateLightInstance(
              riley::GeometryMasterId::k_InvalidId, // no group
              riley::GeometryMasterId::k_InvalidId, // no geo
              riley::MaterialId::k_InvalidId, // no material
              _fallbackLightShader,
              k_NoCoordsys,
              xform,
              _fallbackLightAttrs);
    }

    // Materials
    fallbackMaterial = riley::MaterialId::k_InvalidId;
    {
        std::vector<riley::ShadingNode> materialNodes;

        riley::ShadingNode pxrPrimvar_node;
        pxrPrimvar_node.type = riley::ShadingNode::k_Pattern;
        pxrPrimvar_node.name = us_PxrPrimvar;
        pxrPrimvar_node.handle = us_pv_color;
        pxrPrimvar_node.params.SetString(us_varname, us_displayColor);
        // Note: this 0.5 gray is to match UsdImaging's fallback.
        pxrPrimvar_node.params.SetColor(us_defaultColor, 
                                        RtColorRGB(0.5, 0.5, 0.5));
        pxrPrimvar_node.params.SetString(RixStr.k_type, RixStr.k_color);
        materialNodes.push_back(pxrPrimvar_node);

        riley::ShadingNode pxrSurface_node;
        pxrSurface_node.type = riley::ShadingNode::k_Bxdf;
        pxrSurface_node.name = us_PxrSurface;
        pxrSurface_node.handle = us_simpleTestSurface;
        pxrSurface_node.params.ReferenceColor(us_diffuseColor,
                                          us_pv_color_resultRGB);
        pxrSurface_node.params.SetInteger(us_specularModelType, 1);
        pxrSurface_node.params.SetInteger(us_diffuseDoubleSided, 1);
        pxrSurface_node.params.SetInteger(us_specularDoubleSided, 1);
        pxrSurface_node.params.SetColor(us_specularFaceColor, 
                                        RtColorRGB(0.04f));
        pxrSurface_node.params.SetColor(us_specularEdgeColor, 
                                        RtColorRGB(1.0f));
        materialNodes.push_back(pxrSurface_node);

        fallbackMaterial =
            riley->CreateMaterial(&materialNodes[0], materialNodes.size());
    }
    fallbackVolumeMaterial = riley::MaterialId::k_InvalidId;
    {
        std::vector<riley::ShadingNode> materialNodes;
        riley::ShadingNode pxrVolume_node;
        pxrVolume_node.type = riley::ShadingNode::k_Bxdf;
        pxrVolume_node.name = us_PxrVolume;
        pxrVolume_node.handle = us_simpleVolume;
        pxrVolume_node.params.SetString(us_densityFloatPrimVar, us_density);
        materialNodes.push_back(pxrVolume_node);
        fallbackVolumeMaterial =
            riley->CreateMaterial(materialNodes.data(), materialNodes.size());
    }
}


void HdxPrman_InteractiveContext::StartRender()
{
    // Last chance to set Ri options before starting riley!
    // Called from HdxPrman_RenderPass::_Execute


    // Prepare Riley state for rendering.
    // Pass a valid riley callback pointer during IPR
    if (!_didBeginRiley) {
        riley->Begin();
        renderThread.StartThread();
        _didBeginRiley = true;
    }

    renderThread.StartRender();
}

void HdxPrman_InteractiveContext::End()
{
    if (renderThread.IsThreadRunning()) {
        renderThread.StopThread();
    }

    // Reset to initial state.
    if (riley) {
        riley->End();
    }
    if (mgr) {
        if(riley) {
            mgr->DestroyRiley(riley);
        }
        mgr = nullptr;
    }
    riley = nullptr;
    if (rix) {
        RixXcpt* rix_xcpt = (RixXcpt*)rix->GetRixInterface(k_RixXcpt);
        rix_xcpt->Unregister(&xcpt);
    }
    if (ri) {
        ri->PRManEnd();
        ri = nullptr;
    }
}

void
HdxPrman_InteractiveContext::SetFallbackLightsEnabled(bool enabled)
{
    if (_fallbackLightEnabled == enabled) {
        return;
    }
    _fallbackLightEnabled = enabled;

    StopRender();
    sceneVersion++;

    _fallbackLightAttrs.SetInteger(RixStr.k_lighting_mute, !enabled);

    riley->ModifyLightInstance(
          riley::GeometryMasterId::k_InvalidId, // no group
          _fallbackLight,
          nullptr, // no material change
          nullptr, // no shader change
          nullptr, // no coordsys change
          nullptr, // no xform change
          &_fallbackLightAttrs);
}

void
HdxPrman_InteractiveContext::StopRender()
{
    if (renderThread.IsRendering()) {
        riley->Stop();
        renderThread.StopRender();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
