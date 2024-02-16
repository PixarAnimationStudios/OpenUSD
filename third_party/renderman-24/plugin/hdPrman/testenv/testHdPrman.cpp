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

#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/flatteningSceneIndex.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/utils.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/hd/pluginRenderDelegateUniqueHandle.h"
#include "pxr/imaging/hd/camera.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdRender/product.h"
#include "pxr/usd/usdRender/settings.h"
#include "pxr/usd/usdRender/spec.h"
#include "pxr/usd/usdRender/var.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/renderSettingsFlatteningSceneIndex.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/usdImaging/usdImaging/flattenedDataSourceProviders.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/arch/env.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/work/threadLimits.h"

#include "hdPrman/renderDelegate.h"

#include <fstream>
#include <memory>
#include <stdio.h>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Collection Names
    (testCollection)
);

TF_DEFINE_ENV_SETTING(TEST_HD_PRMAN_ENABLE_SCENE_INDEX, false,
                      "Use Scene Index API for testHdPrman.");

TF_DEFINE_ENV_SETTING(TEST_HD_PRMAN_USE_RENDER_SETTINGS_PRIM, true,
                      "Use the Render Settings Prim instead of the "
                      "UsdRenderSpec for testHdPrman.");

static TfStopwatch timer_prmanRender;

static const GfVec2i _fallbackResolution(512, 512);
static const TfToken _fallbackConformPolicy(UsdRenderTokens->adjustApertureWidth);

struct HydraSetupCameraInfo {
    SdfPath cameraPath;
    GfVec2i resolution;
    float pixelAspectRatio;
    TfToken aspectRatioConformPolicy;
    GfRange2f dataWindowNDC;
};

static
bool
UseRenderSettingsPrim()
{
    static const bool useRenderSettingsPrim =
        TfGetEnvSetting(TEST_HD_PRMAN_USE_RENDER_SETTINGS_PRIM);
    return useRenderSettingsPrim;
}

// Simple Hydra task to Sync and Render the data provided to this test.
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

    void Sync(HdSceneDelegate* delegate,
              HdTaskContext* ctx,
              HdDirtyBits* dirtyBits) override
    {
        _renderPass->Sync();
        *dirtyBits = HdChangeTracker::Clean;
    }

    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override
    {
        _renderPassState->Prepare(renderIndex->GetResourceRegistry());
    }

    void Execute(HdTaskContext* ctx) override
    {
        timer_prmanRender.Start();
        _renderPass->Execute(_renderPassState, _renderTags);
        timer_prmanRender.Stop();
    }

    const TfTokenVector &GetRenderTags() const override
    {
        return _renderTags;
    }

private:
    HdRenderPassSharedPtr _renderPass;
    HdRenderPassStateSharedPtr _renderPassState;
    TfTokenVector _renderTags;
};

GfVec2i
MultiplyAndRound(const GfVec2f &a, const GfVec2i &b)
{
    return GfVec2i(std::roundf(a[0] * b[0]),
                   std::roundf(a[1] * b[1]));
}

CameraUtilFraming
ComputeFraming(const HydraSetupCameraInfo &cameraInfo)
{
    const GfRange2f displayWindow(GfVec2f(0.0f), GfVec2f(cameraInfo.resolution));

    // We use rounding to nearest integer when computing the dataWindow
    // from the dataWindowNDC. This is to conform about the UsdRenderSpec's
    // specification of the pixels that make up the data window, namely it
    // is exactly those pixels whose centers are contained in the dataWindowNDC
    // in NDC space.
    //
    // Note that we subtract 1 from the maximum - that's because of GfRect2i's
    // unusual API.
    const GfRect2i dataWindow(
        MultiplyAndRound(
            cameraInfo.dataWindowNDC.GetMin(), cameraInfo.resolution),
        MultiplyAndRound(
            cameraInfo.dataWindowNDC.GetMax(), cameraInfo.resolution) - GfVec2i(1));

    return CameraUtilFraming(
        displayWindow, dataWindow, cameraInfo.pixelAspectRatio);
}

