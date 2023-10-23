//
// Copyright 2022 Pixar
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

#include "pxr/usdImaging/usdImaging/drawModeStandin.h"

#include "pxr/usdImaging/usdImaging/modelSchema.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/basisCurvesSchema.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hio/image.h"

#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"

#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/range3d.h"

#include <array>
#include <functional>
#include <bitset>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// UsdImaging_DrawModeStandin implementation
///
////////////////////////////////////////////////////////////////////////////////

UsdImaging_DrawModeStandin::~UsdImaging_DrawModeStandin() = default;

const HdSceneIndexPrim &
UsdImaging_DrawModeStandin::GetPrim() const
{
    static HdSceneIndexPrim empty{ TfToken(), nullptr };
    return empty;
}

HdSceneIndexPrim
UsdImaging_DrawModeStandin::GetChildPrim(const TfToken &name) const
{
    return { _GetChildPrimType(name), _GetChildPrimSource(name) };
}

SdfPathVector
UsdImaging_DrawModeStandin::GetChildPrimPaths() const
{
    const TfTokenVector &childNames = _GetChildNames();
    SdfPathVector result;
    result.reserve(childNames.size());
    for (const TfToken &childName : childNames) {
        result.push_back(_path.AppendChild(childName));
    }
    return result;
}

void
UsdImaging_DrawModeStandin::ComputePrimAddedEntries(
    HdSceneIndexObserver::AddedPrimEntries * entries) const
{
    entries->push_back({_path, TfToken()});
    const TfTokenVector &childNames = _GetChildNames();
    for (const TfToken &childName : childNames) {
        const SdfPath childPath = _path.AppendChild(childName);
        entries->push_back( { childPath, _GetChildPrimType(childName) });
    }
}

namespace {

////////////////////////////////////////////////////////////////////////////////
///
/// Helpers and data sources serving as building blocks or base classes.
///
////////////////////////////////////////////////////////////////////////////////

TF_DEFINE_PRIVATE_TOKENS(
    _UsdUVTextureTokens,

    (fallback)
    (file)
    (st)
    (wrapS)
    (wrapT)

    (rgb)
    (a)
    (clamp)
);

TF_DEFINE_PRIVATE_TOKENS(
    _UsdPrimvarReaderTokens,

    (fallback)
    (varname)
    (result)
);

TF_DEFINE_PRIVATE_TOKENS(
    _UsdPreviewSurfaceTokens,

    (diffuseColor)
    (opacity)
    (opacityThreshold)
);

TfTokenVector
_Concat(const TfTokenVector &a, const TfTokenVector &b)
{
    TfTokenVector result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

/// A vec3f color source constructed from a model schema and returning
/// the schema's draw mode color.
///
/// Note that it is querying the drawModeColor from the schema each time,
/// so we can use the same pointer to _DisplayColorVec3fDataSource even if
/// model:drawModeColor was dirtied.
///
class _DisplayColorVec3fDataSource final : public HdVec3fDataSource
{
public:
    HD_DECLARE_DATASOURCE(_DisplayColorVec3fDataSource);

    VtValue GetValue(const Time shutterOffset) {
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfVec3f GetTypedValue(const Time shutterOffset) {
        if (HdVec3fDataSourceHandle src = _schema.GetDrawModeColor()) {
            return src->GetTypedValue(shutterOffset);
        }
        return {0.18f, 0.18f, 0.18f};
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> * outSampleTimes)
    {
        if (HdVec3fDataSourceHandle src = _schema.GetDrawModeColor()) {
            return src->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
        }
        
        return false;
    }

private:
    _DisplayColorVec3fDataSource(const UsdImagingModelSchema schema)
      : _schema(schema)
    {
    }

    UsdImagingModelSchema _schema;
};

/// A vec4f wrapper around a HdVec3fDataSource, for use when a vec4f
/// is needed, e.g., for the UsdUVTexture's input:fallback parameter.
///
class _Vec4fFromVec3fDataSource final : public HdVec4fDataSource
{
public:
    HD_DECLARE_DATASOURCE(_Vec4fFromVec3fDataSource);

    VtValue GetValue(const Time shutterOffset) {
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfVec4f GetTypedValue(const Time shutterOffset) {
        const GfVec3f src = _vec3fSource->GetTypedValue(shutterOffset);
        return { src[0], src[1], src[2], _alpha };
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> * outSampleTimes)
    {
        return _vec3fSource->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    _Vec4fFromVec3fDataSource(
        const HdVec3fDataSourceHandle source, 
        const float alpha)
      : _vec3fSource(source),
        _alpha(alpha)
    {
    }

    HdVec3fDataSourceHandle _vec3fSource;
    float _alpha;
};

/// A convenience data source implementing the primvar schema from
/// a triple of primvar value, interpolation and role. The latter two
/// are given as tokens. The value can be given either as data source
/// or as thunk returning a data source which is evaluated on each
/// Get.
class _PrimvarDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarDataSource);

    TfTokenVector GetNames() override {
        return {HdPrimvarSchemaTokens->primvarValue,
                HdPrimvarSchemaTokens->interpolation,
                HdPrimvarSchemaTokens->role};
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdPrimvarSchemaTokens->primvarValue) {
            return _primvarValueSrc;
        }
        if (name == HdPrimvarSchemaTokens->interpolation) {
            return
                HdPrimvarSchema::BuildInterpolationDataSource(
                    _interpolation);
        }
        if (name == HdPrimvarSchemaTokens->role) {
            return
                HdPrimvarSchema::BuildRoleDataSource(
                    _role);
        }

        return nullptr;
    }

private:
    _PrimvarDataSource(
        const HdDataSourceBaseHandle &primvarValueSrc,
        const TfToken &interpolation,
        const TfToken &role)
      : _primvarValueSrc(primvarValueSrc)
      , _interpolation(interpolation)
      , _role(role)
    {
    }

    HdDataSourceBaseHandle _primvarValueSrc;
    TfToken _interpolation;
    TfToken _role;
};

/// Base class for container data sources providing primvars.
///
/// Provides primvars common to stand-in geometry:
/// - width (constant)
/// - displayOpacity (constant)
/// - displayColor (computed by querying model:drawModeColor from the prim
///   data source).
///
class _PrimvarsDataSource : public HdContainerDataSource
{
public:

    TfTokenVector GetNames() override {
        return {HdPrimvarsSchemaTokens->widths,
                HdTokens->displayColor,
                HdTokens->displayOpacity};
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdPrimvarsSchemaTokens->widths) {
            static HdDataSourceBaseHandle const src =
                _PrimvarDataSource::New(
                    HdRetainedTypedSampledDataSource<VtFloatArray>::New(
                        VtFloatArray{1.0f}),
                    HdPrimvarSchemaTokens->constant,
                    TfToken());
            return src;
        }
        if (name == HdTokens->displayColor) {
            /// If the model:drawModeColor is dirtied on the input scene
            /// index, we need to query the model again for the drawModeColor.
            ///
            /// If we stored a reference to the data source at
            /// model:drawModeColor with the _PrimvarDataSource, we would need
            /// to update that reference when model:drawModeColor is dirtied.
            ///
            /// Instead, we store the _DisplayColorVec3fDataSource with the
            /// _PrimvarDataSource which pulls the drawModeColor from model
            /// every time it is needed.
            ///
            return _PrimvarDataSource::New(
                _DisplayColorVec3fDataSource::New(
                    UsdImagingModelSchema::GetFromParent(_primSource)),
                HdPrimvarSchemaTokens->constant,
                HdPrimvarSchemaTokens->color);
        }
        if (name == HdTokens->displayOpacity) {
            static HdDataSourceBaseHandle const src =
                _PrimvarDataSource::New(
                    HdRetainedTypedSampledDataSource<VtFloatArray>::New(
                        VtFloatArray{1.0f}),
                    HdPrimvarSchemaTokens->constant,
                    TfToken());
            return src;
        }
        return nullptr;
    }

protected:
    _PrimvarsDataSource(
        const HdContainerDataSourceHandle &primSource)
      : _primSource(primSource)
    {
    }

    HdContainerDataSourceHandle _primSource;
};

/// Base class for prim data sources.
///
/// Provides:
/// - xform (from the given prim data source)
/// - purpose (from the given prim data source)
/// - visibility (from the given prim data source)
/// - displayStyle (constant)
///
class _PrimDataSource : public HdContainerDataSource
{
public:

