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

#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/glf/simpleLight.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/tf/staticTokens.h"

#define HDX_UNIT_TEST_TOKENS                             \
    (geometryAndGuides)

TF_DECLARE_PUBLIC_TOKENS(Hdx_UnitTestTokens, HDXLIB_API, HDX_UNIT_TEST_TOKENS);

class Hdx_UnitTestDelegate : public HdSceneDelegate
{
public:
    Hdx_UnitTestDelegate();
    Hdx_UnitTestDelegate(HdRenderIndexSharedPtr const &renderIndex);

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

    /// Shader
    void AddSurfaceShader(SdfPath const &id,
                    std::string const &source,
                    HdShaderParamVector const &params);
    void BindSurfaceShader(SdfPath const &rprimId, SdfPath const &shaderId);

    // prims
    void AddGrid(SdfPath const &id, GfMatrix4d const &transform,
                 bool guide=false, SdfPath const &instancerId=SdfPath());
    void AddCube(SdfPath const &id, GfMatrix4d const &transform,
                 bool guide=false, SdfPath const &instancerId=SdfPath());
    void AddTet(SdfPath const &id, GfMatrix4d const &transform,
                 bool guide=false, SdfPath const &instancerId=SdfPath());
    void SetRefineLevel(SdfPath const &id, int level);

    // delegate methods
    virtual GfRange3d GetExtent(SdfPath const & id);
    virtual GfMatrix4d GetTransform(SdfPath const & id);
    virtual bool GetVisible(SdfPath const& id);
    virtual HdMeshTopology GetMeshTopology(SdfPath const& id);
    virtual VtValue Get(SdfPath const& id, TfToken const& key);
    virtual TfTokenVector GetPrimVarVertexNames(SdfPath const& id);
    virtual TfTokenVector GetPrimVarConstantNames(SdfPath const& id);
    virtual TfTokenVector GetPrimVarInstanceNames(SdfPath const &id);
    virtual VtIntArray GetInstanceIndices(SdfPath const& instancerId,
                                          SdfPath const& prototypeId);

    virtual GfMatrix4d GetInstancerTransform(SdfPath const& instancerId,
                                             SdfPath const& prototypeId);
    virtual int GetRefineLevel(SdfPath const& id);
    virtual bool IsInCollection(SdfPath const& id, TfToken const& collectionName);

    virtual std::string GetSurfaceShaderSource(SdfPath const &shaderId);
    virtual TfTokenVector GetSurfaceShaderParamNames(SdfPath const &shaderId);
    virtual HdShaderParamVector GetSurfaceShaderParams(SdfPath const &shaderId);
    virtual VtValue GetSurfaceShaderParamValue(SdfPath const &shaderId,
                                               TfToken const &paramName);
    virtual HdTextureResource::ID GetTextureResourceID(SdfPath const& textureId);
    virtual HdTextureResourceSharedPtr GetTextureResource(SdfPath const& textureId);

private:
    struct _Mesh {
        _Mesh() { }
        _Mesh(GfMatrix4d const &transform,
              VtVec3fArray const &points,
              VtIntArray const &numVerts,
              VtIntArray const &verts,
              bool guide) :
            transform(transform),
            points(points), numVerts(numVerts), verts(verts), guide(guide) {
            color = GfVec4f(1, 1, 0, 1);
        }

        GfMatrix4d transform;
        VtVec3fArray points;
        VtIntArray numVerts;
        VtIntArray verts;
        GfVec4f color;
        bool guide;
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
    struct _SurfaceShader {
        _SurfaceShader() { }
        _SurfaceShader(std::string const &src, HdShaderParamVector const &pms)
            : source(src)
            , params(pms) {
        }

        std::string source;
        HdShaderParamVector params;
    };
    struct _DrawTarget {
    };
    std::map<SdfPath, _Mesh> _meshes;
    std::map<SdfPath, _Instancer> _instancers;
    std::map<SdfPath, _SurfaceShader> _surfaceShaders;
    std::map<SdfPath, int> _refineLevels;
    std::map<SdfPath, _DrawTarget> _drawTargets;
    int _refineLevel;

    typedef std::map<SdfPath, SdfPath> SdfPathMap;
    SdfPathMap _surfaceShaderBindings;

    typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _ValueCache;
    typedef TfHashMap<SdfPath, _ValueCache, SdfPath::Hash> _ValueCacheMap;
    _ValueCacheMap _valueCacheMap;

    SdfPath _cameraId;
};

#endif  // HDX_UNIT_TEST_DELEGATE
