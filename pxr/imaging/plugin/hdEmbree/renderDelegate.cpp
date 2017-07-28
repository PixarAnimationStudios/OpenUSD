//
// Copyright 2017 Pixar
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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hdEmbree/renderDelegate.h"

#include "pxr/imaging/hdEmbree/config.h"
#include "pxr/imaging/hdEmbree/instancer.h"
#include "pxr/imaging/hdEmbree/renderParam.h"
#include "pxr/imaging/hdEmbree/renderPass.h"

#include "pxr/imaging/hd/resourceRegistry.h"

#include "pxr/imaging/hdEmbree/mesh.h"
//XXX: Add other Rprim types later
#include "pxr/imaging/hdSt/camera.h"
//XXX: Add other Sprim types later
#include "pxr/imaging/hd/bprim.h"
//XXX: Add bprim types

PXR_NAMESPACE_OPEN_SCOPE

const TfTokenVector HdEmbreeRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
};

const TfTokenVector HdEmbreeRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    HdPrimTypeTokens->camera,
};

const TfTokenVector HdEmbreeRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
};

std::atomic_int HdEmbreeRenderDelegate::_counterResourceRegistry;
HdResourceRegistrySharedPtr HdEmbreeRenderDelegate::_resourceRegistry;

/* static */
void
HdEmbreeRenderDelegate::HandleRtcError(const RTCError code, const char* msg)
{
    // Forward RTC error messages through to hydra logging.
    switch (code) {
        case RTC_UNKNOWN_ERROR:
            TF_CODING_ERROR("Embree unknown error: %s", msg);
            break;
        case RTC_INVALID_ARGUMENT:
            TF_CODING_ERROR("Embree invalid argument: %s", msg);
            break;
        case RTC_INVALID_OPERATION:
            TF_CODING_ERROR("Embree invalid operation: %s", msg);
            break;
        case RTC_OUT_OF_MEMORY:
            TF_CODING_ERROR("Embree out of memory: %s", msg);
            break;
        case RTC_UNSUPPORTED_CPU:
            TF_CODING_ERROR("Embree unsupported CPU: %s", msg);
            break;
        case RTC_CANCELLED:
            TF_CODING_ERROR("Embree cancelled: %s", msg);
            break;
        default:
            TF_CODING_ERROR("Embree invalid error code: %s", msg);
            break;
    }
}

HdEmbreeRenderDelegate::HdEmbreeRenderDelegate()
{
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
    // passed to prims during Sync().
    _renderParam =
        std::make_shared<HdEmbreeRenderParam>(_rtcDevice, _rtcScene);

    // Initialize one resource registry for all embree plugins
    if (_counterResourceRegistry.fetch_add(1) == 0) {
        _resourceRegistry.reset( new HdResourceRegistry() );
    }
}

HdEmbreeRenderDelegate::~HdEmbreeRenderDelegate()
{
    // Clean the resource registry only when it is the last Embree delegate
    if (_counterResourceRegistry.fetch_sub(1) == 1) {
        _resourceRegistry.reset();
    }

    // Destroy embree library and scene state.
    _renderParam.reset();
    rtcDeleteScene(_rtcScene);
    rtcDeleteDevice(_rtcDevice);
}

HdRenderParam*
HdEmbreeRenderDelegate::GetRenderParam() const
{
    return _renderParam.get();
}

void
HdEmbreeRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
    // CommitResources() is called after prim sync has finished, but before any
    // tasks (such as draw tasks) have run. HdEmbree primitives have already
    // updated embree buffer pointers and dirty state in prim Sync(), but we
    // still need to rebuild acceleration datastructures here with rtcCommit().
    //
    // During task execution, the embree scene is treated as read-only by the
    // drawing code; the BVH won't be updated until the next time through
    // HdEngine::Execute().
    rtcCommit(_rtcScene);
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

HdRenderPassSharedPtr
HdEmbreeRenderDelegate::CreateRenderPass(HdRenderIndex *index,
                            HdRprimCollection const& collection)
{
    return HdRenderPassSharedPtr(
        new HdEmbreeRenderPass(index, collection, _rtcScene));
}

HdInstancer *
HdEmbreeRenderDelegate::CreateInstancer(HdSceneDelegate *delegate,
                                        SdfPath const& id,
                                        SdfPath const& instancerId)
{
    return new HdEmbreeInstancer(delegate, id, instancerId);
}

void
HdEmbreeRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
    delete instancer;
}

HdRprim *
HdEmbreeRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId,
                                    SdfPath const& instancerId)
{
    if (typeId == HdPrimTypeTokens->mesh) {
        return new HdEmbreeMesh(rprimId, instancerId);
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
        return new HdStCamera(sprimId);
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
        return new HdStCamera(SdfPath::EmptyPath());
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
    TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    return nullptr;
}

HdBprim *
HdEmbreeRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    return nullptr;
}

void
HdEmbreeRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    delete bPrim;
}

PXR_NAMESPACE_CLOSE_SCOPE
