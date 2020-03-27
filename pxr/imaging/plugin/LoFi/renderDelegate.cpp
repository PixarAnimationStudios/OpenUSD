//
// Copyright 2020 Pixar
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
#include <iostream>
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/renderDelegate.h"
#include "pxr/imaging/plugin/LoFi/renderParam.h"
#include "pxr/imaging/plugin/LoFi/renderPass.h"

#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/plugin/LoFi/mesh.h"
//XXX: Add other Rprim types later
#include "pxr/imaging/hd/camera.h"
//XXX: Add other Sprim types later
#include "pxr/imaging/hd/bprim.h"
//XXX: Add bprim types

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(LoFiRenderSettingsTokens, LOFI_RENDER_SETTINGS_TOKENS);


const TfTokenVector LoFiRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
};

const TfTokenVector LoFiRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
};

const TfTokenVector LoFiRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
};

std::mutex LoFiRenderDelegate::_mutexResourceRegistry;
std::atomic_int LoFiRenderDelegate::_counterResourceRegistry;
HdResourceRegistrySharedPtr LoFiRenderDelegate::_resourceRegistry;

LoFiRenderDelegate::LoFiRenderDelegate()
    : HdRenderDelegate()
{
    _Initialize();
}

LoFiRenderDelegate::LoFiRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
    : HdRenderDelegate(settingsMap)
{
    _Initialize();
}

void
LoFiRenderDelegate::_Initialize()
{
  /*
  // Initialize the settings and settings descriptors.
  _settingDescriptors.resize(4);
  _settingDescriptors[0] = { "Enable Lights",
      LoFiRenderSettingsTokens->enableLights,
      VtValue(LoFiConfig::GetInstance().useFaceColors) };
  _settingDescriptors[1] = { "Enable Ambient Occlusion",
      LoFiRenderSettingsTokens->enableAmbientOcclusion,
      VtValue(HdEmbreeConfig::GetInstance().ambientOcclusionSamples > 0) };
  _settingDescriptors[2] = { "Ambient Occlusion Samples",
      LoFiRenderSettingsTokens->ambientOcclusionSamples,
      VtValue(int(HdEmbreeConfig::GetInstance().ambientOcclusionSamples)) };
  _settingDescriptors[3] = { "Samples To Convergence",
      HdRenderSettingsTokens->convergedSamplesPerPixel,
      VtValue(int(HdEmbreeConfig::GetInstance().samplesToConvergence)) };
  _PopulateDefaultSettings(_settingDescriptors);
*/
  // Create the top-level scene.
  _scene = new LoFiScene;

  // Store top-level LoFi objects inside a render param that can be
  // passed to prims during Sync(). Also pass a handle to the render thread.
  _renderParam = 
    std::make_shared<LoFiRenderParam>(
      _scene, &_renderThread, &_sceneVersion);

  // Set the background render thread's rendering entrypoint to
  // LoFiRenderer::Render.
  //_renderThread.SetRenderCallback(
  //    std::bind(_RenderCallback, &_renderer, &_renderThread));
  // Start the background render thread.
  //_renderThread.StartThread();

  // Initialize one resource registry for all embree plugins
  std::lock_guard<std::mutex> guard(_mutexResourceRegistry);

  if (_counterResourceRegistry.fetch_add(1) == 0) {
      _resourceRegistry.reset( new HdResourceRegistry() );
  }

    std::cout << "### Creating LoFi RenderDelegate" << std::endl;
    _resourceRegistry.reset(new HdResourceRegistry());


}

/*
void
HdEmbreeRenderDelegate::_Initialize()
{
    // Initialize the settings and settings descriptors.
    _settingDescriptors.resize(4);
    _settingDescriptors[0] = { "Enable Scene Colors",
        LoFiRenderSettingsTokens->enableSceneColors,
        VtValue(HdEmbreeConfig::GetInstance().useFaceColors) };
    _settingDescriptors[1] = { "Enable Ambient Occlusion",
        LoFiRenderSettingsTokens->enableAmbientOcclusion,
        VtValue(HdEmbreeConfig::GetInstance().ambientOcclusionSamples > 0) };
    _settingDescriptors[2] = { "Ambient Occlusion Samples",
        LoFiRenderSettingsTokens->ambientOcclusionSamples,
        VtValue(int(HdEmbreeConfig::GetInstance().ambientOcclusionSamples)) };
    _settingDescriptors[3] = { "Samples To Convergence",
        HdRenderSettingsTokens->convergedSamplesPerPixel,
        VtValue(int(HdEmbreeConfig::GetInstance().samplesToConvergence)) };
    _PopulateDefaultSettings(_settingDescriptors);

    // Initialize the embree library handle (_rtcDevice).
    _rtcDevice = rtcNewDevice(nullptr);

    // Register our error message callback.
    rtcDeviceSetErrorFunction(_rtcDevice, HandleRtcError);

    // Embree has an internal cache for subdivision surface computations.
    // HdEmbree exposes the size as an environment variable.
    unsigned int subdivisionCache =
        HdEmbreeConfig::GetInstance().subdivisionCache;
    rtcDeviceSetParameter1i(_rtcDevice, RTC_SOFTWARE_CACHE_SIZE,
        subdivisionCache);

    // Create the top-level scene.
    //
    // RTC_SCENE_DYNAMIC indicates we'll be updating the scene between draw
    // calls. RTC_INTERSECT1 indicates we'll be casting single rays, and
    // RTC_INTERPOLATE indicates we'll be storing primvars in embree objects
    // and querying them with rtcInterpolate.
    //
    // XXX: Investigate ray packets.
    _rtcScene = rtcDeviceNewScene(_rtcDevice, RTC_SCENE_DYNAMIC,
        RTC_INTERSECT1 | RTC_INTERPOLATE);

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
        _resourceRegistry.reset( new HdResourceRegistry() );
    }
}

*/

