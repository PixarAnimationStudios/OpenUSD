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
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/material.h"
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
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdUnitTestDelegate
///
/// A simple delegate class for unit test driver.
///
class HdUnitTestDelegate : public HdSceneDelegate {
public:
    HD_API
    HdUnitTestDelegate(HdRenderIndex *parentIndex,
                        SdfPath const& delegateID);

    void SetUseInstancePrimvars(bool v) { _hasInstancePrimvars = v; }

    HD_API
    void SetRefineLevel(int level);

    HD_API
    void SetVisibility(bool vis);

    // -----------------------------------------------------------------------

    HD_API
    void AddMesh(SdfPath const& id);

    HD_API
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

    HD_API
    void AddMesh(SdfPath const &id,
                 GfMatrix4f const &transform,
                 VtVec3fArray const &points,
                 VtIntArray const &numVerts,
                 VtIntArray const &verts,
                 VtIntArray const &holes,
                 PxOsdSubdivTags const &subdivTags,
                 VtValue const &color,
                 HdInterpolation colorInterpolation,
                 bool guide=false,
                 SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmark,
                 TfToken const &orientation=HdTokens->rightHanded,
                 bool doubleSided=false);

    /// Add a cube
    HD_API
    void AddCube(SdfPath const &id, GfMatrix4f const &transform, bool guide=false,
                 SdfPath const &instancerId=SdfPath(),
                 TfToken const &scheme=PxOsdOpenSubdivTokens->catmark);

    /// Add a grid with division x*y
    HD_API
    void AddGrid(SdfPath const &id, int x, int y, GfMatrix4f const &transform,
                 bool rightHanded=true, bool doubleSided=false,
                 SdfPath const &instancerId=SdfPath());

    /// Add a grid with division x*y
    HD_API
    void AddGridWithFaceColor(SdfPath const &id, int x, int y,
                              GfMatrix4f const &transform,
                              bool rightHanded=true, bool doubleSided=false,
                              SdfPath const &instancerId=SdfPath());

    /// Add a grid with division x*y
    HD_API
    void AddGridWithVertexColor(SdfPath const &id, int x, int y,
                                GfMatrix4f const &transform,
                                bool rightHanded=true, bool doubleSided=false,
                                SdfPath const &instancerId=SdfPath());

    /// Add a grid with division x*y
    HD_API
    void AddGridWithFaceVaryingColor(SdfPath const &id, int x, int y,
                                     GfMatrix4f const &transform,
                                     bool rightHanded=true, bool doubleSided=false,
                                     SdfPath const &instancerId=SdfPath());

    // Add a grid with division x*y and a custom primvar
    HD_API
    void AddGridWithPrimvar(SdfPath const &id, int nx, int ny,
                            GfMatrix4f const &transform,
                            VtValue const &primvar,
                            HdInterpolation primvarInterpolation,
                            bool rightHanded=true, bool doubleSided=false,
                            SdfPath const &instancerId=SdfPath());

    /// Add a triangle, quad and pentagon.
    HD_API
    void AddPolygons(SdfPath const &id, GfMatrix4f const &transform,
                     HdInterpolation colorInterp,
                     SdfPath const &instancerId=SdfPath());

    /// Add a subdiv with various tags
    HD_API
    void AddSubdiv(SdfPath const &id, GfMatrix4f const &transform,
                   SdfPath const &insatancerId=SdfPath());

    // -----------------------------------------------------------------------

    HD_API
    void AddBasisCurves(SdfPath const &id,
                        VtVec3fArray const &points,
                        VtIntArray const &curveVertexCounts,
                        VtVec3fArray const &normals,
                        TfToken const &type,
                        TfToken const &basis,
                        VtValue const &color,
                        HdInterpolation colorInterpolation,
                        VtValue const &width,
                        HdInterpolation widthInterpolation,
                        SdfPath const &instancerId=SdfPath());

    /// Add a basis curves prim containing two curves
    HD_API
    void AddCurves(SdfPath const &id, TfToken const &type, TfToken const &basis,
                   GfMatrix4f const &transform,
                   HdInterpolation colorInterp=HdInterpolationConstant,
                   HdInterpolation widthInterp=HdInterpolationConstant,
                   bool authoredNormals=false,
                   SdfPath const &instancerId=SdfPath());

    HD_API
    void AddPoints(SdfPath const &id,
                   VtVec3fArray const &points,
                   VtValue const &color,
                   HdInterpolation colorInterpolation,
                   VtValue const &width,
                   HdInterpolation widthInterpolation,
                   SdfPath const &instancerId=SdfPath());

    /// Add a points prim
    HD_API
    void AddPoints(SdfPath const &id,
                   GfMatrix4f const &transform,
                   HdInterpolation colorInterp=HdInterpolationConstant,
                   HdInterpolation widthInterp=HdInterpolationConstant,
                   SdfPath const &instancerId=SdfPath());

