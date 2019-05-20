//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hdx/unitTestDelegate.h"

#include "pxr/base/gf/frustum.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/texture.h"
#include "pxr/imaging/hd/textureResource.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hdSt/drawTarget.h"
#include "pxr/imaging/hdSt/drawTargetAttachmentDescArray.h"
#include "pxr/imaging/hdSt/light.h"

#include "pxr/imaging/hdx/drawTargetTask.h"
#include "pxr/imaging/hdx/drawTargetResolveTask.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/shadowMatrixComputation.h"

#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/pxOsd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (rotate)
    (scale)
    (translate)
);

static void
_CreateGrid(int nx, int ny, VtVec3fArray *points,
            VtIntArray *numVerts, VtIntArray *verts)
{
    // create a unit plane (-1 ~ 1)
    for (int y = 0; y <= ny; ++y) {
        for (int x = 0; x <= nx; ++x) {
            GfVec3f p(2.0*x/float(nx) - 1.0,
                      2.0*y/float(ny) - 1.0,
                      0);
            points->push_back(p);
        }
    }
    for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; ++x) {
            numVerts->push_back(4);
            verts->push_back(    y*(nx+1) + x    );
            verts->push_back(    y*(nx+1) + x + 1);
            verts->push_back((y+1)*(nx+1) + x + 1);
            verts->push_back((y+1)*(nx+1) + x    );
        }
    }
}

namespace {
class ShadowMatrix : public HdxShadowMatrixComputation
{
public:
    ShadowMatrix(GlfSimpleLight const &light) {
        GfFrustum frustum;
        frustum.SetProjectionType(GfFrustum::Orthographic);
        frustum.SetWindow(GfRange2d(GfVec2d(-10, -10), GfVec2d(10, 10)));
        frustum.SetNearFar(GfRange1d(0, 100));
        GfVec4d pos = light.GetPosition();
        frustum.SetPosition(GfVec3d(0, 0, 10));
        frustum.SetRotation(GfRotation(GfVec3d(0, 0, 1),
                                       GfVec3d(pos[0], pos[1], pos[2])));

        _shadowMatrix =
            frustum.ComputeViewMatrix() * frustum.ComputeProjectionMatrix();
    }

    virtual GfMatrix4d Compute(const GfVec4f &viewport,
                               CameraUtilConformWindowPolicy policy) {
        return _shadowMatrix;
    }
private:
    GfMatrix4d _shadowMatrix;
};

class DrawTargetTextureResource : public HdTextureResource
{
public:
    DrawTargetTextureResource(GlfDrawTargetRefPtr const &drawTarget)
        : _drawTarget(drawTarget) {
    }
    virtual ~DrawTargetTextureResource() {
    };

    virtual HdTextureType GetTextureType() const override {
        return HdTextureType::Uv;
    }

    virtual GLuint GetTexelsTextureId() {
        return _drawTarget->GetAttachment("color")->GetGlTextureName();
    }
    virtual GLuint GetTexelsSamplerId() {
        return 0;
    }
    virtual uint64_t GetTexelsTextureHandle() {
        return 0;
    }

    virtual GLuint GetLayoutTextureId() {
        return 0;
    }
    virtual uint64_t GetLayoutTextureHandle() {
        return 0;
    }

    virtual size_t GetMemoryUsed() {
        return 0;
    }

private:
    GlfDrawTargetRefPtr _drawTarget;
};

}

// ------------------------------------------------------------------------
Hdx_UnitTestDelegate::Hdx_UnitTestDelegate(HdRenderIndex *index)
    : HdSceneDelegate(index, SdfPath::AbsoluteRootPath())
    , _refineLevel(0)
{
    // add camera
    _cameraId = SdfPath("/camera");
    GetRenderIndex().InsertSprim(HdPrimTypeTokens->camera, this, _cameraId);
    GfFrustum frustum;
    frustum.SetPosition(GfVec3d(0, 0, 3));
    SetCamera(frustum.ComputeViewMatrix(), frustum.ComputeProjectionMatrix());

    // Add draw target state tracking support.
    GetRenderIndex().GetChangeTracker().AddState(
            HdStDrawTargetTokens->drawTargetSet);
}

