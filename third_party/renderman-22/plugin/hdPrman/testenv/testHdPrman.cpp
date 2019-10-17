//
// Copyright 2018 Pixar
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
//
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/usdLux/listAPI.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/setenv.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/registryManager.h"

#include "hdPrman/context.h"
#include "hdPrman/renderDelegate.h"

#include "Riley.h"
#include "RixParamList.h"
#include "RixRiCtl.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

#include "hdPrman/rixStrings.h"

#include <memory>
#include <stdio.h>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Collection Names
    (testCollection)
);

// Simple placeholder Hydra task to Sync the scene data only.
class Hd_DrawTask final : public HdTask
{
public:
    Hd_DrawTask(HdRenderPassSharedPtr const &renderPass,
                HdRenderPassStateSharedPtr const &renderPassState,
                TfTokenVector const &renderTags)
    : HdTask(SdfPath::EmptyPath())
    , _renderPass(renderPass)
    , _renderPassState(renderPassState)
    , _renderTags(renderTags)
    {
    }

    virtual void Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits) override
    {
        _renderPass->Sync();

        *dirtyBits = HdChangeTracker::Clean;
    }

    virtual void Prepare(HdTaskContext* ctx,
                         HdRenderIndex* renderIndex) override
    {
        _renderPassState->Prepare(renderIndex->GetResourceRegistry());

    }


    virtual void Execute(HdTaskContext* ctx) override
    {
        // XXX Currently HdPrman is tied into the Hdx style of
        // presentation, which uses GL.  Skip all that.
        //        _renderPassState->Bind();
        //        _renderPass->Execute(_renderPassState);
        //        _renderPassState->Unbind();
    }

    virtual const TfTokenVector &GetRenderTags() const override
    {
        return _renderTags;
    }


private:
    HdRenderPassSharedPtr _renderPass;
    HdRenderPassStateSharedPtr _renderPassState;
    TfTokenVector _renderTags;
};


void
PrintUsage(const char* cmd, const char *err=nullptr)
{
    if (err) {
        fprintf(stderr, err);
    }
    fprintf(stderr, "Usage: %s INPUT.usd OUTPUT.{exr,png} "
            "[FRAME] [CAM PROJECTION]\n"
            "FRAME defaults to 0, if not specified.\n"
            "CAM PROJECTION default to PxrPerspective if not specified\n", cmd);
}

