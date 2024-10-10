//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/plugin/hdEmbree/renderDelegate.h"

#include "pxr/imaging/plugin/hdEmbree/config.h"
#include "pxr/imaging/plugin/hdEmbree/instancer.h"
#include "pxr/imaging/plugin/hdEmbree/renderParam.h"
#include "pxr/imaging/plugin/hdEmbree/renderPass.h"

#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/plugin/hdEmbree/mesh.h"
//XXX: Add other Rprim types later
#include "pxr/imaging/hd/camera.h"
//XXX: Add other Sprim types later
#include "pxr/imaging/hd/bprim.h"
//XXX: Add bprim types

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdEmbreeRenderSettingsTokens, HDEMBREE_RENDER_SETTINGS_TOKENS);

const TfTokenVector HdEmbreeRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
};

const TfTokenVector HdEmbreeRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    HdPrimTypeTokens->camera,
    HdPrimTypeTokens->extComputation,
};

const TfTokenVector HdEmbreeRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
    HdPrimTypeTokens->renderBuffer,
};

std::mutex HdEmbreeRenderDelegate::_mutexResourceRegistry;
std::atomic_int HdEmbreeRenderDelegate::_counterResourceRegistry;
HdResourceRegistrySharedPtr HdEmbreeRenderDelegate::_resourceRegistry;

/* static */
void HdEmbreeRenderDelegate::HandleRtcError (void* userPtr, RTCError code, const char* msg)
{
    // Forward RTC error messages through to hydra logging.
    switch (code) {
        case RTC_ERROR_UNKNOWN:
            TF_CODING_ERROR("Embree unknown error: %s", msg);
            break;
        case RTC_ERROR_INVALID_ARGUMENT:
            TF_CODING_ERROR("Embree invalid argument: %s", msg);
            break;
        case RTC_ERROR_INVALID_OPERATION:
            TF_CODING_ERROR("Embree invalid operation: %s", msg);
            break;
        case RTC_ERROR_OUT_OF_MEMORY:
            TF_CODING_ERROR("Embree out of memory: %s", msg);
            break;
        case RTC_ERROR_UNSUPPORTED_CPU:
            TF_CODING_ERROR("Embree unsupported CPU: %s", msg);
            break;
        case RTC_ERROR_CANCELLED:
            TF_CODING_ERROR("Embree cancelled: %s", msg);
            break;
        default:
            TF_CODING_ERROR("Embree invalid error code: %s", msg);
            break;
    }
}

static void _RenderCallback(HdEmbreeRenderer *renderer,
                            HdRenderThread *renderThread)
{
    renderer->Clear();
    renderer->Render(renderThread);
}

HdEmbreeRenderDelegate::HdEmbreeRenderDelegate()
    : HdRenderDelegate()
{
    _Initialize();
}

HdEmbreeRenderDelegate::HdEmbreeRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
    : HdRenderDelegate(settingsMap)
{
    _Initialize();
}