void
Hdx_UnitTestDelegate::SetRefineLevel(int level)
{
    _refineLevel = level;
    TF_FOR_ALL (it, _meshes) {
        GetRenderIndex().GetChangeTracker().MarkRprimDirty(
            it->first, HdChangeTracker::DirtyDisplayStyle);
    }
    TF_FOR_ALL (it, _refineLevels) {
        it->second = level;
    }
}

void
Hdx_UnitTestDelegate::SetCamera(GfMatrix4d const &viewMatrix,
                                GfMatrix4d const &projMatrix)
{
    SetCamera(_cameraId, viewMatrix, projMatrix);
}

void
Hdx_UnitTestDelegate::SetCamera(SdfPath const &cameraId,
                                GfMatrix4d const &viewMatrix,
                                GfMatrix4d const &projMatrix)
{
    _ValueCache &cache = _valueCacheMap[cameraId];
    cache[HdCameraTokens->windowPolicy] = VtValue(CameraUtilFit);
    cache[HdCameraTokens->worldToViewMatrix] = VtValue(viewMatrix);
    cache[HdCameraTokens->projectionMatrix] = VtValue(projMatrix);

    GetRenderIndex().GetChangeTracker().MarkSprimDirty(cameraId,
                                                       HdCamera::AllDirty);
}

void
Hdx_UnitTestDelegate::AddCamera(SdfPath const &id)
{
    // add a camera
    GetRenderIndex().InsertSprim(HdPrimTypeTokens->camera, this, id);
    _ValueCache &cache = _valueCacheMap[id];
    cache[HdCameraTokens->windowPolicy] = VtValue(CameraUtilFit);
    cache[HdCameraTokens->worldToViewMatrix] = VtValue(GfMatrix4d(1.0));
    cache[HdCameraTokens->projectionMatrix] = VtValue(GfMatrix4d(1.0));
}

void
Hdx_UnitTestDelegate::AddLight(SdfPath const &id, GlfSimpleLight const &light)
{
    // add light
    GetRenderIndex().InsertSprim(HdPrimTypeTokens->simpleLight, this, id);
    _ValueCache &cache = _valueCacheMap[id];

    HdxShadowParams shadowParams;
    shadowParams.enabled = light.HasShadow();
    shadowParams.resolution = 512;
    shadowParams.shadowMatrix
        = HdxShadowMatrixComputationSharedPtr(new ShadowMatrix(light));
    shadowParams.bias = -0.001;
    shadowParams.blur = 0.1;

    cache[HdLightTokens->params] = light;
    cache[HdLightTokens->shadowParams] = shadowParams;
    cache[HdLightTokens->shadowCollection]
        = HdRprimCollection(HdTokens->geometry, 
                HdReprSelector(HdReprTokens->refined));
}

void
Hdx_UnitTestDelegate::SetLight(SdfPath const &id, TfToken const &key,
                               VtValue value)
{
    _ValueCache &cache = _valueCacheMap[id];
    cache[key] = value;
    if (key == HdLightTokens->params) {
        // update shadow matrix too
        GlfSimpleLight light = value.Get<GlfSimpleLight>();
        HdxShadowParams shadowParams
            = cache[HdLightTokens->shadowParams].Get<HdxShadowParams>();
        shadowParams.shadowMatrix
            = HdxShadowMatrixComputationSharedPtr(new ShadowMatrix(light));

        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            id, HdLight::DirtyParams|HdLight::DirtyShadowParams);
        cache[HdLightTokens->shadowParams] = shadowParams;
    } else if (key == HdLightTokens->transform) {
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            id, HdLight::DirtyTransform);
    } else if (key == HdLightTokens->shadowCollection) {
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            id, HdLight::DirtyCollection);
    }
}