    /// Instancer
    HD_API
    void AddInstancer(SdfPath const &id,
                      SdfPath const &parentId=SdfPath(),
                      GfMatrix4f const &rootTransform=GfMatrix4f(1));

    HD_API
    void SetInstancerProperties(SdfPath const &id,
                                VtIntArray const &prototypeIndex,
                                VtVec3fArray const &scale,
                                VtVec4fArray const &rotate,
                                VtVec3fArray const &translate);

    /// XXX : This will be removed as we integrate materials into the
    ///       new material resource pipeline.
    HD_API
    void AddMaterialHydra(SdfPath const &id,
                          std::string const &sourceSurface,
                          std::string const &sourceDisplacement,
                          HdMaterialParamVector const &params);
    
    /// Material
    HD_API
    void AddMaterialResource(SdfPath const &id,
                             VtValue materialResource);

    /// Update a material resource 
    HD_API
    void UpdateMaterialResource(SdfPath const &materialId, 
                                VtValue materialResource);

    HD_API
    void BindMaterial(SdfPath const &rprimId, SdfPath const &materialId);

    /// Example to update a material binding on the fly
    HD_API
    void RebindMaterial(SdfPath const &rprimId, SdfPath const &materialId);

    /// Render buffers
    HD_API
    void AddRenderBuffer(SdfPath const &id, GfVec3i const& dims,
                         HdFormat format, bool multiSampled);

    /// Camera
    HD_API
    void AddCamera(SdfPath const &id);
    HD_API
    void UpdateCamera(SdfPath const &id, TfToken const &key, VtValue value);

    /// Tasks
    template<typename T>
    void AddTask(SdfPath const &id) {
        GetRenderIndex().InsertTask<T>(this, id);
        _tasks[id] = _Task();
    }
    HD_API
    void UpdateTask(SdfPath const &id, TfToken const &key, VtValue value);

    /// Remove a prim
    HD_API
    void Remove(SdfPath const &id);

    /// Clear all prims
    HD_API
    void Clear();

    // Hides an rprim, invalidating all collections it was in.
    HD_API
    void HideRprim(SdfPath const &id);

    // Un-hides an rprim, invalidating all collections it was in.
    HD_API
    void UnhideRprim(SdfPath const &id);

    // set per-prim repr
    HD_API
    void SetReprName(SdfPath const &id, TfToken const &reprName);

    // set per-prim refine level
    HD_API
    void SetRefineLevel(SdfPath const &id, int refineLevel);

    // set per-prim visibility
    HD_API
    void SetVisibility(SdfPath const &id, bool vis);

    /// Marks an rprim in the RenderIndex as dirty with the given dirty flags.
    HD_API
    void MarkRprimDirty(SdfPath path, HdDirtyBits flag);

    HD_API
    void UpdatePositions(SdfPath const &id, float time);
    HD_API
    void UpdateRprims(float time);
    HD_API
    void UpdateInstancerPrimvars(float time);
    HD_API
    void UpdateInstancerPrototypes(float time);
    HD_API
    void UpdateCurvePrimvarsInterpMode(float time);

    // ---------------------------------------------------------------------- //
    // utility functions generating test case
    // ---------------------------------------------------------------------- //
    HD_API
    GfVec3f PopulateBasicTestSet();
    HD_API
    GfVec3f PopulateInvalidPrimsSet();

    // ---------------------------------------------------------------------- //
    // See HdSceneDelegate for documentation of virtual methods.
    // ---------------------------------------------------------------------- //
    HD_API
    virtual HdMeshTopology GetMeshTopology(SdfPath const& id);
    HD_API
    virtual HdBasisCurvesTopology GetBasisCurvesTopology(SdfPath const& id);
    HD_API
    virtual TfToken GetRenderTag(SdfPath const& id, TfToken const& reprName);
    HD_API
    virtual PxOsdSubdivTags GetSubdivTags(SdfPath const& id);
    HD_API
    virtual GfRange3d GetExtent(SdfPath const & id);
    HD_API
    virtual GfMatrix4d GetTransform(SdfPath const & id);
    HD_API
    virtual bool GetVisible(SdfPath const & id);
    HD_API
    virtual bool GetDoubleSided(SdfPath const & id);
    HD_API
    virtual HdDisplayStyle GetDisplayStyle(SdfPath const & id) override;
    HD_API
    virtual VtValue Get(SdfPath const& id, TfToken const& key);
    HD_API
    virtual TfToken GetReprName(SdfPath const &id);
    HD_API
    virtual HdPrimvarDescriptorVector
    GetPrimvarDescriptors(SdfPath const& id,
                          HdInterpolation interpolation) override;

    HD_API
    virtual VtIntArray GetInstanceIndices(SdfPath const& instancerId,
                                          SdfPath const& prototypeId);

    HD_API
    virtual GfMatrix4d GetInstancerTransform(SdfPath const& instancerId,
                                             SdfPath const& prototypeId);