    TfTokenVector GetNames() override {
        return {
            HdXformSchemaTokens->xform,
            HdPurposeSchemaTokens->purpose,
            HdVisibilitySchemaTokens->visibility,
            HdInstancedBySchemaTokens->instancedBy,
            HdLegacyDisplayStyleSchemaTokens->displayStyle };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdXformSchemaTokens->xform ||
            name == HdPurposeSchemaTokens->purpose ||
            name == HdVisibilitySchemaTokens->visibility ||
            name == HdInstancedBySchemaTokens->instancedBy) {
            if (_primSource) {
                return _primSource->Get(name);
            }
            return nullptr;
        }
        if (name == HdLegacyDisplayStyleSchemaTokens->displayStyle) {
            static const HdDataSourceBaseHandle src =
                HdLegacyDisplayStyleSchema::Builder()
                    .SetCullStyle(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdCullStyleTokens->back))
                    .Build();
            return src;
        }
        return nullptr;
    }

protected:
    _PrimDataSource(const HdContainerDataSourceHandle &primSource)
      : _primSource(primSource)
    {
    }

    HdContainerDataSourceHandle _primSource;
};


namespace _BoundsDrawMode {

////////////////////////////////////////////////////////////////////////////////
///
/// Implements stand-in for bounds draw mode.
///
/// It is drawing the edges of a box (using basis curves) determined by the
/// extents.
///
////////////////////////////////////////////////////////////////////////////////

TF_DEFINE_PRIVATE_TOKENS(
    _primNameTokens,

    (boundsCurves)
);

/// Data source for primvars:points:primvarValue
///
/// Computes 8 vertices of a box determined by extent of a given prim
/// data source.
///
class _BoundsPointsPrimvarValueDataSource final : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_BoundsPointsPrimvarValueDataSource);

    VtValue GetValue(Time shutterOffset) {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(Time shutterOffset) {
        // Get extent from given prim source.
        HdExtentSchema extentSchema =
            HdExtentSchema::GetFromParent(_primSource);

        GfVec3f exts[2] = { GfVec3f(0.0f), GfVec3f(0.0f) };
        if (HdVec3dDataSourceHandle src = extentSchema.GetMin()) {
            exts[0] = GfVec3f(src->GetTypedValue(shutterOffset));
        }
        if (HdVec3dDataSourceHandle src = extentSchema.GetMax()) {
            exts[1] = GfVec3f(src->GetTypedValue(shutterOffset));
        }

        /// Compute 8 points on box.
        VtVec3fArray pts(8);
        size_t i = 0;
        for (size_t j0 = 0; j0 < 2; j0++) {
            for (size_t j1 = 0; j1 < 2; j1++) {
                for (size_t j2 = 0; j2 < 2; j2++) {
                    pts[i] = { exts[j0][0], exts[j1][1], exts[j2][2] };
                    ++i;
                }
            }
        }

        return pts;
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> * outSampleTimes)
    {
        HdExtentSchema extentSchema =
            HdExtentSchema::GetFromParent(_primSource);

        HdSampledDataSourceHandle srcs[] = {
            extentSchema.GetMin(), extentSchema.GetMax() };

        return HdGetMergedContributingSampleTimesForInterval(
            TfArraySize(srcs), srcs,
            startTime, endTime, outSampleTimes);
    }

private:
    _BoundsPointsPrimvarValueDataSource(
        const HdContainerDataSourceHandle &primSource)
      : _primSource(primSource)
    {
    }

    HdContainerDataSourceHandle _primSource;
};

/// Data source for primvars.
///
/// Provides (on top of the base class):
/// - points (using the above data source)
///
class _BoundsPrimvarsDataSource final : public _PrimvarsDataSource
{
public:
    HD_DECLARE_DATASOURCE(_BoundsPrimvarsDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector result = _Concat(
            _PrimvarsDataSource::GetNames(),
            { HdPrimvarsSchemaTokens->points });
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdPrimvarsSchemaTokens->points) {
            return _PrimvarDataSource::New(
                _BoundsPointsPrimvarValueDataSource::New(_primSource),
                HdPrimvarSchemaTokens->vertex,
                HdPrimvarSchemaTokens->point);
        }
        return _PrimvarsDataSource::Get(name);
    }

private:
    _BoundsPrimvarsDataSource(
        const HdContainerDataSourceHandle &primSource)
      : _PrimvarsDataSource(primSource)
    {
    }
};

HdContainerDataSourceHandle
_ComputeBoundsTopology()
{
    // Segments: CCW bottom face starting at (-x, -y, -z)
    //           CCW top face starting at (-x, -y, z)
    //           CCW vertical edges, starting at (-x, -y)
    const VtIntArray curveIndices{
            /* bottom face */ 0, 4, 4, 6, 6, 2, 2, 0,
            /* top face */    1, 5, 5, 7, 7, 3, 3, 1,
            /* edge pairs */  0, 1, 4, 5, 6, 7, 2, 3 };
    const VtIntArray curveVertexCounts{
            static_cast<int>(curveIndices.size()) };
    
    return HdBasisCurvesTopologySchema::Builder()
        .SetCurveVertexCounts(
                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    curveVertexCounts))
        .SetCurveIndices(
            HdRetainedTypedSampledDataSource<VtIntArray>::New(
                curveIndices))
        .SetBasis(
            HdRetainedTypedSampledDataSource<TfToken>::New(
                HdTokens->bezier))
        .SetType(
            HdRetainedTypedSampledDataSource<TfToken>::New(
                HdTokens->linear))
        .SetWrap(
            HdRetainedTypedSampledDataSource<TfToken>::New(
                HdTokens->segmented))
        .Build();
}

/// Prim data source.
///
/// Provides (on top of the base class):
/// - basisCurves (constant using above topology)
/// - primvars (using above data source)
/// - extent (from the original prim source)
///
class _BoundsPrimDataSource : public _PrimDataSource
{
public:
    HD_DECLARE_DATASOURCE(_BoundsPrimDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector result = _Concat(
            _PrimDataSource::GetNames(),
            { HdBasisCurvesSchemaTokens->basisCurves,
              HdPrimvarsSchemaTokens->primvars,
              HdExtentSchemaTokens->extent });
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdBasisCurvesSchemaTokens->basisCurves) {
            static const HdDataSourceBaseHandle basisCurvesSrc =
                HdBasisCurvesSchema::Builder()
                .SetTopology(_ComputeBoundsTopology())
                .Build();
            return basisCurvesSrc;
        }
        if (name == HdPrimvarsSchemaTokens->primvars) {
            return _BoundsPrimvarsDataSource::New(_primSource);
        }
        if (name == HdExtentSchemaTokens->extent) {
            if (_primSource) {
                return _primSource->Get(name);
            }
            return nullptr;
        }
        return _PrimDataSource::Get(name);
    }

private:
    _BoundsPrimDataSource(
        const HdContainerDataSourceHandle &primSource)
      : _PrimDataSource(primSource)
    {
    }
};

/// Stand-in consisting of a basis curves prim called boundsCurves showing the edges
/// of the box defined by extent.
///
class _BoundsStandin : public UsdImaging_DrawModeStandin
{
public:
    _BoundsStandin(const SdfPath &path,
             const HdContainerDataSourceHandle &primSource)
      : UsdImaging_DrawModeStandin(path, primSource)
    {
    }

    const TfTokenVector
    _GetChildNames() const override {
        static const TfTokenVector childNames{ _primNameTokens->boundsCurves };
        return childNames;
    }

    TfToken
    _GetChildPrimType(const TfToken &name) const override {
        return HdPrimTypeTokens->basisCurves;
    }

    HdContainerDataSourceHandle
    _GetChildPrimSource(const TfToken &name) const override {
        return _BoundsPrimDataSource::New(_primSource);
    }