void
Hdx_UnitTestDelegate::AddDrawTarget(SdfPath const &id)
{
    GetRenderIndex().InsertSprim(HdPrimTypeTokens->drawTarget, this, id);
    _ValueCache &cache = _valueCacheMap[id];

    HdStDrawTargetAttachmentDescArray attachments;
    attachments.AddAttachment("color",
                              HdFormatUNorm8Vec4,
                              VtValue(GfVec4f(1,1,0,1)),
                              HdWrapRepeat,
                              HdWrapRepeat,
                              HdMinFilterLinear,
                              HdMagFilterLinear);

    cache[HdStDrawTargetTokens->enable]          = VtValue(true);
    cache[HdStDrawTargetTokens->camera]          = VtValue(SdfPath());
    cache[HdStDrawTargetTokens->resolution]      = VtValue(GfVec2i(256, 256));
    cache[HdStDrawTargetTokens->attachments]     = VtValue(attachments);
    cache[HdStDrawTargetTokens->depthClearValue] = VtValue(1.0f);
    cache[HdStDrawTargetTokens->collection]      =
        VtValue(HdRprimCollection(HdTokens->geometry, 
            HdReprSelector(HdReprTokens->hull)));

    GetRenderIndex().InsertBprim(HdPrimTypeTokens->texture, this, id);
    _drawTargets[id] = _DrawTarget();

    GetRenderIndex().GetChangeTracker().MarkStateDirty(
        HdStDrawTargetTokens->drawTargetSet);
}

void
Hdx_UnitTestDelegate::SetDrawTarget(SdfPath const &id, TfToken const &key,
                                    VtValue value)
{
    _ValueCache &cache = _valueCacheMap[id];
    cache[key] = value;
    if (key == HdStDrawTargetTokens->enable) {
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            id, HdStDrawTarget::DirtyDTEnable);
    } else if (key == HdStDrawTargetTokens->camera) {
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            id, HdStDrawTarget::DirtyDTCamera);
    } else if (key == HdStDrawTargetTokens->resolution) {
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            id, HdStDrawTarget::DirtyDTResolution);
    } else if (key == HdStDrawTargetTokens->attachments) {
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            id, HdStDrawTarget::DirtyDTAttachment);
    } else if (key == HdStDrawTargetTokens->depthClearValue) {
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            id, HdStDrawTarget::DirtyDTDepthClearValue);
    } else if (key == HdStDrawTargetTokens->collection) {
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            id, HdStDrawTarget::DirtyDTCollection);
    }
}

void
Hdx_UnitTestDelegate::AddRenderTask(SdfPath const &id)
{
    GetRenderIndex().InsertTask<HdxRenderTask>(this, id);
    _ValueCache &cache = _valueCacheMap[id];
    cache[HdTokens->collection]
        = HdRprimCollection(HdTokens->geometry, 
            HdReprSelector(HdReprTokens->smoothHull));
}

void
Hdx_UnitTestDelegate::AddRenderSetupTask(SdfPath const &id)
{
    GetRenderIndex().InsertTask<HdxRenderSetupTask>(this, id);
    _ValueCache &cache = _valueCacheMap[id];
    HdxRenderTaskParams params;
    params.camera = _cameraId;
    params.viewport = GfVec4f(0, 0, 512, 512);
    cache[HdTokens->params] = VtValue(params);
}

void
Hdx_UnitTestDelegate::AddSimpleLightTask(SdfPath const &id)
{
    GetRenderIndex().InsertTask<HdxSimpleLightTask>(this, id);
    _ValueCache &cache = _valueCacheMap[id];
    HdxSimpleLightTaskParams params;
    params.cameraPath = _cameraId;
    params.viewport = GfVec4f(0,0,512,512);
    params.enableShadows = true;
    
    cache[HdTokens->params] = VtValue(params);

}

void
Hdx_UnitTestDelegate::AddShadowTask(SdfPath const &id)
{
    GetRenderIndex().InsertTask<HdxShadowTask>(this, id);
    _ValueCache &cache = _valueCacheMap[id];
    HdxShadowTaskParams params;
    params.camera = _cameraId;
    params.viewport = GfVec4f(0,0,512,512);
    cache[HdTokens->params] = VtValue(params);
}

