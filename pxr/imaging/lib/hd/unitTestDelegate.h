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
#ifndef HD_UNIT_TEST_DELEGATE
#define HD_UNIT_TEST_DELEGATE

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/tf/staticTokens.h"

#define HD_UNIT_TEST_TOKENS                             \
    (geometryAndGuides)

TF_DECLARE_PUBLIC_TOKENS(Hd_UnitTestTokens, HDLIB_API, HD_UNIT_TEST_TOKENS);

/// \class Hd_UnitTestDelegate
///
/// A simple delegate class for unit test driver.
///
class Hd_UnitTestDelegate : public HdSceneDelegate {
public:
    HDLIB_API
    Hd_UnitTestDelegate();

    void SetUseInstancePrimVars(bool v) { _hasInstancePrimVars = v; }

    HDLIB_API
    void SetRefineLevel(int level);

    enum Interpolation { VERTEX, UNIFORM, CONSTANT, FACEVARYING, VARYING };

    // -----------------------------------------------------------------------

    HDLIB_API
    void AddMesh(SdfPath const& id);

    HDLIB_API
    void AddMesh(SdfPath const &id,
                 GfMatrix4f const &transform,
                 VtVec3fArray const &points,
                 VtIntArray const &numVerts,
                 VtIntArray const &verts,
                 bool guide=false,
                 SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmark,
                 TfToken const &orientation=HdTokens->rightHanded,
                 bool doubleSided=false);

    HDLIB_API
    void AddMesh(SdfPath const &id,
                 GfMatrix4f const &transform,
                 VtVec3fArray const &points,
                 VtIntArray const &numVerts,
                 VtIntArray const &verts,
                 PxOsdSubdivTags const &subdivTags,
                 VtValue const &color,
                 Interpolation colorInterpolation,
                 bool guide=false,
                 SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmark,
                 TfToken const &orientation=HdTokens->rightHanded,
                 bool doubleSided=false);

    /// Add a cube
    HDLIB_API
    void AddCube(SdfPath const &id, GfMatrix4f const &transform, bool guide=false,
                 SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmark);

    /// Add a grid with division x*y
    HDLIB_API
    void AddGrid(SdfPath const &id, int x, int y, GfMatrix4f const &transform,
                 bool rightHanded=true, bool doubleSided=false,
                 SdfPath const &instancerId=SdfPath());

    /// Add a grid with division x*y
    HDLIB_API
    void AddGridWithFaceColor(SdfPath const &id, int x, int y,
                              GfMatrix4f const &transform,
                              bool rightHanded=true, bool doubleSided=false,
                              SdfPath const &instancerId=SdfPath());

    /// Add a grid with division x*y
    HDLIB_API
    void AddGridWithVertexColor(SdfPath const &id, int x, int y,
                                GfMatrix4f const &transform,
                                bool rightHanded=true, bool doubleSided=false,
                                SdfPath const &instancerId=SdfPath());

    /// Add a grid with division x*y
    HDLIB_API
    void AddGridWithFaceVaryingColor(SdfPath const &id, int x, int y,
                                     GfMatrix4f const &transform,
                                     bool rightHanded=true, bool doubleSided=false,
                                     SdfPath const &instancerId=SdfPath());

    /// Add a triangle, quad and pentagon.
    HDLIB_API
    void AddPolygons(SdfPath const &id, GfMatrix4f const &transform,
                     Hd_UnitTestDelegate::Interpolation colorInterp,
                     SdfPath const &instancerId=SdfPath());

    /// Add a subdiv with various tags
    HDLIB_API
    void AddSubdiv(SdfPath const &id, GfMatrix4f const &transform,
                   SdfPath const &insatancerId=SdfPath());

    // -----------------------------------------------------------------------
    HDLIB_API
    void AddBasisCurves(SdfPath const &id,
                        VtVec3fArray const &points,
                        VtIntArray const &curveVertexCounts,
                        VtVec3fArray const &normals,
                        TfToken const &basis,
                        VtValue const &color,
                        Interpolation colorInterpolation,
                        VtValue const &width,
                        Interpolation widthInterpolation,
                        SdfPath const &instancerId=SdfPath());

    /// Add a basis curves prim containing two curves
    HDLIB_API
    void AddCurves(SdfPath const &id, TfToken const &basis,
                   GfMatrix4f const &transform,
                   Hd_UnitTestDelegate::Interpolation colorInterp=Hd_UnitTestDelegate::CONSTANT,
                   Hd_UnitTestDelegate::Interpolation widthInterp=Hd_UnitTestDelegate::CONSTANT,
                   bool authoredNormals=false,
                   SdfPath const &instancerId=SdfPath());