void
HdEmbreeRenderDelegate::_Initialize()
{
    // Initialize the settings and settings descriptors.
    _settingDescriptors.resize(5);
    _settingDescriptors[0] = { "Enable Scene Colors",
        HdEmbreeRenderSettingsTokens->enableSceneColors,
        VtValue(HdEmbreeConfig::GetInstance().useFaceColors) };
    _settingDescriptors[1] = { "Enable Ambient Occlusion",
        HdEmbreeRenderSettingsTokens->enableAmbientOcclusion,
        VtValue(HdEmbreeConfig::GetInstance().ambientOcclusionSamples > 0) };
    _settingDescriptors[2] = { "Ambient Occlusion Samples",
        HdEmbreeRenderSettingsTokens->ambientOcclusionSamples,
        VtValue(int(HdEmbreeConfig::GetInstance().ambientOcclusionSamples)) };
    _settingDescriptors[3] = { "Samples To Convergence",
        HdRenderSettingsTokens->convergedSamplesPerPixel,
        VtValue(int(HdEmbreeConfig::GetInstance().samplesToConvergence)) };
    _settingDescriptors[4] = { "Random Number Seed",
        HdEmbreeRenderSettingsTokens->randomNumberSeed,
        VtValue(HdEmbreeConfig::GetInstance().randomNumberSeed) };
    _PopulateDefaultSettings(_settingDescriptors);

    // Initialize the embree library handle (_rtcDevice).
    _rtcDevice = rtcNewDevice(nullptr);

    // Register our error message callback.
    rtcSetDeviceErrorFunction(_rtcDevice,HandleRtcError,NULL);

    // Create the top-level scene.
    //
    // RTC_SCENE_DYNAMIC indicates we'll be updating the scene between draw
    // calls. RTC_INTERSECT1 indicates we'll be casting single rays, and
    // RTC_INTERPOLATE indicates we'll be storing primvars in embree objects
    // and querying them with rtcInterpolate.
    //
    // XXX: Investigate ray packets.
    _rtcScene = rtcNewScene(_rtcDevice);

    // RTC_SCENE_FLAG_DYNAMIC: Provides better build performance for dynamic
    // scenes (but also higher memory consumption).
    rtcSetSceneFlags(_rtcScene, RTC_SCENE_FLAG_DYNAMIC);

    // RTC_BUILD_QUALITY_LOW: Create lower quality data structures,
    // e.g. for dynamic scenes. A two-level spatial index structure is built
    // when enabling this mode, which supports fast partial scene updates,
    // and allows for setting a per-geometry build quality through
    // the rtcSetGeometryBuildQuality function.
    rtcSetSceneBuildQuality(_rtcScene, RTC_BUILD_QUALITY_LOW);

    // Store top-level embree objects inside a render param that can be
    // passed to prims during Sync(). Also pass a handle to the render thread.
    _renderParam = std::make_shared<HdEmbreeRenderParam>(
        _rtcDevice, _rtcScene, &_renderThread, &_sceneVersion);

    // Pass the scene handle to the renderer.
    _renderer.SetScene(_rtcScene);

    // Set the background render thread's rendering entrypoint to
    // HdEmbreeRenderer::Render.
    _renderThread.SetRenderCallback(
        std::bind(_RenderCallback, &_renderer, &_renderThread));
    // Start the background render thread.
    _renderThread.StartThread();

    // Initialize one resource registry for all embree plugins
    std::lock_guard<std::mutex> guard(_mutexResourceRegistry);

    if (_counterResourceRegistry.fetch_add(1) == 0) {
        _resourceRegistry = std::make_shared<HdResourceRegistry>();
    }
}

HdEmbreeRenderDelegate::~HdEmbreeRenderDelegate()
{
    // Clean the resource registry only when it is the last Embree delegate
    {
        std::lock_guard<std::mutex> guard(_mutexResourceRegistry);
        if (_counterResourceRegistry.fetch_sub(1) == 1) {
            _resourceRegistry.reset();
        }
    }

    _renderThread.StopThread();

    // Destroy embree library and scene state.
    _renderParam.reset();
    rtcReleaseScene(_rtcScene);
    rtcReleaseDevice (_rtcDevice);
}

HdRenderSettingDescriptorList
HdEmbreeRenderDelegate::GetRenderSettingDescriptors() const
{
    return _settingDescriptors;
}

HdRenderParam*
HdEmbreeRenderDelegate::GetRenderParam() const
{
    return _renderParam.get();
}

void
HdEmbreeRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
}

TfTokenVector const&
HdEmbreeRenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

TfTokenVector const&
HdEmbreeRenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

TfTokenVector const&
HdEmbreeRenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}

HdResourceRegistrySharedPtr
HdEmbreeRenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