void
Hdx_UnitTestDelegate::AddSelectionTask(SdfPath const &id)
{
    GetRenderIndex().InsertTask<HdxSelectionTask>(this, id);
}

void
Hdx_UnitTestDelegate::AddDrawTargetTask(SdfPath const &id)
{
    GetRenderIndex().InsertTask<HdxDrawTargetTask>(this, id);
    _ValueCache &cache = _valueCacheMap[id];

    HdxDrawTargetTaskParams params;
    params.enableLighting = true;
    cache[HdTokens->params] = params;
}

void
Hdx_UnitTestDelegate::AddDrawTargetResolveTask(SdfPath const &id)
{
    GetRenderIndex().InsertTask<HdxDrawTargetResolveTask>(this, id);
}

void
Hdx_UnitTestDelegate::SetTaskParam(
    SdfPath const &id, TfToken const &name, VtValue val)
{
    _ValueCache &cache = _valueCacheMap[id];
    cache[name] = val;

    if (name == HdTokens->collection) {
        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            id, HdChangeTracker::DirtyCollection);
    } else if (name == HdTokens->params) {
        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            id, HdChangeTracker::DirtyParams);
    }
}

VtValue
Hdx_UnitTestDelegate::GetTaskParam(SdfPath const &id, TfToken const &name)
{
    return _valueCacheMap[id][name];
}

void
Hdx_UnitTestDelegate::AddInstancer(SdfPath const &id,
                                   SdfPath const &parentId,
                                   GfMatrix4f const &rootTransform)
{
    HD_TRACE_FUNCTION();

    HdRenderIndex& index = GetRenderIndex();
    // add instancer
    index.InsertInstancer(this, id, parentId);
    _instancers[id] = _Instancer();
    _instancers[id].rootTransform = rootTransform;

    if (!parentId.IsEmpty()) {
        _instancers[parentId].prototypes.push_back(id);
    }
}

void
Hdx_UnitTestDelegate::SetInstancerProperties(SdfPath const &id,
                                             VtIntArray const &prototypeIndex,
                                             VtVec3fArray const &scale,
                                             VtVec4fArray const &rotate,
                                             VtVec3fArray const &translate)
{
    HD_TRACE_FUNCTION();

    if (!TF_VERIFY(prototypeIndex.size() == scale.size())  ||
        !TF_VERIFY(prototypeIndex.size() == rotate.size()) || 
        !TF_VERIFY(prototypeIndex.size() == translate.size())) {
        return;
    }

    _instancers[id].scale = scale;
    _instancers[id].rotate = rotate;
    _instancers[id].translate = translate;
    _instancers[id].prototypeIndices = prototypeIndex;
}

//------------------------------------------------------------------------------
//                                  PRIMS
//------------------------------------------------------------------------------
void
Hdx_UnitTestDelegate::AddMesh(SdfPath const &id,
                             GfMatrix4d const &transform,
                             VtVec3fArray const &points,
                             VtIntArray const &numVerts,
                             VtIntArray const &verts,
                             bool guide,
                             SdfPath const &instancerId,
                             TfToken const &scheme,
                             TfToken const &orientation,
                             bool doubleSided)
{
    HdRenderIndex& index = GetRenderIndex();
    index.InsertRprim(HdPrimTypeTokens->mesh, this, id, instancerId);

    _meshes[id] = _Mesh(scheme, orientation, transform,
                        points, numVerts, verts, PxOsdSubdivTags(),
                        /*color=*/VtValue(GfVec3f(1, 1, 0)),
                        /*colorInterpolation=*/HdInterpolationConstant,
                        /*opacity=*/VtValue(1.0f),
                        HdInterpolationConstant,
                        guide, doubleSided);
    if (!instancerId.IsEmpty()) {
        _instancers[instancerId].prototypes.push_back(id);
    }
}