CameraUtilConformWindowPolicy
_RenderSettingsTokenToConformWindowPolicy(const TfToken &usdToken)
{
    if (usdToken == UsdRenderTokens->adjustApertureWidth) {
        return CameraUtilMatchVertically;
    }
    if (usdToken == UsdRenderTokens->adjustApertureHeight) {
        return CameraUtilMatchHorizontally;
    }
    if (usdToken == UsdRenderTokens->expandAperture) {
        return CameraUtilFit;
    }
    if (usdToken == UsdRenderTokens->cropAperture) {
        return CameraUtilCrop;
    }
    if (usdToken == UsdRenderTokens->adjustPixelAspectRatio) {
        return CameraUtilDontConform;
    }

    TF_WARN(
        "Invalid aspectRatioConformPolicy value '%s', "
        "falling back to expandAperture.", usdToken.GetText());
    
    return CameraUtilFit;
}

void
PopulateFallbackRenderSpec(
    std::string const& outputFilename, UsdRenderSpec *renderSpec) 
{   
    *renderSpec = {
        /* products */
        {
            UsdRenderSpec::Product {
                SdfPath("/Render/Products/Fallback"),   // product path
                TfToken("raster"),                      // type
                TfToken(outputFilename),                // name
                SdfPath(),                              // camera path
                false,                                  // disableMotionBlur
                false,                                  // disableDepthOfField
                _fallbackResolution,                    // resolution
                1.0f,                                   // PixelAspectRatio
                _fallbackConformPolicy,                 // aspectRatioConformPolicy 
                GfVec2f(2.0, 2.0),                      // aperture size
                GfRange2f(GfVec2f(0.0f), GfVec2f(1.0f)),// data window
                { 0, 1 },                               // renderVarIndices
            },
        },
        /* renderVars */
        {
            UsdRenderSpec::RenderVar {
                SdfPath("/Render/Vars/Ci"),     // renderVarPath
                TfToken("color3f"),             // dataType
                TfToken("Ci")                   // sourceName
            },
            UsdRenderSpec::RenderVar {
                SdfPath("/Render/Vars/Alpha"),  // renderVarPath
                TfToken("float"),               // dataType
                TfToken("a")                    // sourceName
            }
        }
    };
}

UsdGeomCamera
CreateFallbackCamera(
    UsdStageRefPtr const &stage,
    SdfPath const &fallbackCameraPath)
{
    UsdGeomCamera fallbackCamera =
        UsdGeomCamera::Define(stage, fallbackCameraPath);

    const GfMatrix4d m =
        GfMatrix4d().SetDiagonal(GfVec4d(1.0, 1.0, -1.0, 1.0)) *
        GfMatrix4d().SetTranslate(GfVec3d(0,0,-10));
    fallbackCamera.AddTransformOp(UsdGeomXformOp::PrecisionFloat).Set(VtValue(m));

    fallbackCamera.CreateFocalLengthAttr(VtValue(1.0f));
    const float apertureSize = 2.0f * tan(GfDegreesToRadians(60.0f) / 2.0f);
    fallbackCamera.CreateHorizontalApertureAttr(VtValue(apertureSize));
    fallbackCamera.CreateVerticalApertureAttr(VtValue(apertureSize));
    return fallbackCamera;
}