HdAovDescriptor
HdEmbreeRenderDelegate::GetDefaultAovDescriptor(TfToken const& name) const
{
    if (name == HdAovTokens->color) {
        return HdAovDescriptor(HdFormatUNorm8Vec4, true,
                               VtValue(GfVec4f(0.0f)));
    } else if (name == HdAovTokens->normal || name == HdAovTokens->Neye) {
        return HdAovDescriptor(HdFormatFloat32Vec3, false,
                               VtValue(GfVec3f(-1.0f)));
    } else if (name == HdAovTokens->depth) {
        return HdAovDescriptor(HdFormatFloat32, false, VtValue(1.0f));
    } else if (name == HdAovTokens->cameraDepth) {
        return HdAovDescriptor(HdFormatFloat32, false, VtValue(0.0f));
    } else if (name == HdAovTokens->primId ||
               name == HdAovTokens->instanceId ||
               name == HdAovTokens->elementId) {
        return HdAovDescriptor(HdFormatInt32, false, VtValue(-1));
    } else {
        HdParsedAovToken aovId(name);
        if (aovId.isPrimvar) {
            return HdAovDescriptor(HdFormatFloat32Vec3, false,
                                   VtValue(GfVec3f(0.0f)));
        }
    }

    return HdAovDescriptor();
}

VtDictionary 
HdEmbreeRenderDelegate::GetRenderStats() const
{
    VtDictionary stats;
    stats[HdPerfTokens->numCompletedSamples.GetString()] = 
        _renderer.GetCompletedSamples();
    return stats;
}

bool
HdEmbreeRenderDelegate::IsPauseSupported() const
{
    return true;
}

bool
HdEmbreeRenderDelegate::Pause()
{
    _renderThread.PauseRender();
    return true;
}

bool
HdEmbreeRenderDelegate::Resume()
{
    _renderThread.ResumeRender();
    return true;
}

HdRenderPassSharedPtr
HdEmbreeRenderDelegate::CreateRenderPass(HdRenderIndex *index,
                            HdRprimCollection const& collection)
{
    return HdRenderPassSharedPtr(new HdEmbreeRenderPass(
        index, collection, &_renderThread, &_renderer, &_sceneVersion));
}

HdInstancer *
HdEmbreeRenderDelegate::CreateInstancer(HdSceneDelegate *delegate,
                                        SdfPath const& id)
{
    return new HdEmbreeInstancer(delegate, id);
}

void
HdEmbreeRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
    delete instancer;
}

HdRprim *
HdEmbreeRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId)
{
    if (typeId == HdPrimTypeTokens->mesh) {
        return new HdEmbreeMesh(rprimId);
    } else {
        TF_CODING_ERROR("Unknown Rprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
HdEmbreeRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    delete rPrim;
}

HdSprim *
HdEmbreeRenderDelegate::CreateSprim(TfToken const& typeId,
                                    SdfPath const& sprimId)
{
    if (typeId == HdPrimTypeTokens->camera) {
        return new HdCamera(sprimId);
    } else if (typeId == HdPrimTypeTokens->extComputation) {
        return new HdExtComputation(sprimId);
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}

HdSprim *
HdEmbreeRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    // For fallback sprims, create objects with an empty scene path.
    // They'll use default values and won't be updated by a scene delegate.
    if (typeId == HdPrimTypeTokens->camera) {
        return new HdCamera(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->extComputation) {
        return new HdExtComputation(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
HdEmbreeRenderDelegate::DestroySprim(HdSprim *sPrim)
{
    delete sPrim;
}

HdBprim *
HdEmbreeRenderDelegate::CreateBprim(TfToken const& typeId,
                                    SdfPath const& bprimId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdEmbreeRenderBuffer(bprimId);
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }
    return nullptr;
}

HdBprim *
HdEmbreeRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdEmbreeRenderBuffer(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }
    return nullptr;
}

void
HdEmbreeRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    delete bPrim;
}

PXR_NAMESPACE_CLOSE_SCOPE