void
Hdx_UnitTestDelegate::AddMesh(SdfPath const &id,
                             GfMatrix4d const &transform,
                             VtVec3fArray const &points,
                             VtIntArray const &numVerts,
                             VtIntArray const &verts,
                             PxOsdSubdivTags const &subdivTags,
                             VtValue const &color,
                             HdInterpolation colorInterpolation,
                             VtValue const &opacity,
                             HdInterpolation opacityInterpolation,
                             bool guide,
                             SdfPath const &instancerId,
                             TfToken const &scheme,
                             TfToken const &orientation,
                             bool doubleSided)
{
    HdRenderIndex& index = GetRenderIndex();
    index.InsertRprim(HdPrimTypeTokens->mesh, this, id, instancerId);

    _meshes[id] = _Mesh(scheme, orientation, transform,
                        points, numVerts, verts, subdivTags,
                        color, colorInterpolation, opacity,
                        opacityInterpolation, guide, doubleSided);
    if (!instancerId.IsEmpty()) {
        _instancers[instancerId].prototypes.push_back(id);
    }
}

void
Hdx_UnitTestDelegate::AddCube(SdfPath const &id, GfMatrix4d const &transform, 
                              bool guide, SdfPath const &instancerId, 
                              TfToken const &scheme, VtValue const &color,
                              HdInterpolation colorInterpolation,
                              VtValue const &opacity,
                              HdInterpolation opacityInterpolation)
{
    GfVec3f points[] = {
        GfVec3f( 1.0f, 1.0f, 1.0f ),
        GfVec3f(-1.0f, 1.0f, 1.0f ),
        GfVec3f(-1.0f,-1.0f, 1.0f ),
        GfVec3f( 1.0f,-1.0f, 1.0f ),
        GfVec3f(-1.0f,-1.0f,-1.0f ),
        GfVec3f(-1.0f, 1.0f,-1.0f ),
        GfVec3f( 1.0f, 1.0f,-1.0f ),
        GfVec3f( 1.0f,-1.0f,-1.0f ),
    };

    if (scheme == PxOsdOpenSubdivTokens->loop) {
        int numVerts[] = { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
        int verts[] = {
            0, 1, 2, 0, 2, 3,
            4, 5, 6, 4, 6, 7,
            0, 6, 5, 0, 5, 1,
            4, 7, 3, 4, 3, 2,
            0, 3, 7, 0, 7, 6,
            4, 2, 1, 4, 1, 5,
        };
        AddMesh(
            id,
            transform,
            _BuildArray(points, sizeof(points)/sizeof(points[0])),
            _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])),
            _BuildArray(verts, sizeof(verts)/sizeof(verts[0])),
            PxOsdSubdivTags(),
            color,
            colorInterpolation,
            opacity,
            opacityInterpolation,
            guide,
            instancerId,
            scheme);
    } else {
        int numVerts[] = { 4, 4, 4, 4, 4, 4 };
        int verts[] = {
            0, 1, 2, 3,
            4, 5, 6, 7,
            0, 6, 5, 1,
            4, 7, 3, 2,
            0, 3, 7, 6,
            4, 2, 1, 5,
        };
        AddMesh(
            id,
            transform,
            _BuildArray(points, sizeof(points)/sizeof(points[0])),
            _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])),
            _BuildArray(verts, sizeof(verts)/sizeof(verts[0])),
            PxOsdSubdivTags(),
            color,
            colorInterpolation,
            opacity,
            opacityInterpolation,
            guide,
            instancerId,
            scheme);
    }
}

void
Hdx_UnitTestDelegate::AddGrid(SdfPath const &id,
                             GfMatrix4d const &transform,
                             bool guide,
                             SdfPath const &instancerId)
{
    VtVec3fArray points;
    VtIntArray numVerts;
    VtIntArray verts;
    _CreateGrid(10, 10, &points, &numVerts, &verts);

    AddMesh(id,
            transform,
            _BuildArray(&points[0], points.size()),
            _BuildArray(&numVerts[0], numVerts.size()),
            _BuildArray(&verts[0], verts.size()),
            PxOsdSubdivTags(),
            /*color=*/VtValue(GfVec3f(1,1,0)),
            /*colorInterpolation=*/HdInterpolationConstant,
            /*opacity=*/VtValue(1.0f),
            HdInterpolationConstant,
            false,
            instancerId);
}