    HD_API
    virtual SdfPath GetMaterialId(SdfPath const& rprimId);
    HD_API
    virtual std::string GetSurfaceShaderSource(SdfPath const &materialId);
    HD_API
    virtual std::string GetDisplacementShaderSource(SdfPath const &materialId);    
    HD_API
    virtual HdMaterialParamVector GetMaterialParams(SdfPath const &materialId);
    HD_API
    virtual VtValue GetMaterialParamValue(SdfPath const &materialId, 
                                          TfToken const &paramName);
    HD_API
    virtual HdTextureResource::ID GetTextureResourceID(SdfPath const& textureId);
    HD_API
    virtual HdTextureResourceSharedPtr GetTextureResource(SdfPath const& textureId);
    HD_API 
    virtual VtValue GetMaterialResource(SdfPath const &materialId);
    HD_API
    virtual HdRenderBufferDescriptor GetRenderBufferDescriptor(SdfPath const& id);

private:
    struct _Mesh {
        _Mesh() { }
        _Mesh(TfToken const &scheme,
              TfToken const &orientation,
              GfMatrix4f const &transform,
              VtVec3fArray const &points,
              VtIntArray const &numVerts,
              VtIntArray const &verts,
              VtIntArray const &holes,
              PxOsdSubdivTags const &subdivTags,
              VtValue const &color,
              HdInterpolation colorInterpolation,
              bool guide,
              bool doubleSided) :
            scheme(scheme), orientation(orientation),
            transform(transform),
            points(points), numVerts(numVerts), verts(verts),
            holes(holes), subdivTags(subdivTags), color(color),
            colorInterpolation(colorInterpolation), guide(guide),
            doubleSided(doubleSided) { }

        TfToken scheme;
        TfToken orientation;
        GfMatrix4f transform;
        VtVec3fArray points;
        VtIntArray numVerts;
        VtIntArray verts;
        VtIntArray holes;
        PxOsdSubdivTags subdivTags;
        VtValue color;
        HdInterpolation colorInterpolation;
        bool guide;
        bool doubleSided;
        TfToken reprName;
    };
    struct _Curves {
        _Curves() { }
        _Curves(VtVec3fArray const &points,
                VtIntArray const &curveVertexCounts,
                VtVec3fArray const &normals,
                TfToken const &type,
                TfToken const &basis,
                VtValue const &color,
                HdInterpolation colorInterpolation,
                VtValue const &width,
                HdInterpolation widthInterpolation) :
            points(points), curveVertexCounts(curveVertexCounts), 
            normals(normals),
            type(type),
            basis(basis),
            color(color), colorInterpolation(colorInterpolation),
            width(width), widthInterpolation(widthInterpolation) { }

        VtVec3fArray points;
        VtIntArray curveVertexCounts;
        VtVec3fArray normals;
        TfToken type;
        TfToken basis;
        VtValue color;
        HdInterpolation colorInterpolation;
        VtValue width;
        HdInterpolation widthInterpolation;
    };
    struct _Points {
        _Points() { }
        _Points(VtVec3fArray const &points,
                VtValue const &color,
                HdInterpolation colorInterpolation,
                VtValue const &width,
                HdInterpolation widthInterpolation) :
            points(points),
            color(color), colorInterpolation(colorInterpolation),
            width(width), widthInterpolation(widthInterpolation) { }

        VtVec3fArray points;
        VtValue color;
        HdInterpolation colorInterpolation;
        VtValue width;
        HdInterpolation widthInterpolation;
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
    struct _MaterialHydra {
        _MaterialHydra() { }
        _MaterialHydra(std::string const &srcSurface, 
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
    struct _Camera {
        VtDictionary params;
    };
    struct _Light {
        VtDictionary params;
    };
    struct _Task {
        VtDictionary params;
    };
    struct _RenderBuffer {
        _RenderBuffer() {}
        _RenderBuffer(GfVec3i const &d, HdFormat f, bool ms)
            : dims(d), format(f), multiSampled(ms) {}
        GfVec3i dims;
        HdFormat format;
        bool multiSampled;
    };

    std::map<SdfPath, _Mesh> _meshes;
    std::map<SdfPath, _Curves> _curves;
    std::map<SdfPath, _Points> _points;
    std::map<SdfPath, _Instancer> _instancers;
    std::map<SdfPath, _MaterialHydra> _materialsHydra;
    std::map<SdfPath, VtValue> _materials;
    std::map<SdfPath, _Camera> _cameras;
    std::map<SdfPath, _RenderBuffer> _renderBuffers;
    std::map<SdfPath, _Light> _lights;
    std::map<SdfPath, _Task> _tasks;
    TfHashSet<SdfPath, SdfPath::Hash> _hiddenRprims;

    typedef std::map<SdfPath, SdfPath> SdfPathMap;
    SdfPathMap _materialBindings;

    bool _hasInstancePrimvars;
    int _refineLevel;
    bool _visibility;
    std::map<SdfPath, int> _refineLevels;
    std::map<SdfPath, bool> _visibilities;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_UNIT_TEST_DELEGATE
