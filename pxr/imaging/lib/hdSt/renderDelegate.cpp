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
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/renderDelegate.h"

#include "pxr/imaging/hdSt/basisCurves.h"
#include "pxr/imaging/hdSt/camera.h"
#include "pxr/imaging/hdSt/drawTarget.h"
#include "pxr/imaging/hdSt/glslfxShader.h"
#include "pxr/imaging/hdSt/instancer.h"
#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdSt/mesh.h"
#include "pxr/imaging/hdSt/points.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/shader.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/package.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/texture.h"

#include "pxr/imaging/glf/glslfx.h"


PXR_NAMESPACE_OPEN_SCOPE

const TfTokenVector HdStRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
    HdPrimTypeTokens->basisCurves,
    HdPrimTypeTokens->points
};

const TfTokenVector HdStRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    HdPrimTypeTokens->camera,
    HdPrimTypeTokens->light,
    HdPrimTypeTokens->drawTarget,
    HdPrimTypeTokens->shader
};

const TfTokenVector HdStRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
    HdPrimTypeTokens->texture
};

std::mutex HdStRenderDelegate::_mutexResourceRegistry;
std::atomic_int HdStRenderDelegate::_counterResourceRegistry;
HdStResourceRegistrySharedPtr HdStRenderDelegate::_resourceRegistry;

HdStRenderDelegate::HdStRenderDelegate()
{
    // Initialize one resource registry for all St plugins
    // It will also add the resource to the logging object so we
    // can query the resources used by all St plugins later
    std::lock_guard<std::mutex> guard(_mutexResourceRegistry);
    
    if (_counterResourceRegistry.fetch_add(1) == 0) {
        _resourceRegistry.reset( new HdStResourceRegistry() );
        HdPerfLog::GetInstance().AddResourceRegistry(_resourceRegistry);
    }
}

HdStRenderDelegate::~HdStRenderDelegate()
{
    // Here we could destroy the resource registry when the last render
    // delegate HdSt is destroyed, however we prefer to keep the resources
    // around to match previous singleton behaviour (for now).
}

const TfTokenVector &
HdStRenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

const TfTokenVector &
HdStRenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

const TfTokenVector &
HdStRenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}

HdRenderParam *
HdStRenderDelegate::GetRenderParam() const
{
    return nullptr;
}

HdResourceRegistrySharedPtr
HdStRenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

HdRenderPassSharedPtr
HdStRenderDelegate::CreateRenderPass(HdRenderIndex *index,
                        HdRprimCollection const& collection)
{
    return HdRenderPassSharedPtr(new HdSt_RenderPass(index, collection));
}

HdInstancer *
HdStRenderDelegate::CreateInstancer(HdSceneDelegate *delegate,
                                    SdfPath const& id,
                                    SdfPath const& instancerId)
{
    return new HdStInstancer(delegate, id, instancerId);
}

void
HdStRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
    delete instancer;
}

HdRprim *
HdStRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId,
                                    SdfPath const& instancerId)
{
    if (typeId == HdPrimTypeTokens->mesh) {
        return new HdStMesh(rprimId, instancerId);
    } else if (typeId == HdPrimTypeTokens->basisCurves) {
        return new HdStBasisCurves(rprimId, instancerId);
    } else  if (typeId == HdPrimTypeTokens->points) {
        return new HdStPoints(rprimId, instancerId);
    } else {
        TF_CODING_ERROR("Unknown Rprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
HdStRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    delete rPrim;
}

HdSprim *
HdStRenderDelegate::CreateSprim(TfToken const& typeId,
                                    SdfPath const& sprimId)
{
    if (typeId == HdPrimTypeTokens->camera) {
        return new HdStCamera(sprimId);
    } else if (typeId == HdPrimTypeTokens->light) {
        return new HdStLight(sprimId);
    } else  if (typeId == HdPrimTypeTokens->drawTarget) {
        return new HdStDrawTarget(sprimId);
    } else  if (typeId == HdPrimTypeTokens->shader) {
        return new HdStShader(sprimId);
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}

HdSprim *
HdStRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    if (typeId == HdPrimTypeTokens->camera) {
        return new HdStCamera(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->light) {
        return new HdStLight(SdfPath::EmptyPath());
    } else  if (typeId == HdPrimTypeTokens->drawTarget) {
        return new HdStDrawTarget(SdfPath::EmptyPath());
    } else  if (typeId == HdPrimTypeTokens->shader) {
        return _CreateFallbackShaderPrim();
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}


void
HdStRenderDelegate::DestroySprim(HdSprim *sPrim)
{
    delete sPrim;
}

HdBprim *
HdStRenderDelegate::CreateBprim(TfToken const& typeId,
                                    SdfPath const& bprimId)
{
    if (typeId == HdPrimTypeTokens->texture) {
        return new HdTexture(bprimId);
    } else  {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }


    return nullptr;
}

HdBprim *
HdStRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    if (typeId == HdPrimTypeTokens->texture) {
        return new HdTexture(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
HdStRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    delete bPrim;
}

HdSprim *
HdStRenderDelegate::_CreateFallbackShaderPrim()
{
    GlfGLSLFXSharedPtr glslfx(new GlfGLSLFX(HdPackageFallbackSurfaceShader()));

    HdStSurfaceShaderSharedPtr fallbackShaderCode(new HdStGLSLFXShader(glslfx));

    HdStShader *shader = new HdStShader(SdfPath::EmptyPath());
    shader->SetSurfaceShader(fallbackShaderCode);

    return shader;
}


void
HdStRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
    // --------------------------------------------------------------------- //
    // RESOLVE, COMPUTE & COMMIT PHASE
    // --------------------------------------------------------------------- //
    // All the required input data is now resident in memory, next we must:
    //
    //     1) Execute compute as needed for normals, tessellation, etc.
    //     2) Commit resources to the GPU.
    //     3) Update any scene-level acceleration structures.

    // Commit all pending source data.
    _resourceRegistry->Commit();

    if (tracker->IsGarbageCollectionNeeded()) {
        _resourceRegistry->GarbageCollect();
        tracker->ClearGarbageCollectionNeeded();
        tracker->MarkAllCollectionsDirty();
    }

    // see bug126621. currently dispatch buffers need to be released
    //                more frequently than we expect.
    _resourceRegistry->GarbageCollectDispatchBuffers();
}


PXR_NAMESPACE_CLOSE_SCOPE