// Add Fallback values needed for the test, if they are not already authored.
void
PopulateFallbackRenderSettings(
    UsdStageRefPtr const &stage,
    std::string const &outputFilename,
    SdfPath const &sceneCamPath,
    UsdRenderSettings *settings)
{
    fprintf(stdout, "Populate RenderSettings Prim with fallback values.\n");
    // If no renderSettings prim was found create a fallback prim.
    if (settings->GetPath().IsEmpty()) {
        SdfPath fallbackRenderSettingsPath("/Render/Settings/Fallback");
        *settings = 
            UsdRenderSettings::Define(stage, fallbackRenderSettingsPath);
    }
    
    // Set the fallback Resolution and Aspect Ratio Conform Policy
    if (!settings->GetResolutionAttr().HasAuthoredValue()) {
        settings->CreateResolutionAttr(VtValue(_fallbackResolution));
    }
    if (!settings->GetAspectRatioConformPolicyAttr().HasAuthoredValue()) {
        settings->CreateAspectRatioConformPolicyAttr(
            VtValue(_fallbackConformPolicy));
    }

    // Set the Camera
    SdfPathVector cameraTargets; 
    settings->GetCameraRel().GetForwardedTargets(&cameraTargets);
    if (cameraTargets.empty()) {
        if (sceneCamPath.IsEmpty()) {
            SdfPath fallbackCameraPath("/Fallback/Camera");
            UsdGeomCamera fallbackCamera = 
                CreateFallbackCamera(stage, fallbackCameraPath);
            settings->GetCameraRel().AddTarget(fallbackCameraPath);
        }
        else {
            settings->GetCameraRel().AddTarget(sceneCamPath);
        }
    }

    // Check if there are any authored Render Products connected
    SdfPathVector renderProductTargets; 
    settings->GetProductsRel().GetForwardedTargets(&renderProductTargets);
    if (!renderProductTargets.empty()) {
        return;
    }

    fprintf(stdout, "Add Fallback Render Product and Vars.\n");
    // Create the fallback Render Product using the outputFilename
    SdfPath fallbackProductPath("/Render/Products/Fallback");
    UsdRenderProduct fallbackProduct =
        UsdRenderProduct::Define(stage, fallbackProductPath);
    fallbackProduct.CreateProductNameAttr(VtValue(TfToken(outputFilename)));
    settings->GetProductsRel().AddTarget(fallbackProductPath);

    // Create the fallback Render Vars
    SdfPath fallbackVarCiPath("/Render/Vars/Ci");
    UsdRenderVar fallbackVarCi =
        UsdRenderVar::Define(stage, fallbackVarCiPath);
    fallbackVarCi.CreateDataTypeAttr(VtValue(TfToken("color3f")));
    fallbackVarCi.CreateSourceNameAttr(VtValue(std::string("Ci")));
    fallbackProduct.GetOrderedVarsRel().AddTarget(fallbackVarCiPath);

    SdfPath fallbackVarAlphaPath("/Render/Vars/Alpha");
    UsdRenderVar fallbackVarAlpha =
        UsdRenderVar::Define(stage, fallbackVarAlphaPath);
    fallbackVarAlpha.CreateDataTypeAttr(VtValue(TfToken("float")));
    fallbackVarAlpha.CreateSourceNameAttr(VtValue(std::string("a")));
    fallbackProduct.GetOrderedVarsRel().AddTarget(fallbackVarAlphaPath);
}

VtDictionary
CreateRenderSpecDict(
    UsdRenderSpec const &renderSpec, UsdRenderSpec::Product const &product)
{
    // RenderSpecDict contains: camera, renderVars, and renderProducts
    VtDictionary renderSpecDict;

    // Camera
    renderSpecDict[HdPrmanExperimentalRenderSpecTokens->camera] =
        product.cameraPath;
    // Render Vars
    {
        std::vector<VtValue> renderVarDicts;

        // Displays & Display Channels
        for (size_t index: product.renderVarIndices) {
            auto const& renderVar = renderSpec.renderVars[index];

            // Map source to Ri name.
            std::string name = renderVar.sourceName;
            if (renderVar.sourceType == UsdRenderTokens->lpe) {
                name = "lpe:" + name;
            }

            VtDictionary renderVarDict;
            renderVarDict[HdPrmanExperimentalRenderSpecTokens->name] =
                name;
            renderVarDict[HdPrmanExperimentalRenderSpecTokens->type] =
                renderVar.dataType.GetString();
            renderVarDict[HdPrmanExperimentalRenderSpecTokens->params] =
                renderVar.namespacedSettings;

            renderVarDicts.push_back(VtValue(renderVarDict));
        }
        
        renderSpecDict[HdPrmanExperimentalRenderSpecTokens->renderVars] =
            renderVarDicts;
    }
    // Render Products
    {
        std::vector<VtValue> renderProducts;
        {
            VtDictionary renderProduct;
            renderProduct[HdPrmanExperimentalRenderSpecTokens->name] =
                product.name.GetString();
            {
                VtIntArray renderVarIndices;
                const size_t num = product.renderVarIndices.size();
                for (size_t i = 0; i < num; i++) {
                    renderVarIndices.push_back(i);
                }
                renderProduct[
                    HdPrmanExperimentalRenderSpecTokens->renderVarIndices] =
                    renderVarIndices;
            }
            renderProducts.push_back(VtValue(renderProduct));
        }
        renderSpecDict[HdPrmanExperimentalRenderSpecTokens->renderProducts]=
            renderProducts;
    }
    return renderSpecDict;
}