void
Hdx_UnitTestDelegate::AddTet(SdfPath const &id, GfMatrix4d const &transform,

                             bool guide, SdfPath const &instancerId,
                             TfToken const &scheme)
{
    GfVec3f points[] = {
        GfVec3f(-1, -1, -1),
        GfVec3f(-1, -1, -1),
        GfVec3f(1, 1, -1),
        GfVec3f(1, -1, 1),
        GfVec3f(-1, 1, 1),
        GfVec3f(-0.3, -0.3, -0.3),
        GfVec3f(0.3, 0.3, -0.3),
        GfVec3f(0.3, -0.3, 0.3),
        GfVec3f(-0.3, 0.3, 0.3),
        GfVec3f(-0.2, -0.6, -0.6),
        GfVec3f(0.6, 0.2, -0.6),
        GfVec3f(0.6, -0.6, 0.2),
        GfVec3f(-0.6, -0.6, -0.2),
        GfVec3f(0.2, -0.6, 0.6),
        GfVec3f(-0.6, 0.2, 0.6),
        GfVec3f(-0.6, -0.2, -0.6),
        GfVec3f(-0.6, 0.6, 0.2),
        GfVec3f(0.2, 0.6, -0.6),
        GfVec3f(0.6, 0.6, -0.2),
        GfVec3f(-0.2, 0.6, 0.6),
        GfVec3f(0.6, -0.2, 0.6) };

    int numVerts[] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                       4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };
    int verts[] = { 1, 2, 10, 9, 9, 10, 6, 5, 2, 3, 11, 10,
                    10, 11, 7, 6, 3, 1, 9, 11, 11, 9, 5, 7,
                    1, 3, 13, 12, 12, 13, 7, 5, 3, 4, 14, 13,
                    13, 14, 8, 7, 4, 1, 12, 14, 14, 12, 5, 8,
                    1, 4, 16, 15, 15, 16, 8, 5, 4, 2, 17, 16,
                    16, 17, 6, 8, 2, 1, 15, 17, 17, 15, 5, 6,
                    2, 4, 19, 18, 18, 19, 8, 6, 4, 3, 20, 19,
                    19, 20, 7, 8, 3, 2, 18, 20, 20, 18, 6, 7 };

     AddMesh(
            id,
            transform,
            _BuildArray(points, sizeof(points)/sizeof(points[0])),
            _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])),
            _BuildArray(verts, sizeof(verts)/sizeof(verts[0])),
            PxOsdSubdivTags(),
            /*color=*/VtValue(GfVec3f(1,1,1)),
            /*colorInterpolation=*/HdInterpolationConstant,
            /*opacity=*/VtValue(1.0f),
            HdInterpolationConstant,
            guide,
            instancerId,
            scheme);
}

void
Hdx_UnitTestDelegate::SetRefineLevel(SdfPath const &id, int level)
{
    _refineLevels[id] = level;
    GetRenderIndex().GetChangeTracker().MarkRprimDirty(
        id, HdChangeTracker::DirtyDisplayStyle);
}

HdReprSelector
Hdx_UnitTestDelegate::GetReprSelector(SdfPath const &id)
{
    if (_meshes.find(id) != _meshes.end()) {
        return HdReprSelector(_meshes[id].reprName);
    }

    return HdReprSelector();
}

void
Hdx_UnitTestDelegate::SetReprName(SdfPath const &id, TfToken const &reprName)
{
   if (_meshes.find(id) != _meshes.end()) {
        _meshes[id].reprName = reprName;
        GetRenderIndex().GetChangeTracker().MarkRprimDirty(
            id, HdChangeTracker::DirtyRepr);
   }
}

GfRange3d
Hdx_UnitTestDelegate::GetExtent(SdfPath const & id)
{
    GfRange3d range;
    VtVec3fArray points;
    if(_meshes.find(id) != _meshes.end()) {
        points = _meshes[id].points; 
    }
    TF_FOR_ALL(it, points) {
        range.UnionWith(*it);
    }
    return range;
}