    void
    ProcessDirtyLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdSceneIndexObserver::DirtiedPrimEntries * entries,
        bool * needsRefresh) override
    {
        // Note that we do not remove the model locator from the dirty locators
        // we send to the scene index observer.

        // Check whether extent are dirty on input scene index
        const bool dirtyExtent =
            dirtyLocators.Intersects(
                HdExtentSchema::GetDefaultLocator());

        // Check whether model:drawModeColor is dirty.
        static const HdDataSourceLocator colorLocator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->drawModeColor);
        const bool dirtyColor =
            dirtyLocators.Intersects(colorLocator);
        
        if (dirtyExtent || dirtyColor) {
            HdDataSourceLocatorSet primDirtyLocators = dirtyLocators;
            if (dirtyExtent) {
                // Points depends on extent, so dirty it as well.
                static const HdDataSourceLocator pointsValue = 
                    HdPrimvarsSchema::GetPointsLocator()
                        .Append(HdPrimvarSchemaTokens->primvarValue);
                primDirtyLocators.insert(pointsValue);
            }
            if (dirtyColor) {
                // Display color is given by model:drawModeColor, so
                // dirty it as well.
                static const HdDataSourceLocator displayColor =
                    HdPrimvarsSchema::GetDefaultLocator()
                        .Append(HdTokens->displayColor);
                primDirtyLocators.insert(displayColor);
            }
            for (const SdfPath &path : GetChildPrimPaths()) {
                entries->push_back({path, primDirtyLocators});
            }
        } else {
            // Can just forward the dirty locators to the basis curves prim.
            for (const SdfPath &path : GetChildPrimPaths()) {
                entries->push_back({path, dirtyLocators});
            }
        }
    }

    TfToken GetDrawMode() const override {
        return UsdGeomTokens->bounds;
    }
};

}

namespace _OriginDrawMode {

////////////////////////////////////////////////////////////////////////////////
///
/// Implements stand-in for origin draw mode.
///
/// It is drawing three perpendicular line segments starting at the orgin with
/// unit length.
///
////////////////////////////////////////////////////////////////////////////////

TF_DEFINE_PRIVATE_TOKENS(
    _primNameTokens,

    (originCurves)
);

/// Data source for primvars.
///
/// Provides (on top of the base class):
/// -points (constant)
///
class _OriginPrimvarsDataSource : public _PrimvarsDataSource
{
public:
    HD_DECLARE_DATASOURCE(_OriginPrimvarsDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector result = _Concat(
            _PrimvarsDataSource::GetNames(),
            { HdPrimvarsSchemaTokens->points });
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdPrimvarsSchemaTokens->points) {
            static const HdDataSourceBaseHandle src =
                _PrimvarDataSource::New(
                    HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
                        { { 0, 0, 0 },
                          { 1, 0, 0 },
                          { 0, 1, 0 },
                          { 0, 0, 1 } }),
                    HdPrimvarSchemaTokens->vertex,
                    HdPrimvarSchemaTokens->point);
            return src;
        }
        return _PrimvarsDataSource::Get(name);
    }

private:
    _OriginPrimvarsDataSource(
        const HdContainerDataSourceHandle &primSource)
      : _PrimvarsDataSource(primSource)
    {
    }
};

HdContainerDataSourceHandle
_ComputeOriginTopology()
{
    // Origin: vertices are (0,0,0); (1,0,0); (0,1,0); (0,0,1)
    const VtIntArray curveIndices{ 0, 1, 0, 2, 0, 3};
    const VtIntArray curveVertexCounts{
            static_cast<int>(curveIndices.size()) };
    
    return HdBasisCurvesTopologySchema::Builder()
        .SetCurveVertexCounts(
                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    curveVertexCounts))
        .SetCurveIndices(
            HdRetainedTypedSampledDataSource<VtIntArray>::New(
                curveIndices))
        .SetBasis(
            HdRetainedTypedSampledDataSource<TfToken>::New(
                HdTokens->bezier))
        .SetType(
            HdRetainedTypedSampledDataSource<TfToken>::New(
                HdTokens->linear))
        .SetWrap(
            HdRetainedTypedSampledDataSource<TfToken>::New(
                HdTokens->segmented))
        .Build();
}

/// Prim data source.
///
/// Provides (on top of the base class):
/// - basis curves (constant using above topology)
/// - primvars (using above data source)
/// - extent (from the original prim source)
///
class _OriginPrimDataSource : public _PrimDataSource
{
public:
    HD_DECLARE_DATASOURCE(_OriginPrimDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector result = _Concat(
            _PrimDataSource::GetNames(),
            { HdBasisCurvesSchemaTokens->basisCurves,
              HdPrimvarsSchemaTokens->primvars,
              HdExtentSchemaTokens->extent });
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdBasisCurvesSchemaTokens->basisCurves) {
            static const HdDataSourceBaseHandle basisCurvesSrc =
                HdBasisCurvesSchema::Builder()
                .SetTopology(_ComputeOriginTopology())
                .Build();
            return basisCurvesSrc;
        }
        if (name == HdPrimvarsSchemaTokens->primvars) {
            return _OriginPrimvarsDataSource::New(_primSource);
        }
        if (name == HdExtentSchemaTokens->extent) {
            if (_primSource) {
                return _primSource->Get(name);
            }
            return nullptr;
        }
        return _PrimDataSource::Get(name);
    }

private:
    _OriginPrimDataSource(
        const HdContainerDataSourceHandle &primSource)
      : _PrimDataSource(primSource)
    {
    }
};

/// Stand-in consisting of a basis curves prim called originCurves showing three
/// perpendicular lines of unit length starting from the origin of the prim.
///
class _OriginStandin : public UsdImaging_DrawModeStandin
{
public:
    _OriginStandin(const SdfPath &path,
                   const HdContainerDataSourceHandle &primSource)
      : UsdImaging_DrawModeStandin(path, primSource)
    {
    }

    const TfTokenVector
    _GetChildNames() const override {
        static const TfTokenVector childNames{ _primNameTokens->originCurves };
        return childNames;
    }

    TfToken
    _GetChildPrimType(const TfToken &name) const override {
        return HdPrimTypeTokens->basisCurves;
    }

    HdContainerDataSourceHandle
    _GetChildPrimSource(const TfToken &name) const override {
        return _OriginPrimDataSource::New(_primSource);
    }

    void ProcessDirtyLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdSceneIndexObserver::DirtiedPrimEntries * entries,
        bool * needsRefresh) override
    {
        // Note that we do not remove the model locator from the dirty locators
        // we send to the observer.

        // Check whether model:drawModeColor is dirty.
        static const HdDataSourceLocator colorLocator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->drawModeColor);
        const bool dirtyColor =
            dirtyLocators.Intersects(colorLocator);
        
        if (dirtyColor) {
            // Display color is given by model:drawModeColor, so
            // dirty it as well.
            HdDataSourceLocatorSet primDirtyLocators = dirtyLocators;
            static const HdDataSourceLocator displayColorValue =
                HdPrimvarsSchema::GetDefaultLocator()
                    .Append(HdTokens->displayColor);
            primDirtyLocators.insert(displayColorValue);
            for (const SdfPath &path : GetChildPrimPaths()) {
                entries->push_back({path, primDirtyLocators});
            }
        } else {
            for (const SdfPath &path : GetChildPrimPaths()) {
                entries->push_back({path, dirtyLocators});
            }
        }
    }

    TfToken GetDrawMode() const override {
        return UsdGeomTokens->origin;
    }
};

}

