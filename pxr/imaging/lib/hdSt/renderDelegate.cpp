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
#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdSt/mesh.h"
#include "pxr/imaging/hdSt/points.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/shader.h"

#include "pxr/imaging/hd/glslfxShader.h"
#include "pxr/imaging/hd/package.h"
#include "pxr/imaging/hd/resourceRegistry.h"
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

HdStRenderDelegate::HdStRenderDelegate()
{
    static std::once_flag reprsOnce;
    std::call_once(reprsOnce, _ConfigureReprs);
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

HdRenderPassSharedPtr
HdStRenderDelegate::CreateRenderPass(HdRenderIndex *index)
{
    return HdRenderPassSharedPtr(new HdSt_RenderPass(index));
}

HdRenderPassSharedPtr
HdStRenderDelegate::CreateRenderPass(HdRenderIndex *index,
                        HdRprimCollection const& collection)
{
    return HdRenderPassSharedPtr(new HdSt_RenderPass(index, collection));
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

// static
void
HdStRenderDelegate::_ConfigureReprs()
{
    // pre-defined reprs (to be deprecated or minimalized)
    HdStMesh::ConfigureRepr(HdTokens->hull,
                            HdStMeshReprDesc(HdMeshGeomStyleHull,
                                             HdCullStyleDontCare,
                                             /*lit=*/true,
                                             /*smoothNormals=*/false,
                                             /*blendWireframeColor=*/false));
    HdStMesh::ConfigureRepr(HdTokens->smoothHull,
                            HdStMeshReprDesc(HdMeshGeomStyleHull,
                                             HdCullStyleDontCare,
                                             /*lit=*/true,
                                             /*smoothNormals=*/true,
                                             /*blendWireframeColor=*/false));
    HdStMesh::ConfigureRepr(HdTokens->wire,
                            HdStMeshReprDesc(HdMeshGeomStyleHullEdgeOnly,
                                             HdCullStyleDontCare,
                                             /*lit=*/true,
                                             /*smoothNormals=*/true,
                                             /*blendWireframeColor=*/true));
    HdStMesh::ConfigureRepr(HdTokens->wireOnSurf,
                            HdStMeshReprDesc(HdMeshGeomStyleHullEdgeOnSurf,
                                             HdCullStyleDontCare,
                                             /*lit=*/true,
                                             /*smoothNormals=*/true,
                                             /*blendWireframeColor=*/true));

    HdStMesh::ConfigureRepr(HdTokens->refined,
                            HdStMeshReprDesc(HdMeshGeomStyleSurf,
                                             HdCullStyleDontCare,
                                             /*lit=*/true,
                                             /*smoothNormals=*/true,
                                             /*blendWireframeColor=*/false));
    HdStMesh::ConfigureRepr(HdTokens->refinedWire,
                            HdStMeshReprDesc(HdMeshGeomStyleEdgeOnly,
                                             HdCullStyleDontCare,
                                             /*lit=*/true,
                                             /*smoothNormals=*/true,
                                             /*blendWireframeColor=*/true));
    HdStMesh::ConfigureRepr(HdTokens->refinedWireOnSurf,
                            HdStMeshReprDesc(HdMeshGeomStyleEdgeOnSurf,
                                             HdCullStyleDontCare,
                                             /*lit=*/true,
                                             /*smoothNormals=*/true,
                                             /*blendWireframeColor=*/true));

    HdStBasisCurves::ConfigureRepr(HdTokens->hull,
                                   HdBasisCurvesGeomStyleLine);
    HdStBasisCurves::ConfigureRepr(HdTokens->smoothHull,
                                   HdBasisCurvesGeomStyleLine);
    HdStBasisCurves::ConfigureRepr(HdTokens->wire,
                                   HdBasisCurvesGeomStyleLine);
    HdStBasisCurves::ConfigureRepr(HdTokens->wireOnSurf,
                                   HdBasisCurvesGeomStyleLine);
    HdStBasisCurves::ConfigureRepr(HdTokens->refined,
                                   HdBasisCurvesGeomStyleRefined);
    // XXX: draw coarse line for refinedWire (filed as bug 129550)
    HdStBasisCurves::ConfigureRepr(HdTokens->refinedWire,
                                   HdBasisCurvesGeomStyleLine);
    HdStBasisCurves::ConfigureRepr(HdTokens->refinedWireOnSurf,
                                   HdBasisCurvesGeomStyleRefined);

    HdStPoints::ConfigureRepr(HdTokens->hull,
                              HdPointsGeomStylePoints);
    HdStPoints::ConfigureRepr(HdTokens->smoothHull,
                              HdPointsGeomStylePoints);
    HdStPoints::ConfigureRepr(HdTokens->wire,
                              HdPointsGeomStylePoints);
    HdStPoints::ConfigureRepr(HdTokens->wireOnSurf,
                              HdPointsGeomStylePoints);
    HdStPoints::ConfigureRepr(HdTokens->refined,
                              HdPointsGeomStylePoints);
    HdStPoints::ConfigureRepr(HdTokens->refinedWire,
                              HdPointsGeomStylePoints);
    HdStPoints::ConfigureRepr(HdTokens->refinedWireOnSurf,
                              HdPointsGeomStylePoints);
}

HdSprim *
HdStRenderDelegate::_CreateFallbackShaderPrim()
{
    GlfGLSLFXSharedPtr glslfx(new GlfGLSLFX(HdPackageFallbackSurfaceShader()));

    HdSurfaceShaderSharedPtr fallbackShaderCode(new HdGLSLFXShader(glslfx));

    HdStShader *shader = new HdStShader(SdfPath::EmptyPath());
    shader->SetSurfaceShader(fallbackShaderCode);

    return shader;
}


void
HdStRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
    HdResourceRegistry &resourceRegistry = HdResourceRegistry::GetInstance();

    // --------------------------------------------------------------------- //
    // RESOLVE, COMPUTE & COMMIT PHASE
    // --------------------------------------------------------------------- //
    // All the required input data is now resident in memory, next we must:
    //
    //     1) Execute compute as needed for normals, tessellation, etc.
    //     2) Commit resources to the GPU.
    //     3) Update any scene-level acceleration structures.

    // Commit all pending source data.
    resourceRegistry.Commit();

    if (tracker->IsGarbageCollectionNeeded()) {
        resourceRegistry.GarbageCollect();
        tracker->ClearGarbageCollectionNeeded();
        tracker->MarkAllCollectionsDirty();
    }

    // see bug126621. currently dispatch buffers need to be released
    //                more frequently than we expect.
    resourceRegistry.GarbageCollectDispatchBuffers();
}

PXR_NAMESPACE_CLOSE_SCOPE