// Add the integratorName and any associated values to the settingsMap based 
// on the VisualizerStyle
void
AddVisualizerStyle(
    std::string const &visualizerStyle, HdRenderSettingsMap *settingsMap)
{
    if (!visualizerStyle.empty()) {
        const std::string integratorName("PxrVisualizer");
        
        // TODO Figure out how to represent this in UsdRi.
        // Perhaps a UsdRiIntegrator prim, plus an adapter
        // in UsdImaging that adds it as an sprim?
        (*settingsMap)[HdPrmanRenderSettingsTokens->integratorName] =
            integratorName;
        
        const std::string prefix = 
            "ri:integrator:" + integratorName + ":";
        
        (*settingsMap)[TfToken(prefix + "wireframe")] = 1;
        (*settingsMap)[TfToken(prefix + "style")] = visualizerStyle;
    } else {
        const std::string integratorName("PxrPathTracer");
        (*settingsMap)[HdPrmanRenderSettingsTokens->integratorName] =
            integratorName;
    }
}

// Add the Namespaced Settings to the settingsMap making sure to add the 
// fallback settings specific to testHdPrman
void 
AddNamespacedSettings(
    VtDictionary const &namespacedSettings, HdRenderSettingsMap *settingsMap)
{
    // Add fallback settings specific to testHdPrman 
    // Note: 'ri:trace:maxdepth' cannot be found in the applied schemas 
    (*settingsMap)[TfToken("ri:trace:maxdepth")] = 10; 
    (*settingsMap)[TfToken("ri:hider:jitter")] = 1;
    (*settingsMap)[TfToken("ri:hider:minsamples")] = 32;
    (*settingsMap)[TfToken("ri:hider:maxsamples")] = 64;
    (*settingsMap)[TfToken("ri:Ri:PixelVariance")] = 0.01;

    // Set namespaced settings 
    for (const auto &item : namespacedSettings) {
        (*settingsMap)[TfToken(item.first)] = item.second;
    }
}

// Get the Camera information from the Render Spec and the command line, and
// apply those command line overrides to the product itself. 
HydraSetupCameraInfo
GetCameraInfoAndUpdateProduct(
    SdfPath const &sceneCamPath,
    float const sceneCamAspect,
    UsdRenderSpec::Product *product)
{
    // Apply Command line overrides to the product since it will be used to 
    // create the RenderSpecDict that HdPrman_RenderPass will use.
    if (!sceneCamPath.IsEmpty()) {
        product->cameraPath = sceneCamPath;
    }
    if (sceneCamAspect > 0.0) {
        product->resolution[1] = (int)(product->resolution[0]/sceneCamAspect);
        product->apertureSize[1] = product->apertureSize[0]/sceneCamAspect;
    }

    HydraSetupCameraInfo camInfo;
    camInfo.cameraPath = product->cameraPath;
    camInfo.resolution = product->resolution;
    camInfo.pixelAspectRatio = product->pixelAspectRatio;
    camInfo.aspectRatioConformPolicy = product->aspectRatioConformPolicy;
    camInfo.dataWindowNDC = product->dataWindowNDC;

    return camInfo;
}