namespace _CardsDrawMode {

TF_DEFINE_PRIVATE_TOKENS(
    _primNameTokens,

    (cardsMesh)
);

TF_DEFINE_PRIVATE_TOKENS(
    _primvarNameTokens,

    (cardsUv)
    (displayRoughness)
);

TF_DEFINE_PRIVATE_TOKENS(
    _materialNodeNameTokens,
    (cardSurface)
    (cardTexture)
    (cardUvCoords)
);

TF_DEFINE_PRIVATE_TOKENS(
    _imageMetadataTokens,

    (worldtoscreen)
    (worldToNDC)
);

// Helper to produce, e.g., FooXPosBar.
std::array<TfToken, 6>
_AddAxesToNames(const std::string &prefix, const std::string &postfix) {
    return {
        TfToken(prefix + "XPos" + postfix),
        TfToken(prefix + "YPos" + postfix),
        TfToken(prefix + "ZPos" + postfix),
        TfToken(prefix + "XNeg" + postfix),
        TfToken(prefix + "YNeg" + postfix),
        TfToken(prefix + "ZNeg" + postfix) };
}

using _CardsDataCacheSharedPtr = std::shared_ptr<class _CardsDataCache>;
using _MaterialsDict = std::unordered_map<
    TfToken, HdContainerDataSourceHandle, TfToken::HashFunctor>;

////////////////////////////////////////////////////////////////////////////////
///
/// Implements stand-in for cards draw mode.
///
/// It is providing a mesh with a material. The mesh consists of up to 6 quads.
/// Besides points, it has the vertex-varying cardsUv and face-varying
/// cardsTexAssgin - determining where to sample which of the up to 6 textures
/// that can be specified by the UsdImagingModelSchema.
///
/// Details vary based on the card geometry which is box, cross, or fromTexture.
///
////////////////////////////////////////////////////////////////////////////////

/// Caches data needed by the stand-in, created from primSource.
///
class _CardsDataCache
{
public:
    _CardsDataCache(const SdfPath &primPath, 
        const HdContainerDataSourceHandle &primSource)
      : _primPath(primPath)
      , _primSource(primSource)
    {
    }

    /// Card geometry, that is, box, cross, or fromTexture.
    TfToken
    GetCardGeometry() { return _GetCardsData()->cardGeometry; }

    /// Positions of mesh points not accounting for the extent.
    /// Note that the positions need to be transformed using the
    /// extent if card geometry is box or cross.
    VtVec3fArray
    GetPoints() { return _GetCardsData()->points; }

    /// If card geometry is fromTexture, the extent computed from
    /// the above points. Otherwise, nullptr - since we can just use
    /// the extent from the original prim source.
    HdContainerDataSourceHandle
    GetExtent() { return _GetCardsData()->extent; }

    /// The value for the cardsUV primvar.
    HdDataSourceBaseHandle
    GetUVs() { return _GetCardsData()->uvs; }

    /// The individual face geometry subsets.
    HdContainerDataSourceHandle
    GetGeomSubsets() { return _GetCardsData()->geomSubsets; }

    /// The topology.
    HdContainerDataSourceHandle
    GetMeshTopology() { return _GetCardsData()->meshTopology; }

    /// The materials.
    const _MaterialsDict&
    GetMaterials() { return _GetCardsData()->materials; }

    /// Reset the cache.
    void Reset() {
        static const std::shared_ptr<_CardsData> null;
        std::atomic_store(&_data, null);
    }

private:
    /// A helper extracing values from UsdImagingModelSchema.
    ///
    /// Note that the order of the six given textures is assumed to be:
    /// XPos, YPos, ZPos, XNeg, YNeg, ZNeg.
    ///
    /// Note that we store the values for cardGeometry, ... only for
    /// the sample at shutter offset 0.
    ///
    /// So we do not support motion-blur for these attributes.
    struct _SchemaValues
    {
        _SchemaValues(UsdImagingModelSchema schema);

        /// Card geometry, that is box, cross, or fromTexture.
        TfToken cardGeometry;
        /// For card geometry fromTexture, the worldToScreen matrix
        /// stored in the texture's metadata.
        GfMatrix4d worldToScreen[6];
        /// Was a non-empty asset path authored for the texture.
        std::bitset<6> hasTexture;
        /// Do we draw the face of the box.
        std::bitset<6> hasFace;

        /// The texture asset paths.
        HdAssetPathDataSourceHandle texturePaths[6];
        /// Data source providing the current drawModeColor.
        ///
        /// Note that this is a pointer to _DisplayColorVec3fDataSource
        /// rather than the data source returned by model:drawModeColor.
        /// That way, we do not need to update the pointer stored here
        /// when model:drawModeColor gets dirtied.
        HdVec3fDataSourceHandle drawModeColor;
    };

    /// The cached data.
    struct _CardsData
    {
        _CardsData(const _SchemaValues &values, const SdfPath &primPath);

        TfToken cardGeometry;
        VtVec3fArray points;
        HdContainerDataSourceHandle extent;
        HdDataSourceBaseHandle uvs;
        HdContainerDataSourceHandle geomSubsets;
        HdContainerDataSourceHandle meshTopology;
        _MaterialsDict materials;
        
    private:
        static
        VtVec3fArray
        _ComputePoints(const _SchemaValues &values);
        static
        VtVec2fArray
        _ComputeUVs(const _SchemaValues &values);
        static
        HdContainerDataSourceHandle
        _ComputeGeomSubsets(const _SchemaValues &values, 
            const SdfPath &primPath);
        static const
        _MaterialsDict
        _ComputeMaterials(const _SchemaValues &values);
        static
        HdContainerDataSourceHandle
        _ComputeExtent(
            const TfToken &cardGeometry,
            const VtVec3fArray &points);
    };

    /// Thread-safe way to get the cached cards data.
    std::shared_ptr<_CardsData> _GetCardsData() {
        if (auto cached = std::atomic_load(&_data)) {
            return cached;
        }
        auto data = std::make_shared<_CardsData>(
            _SchemaValues(UsdImagingModelSchema::GetFromParent(_primSource)),
            _primPath
        );

        std::atomic_store(&_data, data);
        return data;
    }