    HDLIB_API
    void AddPoints(SdfPath const &id,
                   VtVec3fArray const &points,
                   VtValue const &color,
                   Interpolation colorInterpolation,
                   VtValue const &width,
                   Interpolation widthInterpolation,
                   SdfPath const &instancerId=SdfPath());

    /// Add a points prim
    HDLIB_API
    void AddPoints(SdfPath const &id,
                   GfMatrix4f const &transform,
                   Hd_UnitTestDelegate::Interpolation colorInterp=Hd_UnitTestDelegate::CONSTANT,
                   Hd_UnitTestDelegate::Interpolation widthInterp=Hd_UnitTestDelegate::CONSTANT,
                   SdfPath const &instancerId=SdfPath());

    /// Instancer
    HDLIB_API
    void AddInstancer(SdfPath const &id,
                      SdfPath const &parentId=SdfPath(),
                      GfMatrix4f const &rootTransform=GfMatrix4f(1));

    HDLIB_API
    void SetInstancerProperties(SdfPath const &id,
                                VtIntArray const &prototypeIndex,
                                VtVec3fArray const &scale,
                                VtVec4fArray const &rotate,
                                VtVec3fArray const &translate);

    /// Shader
    HDLIB_API
    void AddSurfaceShader(SdfPath const &id,
                    std::string const &source,
                    HdShaderParamVector const &params);

    HDLIB_API
    void AddTexture(SdfPath const& id, GlfTextureRefPtr const& texture);

    /// Camera
    HDLIB_API
    void AddCamera(SdfPath const &id);

    /// Remove a prim
    HDLIB_API
    void Remove(SdfPath const &id);

    /// Clear all prims
    HDLIB_API
    void Clear();

    // Hides an rprim, invalidating all collections it was in.
    HDLIB_API
    void HideRprim(SdfPath const &id);

    // Un-hides an rprim, invalidating all collections it was in.
    HDLIB_API
    void UnhideRprim(SdfPath const &id);

    // set per-prim repr
    HDLIB_API
    void SetReprName(SdfPath const &id, TfToken const &reprName);

    // set per-prim refine level
    HDLIB_API
    void SetRefineLevel(SdfPath const &id, int refineLevel);

    /// Marks an rprim in the RenderIndex as dirty with the given dirty flags.
    HDLIB_API
    void MarkRprimDirty(SdfPath path, HdChangeTracker::DirtyBits flag);

    HDLIB_API
    void UpdatePositions(SdfPath const &id, float time);

    HDLIB_API
    void UpdateRprims(float time);
    HDLIB_API
    void UpdateInstancerPrimVars(float time);
    HDLIB_API
    void UpdateInstancerPrototypes(float time);

    void UpdateCamera(SdfPath const &id, TfToken const &key, VtValue value);

    void BindSurfaceShader(SdfPath const &rprimId, SdfPath const &shaderId)
    {
        _surfaceShaderBindings[rprimId] = shaderId;
    }

    // ---------------------------------------------------------------------- //
    // utility functions generating test case
    // ---------------------------------------------------------------------- //
    HDLIB_API
    GfVec3f PopulateBasicTestSet();
    HDLIB_API
    GfVec3f PopulateInvalidPrimsSet();

    // ---------------------------------------------------------------------- //
    // See HdSceneDelegate for documentation of virtual methods.
    // ---------------------------------------------------------------------- //
    HDLIB_API
    virtual bool IsInCollection(SdfPath const& id,
                                TfToken const& collectionName);
    HDLIB_API
    virtual HdMeshTopology GetMeshTopology(SdfPath const& id);
    HDLIB_API
    virtual HdBasisCurvesTopology GetBasisCurvesTopology(SdfPath const& id);
    HDLIB_API
    virtual PxOsdSubdivTags GetSubdivTags(SdfPath const& id);
    HDLIB_API
    virtual GfRange3d GetExtent(SdfPath const & id);
    HDLIB_API
    virtual GfMatrix4d GetTransform(SdfPath const & id);
    HDLIB_API
    virtual bool GetVisible(SdfPath const & id);
    HDLIB_API
    virtual bool GetDoubleSided(SdfPath const & id);
    HDLIB_API
    virtual int GetRefineLevel(SdfPath const & id);
    HDLIB_API
    virtual VtValue Get(SdfPath const& id, TfToken const& key);
    HDLIB_API
    virtual TfToken GetReprName(SdfPath const &id);
    HDLIB_API
    virtual TfTokenVector GetPrimVarVertexNames(SdfPath const& id);
    HDLIB_API
    virtual TfTokenVector GetPrimVarVaryingNames(SdfPath const& id);
    HDLIB_API
    virtual TfTokenVector GetPrimVarFacevaryingNames(SdfPath const& id);
    HDLIB_API
    virtual TfTokenVector GetPrimVarUniformNames(SdfPath const& id);
    HDLIB_API
    virtual TfTokenVector GetPrimVarConstantNames(SdfPath const& id);
    HDLIB_API
    virtual TfTokenVector GetPrimVarInstanceNames(SdfPath const& id);
    HDLIB_API
    virtual int GetPrimVarDataType(SdfPath const& id, TfToken const& key);
    HDLIB_API
    virtual int GetPrimVarComponents(SdfPath const& id, TfToken const& key);