// Get the Camera info from the RenderSettings prim and the command line. 
HydraSetupCameraInfo
GetCameraInfo(
    SdfPath const &sceneCamPath,
    float const sceneCamAspect,
    UsdRenderSettings const &settings)
{
    // XXX These attributes are populated from the Render Settings Prim, and  
    // they should eventually come from the Render Product instead.
    HydraSetupCameraInfo camInfo;
    if (sceneCamPath.IsEmpty()) {
        SdfPathVector targets; 
        settings.GetCameraRel().GetForwardedTargets(&targets);
        if (!targets.empty()) {
            camInfo.cameraPath = targets[0];
        }
    }
    settings.GetResolutionAttr().Get(&camInfo.resolution);
    settings.GetPixelAspectRatioAttr().Get(&camInfo.pixelAspectRatio);
    settings.GetAspectRatioConformPolicyAttr().Get(&camInfo.aspectRatioConformPolicy);

    // Convert dataWindowNDC from vec4 to range2.
    GfVec4f dataWindowNDCVec;
    if (settings.GetDataWindowNDCAttr().Get(&dataWindowNDCVec)) {
        camInfo.dataWindowNDC = GfRange2f(
            GfVec2f(dataWindowNDCVec[0], dataWindowNDCVec[1]),
            GfVec2f(dataWindowNDCVec[2], dataWindowNDCVec[3]));
    }

    // Apply Command line overrides.
    if (!sceneCamPath.IsEmpty()) {
        camInfo.cameraPath = sceneCamPath;
    }
    if (sceneCamAspect > 0.0) {
        camInfo.resolution[1] = (int)(camInfo.resolution[0]/sceneCamAspect);
        // camInfo.apertureSize[1] = camInfo.apertureSize[0]/sceneCamAspect;
    }

    return camInfo;
}

void
HydraSetupAndRender(
    HdRenderSettingsMap const &settingsMap, 
    UsdStageRefPtr const &stage, 
    HydraSetupCameraInfo const &cameraInfo,
    const int frameNum, 
    const std::string &cullStyle,
    TfStopwatch *timer_hydra)
{
    // Create the RenderDelegate, passing in the HdRenderSettingsMap
    // Set up frontend -> index -> backend
    // TODO We should configure the render delegate to request
    // the appropriate materialBindingPurposes from the USD scene.
    // We should also configure the scene to filter for the
    // requested includedPurposes.
    // 
    // In order to pick up the plugin scene indices, we need to instantiate
    // the HdPrmanRenderDelegate through the renderer plugin registry.
    HdPluginRenderDelegateUniqueHandle const renderDelegate =
        HdRendererPluginRegistry::GetInstance().CreateRenderDelegate(
            TfToken("HdPrmanLoaderRendererPlugin"),
            settingsMap);

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
    std::unique_ptr<HdRenderIndex> const hdRenderIndex(
        HdRenderIndex::New(renderDelegate.Get(), HdDriverVector()));
    
    std::unique_ptr<UsdImagingDelegate> hdUsdFrontend;

    if (TfGetEnvSetting(TEST_HD_PRMAN_ENABLE_SCENE_INDEX)) {
        UsdImagingStageSceneIndexRefPtr usdStageSceneIndex;
        usdStageSceneIndex = UsdImagingStageSceneIndex::New();
        usdStageSceneIndex->SetStage(stage);
        usdStageSceneIndex->SetTime(frameNum);

        // Chain scene indices
        HdSceneIndexBaseRefPtr siChainHead;
        siChainHead = UsdImagingRenderSettingsFlatteningSceneIndex::New(
                        usdStageSceneIndex);
        siChainHead = HdFlatteningSceneIndex::New(
            siChainHead, UsdImagingFlattenedDataSourceProviders());

        // Insert scene index chain into the render index.
        hdRenderIndex->InsertSceneIndex(
            siChainHead, SdfPath::AbsoluteRootPath());

    } else {
        hdUsdFrontend = std::make_unique<UsdImagingDelegate>(
            hdRenderIndex.get(),
            SdfPath::AbsoluteRootPath());
        hdUsdFrontend->Populate(stage->GetPseudoRoot());
        hdUsdFrontend->SetTime(frameNum);
        hdUsdFrontend->SetRefineLevelFallback(8); // max refinement
        if (!cameraInfo.cameraPath.IsEmpty()) {
            hdUsdFrontend->SetCameraForSampling(cameraInfo.cameraPath);
        }
        if (!cullStyle.empty()) {
            if (cullStyle == "none") {
                hdUsdFrontend->SetCullStyleFallback(HdCullStyleNothing);
            } else if (cullStyle == "back") {
                hdUsdFrontend->SetCullStyleFallback(HdCullStyleBack);
            } else if (cullStyle == "front") {
                hdUsdFrontend->SetCullStyleFallback(HdCullStyleFront);
            } else if (cullStyle == "backUnlessDoubleSided") {
                hdUsdFrontend->SetCullStyleFallback(
                    HdCullStyleBackUnlessDoubleSided);
            } else if (cullStyle == "frontUnlessDoubleSided") {
                hdUsdFrontend->SetCullStyleFallback(
                    HdCullStyleFrontUnlessDoubleSided);
            }
        }
    }

    const TfTokenVector renderTags{HdRenderTagTokens->geometry};
    // The collection of scene contents to render
    HdRprimCollection hdCollection(
        _tokens->testCollection,
        HdReprSelector(HdReprTokens->smoothHull));
    HdChangeTracker &tracker = hdRenderIndex->GetChangeTracker();
    tracker.AddCollection(_tokens->testCollection);

    // We don't need multi-pass rendering with a pathtracer
    // so we use a single, simple render pass.
    HdRenderPassSharedPtr const hdRenderPass =
        renderDelegate->CreateRenderPass(hdRenderIndex.get(),
                                            hdCollection);
    HdRenderPassStateSharedPtr const hdRenderPassState =
        renderDelegate->CreateRenderPassState();

    const HdCamera * const camera = 
        dynamic_cast<const HdCamera*>(
            hdRenderIndex->GetSprim(HdTokens->camera, cameraInfo.cameraPath));

    hdRenderPassState->SetCamera(camera);
    hdRenderPassState->SetFraming(ComputeFraming(cameraInfo));
    hdRenderPassState->SetOverrideWindowPolicy(
#if HD_API_VERSION >= 57
            HdUtils::ToConformWindowPolicy(
                cameraInfo.aspectRatioConformPolicy));
#else
            { true, HdUtils::ToConformWindowPolicy(
                                    cameraInfo.aspectRatioConformPolicy) });
