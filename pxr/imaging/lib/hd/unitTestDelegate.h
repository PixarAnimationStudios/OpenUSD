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

#include "pxr/pxr.h"
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

PXR_NAMESPACE_OPEN_SCOPE


#define HD_UNIT_TEST_TOKENS                             \
    (geometryAndGuides)

TF_DECLARE_PUBLIC_TOKENS(Hd_UnitTestTokens, HD_UNIT_TEST_TOKENS);

/// \class Hd_UnitTestDelegate
///
/// A simple delegate class for unit test driver.
///
class Hd_UnitTestDelegate : public HdSceneDelegate {
public:
    Hd_UnitTestDelegate();

    void SetUseInstancePrimVars(bool v) { _hasInstancePrimVars = v; }

    void SetRefineLevel(int level);

    enum Interpolation { VERTEX, UNIFORM, CONSTANT, FACEVARYING, VARYING };

    // -----------------------------------------------------------------------

    void AddMesh(SdfPath const& id);

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
    void AddCube(SdfPath const &id, GfMatrix4f const &transform, bool guide=false,
                 SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmark);

    /// Add a grid with division x*y
    void AddGrid(SdfPath const &id, int x, int y, GfMatrix4f const &transform,
                 bool rightHanded=true, bool doubleSided=false,
                 SdfPath const &instancerId=SdfPath());

    /// Add a grid with division x*y
    void AddGridWithFaceColor(SdfPath const &id, int x, int y,
                              GfMatrix4f const &transform,
                              bool rightHanded=true, bool doubleSided=false,
                              SdfPath const &instancerId=SdfPath());

    /// Add a grid with division x*y
    void AddGridWithVertexColor(SdfPath const &id, int x, int y,
                                GfMatrix4f const &transform,
                                bool rightHanded=true, bool doubleSided=false,
                                SdfPath const &instancerId=SdfPath());

    /// Add a grid with division x*y
    void AddGridWithFaceVaryingColor(SdfPath const &id, int x, int y,
                                     GfMatrix4f const &transform,
                                     bool rightHanded=true, bool doubleSided=false,
                                     SdfPath const &instancerId=SdfPath());

    /// Add a triangle, quad and pentagon.
    void AddPolygons(SdfPath const &id, GfMatrix4f const &transform,
                     Hd_UnitTestDelegate::Interpolation colorInterp,
                     SdfPath const &instancerId=SdfPath());

    /// Add a subdiv with various tags
    void AddSubdiv(SdfPath const &id, GfMatrix4f const &transform,
                   SdfPath const &insatancerId=SdfPath());

    // -----------------------------------------------------------------------

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
    void AddCurves(SdfPath const &id, TfToken const &basis,
                   GfMatrix4f const &transform,
                   Hd_UnitTestDelegate::Interpolation colorInterp=Hd_UnitTestDelegate::CONSTANT,
                   Hd_UnitTestDelegate::Interpolation widthInterp=Hd_UnitTestDelegate::CONSTANT,
                   bool authoredNormals=false,
                   SdfPath const &instancerId=SdfPath());

    void AddPoints(SdfPath const &id,
                   VtVec3fArray const &points,
                   VtValue const &color,
                   Interpolation colorInterpolation,
                   VtValue const &width,
                   Interpolation widthInterpolation,
                   SdfPath const &instancerId=SdfPath());

    /// Add a points prim
    void AddPoints(SdfPath const &id,
                   GfMatrix4f const &transform,
                   Hd_UnitTestDelegate::Interpolation colorInterp=Hd_UnitTestDelegate::CONSTANT,
                   Hd_UnitTestDelegate::Interpolation widthInterp=Hd_UnitTestDelegate::CONSTANT,
                   SdfPath const &instancerId=SdfPath());

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

    void AddTexture(SdfPath const& id, GlfTextureRefPtr const& texture);

    /// Camera
    void AddCamera(SdfPath const &id);

    /// Remove a prim
    void Remove(SdfPath const &id);

    /// Clear all prims
    void Clear();

    // Hides an rprim, invalidating all collections it was in.
    void HideRprim(SdfPath const &id);

    // Un-hides an rprim, invalidating all collections it was in.
    void UnhideRprim(SdfPath const &id);

    // set per-prim repr
    void SetReprName(SdfPath const &id, TfToken const &reprName);

    // set per-prim refine level
    void SetRefineLevel(SdfPath const &id, int refineLevel);

    /// Marks an rprim in the RenderIndex as dirty with the given dirty flags.
    void MarkRprimDirty(SdfPath path, HdChangeTracker::DirtyBits flag);

    void UpdatePositions(SdfPath const &id, float time);
    void UpdateRprims(float time);
    void UpdateInstancerPrimVars(float time);
    void UpdateInstancerPrototypes(float time);

    void UpdateCamera(SdfPath const &id, TfToken const &key, VtValue value);

    void BindSurfaceShader(SdfPath const &rprimId, SdfPath const &shaderId)
    {
        _surfaceShaderBindings[rprimId] = shaderId;
    }

    // ---------------------------------------------------------------------- //
    // utility functions generating test case
    // ---------------------------------------------------------------------- //
    GfVec3f PopulateBasicTestSet();
    GfVec3f PopulateInvalidPrimsSet();

    // ---------------------------------------------------------------------- //
    // See HdSceneDelegate for documentation of virtual methods.
    // ---------------------------------------------------------------------- //
    virtual bool IsInCollection(SdfPath const& id,
                                TfToken const& collectionName);
    virtual HdMeshTopology GetMeshTopology(SdfPath const& id);
    virtual HdBasisCurvesTopology GetBasisCurvesTopology(SdfPath const& id);
    virtual PxOsdSubdivTags GetSubdivTags(SdfPath const& id);
    virtual GfRange3d GetExtent(SdfPath const & id);
    virtual GfMatrix4d GetTransform(SdfPath const & id);
    virtual bool GetVisible(SdfPath const & id);
    virtual bool GetDoubleSided(SdfPath const & id);
    virtual int GetRefineLevel(SdfPath const & id);
    virtual VtValue Get(SdfPath const& id, TfToken const& key);
    virtual TfToken GetReprName(SdfPath const &id);
    virtual TfTokenVector GetPrimVarVertexNames(SdfPath const& id);
    virtual TfTokenVector GetPrimVarVaryingNames(SdfPath const& id);
    virtual TfTokenVector GetPrimVarFacevaryingNames(SdfPath const& id);
    virtual TfTokenVector GetPrimVarUniformNames(SdfPath const& id);
    virtual TfTokenVector GetPrimVarConstantNames(SdfPath const& id);
    virtual TfTokenVector GetPrimVarInstanceNames(SdfPath const& id);
    virtual int GetPrimVarDataType(SdfPath const& id, TfToken const& key);
    virtual int GetPrimVarComponents(SdfPath const& id, TfToken const& key);

    virtual VtIntArray GetInstanceIndices(SdfPath const& instancerId,
                                          SdfPath const& prototypeId);

    virtual GfMatrix4d GetInstancerTransform(SdfPath const& instancerId,
                                             SdfPath const& prototypeId);

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


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_UNIT_TEST_DELEGATE