int main(int argc, char *argv[])
{
    //////////////////////////////////////////////////////////////////////// 
    //
    // Set up environment
    //

    // Pixar studio config
    TfRegistryManager::GetInstance().SubscribeTo<HdPrman_Context>();

    // Name of orthographic camera projection
    const std::string PxrOrthographic("PxrOrthographic");

    //////////////////////////////////////////////////////////////////////// 
    //
    // Parse args
    //
    if (argc != 3 && argc != 4 && argc != 5) {
        PrintUsage(argv[0]);
        return -1;
    }

    bool isOrthographic = false;
    std::string inputFilename(argv[1]);
    std::string outputFilename(argv[2]);
    std::string cameraProjection("PxrPerspective");

    int frameNum = 0;
    if (argc == 4) {
        frameNum = atoi(argv[3]);
    } else if (argc == 5) {
        cameraProjection = argv[4];
        isOrthographic = cameraProjection == PxrOrthographic;
    }

    //////////////////////////////////////////////////////////////////////// 
    //
    // USD setup
    //
    // Set up USD path resolver, to resolve references
    ArGetResolver().ConfigureResolverForAsset(inputFilename);
    // Load USD file
    UsdStageRefPtr stage = UsdStage::Open(inputFilename);
    if (!stage) {
        PrintUsage(argv[0], "could not load input file");
        return -1;
    }

    //////////////////////////////////////////////////////////////////////// 
    //
    // PRMan setup
    //
    RixContext *rix = RixGetContextViaRMANTREE(nullptr, true);
    RixRiCtl *ri = (RixRiCtl*)rix->GetRixInterface(k_RixRiCtl);
    {
        int argc = 1;
        const char* argv[] = { "-t" }; // -t: use threads
        ri->PRManBegin(argc, const_cast<char**>(argv));
    }

    // Populate RixStr struct
    RixSymbolResolver* sym = (RixSymbolResolver*)rix->GetRixInterface(
        k_RixSymbolResolver);
    sym->ResolvePredefinedStrings(RixStr);

    RixRileyManager *mgr =
        (RixRileyManager*)rix->GetRixInterface(k_RixRileyManager);
    riley::Riley *riley = mgr->CreateRiley(nullptr);

    //////////////////////////////////////////////////////////////////////// 
    //
    // Riley setup
    //
    static const RtUString us_PxrPathTracer("PxrPathTracer");
    static const RtUString us_PathTracer("PathTracer");
    static const RtUString us_main_cam("main_cam");
    static const RtUString us_main_cam_projection("main_cam_projection");
    static const RtUString us_traceLightPaths("traceLightPaths");
    static const RtUString us_lightGroup("lightGroup");
    static const RtUString us_A("A");
    static const RtUString us_PxrDomeLight("PxrDomeLight");
    static const RtUString us_lightA("lightA");
    static const RtUString us_default("default");
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
    static const RtUString us_specularFaceColor("specularFaceColor");
    static const RtUString us_specularEdgeColor("specularEdgeColor");
    static const RtUString us_PxrVolume("PxrVolume");
    static const RtUString us_simpleVolume("simpleVolume");
    static const RtUString us_densityFloatPrimVar("densityFloatPrimVar");
    static const RtUString us_density("density");

    riley::CameraId cameraId;
    riley::ShadingNode cameraNode;
    RtUString cameraName;
    riley::MaterialId fallbackMaterial;
    riley::MaterialId fallbackVolumeMaterial;
    riley::ScopedCoordinateSystem const k_NoCoordsys = { 0, nullptr };
    // Shutter settings from studio katana defaults
    float shutterInterval[2] = { 0.0f, 0.5f };
    float shutterCurve[10] = {0, 0.05, 0, 0, 0, 0, 0.05, 1, 0.35, 0};
    // Options
    {
        RixParamList *options = mgr->CreateRixParamList();
        int format[2] = {512,512};

        // Searchpaths (TEXTUREPATH, etc)
        HdPrman_UpdateSearchPathsFromEnvironment(options);

        // Path tracer config.
        options->SetInteger(RixStr.k_hider_incremental, 1);
        options->SetInteger(RixStr.k_hider_jitter, 1);
        options->SetInteger(RixStr.k_hider_minsamples, 1);
        options->SetInteger(RixStr.k_hider_maxsamples, 64);
        options->SetInteger(RixStr.k_trace_maxdepth, 10);
        options->SetIntegerArray(RixStr.k_Ri_FormatResolution, format, 2);
        options->SetFloat(RixStr.k_Ri_FormatPixelAspectRatio, 1.0f);
        options->SetFloat(RixStr.k_Ri_PixelVariance, 0.01f);

        // Camera lens
        options->SetFloatArray(RixStr.k_Ri_Shutter, shutterInterval, 2);

        riley->SetOptions(*options);
        mgr->DestroyRixParamList(options);
    }
    // Integrator
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
        cameraName = us_main_cam;

        // Camera
        RixParamList *camParams = mgr->CreateRixParamList();
        camParams->SetFloat(RixStr.k_shutterOpenTime, shutterCurve[0]);
        camParams->SetFloat(RixStr.k_shutterCloseTime, shutterCurve[1]);
        camParams->SetFloatArray(RixStr.k_shutteropening, shutterCurve+2, 8);

        // Projection
        RixParamList *projParams = mgr->CreateRixParamList();
        projParams->SetFloat(RixStr.k_fov, 60.0f);
        cameraNode = riley::ShadingNode {
            riley::ShadingNode::k_Projection,
            RtUString(cameraProjection.c_str()),
            us_main_cam_projection,
            projParams
        };

        // Transform
        float const zerotime = 0.0f;
        RtMatrix4x4 matrix = RixConstants::k_IdentityMatrix;

        // Orthographic camera:
        // XXX In HdPrman RenderPass we apply orthographic projection as a
        // scale onto the viewMatrix. This is because we currently cannot
        // update Renderman's `ScreenWindow` once it is running.
        if (isOrthographic) {
            matrix.Scale(10,10,10);
        }

        // Translate camera back a bit
        //
        // XXX Once Hydra supports camera sprims (USD-5241) we
        // should defer to that, and only use this as a fallback.
        matrix.Translate(0.f, 0.f, -10.0f);
        riley::Transform xform = { 1, &matrix, &zerotime };

        cameraId = riley->CreateCamera(cameraName, cameraNode, xform,
                                       *camParams);
        mgr->DestroyRixParamList(camParams);
        mgr->DestroyRixParamList(projParams);
    }
    // Displays & Display Channels
    riley::DisplayChannelId dcid[2];
    riley::RenderTargetId rtid = riley::RenderTargetId::k_InvalidId;
    {
        RixParamList *channelParams = mgr->CreateRixParamList();
        channelParams->SetString(RixStr.k_name, RixStr.k_Ci);
        channelParams->SetInteger(RixStr.k_type,
                                  int32_t(RixDataType::k_color));
        dcid[0] = riley->CreateDisplayChannel(*channelParams);
        channelParams->SetString(RixStr.k_name, RixStr.k_a);
        channelParams->SetInteger(RixStr.k_type,
                                  int32_t(RixDataType::k_float));
        dcid[1] = riley->CreateDisplayChannel(*channelParams);
        mgr->DestroyRixParamList(channelParams);

        RixParamList *displayParams = mgr->CreateRixParamList();
        displayParams->SetString(RixStr.k_Ri_name,
                                 RtUString(outputFilename.c_str()));
        // Use output filename extension directly as display type;
        // this should work for "exr" and "png", at least.
        displayParams->SetString(RixStr.k_Ri_type,
            RtUString(TfGetExtension(outputFilename).c_str()));
        displayParams->SetString(RixStr.k_mode, RixStr.k_rgba);
        rtid = riley->CreateRenderTarget(cameraId, 2, dcid, *displayParams);
        mgr->DestroyRixParamList(displayParams);
    }
    // Set camera & displayComputeLightList
    riley->SetRenderTargetIds(1, &rtid);
    riley->SetActiveCamera(cameraId);
    // Add Fallback lights if no lights present in USD file.
    if (UsdLuxListAPI(stage->GetPseudoRoot()).ComputeLightList(
        UsdLuxListAPI::ComputeModeIgnoreCache).empty()) {
        // Light shader
        RixParamList *params = mgr->CreateRixParamList();
        params->SetFloat(RixStr.k_intensity, 1.0f);
        params->SetInteger(us_traceLightPaths, 1);
        params->SetString(us_lightGroup, us_A);
        riley::ShadingNode lightNode {
            riley::ShadingNode::k_Light, // type
            us_PxrDomeLight, // name
            us_lightA, // handle
            params
        };
        riley::LightShaderId lightShader = riley->CreateLightShader(
            &lightNode, 1, nullptr, 0);
        TF_VERIFY(lightShader != riley::LightShaderId::k_InvalidId);

        // Light instance
        float const zerotime = 0.0f;
        RtMatrix4x4 matrix = RixConstants::k_IdentityMatrix;
        riley::Transform xform = { 1, &matrix, &zerotime };
        RixParamList *lightAttributes = mgr->CreateRixParamList();
        lightAttributes->SetInteger(RixStr.k_visibility_camera, 0);
        lightAttributes->SetInteger(RixStr.k_visibility_indirect, 1);
        lightAttributes->SetInteger(RixStr.k_visibility_transmission, 1);
        lightAttributes->SetString(RixStr.k_grouping_membership, us_default);
        riley::LightInstanceId light = riley->CreateLightInstance(
              riley::GeometryMasterId::k_InvalidId, // no group
              riley::GeometryMasterId::k_InvalidId, // no geo
              riley::MaterialId::k_InvalidId, // no material
              lightShader,
              k_NoCoordsys,
              xform,
              *lightAttributes);
        TF_VERIFY(light != riley::LightInstanceId::k_InvalidId);
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
        pxrSurface_params->SetColor(us_specularFaceColor, RtColorRGB(0.04f));
        pxrSurface_params->SetColor(us_specularEdgeColor, RtColorRGB(1.0f));
        materialNodes.push_back(pxrSurface_node);

        fallbackMaterial =
            riley->CreateMaterial(&materialNodes[0], materialNodes.size());
        mgr->DestroyRixParamList(pxrPrimvar_params);
        mgr->DestroyRixParamList(pxrSurface_params);
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
    riley->Begin(nullptr);

    //////////////////////////////////////////////////////////////////////// 
    //
    // Hydra setup
    //
    // Assemble a Hydra pipeline to feed USD data to Riley. 
    // Scene data flows left-to-right:
    //
    //     => UsdStage
    //       => UsdImagingDelegate (hydra "frontend")
    //         => HdRenderIndex
    //           => HdPrmanRenderDelegate (hydra "backend")
    //             => Riley
    //
    // Note that Hydra is flexible, but that means it takes a few steps
    // to configure the details. This might seem out of proportion in a
    // simple usage example like this, if you don't consider the range of
    // other scenarios Hydra is meant to handle.
    {

        // Handoff some prman context to the hydra backend.
        std::shared_ptr<HdPrman_Context> hdPrmanContext =
            std::make_shared<HdPrman_Context>();
        hdPrmanContext->rix = rix;
        hdPrmanContext->ri = ri;
        hdPrmanContext->mgr = mgr;
        hdPrmanContext->riley = riley;
        hdPrmanContext->fallbackMaterial = fallbackMaterial;
        hdPrmanContext->fallbackVolumeMaterial = fallbackVolumeMaterial;

        // Configure default time samples.
        hdPrmanContext->defaultTimeSamples.push_back(0.0);
        hdPrmanContext->defaultTimeSamples.push_back(1.0);
        hdPrmanContext->timeSampleMap[SdfPath::AbsoluteRootPath()] =
            hdPrmanContext->defaultTimeSamples;

        // Set up frontend -> index -> backend
        HdPrmanRenderDelegate hdPrmanBackend(hdPrmanContext);
        std::unique_ptr<HdRenderIndex> hdRenderIndex(
            HdRenderIndex::New(&hdPrmanBackend));
        UsdImagingDelegate hdUsdFrontend(hdRenderIndex.get(),
                                         SdfPath::AbsoluteRootPath());
        hdUsdFrontend.Populate(stage->GetPseudoRoot());
        hdUsdFrontend.SetTime(frameNum);
        hdUsdFrontend.SetRefineLevelFallback(8); // max refinement

        TfTokenVector renderTags(1, HdTokens->geometry);
        // The collection of scene contents to render
        HdRprimCollection hdCollection(_tokens->testCollection,
                                   HdReprSelector(HdReprTokens->smoothHull));
        HdChangeTracker &tracker = hdRenderIndex->GetChangeTracker();
        tracker.AddCollection(_tokens->testCollection);

        // We don't need multi-pass rendering with a pathtracer
        // so we use a single, simple render pass.
        HdRenderPassSharedPtr hdRenderPass =
            hdPrmanBackend.CreateRenderPass(hdRenderIndex.get(), hdCollection);
        HdRenderPassStateSharedPtr hdRenderPassState =
            hdPrmanBackend.CreateRenderPassState();

        // The task execution graph and engine configuration is also simple.
        HdTaskSharedPtrVector tasks = {
            boost::make_shared<Hd_DrawTask>(hdRenderPass,
                                            hdRenderPassState,
                                            renderTags)
        };
        HdEngine hdEngine;
        hdEngine.Execute(hdRenderIndex.get(), &tasks);

        //////////////////////////////////////////////////////////////////// 
        //
        // Render to image
        //
        printf("Rendering...\n");
        riley->Render();
        printf("Rendered %s\n", outputFilename.c_str());
    }

    //////////////////////////////////////////////////////////////////////// 
    //
    // Shutdown
    //
    riley->End();
    mgr->DestroyRiley(riley);
    mgr = nullptr;
    riley = nullptr;
    ri->PRManEnd();

}