#endif

    // The task execution graph and engine configuration is also simple.
    HdTaskSharedPtrVector tasks = {
        std::make_shared<Hd_DrawTask>(hdRenderPass,
                                      hdRenderPassState,
                                      renderTags)
    };
    HdEngine hdEngine;
    timer_hydra->Start();
    hdEngine.Execute(hdRenderIndex.get(), &tasks);
    timer_hydra->Stop();
}


void
PrintUsage(const char* cmd, const char *err=nullptr)
{
    if (err) {
        fprintf(stderr, "%s\n", err);
    }
    fprintf(stderr, "Usage: %s INPUT.usd "
            "[--out|-o OUTPUT] [--frame|-f FRAME] [--env|-e NAME=VALUE]"
            "[--sceneCamPath|-c CAM_PATH] [--settings|-s RENDERSETTINGS_PATH] "
            "[--sceneCamAspect|-a aspectRatio] [--cullStyle|-k CULL_STYLE] "
            "[--visualize|-z STYLE] [--perf|-p PERF] [--trace|-t TRACE]\n"
            "Single-hyphen options still need a space before the value!\n"
            "OUTPUT defaults to UsdRenderSettings if not specified.\n"
            "FRAME defaults to 0 if not specified.\n"
            "NAME & VALUE are an environment variable and value to set with "
            "ArchSetEnv; use multiple --env tags to set multiple variables\n"
            "CAM_PATH defaults to empty path if not specified\n"
            "RENDERSETTINGS_PATH defaults to empty path is not specified\n"
            "STYLE indicates a PxrVisualizer style to use instead of "
            "the default integrator\n"
            "PERF indicates a json file to record performance measurements\n"
            "TRACE indicates a text file to record trace measurements\n"
            "CULL_STYLE selects the fallback cull style and may be one of: "
            "none|back|front|backUnlessDoubleSided|frontUnlessDoubleSided\n",
            cmd);
}