LoFiRenderDelegate::~LoFiRenderDelegate()
{
  // Clean the resource registry only when it is the last LoFi delegate
  {
    std::lock_guard<std::mutex> guard(_mutexResourceRegistry);
    if (_counterResourceRegistry.fetch_sub(1) == 1) {
        _resourceRegistry.reset();
    }
  }

  _renderThread.StopThread();

  // Destroy embree library and scene state.
  _renderParam.reset();
  delete _scene;
}

HdRenderSettingDescriptorList
LoFiRenderDelegate::GetRenderSettingDescriptors() const
{
    return _settingDescriptors;
}

void 
LoFiRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
}

TfTokenVector const&
LoFiRenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

TfTokenVector const&
LoFiRenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

TfTokenVector const&
LoFiRenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}



HdResourceRegistrySharedPtr
LoFiRenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

HdAovDescriptor
LoFiRenderDelegate::GetDefaultAovDescriptor(TfToken const& name) const
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

HdRenderPassSharedPtr 
LoFiRenderDelegate::CreateRenderPass(
    HdRenderIndex *index,
    HdRprimCollection const& collection)
{
    //std::cout << "Create RenderPass with Collection=" 
    //    << collection.GetName() << std::endl; 

    return HdRenderPassSharedPtr(new LoFiRenderPass(index, collection));  
}

HdRprim *
LoFiRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId,
                                    SdfPath const& instancerId)
{
    /*std::cout << "Create LoFi Rprim type=" << typeId.GetText() 
        << " id=" << rprimId 
        << " instancerId=" << instancerId 
        << std::endl;*/

    if (typeId == HdPrimTypeTokens->mesh) {
        return new LoFiMesh(rprimId, instancerId);
    } else {
        TF_CODING_ERROR("Unknown Rprim type=%s id=%s", 
            typeId.GetText(), 
            rprimId.GetText());
    }
    return nullptr;
}

void
LoFiRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    //std::cout << "Destroy LoFi Rprim id=" << rPrim->GetId() << std::endl;
    delete rPrim;
}

HdSprim *
LoFiRenderDelegate::CreateSprim(TfToken const& typeId,
                                    SdfPath const& sprimId)
{
    TF_CODING_ERROR("Unknown Sprim type=%s id=%s", 
        typeId.GetText(), 
        sprimId.GetText());
    return nullptr;
}

HdSprim *
LoFiRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    TF_CODING_ERROR("Creating unknown fallback sprim type=%s", 
        typeId.GetText()); 
    return nullptr;
}

void
LoFiRenderDelegate::DestroySprim(HdSprim *sPrim)
{
    TF_CODING_ERROR("Destroy Sprim not supported");
}

HdBprim *
LoFiRenderDelegate::CreateBprim(TfToken const& typeId, SdfPath const& bprimId)
{
    TF_CODING_ERROR("Unknown Bprim type=%s id=%s", 
        typeId.GetText(), 
        bprimId.GetText());
    return nullptr;
}

HdBprim *
LoFiRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    TF_CODING_ERROR("Creating unknown fallback bprim type=%s", 
        typeId.GetText()); 
    return nullptr;
}

void
LoFiRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    TF_CODING_ERROR("Destroy Bprim not supported");
}

HdInstancer *
LoFiRenderDelegate::CreateInstancer(
    HdSceneDelegate *delegate,
    SdfPath const& id,
    SdfPath const& instancerId)
{
    TF_CODING_ERROR("Creating Instancer not supported id=%s instancerId=%s", 
        id.GetText(), instancerId.GetText());
    return nullptr;
}

void 
LoFiRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
    TF_CODING_ERROR("Destroy instancer not supported");
}

HdRenderParam*
LoFiRenderDelegate::GetRenderParam() const
{
    return _renderParam.get();
}


PXR_NAMESPACE_CLOSE_SCOPE