    std::shared_ptr<_CardsData> _data;
    SdfPath const _primPath;
    HdContainerDataSourceHandle const _primSource;
};

template <class Vec>
bool
_ConvertToMatrix(const Vec &mvec, GfMatrix4d *mat)
{
    if (mvec.size() == 16) {
        mat->Set(mvec[ 0], mvec[ 1], mvec[ 2], mvec[ 3],
                 mvec[ 4], mvec[ 5], mvec[ 6], mvec[ 7],
                 mvec[ 8], mvec[ 9], mvec[10], mvec[11],
                 mvec[12], mvec[13], mvec[14], mvec[15]);
        return true;
    }

    TF_WARN(
        "worldtoscreen metadata expected 16 values, got %zu",
        mvec.size());
    return false;
}

/// Open image to extract worldtoscreen matrix.
bool
GetWorldToScreenFromImageMetadata(
    HdAssetPathDataSourceHandle const &src,
    GfMatrix4d * const mat)
{
    if (!src) {
        return false;
    }

    const SdfAssetPath asset = src->GetTypedValue(0.0f);

    // If the literal path is empty, ignore this attribute.
    if (asset.GetAssetPath().empty()) {
        return false;
    }

    std::string file = asset.GetResolvedPath();
    // Fallback to the literal path if it couldn't be resolved.
    if (file.empty()) {
        file = asset.GetAssetPath();
    }

    HioImageSharedPtr const img = HioImage::OpenForReading(file);
    if (!img) {
        return false;
    }

    // Read the "worldtoscreen" metadata. This metadata specifies a 4x4
    // matrix but may be given as any the following data types, since
    // some image formats may support certain metadata types but not others.
    //
    // - std::vector<float> or std::vector<double> with 16 elements
    //   in row major order.
    // - GfMatrix4f or GfMatrix4d
    VtValue worldtoscreen;

    // XXX: OpenImageIO >= 2.2 no longer flips 'worldtoscreen' with 'worldToNDC'
    // on read and write, so assets where 'worldtoscreen' was written with > 2.2
    // have 'worldToNDC' actually in the metadata, and OIIO < 2.2 would read
    // and return 'worldToNDC' from the file in response to a request for 
    // 'worldtoscreen'. OIIO >= 2.2 no longer does either, so 'worldtoscreen'
    // gets written as 'worldtoscreen' and returned when asked for
    // 'worldtoscreen'. Issues only arise when trying to read 'worldtoscreen'
    // from an asset written with OIIO < 2.2, when the authoring program told
    // OIIO to write it as 'worldtoscreen'. Old OIIO flipped it to 'worldToNDC'.
    // So new OIIO needs to read 'worldToNDC' to retrieve it.
    //
    // See https://github.com/OpenImageIO/oiio/pull/2609
    //
    // OIIO's change is correct -- the two metadata matrices have different
    // semantic meanings, and should not be conflated. Unfortunately, users will
    // have to continue to conflate them for a while as assets transition into
    // vfx2022 (which uses OIIO 2.3). So we will need to check for both.
    
    if (!img->GetMetadata(_imageMetadataTokens->worldtoscreen, &worldtoscreen)) {
        if (img->GetMetadata(_imageMetadataTokens->worldToNDC, &worldtoscreen)) {
            TF_WARN("The texture asset '%s' may have been authored by an "
            "earlier version of the VFX toolset. To silence this warning, "
            "please regenerate the asset with the current toolset.",
            file.c_str());
        } else {
            TF_WARN("The texture asset '%s' lacks a worldtoscreen matrix in "
            "metadata. Cards draw mode may not appear as expected.", 
            file.c_str());
            return false;
        }
    }
    
    if (worldtoscreen.IsHolding<std::vector<float>>()) {
        return _ConvertToMatrix(
            worldtoscreen.UncheckedGet<std::vector<float>>(), mat);
    } else if (worldtoscreen.IsHolding<std::vector<double>>()) {
        return _ConvertToMatrix(
            worldtoscreen.UncheckedGet<std::vector<double>>(), mat);
    } else if (worldtoscreen.IsHolding<GfMatrix4f>()) {
        *mat = GfMatrix4d(worldtoscreen.UncheckedGet<GfMatrix4f>());
        return true;
    } else if (worldtoscreen.IsHolding<GfMatrix4d>()) {
        *mat = worldtoscreen.UncheckedGet<GfMatrix4d>();
        return true;
    }
    TF_WARN("worldtoscreen metadata holding unexpected type '%s'",
        worldtoscreen.GetTypeName().c_str());
    return false;
}

_CardsDataCache::_SchemaValues::_SchemaValues(UsdImagingModelSchema schema)
{
    if (HdTokenDataSourceHandle src = schema.GetCardGeometry()) {
        cardGeometry = src->GetTypedValue(0.0f);
    }

    // texturePaths, hasTexture, and hasFace are all in this order:
    // [ XPos, YPos, ZPos, XNeg, YNeg, ZNeg ]

    texturePaths[0] = schema.GetCardTextureXPos();
    texturePaths[1] = schema.GetCardTextureYPos();
    texturePaths[2] = schema.GetCardTextureZPos();
    texturePaths[3] = schema.GetCardTextureXNeg();
    texturePaths[4] = schema.GetCardTextureYNeg();
    texturePaths[5] = schema.GetCardTextureZNeg();

    if (cardGeometry == UsdGeomTokens->fromTexture) {
        for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 2; j++) {
                const size_t k = i + 3 * j;
                if (GetWorldToScreenFromImageMetadata(
                        texturePaths[k], &worldToScreen[k])) {
                    hasTexture[k] = true;
                    hasFace[k] = true;
                }
            }
        }
    } else {
        for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 2; j++) {
                // k and l are indices of opposite faces of the box.
                const size_t k = i + 3 * j;
                const size_t l = i + 3 * (1 - j);
                if (HdAssetPathDataSourceHandle const src = texturePaths[k]) {
                    if (!src->GetTypedValue(0.0f).GetAssetPath().empty()) {
                        hasTexture[k] = true;
                        // If we have a texture for one face, we also draw
                        // the opposite face (using the same texture if only
                        // one texture for a pair of opposite faces was
                        // specified).
                        hasFace[k] = true;
                        hasFace[l] = true;
                    }
                }
            }
        }
        // If no texture was given, force all faces drawing the box in the
        // draw mode color.
        if (hasFace.none()) {
            hasFace.set();
        }
    }

    drawModeColor = _DisplayColorVec3fDataSource::New(schema);
}

VtIntArray
_Range(const size_t n)
{
    VtIntArray result(n);
    for (size_t i = 0; i < n; i++) {
        result[i] = i;
    }
    return result;
}

// Creates topology consisting of a quad spanned by vertices
// 0, 1, 2, 3 and 4, 5, 6, 7 ... and 4*(n-1), 4*(n-1)+1, 4*(n-1)+2, 4*(n-1)+3.
HdContainerDataSourceHandle
_DisjointQuadTopology(const size_t n)
{
    return
        HdMeshTopologySchema::Builder()
            .SetFaceVertexCounts(
                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    VtIntArray(n, 4)))
            .SetFaceVertexIndices(
                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    _Range(4 * n)))
            .SetOrientation(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdMeshTopologySchemaTokens->rightHanded))
            .Build();
}

/// Compute cached data from schema values.
_CardsDataCache::_CardsData::_CardsData(const _SchemaValues &values,
    const SdfPath &primPath)
  : cardGeometry(values.cardGeometry)
  , points(_ComputePoints(values))
  , extent(_ComputeExtent(cardGeometry, points))
  , uvs(HdRetainedTypedSampledDataSource<VtVec2fArray>::New(_ComputeUVs(values)))
  , geomSubsets(_ComputeGeomSubsets(values, primPath))
  , meshTopology(_DisjointQuadTopology(values.hasFace.count()))
  , materials(_ComputeMaterials(values))
{
}

GfVec3f
_Transform(const GfVec3f &v, size_t i) {
    switch(i) {
    default: // Make compiler happy
    case 0:
        // For x-axis, quad is already in correct configuration.
        return v;
    case 1:
        // For y-axis, we rotate by 90 degrees about z-axis.
        return { 1.0f - v[1], v[0], v[2] };
    case 2:
        // For z-axis, we rotate by 120 degrees about space diagonal.
        return { v[1], v[2], v[0] };
    }
}

VtVec3fArray
_CardsDataCache::_CardsData::_ComputePoints(const _SchemaValues &values)
{
    VtVec3fArray points;

    points.reserve(4 * values.hasFace.count());

    // Points are pushed for faces (that exist) in this order:
    // [ XPos, XNeg, YPos, YNeg, ZPos, ZNeg ]

    if (values.cardGeometry == UsdGeomTokens->fromTexture) {
        // This card geometry computes the points using the
        // metadata from the images.
        
        static const GfVec3f pts[4] = {
            GfVec3f( 1, -1, 0),
            GfVec3f(-1, -1, 0),
            GfVec3f(-1,  1, 0),
            GfVec3f( 1,  1, 0)
        };

        for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 2; j++) {
                const size_t k = i + 3 * j;
                if (values.hasFace[k]) {
                    const GfMatrix4d transform = values.worldToScreen[k].GetInverse();
                    for (size_t l = 0; l < 4; l++) {
                        points.push_back(transform.Transform(pts[l]));
                    }
                }
            }
        }
    } else {
        // Compute the points for the unit cube [0,1]x[0,1]x[0,1] here - the
        // _CardsPointsPrimvarValueDataSource will apply the extent.
        //
        // cardGeometry = box: draw the faces of that unit cube (here).
        // cardGeometry = cross: draw the orthogonal quads that divides the
        //                       unit cube in 8 equal cubes half-the size.
        //
        // For cardGeometry = cross, we draw two quads with the same vertices
        // but different orientations. We cull the back so that we do not see
        // z-fighting.
        
        // Start with the face of the cube parallel to the y-z-plane and with
        // outward-facing normal being the positive x-axis - or the quad parallel
        // to that face dividing the cube in two equal boxes.
        //
        const float x = values.cardGeometry == UsdGeomTokens->box ? 1.0f : 0.5f;
        const GfVec3f pts[4] = {
            { x, 1, 1 },
            { x, 0, 1 },
            { x, 0, 0 },
            { x, 1, 0 } };

        static const GfVec3f one(1.0f);

        // For each pair of opposite faces.
        for (size_t i = 0; i < 3; i++) {
            if (values.hasFace[i]) {
                // Process one face.
                for (size_t k = 0; k < 4; k++) {
                    // Apply transform so that face is suitable for
                    // required axis.
                    points.push_back(
                        _Transform(pts[k], i));
                }
            }
            if (values.hasFace[i + 3]) {
                // Process the opposite face.
                for (size_t k = 0; k < 4; k++) {
                    // To obtain the opposite face, we apply the point
                    // symmetry about the center of the box.
                    // We also reverse the order of the points.
                    points.push_back(
                        one - _Transform(pts[3 - k], i));
                }
            }
        }
    }

    return points;
}