    HDLIB_API
    virtual VtIntArray GetInstanceIndices(SdfPath const& instancerId,
                                          SdfPath const& prototypeId);

    HDLIB_API
    virtual GfMatrix4d GetInstancerTransform(SdfPath const& instancerId,
                                             SdfPath const& prototypeId);

    HDLIB_API
    virtual std::string GetSurfaceShaderSource(SdfPath const &shaderId);
    HDLIB_API
    virtual TfTokenVector GetSurfaceShaderParamNames(SdfPath const &shaderId);
    HDLIB_API
    virtual HdShaderParamVector GetSurfaceShaderParams(SdfPath const &shaderId);
    HDLIB_API
    virtual VtValue GetSurfaceShaderParamValue(SdfPath const &shaderId, 
                                  TfToken const &paramName);
    HDLIB_API
    virtual HdTextureResource::ID GetTextureResourceID(SdfPath const& textureId);
    HDLIB_API
    virtual HdTextureResourceSharedPtr GetTextureResource(SdfPath const& textureId);

private:
    struct _Mesh {
        _Mesh() { }
        _Mesh(TfToken const &scheme,
              TfToken const &orientation,
              GfMatrix4f const &transform,
              VtVec3fArray const &points,
              VtIntArray const &numVerts,
              VtIntArray const &verts,
              PxOsdSubdivTags const &subdivTags,
              VtValue const &color,
              Interpolation colorInterpolation,
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
        GfMatrix4f transform;
        VtVec3fArray points;
        VtIntArray numVerts;
        VtIntArray verts;
        PxOsdSubdivTags subdivTags;
        VtValue color;
        Interpolation colorInterpolation;
        bool guide;
        bool doubleSided;
        TfToken reprName;
    };
    struct _Curves {
        _Curves() { }
        _Curves(VtVec3fArray const &points,
                VtIntArray const &curveVertexCounts,
                VtVec3fArray const &normals,
                TfToken const &basis,
                VtValue const &color,
                Interpolation colorInterpolation,
                VtValue const &width,
                Interpolation widthInterpolation) :
            points(points), curveVertexCounts(curveVertexCounts), 
            normals(normals),
            basis(basis),
            color(color), colorInterpolation(colorInterpolation),
            width(width), widthInterpolation(widthInterpolation) { }

        VtVec3fArray points;
        VtIntArray curveVertexCounts;
        VtVec3fArray normals;
        TfToken basis;
        VtValue color;
        Interpolation colorInterpolation;
        VtValue width;
        Interpolation widthInterpolation;
    };
    struct _Points {
        _Points() { }
        _Points(VtVec3fArray const &points,
                VtValue const &color,
                Interpolation colorInterpolation,
                VtValue const &width,
                Interpolation widthInterpolation) :
            points(points),
            color(color), colorInterpolation(colorInterpolation),
            width(width), widthInterpolation(widthInterpolation) { }

        VtVec3fArray points;
        VtValue color;
        Interpolation colorInterpolation;
        VtValue width;
        Interpolation widthInterpolation;
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
    struct _Texture {
        _Texture() {}
        _Texture(GlfTextureRefPtr const &tex)
            : texture(tex) {
        }
        GlfTextureRefPtr texture;
    };
    struct _Camera {
        VtDictionary params;
    };
    struct _Light {
        VtDictionary params;
    };

    std::map<SdfPath, _Mesh> _meshes;
    std::map<SdfPath, _Curves> _curves;
    std::map<SdfPath, _Points> _points;
    std::map<SdfPath, _Instancer> _instancers;
    std::map<SdfPath, _SurfaceShader> _surfaceShaders;
    std::map<SdfPath, _Texture> _textures;
    std::map<SdfPath, _Camera> _cameras;
    std::map<SdfPath, _Light> _lights;
    TfHashSet<SdfPath, SdfPath::Hash> _hiddenRprims;

    typedef std::map<SdfPath, SdfPath> SdfPathMap;
    SdfPathMap _surfaceShaderBindings;

    bool _hasInstancePrimVars;
    int _refineLevel;
    std::map<SdfPath, int> _refineLevels;
};

#endif  // HD_UNIT_TEST_DELEGATE