GfMatrix4d
Hdx_UnitTestDelegate::GetTransform(SdfPath const & id)
{
    if(_meshes.find(id) != _meshes.end()) {
        return _meshes[id].transform;
    }
    return GfMatrix4d(1);
}

bool
Hdx_UnitTestDelegate::GetVisible(SdfPath const& id)
{
    return true;
}

HdMeshTopology
Hdx_UnitTestDelegate::GetMeshTopology(SdfPath const& id)
{
    HdMeshTopology topology;
    const _Mesh &mesh = _meshes[id];

    return HdMeshTopology(PxOsdOpenSubdivTokens->catmark,
                          HdTokens->rightHanded,
                          mesh.numVerts,
                          mesh.verts);
}

VtValue
Hdx_UnitTestDelegate::Get(SdfPath const& id, TfToken const& key)
{
    // tasks
    _ValueCache *vcache = TfMapLookupPtr(_valueCacheMap, id);
    VtValue ret;
    if (vcache && TfMapLookup(*vcache, key, &ret)) {
        return ret;
    }

    // prims
    if (key == HdTokens->points) {
        if(_meshes.find(id) != _meshes.end()) {
            return VtValue(_meshes[id].points);
        }
    } else if (key == HdTokens->displayColor) {
        if(_meshes.find(id) != _meshes.end()) {
            return VtValue(_meshes[id].color);
        }
    } else if (key == HdTokens->displayOpacity) {
        if(_meshes.find(id) != _meshes.end()) {
            return VtValue(_meshes[id].opacity);
        }
    } else if (key == _tokens->scale) {
        if (_instancers.find(id) != _instancers.end()) {
            return VtValue(_instancers[id].scale);
        }
    } else if (key == _tokens->rotate) {
        if (_instancers.find(id) != _instancers.end()) {
            return VtValue(_instancers[id].rotate);
        }
    } else if (key == _tokens->translate) {
        if (_instancers.find(id) != _instancers.end()) {
            return VtValue(_instancers[id].translate);
        }
    }
    return VtValue();
}

/*virtual*/
VtIntArray
Hdx_UnitTestDelegate::GetInstanceIndices(SdfPath const& instancerId,
                                        SdfPath const& prototypeId)
{
    HD_TRACE_FUNCTION();
    VtIntArray indices(0);
    //
    // XXX: this is very naive implementation for unit test.
    //
    //   transpose prototypeIndices/instances to instanceIndices/prototype
    if (_Instancer *instancer = TfMapLookupPtr(_instancers, instancerId)) {
        size_t prototypeIndex = 0;
        for (; prototypeIndex < instancer->prototypes.size(); ++prototypeIndex) {
            if (instancer->prototypes[prototypeIndex] == prototypeId) break;
        }
        if (prototypeIndex == instancer->prototypes.size()) return indices;

        // XXX use const_ptr
        for (size_t i = 0; i < instancer->prototypeIndices.size(); ++i) {
            if (static_cast<size_t>(instancer->prototypeIndices[i]) == prototypeIndex) {
                indices.push_back(i);
            }
        }
    }
    return indices;
}

/*virtual*/
GfMatrix4d
Hdx_UnitTestDelegate::GetInstancerTransform(SdfPath const& instancerId)
{
    HD_TRACE_FUNCTION();
    if (_Instancer *instancer = TfMapLookupPtr(_instancers, instancerId)) {
        return GfMatrix4d(instancer->rootTransform);
    }
    return GfMatrix4d(1);
}

/*virtual*/
HdDisplayStyle
Hdx_UnitTestDelegate::GetDisplayStyle(SdfPath const& id)
{
    if (_refineLevels.find(id) != _refineLevels.end()) {
        return HdDisplayStyle(_refineLevels[id]);
    }
    return HdDisplayStyle(_refineLevel);
}