HdContainerDataSourceHandle
_CardsDataCache::_CardsData::_ComputeGeomSubsets(
    const _SchemaValues &values, const SdfPath &primPath)
{
    static const std::array<TfToken, 6> subsetNameTokens = 
        _AddAxesToNames("subset", "");
    static const std::array<TfToken, 6> materialNameTokens = 
        _AddAxesToNames("subsetMaterial", "");
    
    TfToken purposeToken[] = { HdMaterialBindingsSchemaTokens->allPurpose };

    std::vector<TfToken> subsetNames;
    std::vector<HdDataSourceBaseHandle> subsets;

    // Do not generate subsets if there are no textures for any face.
    // The entire standin prim will use the renderer's fallback material, which 
    // should pick up displayColor and displayOpacity.
    if (values.hasTexture.count()) {

        // The face index we need to build the geomSubset depends on the order
        // in which we created the faces when building the points and on which
        // faces actually got created. So we need to iterate through the faces 
        // in the same order we used before, rather than the order of faces in
        // the values hasFace and hasTexture arrays. The index variable i in
        // this loop shall be the former, and vi will be the recovered index
        // into the values arrays.

        // Face insertion order: [x+,x-,y+,y-,z+,z-] (some may be skipped)
        // Face order in values: [x+,y+,z+,x-,y-,z-] (all are present)

        // Token order in materialNameTokens and subsetNameTokens
        // is the same as in values, so use vi to access those too.
        
        for (size_t i = 0; i < 6; i++) {
            const size_t vi = (i % 2 == 0 ? 0 : 3) + i / 2;
            if (values.hasFace[vi]) {
                static HdTokenDataSourceHandle const typeSource =
                    HdGeomSubsetSchema::BuildTypeDataSource(
                        HdGeomSubsetSchemaTokens->typeFaceSet);
                const int subsetIndex(subsets.size());
                // use the opposite face's material if no texture for this face
                const size_t matIndex = values.hasTexture[vi] ? vi : (vi + 3) % 6;
                static const TfToken purposes[] = {
                    HdMaterialBindingsSchemaTokens->allPurpose
                };
                const SdfPath materialPath =
                    // geomSubset's materialBinding path must be absolute
                    primPath.AppendChild(materialNameTokens[matIndex]);
                HdDataSourceBaseHandle const materialBindingSources[] = {
                    HdMaterialBindingSchema::Builder()
                        .SetPath(
                            HdRetainedTypedSampledDataSource<SdfPath>::New(
                                materialPath))
                        .Build()
                };

                subsetNames.push_back(subsetNameTokens[vi]);
                subsets.push_back(
                    HdOverlayContainerDataSource::New(
                        HdGeomSubsetSchema::Builder()
                            .SetType(typeSource)
                            .SetIndices(
                                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                                    { subsetIndex }))
                            .Build(),
                        HdRetainedContainerDataSource::New(
                            HdMaterialBindingsSchema::GetSchemaToken(),
                            HdMaterialBindingsSchema::BuildRetained(
                                TfArraySize(purposes),
                                purposes,
                                materialBindingSources))));
                        
            }
        }
    }

    if (subsetNames.empty()) {
        return nullptr;
    } else {
        return HdRetainedContainerDataSource::New(
            subsetNames.size(), subsetNames.data(), subsets.data());
    }
}

HdContainerDataSourceHandle
_CardsDataCache::_CardsData::_ComputeExtent(
    const TfToken &cardGeometry,
    const VtVec3fArray &points)
{
    if (cardGeometry != UsdGeomTokens->fromTexture) {
        // box and cross get extent from original prim.
        return nullptr;
    }

    // Compute extent from points.
    GfRange3d extent;
    for (const GfVec3f &pt : points) {
        extent.UnionWith(pt);
    }

    return
        HdExtentSchema::Builder()
            .SetMin(
                HdRetainedTypedSampledDataSource<GfVec3d>::New(
                    extent.GetMin()))
            .SetMax(
                HdRetainedTypedSampledDataSource<GfVec3d>::New(
                    extent.GetMax()))
        .Build();
}

GfVec2f
_GetUV(const float u, const float v,
       const bool flipU, const bool flipV)
{
    return GfVec2f(flipU ? 1.0f - u : u, flipV ? 1.0f - v : v);
}

void
_FillUVs(const bool flipU, const bool flipV,
         VtVec2fArray * uvs)
{
    uvs->push_back(_GetUV(1.0f, 1.0f, flipU, flipV));
    uvs->push_back(_GetUV(0.0f, 1.0f, flipU, flipV));
    uvs->push_back(_GetUV(0.0f, 0.0f, flipU, flipV));
    uvs->push_back(_GetUV(1.0f, 0.0f, flipU, flipV));
}

VtVec2fArray
_CardsDataCache::_CardsData::_ComputeUVs(const _SchemaValues &values)
{
    VtVec2fArray uvs;

    uvs.reserve(4 * values.hasFace.count());

    if (values.cardGeometry == UsdGeomTokens->fromTexture) {
        // fromTexture always uses same UVs.
        for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 2; j++) {
                const size_t k = i + 3 * j;
                if (values.hasFace[k]) {
                    _FillUVs(false, false, &uvs);
                }
            }
        }
    } else {
        for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 2; j++) {
                const size_t k = i + 3 * j;
                if (values.hasFace[k]) {
                    // If we do not have a texture for this face of the
                    // cube (or cross) and use the texture specified for
                    // the opposite face, flip coordinates.
                    _FillUVs(!values.hasTexture[k], false, &uvs);
                }
            }
        }

        // z-Axis is treated with a similar idea, but a bit special.
        if (values.hasFace[2]) {
            _FillUVs(false, !values.hasTexture[2], &uvs);
        }
        if (values.hasFace[5]) {
            _FillUVs(true,  values.hasTexture[5],  &uvs);
        }
    }

    return uvs;
}

// Compute a material connection to given output of given node.
HdDataSourceBaseHandle
_ComputeConnection(const TfToken &nodeName, const TfToken &outputName)
{
    HdDataSourceBaseHandle srcs[] = { 
        HdMaterialConnectionSchema::Builder()
            .SetUpstreamNodePath(
                HdRetainedTypedSampledDataSource<TfToken>::New(nodeName))
            .SetUpstreamNodeOutputName(
                HdRetainedTypedSampledDataSource<TfToken>::New(outputName))
            .Build() };
    return
        HdRetainedSmallVectorDataSource::New(TfArraySize(srcs), srcs);
}

// Create texture reader node using cardsUv primvar for coordinates
// and the given data sources for the file path and fallback value
// (fallback value will be data source returning model:drawModeColor).
HdDataSourceBaseHandle
_CardsTextureNode(const HdAssetPathDataSourceHandle &file,
                  const HdDataSourceBaseHandle &fallback)
{
    static const TfToken inputConnectionNames[] = { _UsdUVTextureTokens->st };
    const HdDataSourceBaseHandle inputConnections[] = {
        _ComputeConnection(
            _materialNodeNameTokens->cardUvCoords, 
            _UsdPrimvarReaderTokens->result) };

    static const TfToken paramsNames[] = {
        _UsdUVTextureTokens->wrapS,
        _UsdUVTextureTokens->wrapT,
        _UsdUVTextureTokens->fallback,
        _UsdUVTextureTokens->file,
        _UsdUVTextureTokens->st
    };
    const HdDataSourceBaseHandle paramsValues[] = {
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<TfToken>::New(
                _UsdUVTextureTokens->clamp))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<TfToken>::New(
                _UsdUVTextureTokens->clamp))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdSampledDataSource::Cast(fallback))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(file)
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<TfToken>::New(
                _primvarNameTokens->cardsUv))
            .Build()
    };

    return
        HdMaterialNodeSchema::Builder()
            .SetNodeIdentifier(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    UsdImagingTokens->UsdUVTexture))
            .SetParameters(
                HdRetainedContainerDataSource::New(
                    TfArraySize(paramsNames),
                    paramsNames,
                    paramsValues))
            .SetInputConnections(
                HdRetainedContainerDataSource::New(
                    TfArraySize(inputConnectionNames),
                    inputConnectionNames,
                    inputConnections))
            .Build();
}

