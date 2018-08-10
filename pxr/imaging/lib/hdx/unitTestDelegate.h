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
#ifndef HDX_UNIT_TEST_DELEGATE
#define HDX_UNIT_TEST_DELEGATE

#include "pxr/pxr.h"
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
#include "pxr/base/tf/staticTokens.h"

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
    Hdx_UnitTestDelegate(HdRenderIndex *renderIndex);

    void SetRefineLevel(int level);

    // camera
    void SetCamera(GfMatrix4d const &viewMatrix, GfMatrix4d const &projMatrix);
    void SetCamera(SdfPath const &id, GfMatrix4d const &viewMatrix, GfMatrix4d const &projMatrix);
    void AddCamera(SdfPath const &id);

    // light
    void AddLight(SdfPath const &id, GlfSimpleLight const &light);
    void SetLight(SdfPath const &id, TfToken const &key, VtValue value);

    // draw target
    void AddDrawTarget(SdfPath const &id);
    void SetDrawTarget(SdfPath const &id, TfToken const &key, VtValue value);

    // tasks
    void AddRenderTask(SdfPath const &id);
    void AddRenderSetupTask(SdfPath const &id);
    void AddSimpleLightTask(SdfPath const &id);
    void AddShadowTask(SdfPath const &id);
    void AddSelectionTask(SdfPath const &id);
    void AddDrawTargetTask(SdfPath const &id);
    void AddDrawTargetResolveTask(SdfPath const &id);

    void SetTaskParam(SdfPath const &id, TfToken const &name, VtValue val);
    VtValue GetTaskParam(SdfPath const &id, TfToken const &name);

    /// Instancer
    void AddInstancer(SdfPath const &id,
                      SdfPath const &parentId=SdfPath(),
                      GfMatrix4f const &rootTransform=GfMatrix4f(1));

    void SetInstancerProperties(SdfPath const &id,
                                VtIntArray const &prototypeIndex,
                                VtVec3fArray const &scale,
                                VtVec4fArray const &rotate,
                                VtVec3fArray const &translate);

    /// Material
    void AddMaterial(SdfPath const &id,
                   std::string const &sourceSurface,
                   std::string const &sourceDisplacement,
                   HdMaterialParamVector const &params);
    void BindMaterial(SdfPath const &rprimId, SdfPath const &materialId);

    // prims    
    void AddMesh(SdfPath const &id,
                 GfMatrix4d const &transform,
                 VtVec3fArray const &points,
                 VtIntArray const &numVerts,
                 VtIntArray const &verts,
                 bool guide=false,
                 SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmark,
                 TfToken const &orientation=HdTokens->rightHanded,
                 bool doubleSided=false);
    
    void AddMesh(SdfPath const &id,
                 GfMatrix4d const &transform,
                 VtVec3fArray const &points,
                 VtIntArray const &numVerts,
                 VtIntArray const &verts,
                 PxOsdSubdivTags const &subdivTags,
                 VtValue const &color,
                 HdInterpolation colorInterpolation,
                 bool guide=false,
                 SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmark,
                 TfToken const &orientation=HdTokens->rightHanded,
                 bool doubleSided=false);

    void AddCube(SdfPath const &id, GfMatrix4d const &transform, bool guide=false,
                 SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmark,
                 VtValue const &color = VtValue(GfVec4f(1,1,1,1)),
                 HdInterpolation colorInterpolation = HdInterpolationConstant);

    void AddGrid(SdfPath const &id, GfMatrix4d const &transform,
                 bool guide=false, SdfPath const &instancerId=SdfPath());

    void AddTet(SdfPath const &id, GfMatrix4d const &transform,
                 bool guide=false, SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmark);

    void SetRefineLevel(SdfPath const &id, int level);

    void SetReprName(SdfPath const &id, TfToken const &reprName);

    // delegate methods
    virtual GfRange3d GetExtent(SdfPath const & id);
    virtual GfMatrix4d GetTransform(SdfPath const & id);
    virtual bool GetVisible(SdfPath const& id);
    virtual HdMeshTopology GetMeshTopology(SdfPath const& id);
    virtual VtValue Get(SdfPath const& id, TfToken const& key);
    virtual HdPrimvarDescriptorVector
        GetPrimvarDescriptors(SdfPath const& id, 
                              HdInterpolation interpolation) override;
    virtual VtIntArray GetInstanceIndices(SdfPath const& instancerId,
                                          SdfPath const& prototypeId);

    virtual GfMatrix4d GetInstancerTransform(SdfPath const& instancerId,
                                             SdfPath const& prototypeId);
    virtual HdDisplayStyle GetDisplayStyle(SdfPath const& id) override;
    virtual TfToken GetReprName(SdfPath const &id);


    virtual SdfPath GetMaterialId(SdfPath const &rprimId);
    virtual std::string GetSurfaceShaderSource(SdfPath const &shaderId);
    virtual std::string GetDisplacementShaderSource(SdfPath const &shaderId);
    virtual HdMaterialParamVector GetMaterialParams(SdfPath const &shaderId);
    virtual VtValue GetMaterialParamValue(SdfPath const &shaderId,
                                          TfToken const &paramName);
    virtual HdTextureResource::ID GetTextureResourceID(SdfPath const& textureId);
    virtual HdTextureResourceSharedPtr GetTextureResource(SdfPath const& textureId);

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
              bool guide,
              bool doubleSided) :
            scheme(scheme), orientation(orientation),
            transform(transform),
            points(points), numVerts(numVerts), verts(verts),
            subdivTags(subdivTags), color(color),
            colorInterpolation(colorInterpolation), guide(guide),
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
    struct _Material {
        _Material() { }
        _Material(std::string const &srcSurface,
                  std::string const &srcDisplacement, 
                  HdMaterialParamVector const &pms)
            : sourceSurface(srcSurface)
            , sourceDisplacement(srcDisplacement)
            , params(pms) {
        }

        std::string sourceSurface;
        std::string sourceDisplacement;
        HdMaterialParamVector params;
    };
    struct _DrawTarget {
    };
    std::map<SdfPath, _Mesh> _meshes;
    std::map<SdfPath, _Instancer> _instancers;
    std::map<SdfPath, _Material> _materials;
    std::map<SdfPath, int> _refineLevels;
    std::map<SdfPath, _DrawTarget> _drawTargets;
    int _refineLevel;

    typedef std::map<SdfPath, SdfPath> SdfPathMap;
    SdfPathMap _materialBindings;

    typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _ValueCache;
    typedef TfHashMap<SdfPath, _ValueCache, SdfPath::Hash> _ValueCacheMap;
    _ValueCacheMap _valueCacheMap;

    SdfPath _cameraId;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDX_UNIT_TEST_DELEGATE