HdPrimvarDescriptorVector
Hdx_UnitTestDelegate::GetPrimvarDescriptors(SdfPath const& id, 
                                            HdInterpolation interpolation)
{       
    HdPrimvarDescriptorVector primvars;
    if (interpolation == HdInterpolationVertex) {
        primvars.emplace_back(HdTokens->points, interpolation,
                              HdPrimvarRoleTokens->point);
    }                       
    if(_meshes.find(id) != _meshes.end()) {
        if (_meshes[id].colorInterpolation == interpolation) {
            primvars.emplace_back(HdTokens->displayColor, interpolation,
                                  HdPrimvarRoleTokens->color);
        }
        if (_meshes[id].opacityInterpolation == interpolation) {
            primvars.emplace_back(HdTokens->displayOpacity, interpolation);
        }
    }
    if (interpolation == HdInterpolationInstance &&
        _instancers.find(id) != _instancers.end()) {
        primvars.emplace_back(_tokens->scale, interpolation);
        primvars.emplace_back(_tokens->rotate, interpolation);
        primvars.emplace_back(_tokens->translate, interpolation);
    }
    return primvars;
}

void
Hdx_UnitTestDelegate::AddMaterial(SdfPath const &id,
                                std::string const &sourceSurface,
                                std::string const &sourceDisplacement,
                                HdMaterialParamVector const &params)
{
    HdRenderIndex& index = GetRenderIndex();
    index.InsertSprim(HdPrimTypeTokens->material, this, id);
    _materials[id] = _Material(sourceSurface, sourceDisplacement, params);
}

void
Hdx_UnitTestDelegate::BindMaterial(SdfPath const &rprimId,
                                 SdfPath const &materialId)
{
    _materialBindings[rprimId] = materialId;
}

/*virtual*/ 
SdfPath 
Hdx_UnitTestDelegate::GetMaterialId(SdfPath const &rprimId)
{
    SdfPath materialId;
    TfMapLookup(_materialBindings, rprimId, &materialId);
    return materialId;
}

/*virtual*/
std::string
Hdx_UnitTestDelegate::GetSurfaceShaderSource(SdfPath const &materialId)
{
    if (_Material *material = TfMapLookupPtr(_materials, materialId)) {
        return material->sourceSurface;
    } else {
        return TfToken();
    }
}

/*virtual*/
std::string
Hdx_UnitTestDelegate::GetDisplacementShaderSource(SdfPath const &materialId)
{
    if (_Material *material = TfMapLookupPtr(_materials, materialId)) {
        return material->sourceDisplacement;
    } else {
        return TfToken();
    }
}

/*virtual*/
HdMaterialParamVector
Hdx_UnitTestDelegate::GetMaterialParams(SdfPath const &materialId)
{
    if (_Material *material = TfMapLookupPtr(_materials, materialId)) {
        return material->params;
    }
    return HdMaterialParamVector();
}

/*virtual*/
VtValue
Hdx_UnitTestDelegate::GetMaterialParamValue(SdfPath const &materialId, 
                                            TfToken const &paramName)
{
    if (_Material *material = TfMapLookupPtr(_materials, materialId)) {
        TF_FOR_ALL(paramIt, material->params) {
            if (paramIt->GetName() == paramName)
                return paramIt->GetFallbackValue();
        }
    }
    return VtValue();
}

HdTextureResourceSharedPtr
Hdx_UnitTestDelegate::GetTextureResource(SdfPath const& textureId)
{
    if (_drawTargets.find(textureId) != _drawTargets.end()) {
        HdStDrawTarget const *drawTarget = static_cast<HdStDrawTarget const *> (
                        GetRenderIndex().GetSprim(HdPrimTypeTokens->drawTarget,
                                                  textureId));

        if (drawTarget != nullptr) {
            HdTextureResourceSharedPtr texResource(
                new DrawTargetTextureResource(
                    drawTarget->GetGlfDrawTarget()));
            return texResource;
        }
    }
    return HdTextureResourceSharedPtr();
}

HdTextureResource::ID
Hdx_UnitTestDelegate::GetTextureResourceID(SdfPath const& textureId)
{
    return SdfPath::Hash()(textureId);
}

PXR_NAMESPACE_CLOSE_SCOPE

