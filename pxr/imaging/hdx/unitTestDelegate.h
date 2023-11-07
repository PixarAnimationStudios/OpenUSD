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
#ifndef PXR_IMAGING_HDX_UNIT_TEST_DELEGATE_H
#define PXR_IMAGING_HDX_UNIT_TEST_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/vt/array.h"

PXR_NAMESPACE_OPEN_SCOPE

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

class Hdx_UnitTestDelegate : public HdSceneDelegate
{
public:
    HDX_API
    Hdx_UnitTestDelegate(HdRenderIndex *renderIndex, 
        SdfPath const &delegateId = SdfPath::AbsoluteRootPath());

    HDX_API
    void SetRefineLevel(int level);

    // camera
    HDX_API
    void SetCamera(GfMatrix4d const &viewMatrix, GfMatrix4d const &projMatrix);
    HDX_API
    void SetCamera(
        SdfPath const &id, 
        GfMatrix4d const &viewMatrix, 
        GfMatrix4d const &projMatrix);

    HDX_API
    void AddCamera(SdfPath const &id);
    HDX_API
    void UpdateCamera(SdfPath const &id, TfToken const &key, VtValue value);

    // light
    HDX_API
    void AddLight(SdfPath const &id, GlfSimpleLight const &light);
    HDX_API
    void SetLight(SdfPath const &id, TfToken const &key, VtValue value);
    HDX_API
    void RemoveLight(SdfPath const &id);

    // transform
    HDX_API
    void UpdateTransform(SdfPath const& id, GfMatrix4f const& mat);

    // render buffer
    HDX_API
    void AddRenderBuffer(SdfPath const &id, 
                         HdRenderBufferDescriptor const &desc);
    HDX_API
    void UpdateRenderBuffer(SdfPath const &id, 
                            HdRenderBufferDescriptor const &desc);

    // draw target
    HDX_API
    void AddDrawTarget(SdfPath const &id);
    HDX_API
    void SetDrawTarget(SdfPath const &id, TfToken const &key, VtValue value);

    // tasks
    HDX_API
    void AddRenderTask(SdfPath const &id);
    HDX_API
    void AddRenderSetupTask(SdfPath const &id);
    HDX_API
    void AddSimpleLightTask(SdfPath const &id);
    HDX_API
    void AddShadowTask(SdfPath const &id);
    HDX_API
    void AddSelectionTask(SdfPath const &id);
    HDX_API
    void AddDrawTargetTask(SdfPath const &id);
    HDX_API
    void AddPickTask(SdfPath const &id);

    HDX_API
    void SetTaskParam(SdfPath const &id, TfToken const &name, VtValue val);
    HDX_API
    VtValue GetTaskParam(SdfPath const &id, TfToken const &name);
    HDX_API
    HdRenderBufferDescriptor GetRenderBufferDescriptor(SdfPath const &id) override;

    /// Instancer
    HDX_API
    void AddInstancer(SdfPath const &id,
                      SdfPath const &parentId=SdfPath(),
                      GfMatrix4f const &rootTransform=GfMatrix4f(1));

    HDX_API
    void SetInstancerProperties(SdfPath const &id,
                                VtIntArray const &prototypeIndex,
                                VtVec3fArray const &scale,
                                VtVec4fArray const &rotate,
                                VtVec3fArray const &translate);

    /// Material
    HDX_API
    void AddMaterialResource(SdfPath const &id,
                             VtValue materialResource);

    HDX_API
    void BindMaterial(SdfPath const &rprimId, SdfPath const &materialId);

    // prims    
    HDX_API
    void AddMesh(SdfPath const &id,
                 GfMatrix4d const &transform,
                 VtVec3fArray const &points,
                 VtIntArray const &numVerts,
                 VtIntArray const &verts,
                 bool guide=false,
                 SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmullClark,
                 TfToken const &orientation=HdTokens->rightHanded,
                 bool doubleSided=false);
    
    HDX_API
    void AddMesh(SdfPath const &id,
                 GfMatrix4d const &transform,
                 VtVec3fArray const &points,
                 VtIntArray const &numVerts,
                 VtIntArray const &verts,
                 PxOsdSubdivTags const &subdivTags,
                 VtValue const &color,
                 HdInterpolation colorInterpolation,
                 VtValue const &opacity,
                 HdInterpolation opacityInterpolation,
                 bool guide=false,
                 SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmullClark,
                 TfToken const &orientation=HdTokens->rightHanded,
                 bool doubleSided=false);

    HDX_API
    void AddCube(SdfPath const &id, GfMatrix4d const &transform, 
                 bool guide=false,
                 SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmullClark,
                 VtValue const &color = VtValue(GfVec3f(1,1,1)),
                 HdInterpolation colorInterpolation = HdInterpolationConstant,
                 VtValue const &opacity = VtValue(1.0f),
                 HdInterpolation opacityInterpolation = HdInterpolationConstant);