////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    //////////////////////////////////////////////////////////////////////// 
    //
    // Parse args
    //
    if (argc < 2) {
        PrintUsage(argv[0]);
        return -1;
    }

    std::string inputFilename(argv[1]);
    std::string outputFilename;
    std::string perfOutput, traceOutput;
    std::string cullStyle;

    int frameNum = 0;
    SdfPath sceneCamPath, renderSettingsPath;
    float sceneCamAspect = -1.0;
    std::string visualizerStyle;
    std::vector<std::pair<std::string, std::string>> env;

    for (int i=2; i<argc-1; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--frame" || arg == "-f") {
            frameNum = atoi(argv[++i]);
        } else if (arg == "--sceneCamPath" || arg == "-c") {
            sceneCamPath = SdfPath(argv[++i]);
        } else if (arg == "--sceneCamAspect" || arg == "-a") {
            sceneCamAspect = atof(argv[++i]);
        } else if (arg == "--out" || arg == "-o") {
            outputFilename = argv[++i];
        } else if (arg == "--settings" || arg == "-s") {
            renderSettingsPath = SdfPath(argv[++i]);
        } else if (arg == "--visualize" || arg == "-z") {
            visualizerStyle = argv[++i];
        } else if (arg == "--perf" || arg == "-p") {
            perfOutput = argv[++i];
        } else if (arg == "--trace" || arg == "-t") {
            traceOutput = argv[++i];
        } else if (arg == "--cullStyle" || arg == "-k") {
            cullStyle = argv[++i];
        } else if (arg == "--env" || arg == "-e") {
            std::vector<std::string> parts = TfStringSplit(argv[++i], "=");
            env.push_back({parts[0], parts[1]});
        }
    }

    if (!env.empty()) {
        for (auto p : env) {
            ArchSetEnv(p.first, p.second, true);
        }
    }

    if (!traceOutput.empty()) {
        TraceCollector::GetInstance().SetEnabled(true);
    }

    //////////////////////////////////////////////////////////////////////// 
    //
    // USD setup
    //

    TfStopwatch timer_usdOpen;
    timer_usdOpen.Start();
    // Load USD file
    UsdStageRefPtr stage = UsdStage::Open(inputFilename);
    if (!stage) {
        PrintUsage(argv[0], "could not load input file");
        return -1;
    }
    timer_usdOpen.Stop();

    //////////////////////////////////////////////////////////////////////// 
    // Render settings
    //

    UsdRenderSettings settings;
    if (renderSettingsPath.IsEmpty()) {
        // Get the RenderSettings prim indicated in the stage metadata
        fprintf(stdout, "Looking for Render Settings based on the metadata.\n");
        settings = UsdRenderSettings::GetStageRenderSettings(stage);
    } else {
        // If a path was specified, try to use the requested settings prim.
        fprintf(stdout, "Looking for Render Settings at the path <%s>.\n",
                renderSettingsPath.GetText());
        settings = UsdRenderSettings(stage->GetPrimAtPath(renderSettingsPath));
    }
    if (settings) {
        fprintf(stdout, "Found the Render Settings Prim <%s>.\n", 
                settings.GetPath().GetText());
    }

    // If we want to use the Render Settings, make sure it is fully populated
    if (UseRenderSettingsPrim()) {
        PopulateFallbackRenderSettings(
            stage, outputFilename, sceneCamPath, &settings);
    }

    UsdRenderSpec renderSpec;
    const TfTokenVector prmanNamespaces{TfToken("ri"), TfToken("outputs:ri")};
    if (!UseRenderSettingsPrim()) {
        if (settings) {
            // Create the RenderSpec from the Render Settings Prim 
            fprintf(stdout, "Create a UsdRenderSpec from the Render Settings "
                    "Prim <%s>.\n", settings.GetPath().GetText());
            renderSpec = UsdRenderComputeSpec(settings, prmanNamespaces);
        } else {
            // Otherwise, provide a built-in render specification.
            fprintf(stdout, "Create the Fallback UsdRenderSpec.\n");
            PopulateFallbackRenderSpec(outputFilename, &renderSpec);
        }
    }

    //////////////////////////////////////////////////////////////////////// 
    //
    // Diagnostic aids
    //

    // These are meant to help keep an eye on how much available
    // concurrency is being used, within an automated test environment.
    printf("Current concurrency limit:  %u\n", WorkGetConcurrencyLimit());
    printf("Physical concurrency limit: %u\n",
        WorkGetPhysicalConcurrencyLimit());

    //////////////////////////////////////////////////////////////////////// 
    //
    // Render
    //

    TfStopwatch timer_hydra;

    if (settings && UseRenderSettingsPrim()) {
        printf("Rendering using the render settings prim <%s>...\n", settings.GetPath().GetText());

        HydraSetupCameraInfo camInfo =
            GetCameraInfo(sceneCamPath, sceneCamAspect, settings);

        // Create HdRenderSettingsMap for the RenderDelegate
        HdRenderSettingsMap settingsMap;

        // Add the Render Settings Prim path to the HdRenderSettingsMap
        // This is what triggers using this prim instead of the RenderSpec
        // inside HdPrman_RenderPass.
        settingsMap[
            HdPrmanRenderSettingsTokens->experimentalRenderSettingsPrimPath] =
            settings.GetPath();
        
        // Add the camera Path to the Settings Map as well so that the Render
        // Delegate can have it before syncing for shutter interval
        settingsMap[HdPrmanRenderSettingsTokens->experimentalSettingsCameraPath] =
            camInfo.cameraPath;

        AddVisualizerStyle(visualizerStyle, &settingsMap);
        AddNamespacedSettings(
            UsdRenderComputeNamespacedSettings(
                settings.GetPrim(), prmanNamespaces),
            &settingsMap);
        settingsMap[HdRenderSettingsTokens->enableInteractive] = false;

        HydraSetupAndRender(
            settingsMap, stage, camInfo, frameNum, cullStyle, &timer_hydra);

        printf("Rendered <%s>\n", settings.GetPath().GetText());
    }
    else {
        // When using the Render Spec dictionary in the legacy render settings
        // map to plumb settings, we specify the settings per product. For 
        // simplicity, we recreate the riley and hydra setup for each product. 
        // Eventually, this path will be deprecated and removed to leverage 
        // hydra's first-class support for render settings scene description.
        fprintf(stdout, "Rendering using the experimentalRenderSpec dictionary...\n");
        for (auto product: renderSpec.products) {
            printf("Rendering product %s...\n", product.name.GetText());

            HydraSetupCameraInfo camInfo = GetCameraInfoAndUpdateProduct(
                sceneCamPath, sceneCamAspect, &product);

            // Create HdRenderSettingsMap for the RenderDelegate
            HdRenderSettingsMap settingsMap;

            // Create and save the RenderSpecDict to the HdRenderSettingsMap
            settingsMap[HdPrmanRenderSettingsTokens->experimentalRenderSpec] =
                CreateRenderSpecDict(renderSpec, product);

            // Only allow "raster" for now.
            TF_VERIFY(product.type == TfToken("raster"));

            AddVisualizerStyle(visualizerStyle, &settingsMap);
            AddNamespacedSettings(product.namespacedSettings, &settingsMap);
            settingsMap[HdRenderSettingsTokens->enableInteractive] = false;

            HydraSetupAndRender(
                settingsMap, stage, camInfo, frameNum, cullStyle, &timer_hydra);

            printf("Rendered %s\n", product.name.GetText());
        }
    }

    if (!traceOutput.empty()) {
        std::ofstream outFile(traceOutput);
        TraceCollector::GetInstance().SetEnabled(false);
        TraceReporter::GetGlobalReporter()->Report(outFile);
    }

    if (!perfOutput.empty()) {
        std::ofstream perfResults(perfOutput);
        perfResults << "{'profile': 'usdOpen',"
            << " 'metric': 'time',"
            << " 'value': " << timer_usdOpen.GetSeconds() << ","
            << " 'samples': 1"
            << " }\n";
        perfResults << "{'profile': 'hydraExecute',"
            << " 'metric': 'time',"
            << " 'value': " << timer_hydra.GetSeconds() << ","
            << " 'samples': 1"
            << " }\n";
        perfResults << "{'profile': 'prmanRender',"
            << " 'metric': 'time',"
            << " 'value': " << timer_prmanRender.GetSeconds() << ","
            << " 'samples': 1"
            << " }\n";
    }
}