HdDataSourceBaseHandle
_CardsSurfaceNode(const bool hasTexture, const HdDataSourceBaseHandle& fallback)
{
    static const HdTokenDataSourceHandle nodeIdentifier =
        HdRetainedTypedSampledDataSource<TfToken>::New(
            UsdImagingTokens->UsdPreviewSurface);
    static const HdDataSourceBaseHandle one =
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<float>::New(1.0f))
            .Build();
    static const HdDataSourceBaseHandle pointOne =
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<float>::New(0.1f))
            .Build();
    static const HdDataSourceBaseHandle fallbackParam = 
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdSampledDataSource::Cast(fallback))
            .Build();

    std::vector<TfToken> parameterNames;
    std::vector<HdDataSourceBaseHandle> parameters;
    std::vector<TfToken> inputConnectionNames;
    std::vector<HdDataSourceBaseHandle> inputConnections;

    if (hasTexture) {
        inputConnectionNames.push_back(_UsdPreviewSurfaceTokens->diffuseColor);
        inputConnections.push_back(_ComputeConnection(
            _materialNodeNameTokens->cardTexture,
            _UsdUVTextureTokens->rgb));
        inputConnectionNames.push_back(_UsdPreviewSurfaceTokens->opacity);
        inputConnections.push_back(_ComputeConnection(
            _materialNodeNameTokens->cardTexture,
            _UsdUVTextureTokens->a));

        // opacityThreshold must be > 0 to achieve desired performance for
        // cutouts in storm, but will produce artifacts around the edges of
        // cutouts in both storm and prman. Per the preview surface spec,
        // cutouts are not combinable with translucency/partial presence.
        parameterNames.push_back(_UsdPreviewSurfaceTokens->opacityThreshold);
        parameters.push_back(pointOne);
    } else {
        parameterNames.push_back(_UsdPreviewSurfaceTokens->diffuseColor);
        parameters.push_back(fallbackParam);
        parameterNames.push_back(_UsdPreviewSurfaceTokens->opacity);
        parameters.push_back(one);
    }

    return HdMaterialNodeSchema::Builder()
        .SetNodeIdentifier(nodeIdentifier)
        .SetParameters(
            HdRetainedContainerDataSource::New(
                parameterNames.size(),
                parameterNames.data(),
                parameters.data()))
        .SetInputConnections(
            HdRetainedContainerDataSource::New(
                inputConnectionNames.size(),
                inputConnectionNames.data(),
                inputConnections.data()))
        .Build();
}

HdDataSourceBaseHandle
_CardsUVNode()
{
    static const TfToken paramsNames[] = { _UsdPrimvarReaderTokens->varname };
    const HdDataSourceBaseHandle paramsValues[] = {
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<TfToken>::New(
                _primvarNameTokens->cardsUv))
            .Build()
    };
    return HdMaterialNodeSchema::Builder()
        .SetNodeIdentifier(
            HdRetainedTypedSampledDataSource<TfToken>::New(
                UsdImagingTokens->UsdPrimvarReader_float2))
        .SetParameters(
            HdRetainedContainerDataSource::New(
                TfArraySize(paramsNames),
                paramsNames,
                paramsValues))
        .Build();
}

const _MaterialsDict
_CardsDataCache::_CardsData::_ComputeMaterials(const _SchemaValues &values)
{
    static const std::array<TfToken, 6> materialNameTokens = 
        _AddAxesToNames("subsetMaterial", "");

    const HdDataSourceBaseHandle vec4Fallback = 
        _Vec4fFromVec3fDataSource::New(values.drawModeColor, 1.0f);

    _MaterialsDict materials;
    
    // do not generate any materials if there are no textures for any face
    if (values.hasTexture.count()) {
        for (auto i = 0; i < 6; ++i) {
            // only generate materials for faces that have textures.
            // textureless faces that are opposite textured faces will
            // use the same material as the textured face.
            if (values.hasTexture[i]) {

                std::vector<TfToken> nodeNames;
                std::vector<HdDataSourceBaseHandle> nodes;
                std::vector<TfToken> networkNames;
                std::vector<HdDataSourceBaseHandle> networks;

                // Card Surface
                nodeNames.push_back(_materialNodeNameTokens->cardSurface);
                nodes.push_back(
                    _CardsSurfaceNode(
                        values.hasTexture[i], values.drawModeColor));
                // Card Texture
                nodeNames.push_back(_materialNodeNameTokens->cardTexture);
                nodes.push_back(
                    _CardsTextureNode(
                        values.texturePaths[i], vec4Fallback));
                // Card UvCords
                nodeNames.push_back(_materialNodeNameTokens->cardUvCoords);
                nodes.push_back(_CardsUVNode());

                // Connect surface terminal to the UsdPreviewSurface node.
                const HdContainerDataSourceHandle terminals =
                    HdRetainedContainerDataSource::New(
                        HdMaterialTerminalTokens->surface,
                        HdMaterialConnectionSchema::Builder()
                            .SetUpstreamNodePath(
                                HdRetainedTypedSampledDataSource<TfToken>::New(
                                    _materialNodeNameTokens->cardSurface))
                            .SetUpstreamNodeOutputName(
                                HdRetainedTypedSampledDataSource<TfToken>::New(
                                    HdMaterialTerminalTokens->surface))
                            .Build());
                
                networkNames.push_back(HdMaterialSchemaTokens->universalRenderContext);
                networks.push_back(HdMaterialNetworkSchema::Builder()
                    .SetNodes(
                        HdRetainedContainerDataSource::New(
                            nodeNames.size(), nodeNames.data(), nodes.data()))
                    .SetTerminals(terminals)
                    .Build());
                materials.emplace(
                    materialNameTokens[i],
                    HdRetainedContainerDataSource::New(
                        HdMaterialSchemaTokens->material,
                        HdMaterialSchema::BuildRetained(
                            networkNames.size(),
                            networkNames.data(),
                            networks.data())));
            }
        }
    }
    return materials;
}

/// Data source for primvars:points:primvarValue.
///
/// Uses _CardsDataCache and applies extent if card geometry is not fromTexture.
///
class _CardsPointsPrimvarValueDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_CardsPointsPrimvarValueDataSource);

    VtValue GetValue(Time shutterOffset) {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(Time shutterOffset) {
        VtVec3fArray pts = _dataCache->GetPoints();
        if (_dataCache->GetCardGeometry() == UsdGeomTokens->fromTexture) {
            return pts;
        }

        HdExtentSchema extentSchema =
            HdExtentSchema::GetFromParent(_primSource);
        GfVec3f min(0.0f);
        if (HdVec3dDataSourceHandle src = extentSchema.GetMin()) {
            min = GfVec3f(src->GetTypedValue(shutterOffset));
        }
        GfVec3f max(0.0f);
        if (HdVec3dDataSourceHandle src = extentSchema.GetMax()) {
            max = GfVec3f(src->GetTypedValue(shutterOffset));
        }

        for (size_t i = 0; i < pts.size(); i++) {
            GfVec3f &pt = pts[i];
            pt = { min[0] * (1.0f - pt[0]) + max[0] * pt[0],
                   min[1] * (1.0f - pt[1]) + max[1] * pt[1],
                   min[2] * (1.0f - pt[2]) + max[2] * pt[2] };
        }
        
        return pts;
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> * outSampleTimes)
    {
        HdExtentSchema extentSchema =
            HdExtentSchema::GetFromParent(_primSource);

        HdSampledDataSourceHandle srcs[] = {
            extentSchema.GetMin(), extentSchema.GetMax() };

        return HdGetMergedContributingSampleTimesForInterval(
            TfArraySize(srcs), srcs,
            startTime, endTime, outSampleTimes);
    }

private:
    _CardsPointsPrimvarValueDataSource(
        const HdContainerDataSourceHandle &primSource,
        const _CardsDataCacheSharedPtr &dataCache)
      : _primSource(primSource)
      , _dataCache(dataCache)
    {
    }

    HdContainerDataSourceHandle _primSource;
    _CardsDataCacheSharedPtr _dataCache;
};

/// Data source for primvars.
///
/// Provides (on top of the base class):
/// - points (using above data source and _CardsDataCache)
/// - cardsUv (from _CardsDataCache)
/// - displayRoughness (constant)