    HDX_API
    void AddGrid(SdfPath const &id, GfMatrix4d const &transform,
                 bool guide=false, SdfPath const &instancerId=SdfPath());

    HDX_API
    void AddTet(SdfPath const &id, GfMatrix4d const &transform,
                 bool guide=false, SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmullClark);

    HDX_API
    void SetRefineLevel(SdfPath const &id, int level);

    HDX_API
    void SetReprName(SdfPath const &id, TfToken const &reprName);

    // delegate methods
    HDX_API
    GfRange3d GetExtent(SdfPath const & id) override;
    HDX_API
    GfMatrix4d GetTransform(SdfPath const & id) override;
    HDX_API
    bool GetVisible(SdfPath const& id) override;
    HDX_API
    HdMeshTopology GetMeshTopology(SdfPath const& id) override;
    HDX_API
    VtValue Get(SdfPath const& id, TfToken const& key) override;
    HDX_API
    HdPrimvarDescriptorVector GetPrimvarDescriptors(
        SdfPath const& id, 
        HdInterpolation interpolation) override;
    HDX_API
    VtIntArray GetInstanceIndices(
        SdfPath const& instancerId,
        SdfPath const& prototypeId) override;
    HDX_API
    SdfPathVector GetInstancerPrototypes(SdfPath const& instancerId) override;
    HDX_API
    GfMatrix4d GetInstancerTransform(SdfPath const& instancerId) override;
    HDX_API
    HdDisplayStyle GetDisplayStyle(SdfPath const& id) override;
    HDX_API
    HdReprSelector GetReprSelector(SdfPath const &id) override;
    HDX_API
    SdfPath GetMaterialId(SdfPath const &rprimId) override;
    HDX_API
    VtValue GetMaterialResource(SdfPath const &materialId) override;

    HDX_API
    SdfPath GetInstancerId(SdfPath const &primId) override;

    HDX_API
    VtValue GetCameraParamValue(
        SdfPath const &cameraId,
        TfToken const &paramName) override;

    HDX_API
    TfTokenVector GetTaskRenderTags(SdfPath const& taskId) override;

    HDX_API
    bool WriteRenderBufferToFile(SdfPath const &id,
                                 std::string const &filePath);

private:
    struct _Mesh {
        _Mesh() { }
        _Mesh(TfToken const &scheme,
              TfToken const &orientation,
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
              bool doubleSided) :
            scheme(scheme), orientation(orientation),
            transform(transform),
            points(points), numVerts(numVerts), verts(verts),
            subdivTags(subdivTags), color(color),
            colorInterpolation(colorInterpolation), opacity(opacity),
            opacityInterpolation(opacityInterpolation), guide(guide),
            doubleSided(doubleSided) { }

        TfToken scheme;
        TfToken orientation;
        GfMatrix4d transform;
        VtVec3fArray points;
        VtIntArray numVerts;
        VtIntArray verts;
        PxOsdSubdivTags subdivTags;
        VtValue color;
        HdInterpolation colorInterpolation;
        VtValue opacity;
        HdInterpolation opacityInterpolation;
        bool guide;
        bool doubleSided;
        TfToken reprName;
    };
    
    struct _Instancer {
        _Instancer() { }
        _Instancer(VtVec3fArray const &scale,
                   VtVec4fArray const &rotate,
                   VtVec3fArray const &translate,
                   GfMatrix4f const &rootTransform) :
            scale(scale), rotate(rotate), translate(translate),
            rootTransform(rootTransform) {
        }
        VtVec3fArray scale;
        VtVec4fArray rotate;
        VtVec3fArray translate;
        VtIntArray prototypeIndices;
        GfMatrix4f rootTransform;

        std::vector<SdfPath> prototypes;
    };
    struct _DrawTarget {
    };
    std::map<SdfPath, _Mesh> _meshes;
    std::map<SdfPath, _Instancer> _instancers;
    std::map<SdfPath, VtValue> _materials;
    std::map<SdfPath, int> _refineLevels;
    std::map<SdfPath, _DrawTarget> _drawTargets;
    std::map<SdfPath, GfMatrix4d> _cameraTransforms;
    int _refineLevel;

    using SdfPathMap = std::map<SdfPath, SdfPath>;
    SdfPathMap _materialBindings;
    SdfPathMap _instancerBindings;

    using _ValueCache = TfHashMap<TfToken, VtValue, TfToken::HashFunctor>;
    using _ValueCacheMap = TfHashMap<SdfPath, _ValueCache, SdfPath::Hash>;
    _ValueCacheMap _valueCacheMap;

    SdfPath _cameraId;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HDX_UNIT_TEST_DELEGATE_H
