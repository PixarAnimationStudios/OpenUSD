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

#include "hdxPrman/context.h"
#include "hdxPrman/rendererPlugin.h"
#include "hdPrman/rixStrings.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/usd/sdr/registry.h"

#include "Riley.h"
#include "RixParamList.h"
#include "RixRiCtl.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

void HdxPrman_RenderThreadCallback(HdxPrman_InteractiveContext *context)
{
    context->riley->Render();
}

HdxPrman_InteractiveContext::HdxPrman_InteractiveContext() :
    sceneLightCount(0),
    _fallbackLightEnabled(false)
{
    TfRegistryManager::GetInstance().SubscribeTo<HdPrman_Context>();
    renderThread.SetRenderCallback(std::bind(
        HdxPrman_RenderThreadCallback, this));
}

HdxPrman_InteractiveContext::~HdxPrman_InteractiveContext()
{
    if (!TF_VERIFY(!renderThread.IsThreadRunning())) {
        End();
    }
}

void HdxPrman_InteractiveContext::Begin(HdRenderDelegate *renderDelegate)
{
    std::string rmantree = TfGetenv("RMANTREE");
    if (rmantree.empty()) {
        // XXX Setting RMANTREE here is already too late.  libloadprman.a
        // has library ctor entries that read the environment when loaded.
        // Currently, we must use libloadprman.a instead of
        // libprman.so because the latter does not use RTLD_GLOBAL
        TF_RUNTIME_ERROR("The HdPrman backend requires $RMANTREE to be "
                         "set before startup.");
        return;
    }
    // Using RixGetContextViaRMANTREE provided by libloadprman.a
    // allows it to internally handle loading libprman.so with RTLD_GLOBAL
    rix = RixGetContextViaRMANTREE( nullptr, /* printerror */ true);
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
    {
        const char* argv[] = { "hdxPrman" };
        ri->PRManBegin(1, const_cast<char**>(argv));
    }

    // Populate RixStr struct
    RixSymbolResolver* sym = (RixSymbolResolver*)rix->GetRixInterface(
        k_RixSymbolResolver);
    sym->ResolvePredefinedStrings(RixStr);

    // Sanity check symbol resolution with a canary symbol, shutterTime.
    // This can catch accidental linking with incompatible versions.
    TF_VERIFY(RixStr.k_shutterOpenTime == RtUString("shutterOpenTime"),
              "Renderman API tokens do not match expected values.  "
              "There may be a compile/link version mismatch.");

    // Register RenderMan display driver
    HdxPrmanFramebuffer::Register(rix);

    // Acquire Riley instance.
    mgr = (RixRileyManager*)rix->GetRixInterface(k_RixRileyManager);
    riley = mgr->CreateRiley(nullptr);

    //////////////////////////////////////////////////////////////////////// 
    //
    // Riley setup
    //
    static const RtUString us_circle("circle");
    static const RtUString us_PxrPathTracer("PxrPathTracer");
    static const RtUString us_PathTracer("PathTracer");
    static const RtUString us_main_cam("main_cam");
    static const RtUString us_PxrPerspective("PxrPerspective");
    static const RtUString us_main_cam_projection("main_cam_projection");
    static const RtUString us_hydra("hydra");
    static const RtUString us_bufferID("bufferID");
    static const RtUString us_shadowFalloff("shadowFalloff");
    static const RtUString us_shadowDistance("shadowDistance");
    static const RtUString us_PxrDomeLight("PxrDomeLight");
    static const RtUString us_lightA("lightA");
    static const RtUString us_PxrPrimvar("PxrPrimvar");
    static const RtUString us_pv_color("pv_color");
    static const RtUString us_varname("varname");
    static const RtUString us_displayColor("displayColor");
    static const RtUString us_defaultColor("defaultColor");
    static const RtUString us_PxrSurface("PxrSurface");
    static const RtUString us_simpleTestSurface("simpleTestSurface");
    static const RtUString us_diffuseColor("diffuseColor");
    static const RtUString us_pv_color_resultRGB("pv_color:resultRGB");
    static const RtUString us_specularModelType("specularModelType");
    static const RtUString us_diffuseDoubleSided("diffuseDoubleSided");
    static const RtUString us_specularDoubleSided("specularDoubleSided");
    static const RtUString us_specularFaceColor("specularFaceColor");
    static const RtUString us_specularEdgeColor("specularEdgeColor");
    static const RtUString us_PxrVolume("PxrVolume");
    static const RtUString us_simpleVolume("simpleVolume");
    static const RtUString us_densityFloatPrimVar("densityFloatPrimVar");
    static const RtUString us_density("density");

    riley::ScopedCoordinateSystem const k_NoCoordsys = { 0, nullptr };

    // Configure default time samples.
    defaultTimeSamples.push_back(0.0);
    defaultTimeSamples.push_back(1.0);
    // XXX In the future, we'll want a way for clients to configure this map.
    timeSampleMap[SdfPath::AbsoluteRootPath()] = defaultTimeSamples;

    // XXX Shutter settings from studio katana defaults:
    // - /root.renderSettings.shutter{Open,Close}
    const float shutterInterval[2] = { 0.0f, 0.5f };
    // - /root.prmanGlobalStatements.camera.shutterOpening.shutteropening
    const float shutterCurve[10] = {0, 0.05, 0, 0, 0, 0, 0.05, 1.0, 0.35, 0.0};

    // Options
    {
        RixParamList *options = mgr->CreateRixParamList();

        // Set thread limit for Renderman. Leave a few threads for app.
        static const unsigned appThreads = 4;
        unsigned nThreads = std::max(WorkGetConcurrencyLimit()-appThreads, 1u);
        options->SetInteger(RixStr.k_limits_threads, nThreads);

        // XXX: Currently, Renderman doesn't support resizing the viewport
        // without re-initializing the scene.  We work around this by allocating
        // a large framebuffer and making lots of use of the crop window, to
        // generate a sub-region of the correct size.
        int format[2] = { 3000, 2000 };
        options->SetIntegerArray(RixStr.k_Ri_FormatResolution, format, 2);
        float cropWindow[4] = {0, 1, 0, 1};
        options->SetFloatArray(RixStr.k_Ri_CropWindow, cropWindow, 4);

        // Read the maxSamples out of settings (if it exists). Use a default
        // of 1024, so we don't cut the progressive render off early.
        // Setting a lower value here would be useful for unit tests.
        const int defaultMaxSamples = 1024;
        int maxSamples = renderDelegate->GetRenderSetting<int>(
            HdRenderSettingsTokens->convergedSamplesPerPixel,
            defaultMaxSamples);
        options->SetInteger(RixStr.k_hider_maxsamples, maxSamples);

        // Searchpaths (TEXTUREPATH, etc)
        HdPrman_UpdateSearchPathsFromEnvironment(options);

        // Path tracer config.
        options->SetInteger(RixStr.k_hider_incremental, 1);
        options->SetInteger(RixStr.k_hider_jitter, 1);
        options->SetInteger(RixStr.k_hider_minsamples, 1);
        options->SetInteger(RixStr.k_trace_maxdepth, 10);
        options->SetFloat(RixStr.k_Ri_FormatPixelAspectRatio, 1.0f);
        options->SetFloat(RixStr.k_Ri_PixelVariance, 0.001f);
        options->SetString(RixStr.k_bucket_order, us_circle);

        // Camera lens
        options->SetFloatArray(RixStr.k_Ri_Shutter, shutterInterval, 2);

        riley->SetOptions(*options);
        mgr->DestroyRixParamList(options);
    }

    // Integrator
    // XXX Experimentally, this seems to need to be set before setting
    // the active render target, below.
    {
        RixParamList *params = mgr->CreateRixParamList();
        riley::ShadingNode integratorNode {
            riley::ShadingNode::k_Integrator,
            us_PxrPathTracer,
            us_PathTracer,
            params
        };
        riley->CreateIntegrator(integratorNode);
        mgr->DestroyRixParamList(params);
    }

    // Camera
    {
        RtUString camName("main_cam");

        // Camera params
        RixParamList *camParams = mgr->CreateRixParamList();
        // Shutter curve (normalized over shutter interval)
        // XXX Riley decomposes the original float[10] style shutter curve
        // as 3 separtae parameters
        camParams->SetFloat(RixStr.k_shutterOpenTime, shutterCurve[0]);
        camParams->SetFloat(RixStr.k_shutterCloseTime, shutterCurve[1]);
        camParams->SetFloatArray(RixStr.k_shutteropening, shutterCurve+2, 8);

        // Projection
        RixParamList *projParams = mgr->CreateRixParamList();
        projParams->SetFloat(RixStr.k_fov, 60.0f);
        cameraNode = riley::ShadingNode {
            riley::ShadingNode::k_Projection,
            us_PxrPerspective,
            us_main_cam_projection,
            projParams
        };

        // Transform
        float const zerotime = 0.0f;
        RtMatrix4x4 matrix = RixConstants::k_IdentityMatrix;
        matrix.Translate(0.f, 0.f, -5.0f);
        riley::Transform xform = { 1, &matrix, &zerotime };

        cameraId = riley->CreateCamera(camName, cameraNode, xform, *camParams);
        mgr->DestroyRixParamList(camParams);
        mgr->DestroyRixParamList(projParams);
    }

    // Displays & Display Channels
    riley::DisplayChannelId dcid[6];
    riley::RenderTargetId rtid = riley::RenderTargetId::k_InvalidId;
    {
        RixParamList *channelParams = mgr->CreateRixParamList();
        channelParams->SetString(RixStr.k_name, RixStr.k_Ci);
        channelParams->SetInteger(RixStr.k_type, int32_t(RixDataType::k_color));
        dcid[0] = riley->CreateDisplayChannel(*channelParams);
        channelParams->SetString(RixStr.k_name, RixStr.k_a);
        channelParams->SetInteger(RixStr.k_type, int32_t(RixDataType::k_float));
        dcid[1] = riley->CreateDisplayChannel(*channelParams);
        channelParams->SetString(RixStr.k_name, RixStr.k_z);
        channelParams->SetString(RixStr.k_rule, RixStr.k_zmin);
        channelParams->SetString(RixStr.k_filter, RixStr.k_box);
        float const filterwidth[] = { 1, 1 };
        channelParams->SetFloatArray(RixStr.k_filterwidth, filterwidth, 2);
        dcid[2] = riley->CreateDisplayChannel(*channelParams);
        channelParams->SetString(RixStr.k_name, RixStr.k_id);
        channelParams->SetInteger(RixStr.k_type, int32_t(RixDataType::k_integer));
        dcid[3] = riley->CreateDisplayChannel(*channelParams);
        channelParams->SetString(RixStr.k_name, RixStr.k_id2);
        dcid[4] = riley->CreateDisplayChannel(*channelParams);
        channelParams->SetString(RixStr.k_name, RixStr.k_faceindex);
        dcid[5] = riley->CreateDisplayChannel(*channelParams);
        mgr->DestroyRixParamList(channelParams);

        RixParamList *displayParams = mgr->CreateRixParamList();
        displayParams->SetString(RixStr.k_Ri_name, RixStr.k_framebuffer); 
        // Request the d_hydra.so display driver plugin here;
        // note that prman adds an implicit "d_" prefix.
        displayParams->SetString(RixStr.k_Ri_type, us_hydra);
        displayParams->SetString(RixStr.k_mode, RixStr.k_rgbaz);
        displayParams->SetInteger(us_bufferID, framebuffer.id);
        rtid = riley->CreateRenderTarget(cameraId, 6, dcid, *displayParams);
        mgr->DestroyRixParamList(displayParams);
    }

    // Clear values...
    framebuffer.clearColor[0] = framebuffer.clearColor[1] =
        framebuffer.clearColor[2] = (uint8_t)(0.0707f * 255);
    framebuffer.clearColor[3] = 255;
    framebuffer.clearDepth = 1.0f;
    framebuffer.clearId = -1;

    // Set camera & display
    riley->SetRenderTargetIds(1, &rtid);
    riley->SetActiveCamera(cameraId);

    // Light
    {
        // Light shader
        RixParamList *params = mgr->CreateRixParamList();
        params->SetFloat(RixStr.k_intensity, 1.0f);
        params->SetFloat(us_shadowFalloff, 100.0);
        params->SetFloat(us_shadowDistance, 5000.0);

        riley::ShadingNode lightNode {
            riley::ShadingNode::k_Light, // type
            us_PxrDomeLight, // name
            us_lightA, // handle
            params
        };
        riley::LightShaderId _fallbackLightShader = riley->CreateLightShader(
            &lightNode, 1, nullptr, 0);

        // Constant identity transform
        float const zerotime = 0.0f;
        RtMatrix4x4 matrix = RixConstants::k_IdentityMatrix;
        riley::Transform xform = { 1, &matrix, &zerotime };

        // Light instance
        _fallbackLightEnabled = true;
        _fallbackLightAttrs = mgr->CreateRixParamList();
        _fallbackLightAttrs->SetInteger(RixStr.k_visibility_camera, 0);
        _fallbackLightAttrs->SetInteger(RixStr.k_visibility_indirect, 1);
        _fallbackLightAttrs->SetInteger(RixStr.k_visibility_transmission, 1);
        _fallbackLight = riley->CreateLightInstance(
              riley::GeometryMasterId::k_InvalidId, // no group
              riley::GeometryMasterId::k_InvalidId, // no geo
              riley::MaterialId::k_InvalidId, // no material
              _fallbackLightShader,
              k_NoCoordsys,
              xform,
              *_fallbackLightAttrs);
    }

    // Materials
    fallbackMaterial = riley::MaterialId::k_InvalidId;
    {
        std::vector<riley::ShadingNode> materialNodes;

        RixParamList *pxrPrimvar_params = mgr->CreateRixParamList();
        riley::ShadingNode pxrPrimvar_node;
        pxrPrimvar_node.type = riley::ShadingNode::k_Pattern;
        pxrPrimvar_node.name = us_PxrPrimvar;
        pxrPrimvar_node.handle = us_pv_color;
        pxrPrimvar_node.params = pxrPrimvar_params;
        pxrPrimvar_params->SetString(us_varname, us_displayColor);
        // Note: this 0.5 gray is to match UsdImaging's fallback.
        pxrPrimvar_params->SetColor(us_defaultColor, RtColorRGB(0.5, 0.5, 0.5));
        pxrPrimvar_params->SetString(RixStr.k_type, RixStr.k_color);
        materialNodes.push_back(pxrPrimvar_node);

        RixParamList *pxrSurface_params = mgr->CreateRixParamList();
        riley::ShadingNode pxrSurface_node;
        pxrSurface_node.type = riley::ShadingNode::k_Bxdf;
        pxrSurface_node.name = us_PxrSurface;
        pxrSurface_node.handle = us_simpleTestSurface;
        pxrSurface_node.params = pxrSurface_params;
        pxrSurface_params->ReferenceColor(us_diffuseColor,
                                          us_pv_color_resultRGB);
        pxrSurface_params->SetInteger(us_specularModelType, 1);
        pxrSurface_params->SetInteger(us_diffuseDoubleSided, 1);
        pxrSurface_params->SetInteger(us_specularDoubleSided, 1);
        pxrSurface_params->SetColor(us_specularFaceColor, RtColorRGB(0.04f));
        pxrSurface_params->SetColor(us_specularEdgeColor, RtColorRGB(1.0f));
        materialNodes.push_back(pxrSurface_node);

        fallbackMaterial =
            riley->CreateMaterial(&materialNodes[0], materialNodes.size());
    }
    fallbackVolumeMaterial = riley::MaterialId::k_InvalidId;
    {
        std::vector<riley::ShadingNode> materialNodes;
        RixParamList *pxrVolume_params = mgr->CreateRixParamList();
        riley::ShadingNode pxrVolume_node;
        pxrVolume_node.type = riley::ShadingNode::k_Bxdf;
        pxrVolume_node.name = us_PxrVolume;
        pxrVolume_node.handle = us_simpleVolume;
        pxrVolume_node.params = pxrVolume_params;
        pxrVolume_params->SetString(us_densityFloatPrimVar, us_density);
        materialNodes.push_back(pxrVolume_node);
        fallbackVolumeMaterial =
            riley->CreateMaterial(&materialNodes[0], materialNodes.size());
        mgr->DestroyRixParamList(pxrVolume_params);
    }

    // Prepare Riley state for rendering.
    riley->Begin(nullptr);

    renderThread.StartThread();
}

void HdxPrman_InteractiveContext::End()
{
    renderThread.StopThread();

    // Reset to initial state.
    if(riley) {
        riley->End();
        riley = nullptr;
    }
    if (mgr) {
        mgr->DestroyRixParamList(_fallbackLightAttrs);
        mgr->DestroyRiley(riley);
        mgr = nullptr;
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

    _fallbackLightAttrs->SetInteger(RixStr.k_lighting_mute, !enabled);

    riley->ModifyLightInstance(
          riley::GeometryMasterId::k_InvalidId, // no group
          _fallbackLight,
          nullptr, // no material change
          nullptr, // no shader change
          nullptr, // no coordsys change
          nullptr, // no xform change
          _fallbackLightAttrs);
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