class _CardsPrimvarsDataSource : public _PrimvarsDataSource
{
public:
    HD_DECLARE_DATASOURCE(_CardsPrimvarsDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector result = _Concat(
            _PrimvarsDataSource::GetNames(),
            { HdPrimvarsSchemaTokens->points,
              _primvarNameTokens->cardsUv,
              _primvarNameTokens->displayRoughness });
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdPrimvarsSchemaTokens->points) {
            return 
                _PrimvarDataSource::New(
                    _CardsPointsPrimvarValueDataSource::New(
                        _primSource, _dataCache),
                    HdPrimvarSchemaTokens->vertex,
                    HdPrimvarSchemaTokens->point);
        }
        if (name == _primvarNameTokens->cardsUv) {
            return 
                _PrimvarDataSource::New(
                    _dataCache->GetUVs(),
                    HdPrimvarSchemaTokens->vertex,
                    TfToken());
        }
        if (name == _primvarNameTokens->displayRoughness) {
            static HdDataSourceBaseHandle const src =
                _PrimvarDataSource::New(
                    HdRetainedTypedSampledDataSource<VtFloatArray>::New(
                        VtFloatArray{1.0f}),
                    HdPrimvarSchemaTokens->constant,
                    TfToken());
            return src;
        }

        return _PrimvarsDataSource::Get(name);
    }

private:
    _CardsPrimvarsDataSource(
        const HdContainerDataSourceHandle &primSource,
        const _CardsDataCacheSharedPtr &dataCache)
      : _PrimvarsDataSource(primSource)
      , _dataCache(dataCache)
    {
    }

    _CardsDataCacheSharedPtr _dataCache;
};

class _CardsPrimDataSource : public _PrimDataSource
{
public:
    HD_DECLARE_DATASOURCE(_CardsPrimDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector result = _Concat(
            _PrimDataSource::GetNames(),
            { 
                HdMeshSchemaTokens->mesh,
                HdPrimvarsSchemaTokens->primvars,
                HdExtentSchemaTokens->extent
            });
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdMeshSchemaTokens->mesh) {
            return HdMeshSchema::Builder()
                .SetTopology(_dataCache->GetMeshTopology())
                .SetDoubleSided(
                    HdRetainedTypedSampledDataSource<bool>::New(false))
                .SetGeomSubsets(_dataCache->GetGeomSubsets())
                .Build();
        }
        if (name == HdPrimvarsSchemaTokens->primvars) {
            return _CardsPrimvarsDataSource::New(_primSource, _dataCache);
        }
        if (name == HdExtentSchemaTokens->extent) {
            if (HdContainerDataSourceHandle const src =
                    _dataCache->GetExtent()) {
                return src;
            }
            return HdExtentSchema::GetFromParent(_primSource).GetContainer();
        }

        return _PrimDataSource::Get(name);
    }

private:
    _CardsPrimDataSource(
        const SdfPath &path,
        const HdContainerDataSourceHandle &primSource,
        const _CardsDataCacheSharedPtr &dataCache)
      : _PrimDataSource(primSource)
      , _path(path)
      , _dataCache(dataCache)
    {
    }

    const SdfPath _path;
    _CardsDataCacheSharedPtr const _dataCache;
};

HdDataSourceLocatorSet
_ComputeMaterialColorInputLocators()
{
    static const auto nodes = 
        HdDataSourceLocator(HdMaterialSchemaTokens->universalRenderContext)
            .Append(HdMaterialNetworkSchemaTokens->nodes);
    return {
        nodes
        .Append(_materialNodeNameTokens->cardTexture)
        .Append(HdMaterialNodeSchemaTokens->parameters)
        .Append(_UsdUVTextureTokens->fallback),
        nodes
        .Append(_materialNodeNameTokens->cardSurface)
        .Append(HdMaterialNodeSchemaTokens->parameters)
        .Append(_UsdPreviewSurfaceTokens->diffuseColor),
        nodes
        .Append(_materialNodeNameTokens->cardSurface)
        .Append(HdMaterialNodeSchemaTokens->parameters)
        .Append(_UsdPreviewSurfaceTokens->opacity)
    };
};


class _CardsStandin : public UsdImaging_DrawModeStandin
{
public:
    _CardsStandin(const SdfPath &path,
             const HdContainerDataSourceHandle &primSource)
      : UsdImaging_DrawModeStandin(path, primSource)
      , _dataCache(std::make_shared<_CardsDataCache>(path, primSource))
    {
    }

    const TfTokenVector
    _GetChildNames() const override {
        TfTokenVector names = { _primNameTokens->cardsMesh };
        const _MaterialsDict mats = _dataCache->GetMaterials();
        for (const auto &kv : mats) {
            names.push_back(kv.first);
        }
        return names;
    }

    TfToken
    _GetChildPrimType(const TfToken &name) const override {
        if (name == _primNameTokens->cardsMesh) {
            return HdPrimTypeTokens->mesh;
        }
        return HdPrimTypeTokens->material;
    }

    HdContainerDataSourceHandle
    _GetChildPrimSource(const TfToken &name) const override {
        // We rely on the consumer calling HdSceneIndex::GetPrim()
        // again when we send a prim dirtied for the material prims
        // with an empty data source locators.
        const _MaterialsDict &materials = _dataCache->GetMaterials();
        if (materials.count(name)) {
            return materials.at(name);
        }
        return _CardsPrimDataSource::New(_path, _primSource, _dataCache);
    }

    void ProcessDirtyLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdSceneIndexObserver::DirtiedPrimEntries * entries,
        bool * needsRefresh) override
    {
        // Note that we do not remove the model locator from the dirty locators
        // we send to the observer.

        static const HdDataSourceLocatorSet cardLocators{
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardGeometry),
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardTextureXPos),
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardTextureYPos),
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardTextureZPos),
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardTextureXNeg),
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardTextureYNeg),
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardTextureZNeg) };
        
        // Blast the entire thing.
        if (dirtyLocators.Intersects(cardLocators)) {
            (*needsRefresh) = true;
            for (const SdfPath &path : GetChildPrimPaths()) {
                static const HdDataSourceLocator empty;
                entries->push_back({path, empty});
            }
            _dataCache->Reset();
            return;
        }

        static const HdDataSourceLocator colorLocator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->drawModeColor);
        if (dirtyLocators.Intersects(colorLocator)) {
            HdDataSourceLocatorSet primDirtyLocators = dirtyLocators;
            static const HdDataSourceLocator displayColorValue =
                HdPrimvarsSchema::GetDefaultLocator()
                    .Append(HdTokens->displayColor)
                    .Append(HdPrimvarSchemaTokens->primvarValue);
            primDirtyLocators.insert(displayColorValue);
            entries->push_back(
                {_path.AppendChild(_primNameTokens->cardsMesh),
                    primDirtyLocators});
            static const HdDataSourceLocatorSet materialColorInputs =
                _ComputeMaterialColorInputLocators();
            for (const auto &kv : _dataCache->GetMaterials()) {
                entries->push_back(
                    { _path.AppendChild(kv.first), materialColorInputs });
            }
            return;
        }

        entries->push_back(
            {_path.AppendChild(_primNameTokens->cardsMesh), dirtyLocators});
    }

    TfToken GetDrawMode() const override {
        return UsdGeomTokens->cards;
    }

private:
    _CardsDataCacheSharedPtr _dataCache;
};

}

}

////////////////////////////////////////////////////////////////////////////////
///
/// Code to instantiate DrawModeStandin.
///
///////////////////////////////////////////////////////////////////////////////

UsdImaging_DrawModeStandinSharedPtr
UsdImaging_GetDrawModeStandin(const TfToken &drawMode,
                              const SdfPath &path,
                              const HdContainerDataSourceHandle &primSource)
{
    if (drawMode.IsEmpty()) {
        return nullptr;
    }
    if (drawMode == UsdGeomTokens->bounds) {
        using _BoundsDrawMode::_BoundsStandin;
        return std::make_shared<_BoundsStandin>(path, primSource);
    }
    if (drawMode == UsdGeomTokens->origin) {
        using _OriginDrawMode::_OriginStandin;
        return std::make_shared<_OriginStandin>(path, primSource);
    }
    if (drawMode == UsdGeomTokens->cards) {
        using _CardsDrawMode::_CardsStandin;
        return std::make_shared<_CardsStandin>(path, primSource);
    }
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
