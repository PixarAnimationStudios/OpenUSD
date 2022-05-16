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

#include "pxr/usdImaging/usdImagingGL/drawModeStandin.h"

#include "pxr/usdImaging/usdImagingGL/package.h"
#include "pxr/usdImaging/usdImaging/modelSchema.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/basisCurvesSchema.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/meshSchema.h"
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

#include <functional>
#include <bitset>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// UsdImagingGL_DrawModeStandin implementation
///
////////////////////////////////////////////////////////////////////////////////

const HdSceneIndexPrim &
UsdImagingGL_DrawModeStandin::GetPrim() const
{
    static HdSceneIndexPrim empty{ TfToken(), nullptr };
    return empty;
}

HdSceneIndexPrim
UsdImagingGL_DrawModeStandin::GetChildPrim(const TfToken &name) const
{
    return { _GetChildPrimType(name), _GetChildPrimSource(name) };
}

SdfPathVector
UsdImagingGL_DrawModeStandin::GetChildPrimPaths() const
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
UsdImagingGL_DrawModeStandin::ComputePrimAddedEntries(
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
    (magFilter)
    (minFilter)
    (st)

    (linear)
    (linearMipmapLinear)

    (rgb)
    (a)
);

TF_DEFINE_PRIVATE_TOKENS(
    _UsdPrimvarReaderTokens,

    (fallback)
    (varname)
    (result)
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
/// so we can use the same pointer to _DisplayColorDataSource even if
/// model:darwModeColor was dirtied.
///
class _DisplayColorDataSource final : public HdVec3fDataSource
{
public:
    HD_DECLARE_DATASOURCE(_DisplayColorDataSource);

    VtValue GetValue(const Time shutterOffset) {
        if (HdVec3fDataSourceHandle src = _schema.GetDrawModeColor()) {
            return src->GetValue(shutterOffset);
        }
        return VtValue();
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
    _DisplayColorDataSource(const UsdImagingModelSchema schema)
      : _schema(schema)
    {
    }

    UsdImagingModelSchema _schema;
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

    bool Has(const TfToken &name) override {
        if (name == HdPrimvarSchemaTokens->primvarValue) {
            return true;
        }
        if (name == HdPrimvarSchemaTokens->interpolation) {
            return true;
        }
        if (name == HdPrimvarSchemaTokens->role) {
            return true;
        }
        return false;
    }

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
    bool Has(const TfToken &name) override {
        if (name == HdPrimvarsSchemaTokens->widths) {
            return true;
        }
        if (name == HdTokens->displayColor) {
            return true;
        }
        if (name == HdTokens->displayOpacity) {
            return true;
        }
        return false;
    }

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
            /// Instead, we store the _DisplayColorDataSource with the
            /// _PrimvarDataSource which pulls the drawModeColor from model
            /// every time it is needed.
            ///
            return _PrimvarDataSource::New(
                _DisplayColorDataSource::New(
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
    bool Has(const TfToken &name) override {
        if (name == HdXformSchemaTokens->xform) {
            return true;
        }
        if (name == HdPurposeSchemaTokens->purpose) {
            return true;
        }
        if (name == HdVisibilitySchemaTokens->visibility) {
            return true;
        }
        if (name == HdLegacyDisplayStyleSchemaTokens->displayStyle) {
            return true;
        }
        return false;
    }
    
    TfTokenVector GetNames() override {
        return {
            HdXformSchemaTokens->xform,
            HdPurposeSchemaTokens->purpose,
            HdVisibilitySchemaTokens->visibility,
            HdLegacyDisplayStyleSchemaTokens->displayStyle };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdXformSchemaTokens->xform) {
            if (_primSource) {
                return _primSource->Get(name);
            }
            return nullptr;
        }
        if (name == HdPurposeSchemaTokens->purpose) {
            if (_primSource) {
                return _primSource->Get(name);
            }
            return nullptr;
        }
        if (name == HdVisibilitySchemaTokens->visibility) {
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

    bool Has(const TfToken &name) override {
        if (name == HdPrimvarsSchemaTokens->points) {
            return true;
        }
        return _PrimvarsDataSource::Has(name);
    }

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

    bool Has(const TfToken &name) override {
        if (name == HdBasisCurvesSchemaTokens->basisCurves) {
            return true;
        }
        if (name == HdPrimvarsSchemaTokens->primvars) {
            return true;
        }
        if (name == HdExtentSchemaTokens->extent) {
            return true;
        }
        return _PrimDataSource::Has(name);
    }

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
class _BoundsStandin : public UsdImagingGL_DrawModeStandin
{
public:
    _BoundsStandin(const SdfPath &path,
             const HdContainerDataSourceHandle &primSource)
      : UsdImagingGL_DrawModeStandin(path, primSource)
    {
    }

    const TfTokenVector
    &_GetChildNames() const override {
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
        HdSceneIndexObserver::DirtiedPrimEntries * entries) override
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

    bool Has(const TfToken &name) override {
        if (name == HdPrimvarsSchemaTokens->points) {
            return true;
        }
        return _PrimvarsDataSource::Has(name);
    }

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

    bool Has(const TfToken &name) override {
        if (name == HdBasisCurvesSchemaTokens->basisCurves) {
            return true;
        }
        if (name == HdPrimvarsSchemaTokens->primvars) {
            return true;
        }
        if (name == HdExtentSchemaTokens->extent) {
            return true;
        }
        return _PrimDataSource::Has(name);
    }

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
class _OriginStandin : public UsdImagingGL_DrawModeStandin
{
public:
    _OriginStandin(const SdfPath &path,
                   const HdContainerDataSourceHandle &primSource)
      : UsdImagingGL_DrawModeStandin(path, primSource)
    {
    }

    const TfTokenVector
    &_GetChildNames() const override {
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
        HdSceneIndexObserver::DirtiedPrimEntries * entries) override
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
    (material)
);

TF_DEFINE_PRIVATE_TOKENS(
    _primvarNameTokens,

    (cardsTexAssign)
    (cardsUv)
    (displayRoughness)
);

TF_DEFINE_PRIVATE_TOKENS(
    _materialNodeNameTokens,

    (shader)
    (cardsTexAssign)
);

TF_DEFINE_PRIVATE_TOKENS(
    _inputConnectionNameTokens,

    (activeTexCard)
);

TF_DEFINE_PRIVATE_TOKENS(
    _imageMetadataTokens,

    (worldtoscreen)
);

using _CardsDataCacheSharedPtr = std::shared_ptr<class _CardsDataCache>;

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
    _CardsDataCache(const HdContainerDataSourceHandle &primSource)
      : _primSource(primSource)
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

    /// The value for the cardsTexAssign primvar.
    HdDataSourceBaseHandle
    GetTexAssign() { return _GetCardsData()->texAssigns; }

    /// The topology.
    HdContainerDataSourceHandle
    GetMeshTopology() { return _GetCardsData()->meshTopology; }

    /// The material.
    HdContainerDataSourceHandle
    GetMaterial() { return _GetCardsData()->material; }

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
        /// Note that this is a pointer to _DisplayColorDataSource
        /// rather than the data source returned by model:drawModeColor.
        /// That way, we do not need to update the pointer stored here
        /// when model:drawModeColor gets dirtied.
        HdDataSourceBaseHandle drawModeColor;
    };

    /// The cached data.
    struct _CardsData
    {
        _CardsData(const _SchemaValues &values);

        TfToken cardGeometry;
        VtVec3fArray points;
        HdContainerDataSourceHandle extent;
        HdDataSourceBaseHandle uvs;
        HdDataSourceBaseHandle texAssigns;
        HdContainerDataSourceHandle meshTopology;
        HdContainerDataSourceHandle material;
        
    private:
        static
        VtVec3fArray
        _ComputePoints(const _SchemaValues &values);
        static
        VtVec2fArray
        _ComputeUVs(const _SchemaValues &values);
        static
        VtIntArray
        _ComputeTexAssigns(const _SchemaValues &values);
        static
        HdContainerDataSourceHandle
        _ComputeMaterial(const _SchemaValues &values);
        static
        HdDataSourceBaseHandle
        _ComputeShaderNode(const _SchemaValues &values);
        static
        HdContainerDataSourceHandle
        _ComputeExtent(
            const TfToken &cardGeometry,
            const VtVec3fArray &points);
    };

    /// Thread-safe way to get the cached cards data.
    std::shared_ptr<_CardsData> _GetCardsData() {
        if (auto cached = std::atomic_load(&_data)) {
            return std::move(cached);
        }
        auto data = std::make_shared<_CardsData>(
            _SchemaValues(
                UsdImagingModelSchema::GetFromParent(
                    _primSource)));

        std::atomic_store(&_data, data);
        return std::move(data);
    }

    std::shared_ptr<_CardsData> _data;
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
    if (!img->GetMetadata(_imageMetadataTokens->worldtoscreen, &worldtoscreen)) {
        return false;
    }

    if (worldtoscreen.IsHolding<std::vector<float>>()) {
        return _ConvertToMatrix(
            worldtoscreen.UncheckedGet<std::vector<float>>(), mat);
    }
    if (worldtoscreen.IsHolding<std::vector<double>>()) {
        return _ConvertToMatrix(
            worldtoscreen.UncheckedGet<std::vector<double>>(), mat);
    }
    if (worldtoscreen.IsHolding<GfMatrix4f>()) {
        *mat = GfMatrix4d(worldtoscreen.UncheckedGet<GfMatrix4f>());
        return true;
    }
    if (worldtoscreen.IsHolding<GfMatrix4d>()) {
        *mat = worldtoscreen.UncheckedGet<GfMatrix4d>();
        return true;
    }

    TF_WARN(
        "worldtoscreen metadata holding unexpected type '%s'",
        worldtoscreen.GetTypeName().c_str());
    return false;
}

_CardsDataCache::_SchemaValues::_SchemaValues(UsdImagingModelSchema schema)
{
    if (HdTokenDataSourceHandle src = schema.GetCardGeometry()) {
        cardGeometry = src->GetTypedValue(0.0f);
    }

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

    drawModeColor = _DisplayColorDataSource::New(schema);
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
_CardsDataCache::_CardsData::_CardsData(const _SchemaValues &values)
  : cardGeometry(values.cardGeometry)
  , points(_ComputePoints(values))
  , extent(_ComputeExtent(cardGeometry, points))
  , uvs(
      HdRetainedTypedSampledDataSource<VtVec2fArray>::New(
          _ComputeUVs(values)))
  , texAssigns(
      HdRetainedTypedSampledDataSource<VtIntArray>::New(
          _ComputeTexAssigns(values)))
  , meshTopology(_DisjointQuadTopology(values.hasFace.count()))
  , material(_ComputeMaterial(values))
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
                        GfVec3f(1.0f) - _Transform(pts[3 - k], i));
                }
            }
        }
    }

    return points;
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

// Compute primvar cardsTexAssign determining which texture is used for
// which face.
VtIntArray
_CardsDataCache::_CardsData::_ComputeTexAssigns(const _SchemaValues &values)
{
    VtIntArray assigns;

    assigns.reserve(4 * values.hasFace.count());

    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 2; j++) {
            // k and l are indices of opposite faces of the box.
            const size_t k = i + 3 * j;
            const size_t l = i + 3 * (1 - j);
            if (values.hasFace[k]) {
                if (values.hasTexture[k]) {
                    // If we have a texture for this face, use it.
                    assigns.push_back(1 << k);
                } else {
                    // Otherwise, use texture specified for the opposite
                    // face.
                    assigns.push_back(1 << l);
                }
            }
        }
    }

    return assigns;
}

// Create primvar reader node for cardsTexAssign primvar specifying
// which texture to use.
HdDataSourceBaseHandle
_CardsTexAssignNode()
{
    return
        HdMaterialNodeSchema::Builder()
            .SetNodeIdentifier(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    UsdImagingTokens->UsdPrimvarReader_int))
            .SetParameters(
                HdRetainedContainerDataSource::New(
                    _UsdPrimvarReaderTokens->fallback,
                    HdRetainedTypedSampledDataSource<int>::New(0),
                    _UsdPrimvarReaderTokens->varname,
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        _primvarNameTokens->cardsTexAssign)))
            .Build();
}

// Create texture reader node using cardsUv primvar for coordinates
// and the given data sources for the file path and fallback value
// (fallback value will be data source returning model:drawModeColor).
HdDataSourceBaseHandle
_CardsTextureNode(const HdAssetPathDataSourceHandle &file,
                  const HdDataSourceBaseHandle &fallback)
                  
{
    return
        HdMaterialNodeSchema::Builder()
            .SetNodeIdentifier(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    UsdImagingTokens->UsdUVTexture))
            .SetParameters(
                HdRetainedContainerDataSource::New(
                    _UsdUVTextureTokens->fallback,
                    fallback,
                    _UsdUVTextureTokens->file,
                    file,
                    _UsdUVTextureTokens->magFilter,
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        _UsdUVTextureTokens->linear),
                    _UsdUVTextureTokens->minFilter,
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        _UsdUVTextureTokens->linearMipmapLinear),
                    _UsdUVTextureTokens->st,
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        _primvarNameTokens->cardsUv)))
            .Build();
}

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

// Get name of texture node for face i of box.
const TfToken &
_GetTextureNodeName(const size_t i) {
    static const std::array<TfToken, 6> names = _AddAxesToNames("cardTexture", "");
    return names[i];
}

// Compute a material connection to given output of given node.
HdDataSourceBaseHandle
_ComputeConnection(const TfToken &nodeName, const TfToken &outputName)
{
    HdDataSourceBaseHandle srcs[] = {
        HdMaterialConnectionSchema::Builder()
            .SetUpstreamNodePath(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    nodeName))
            .SetUpstreamNodeOutputName(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    outputName))
            .Build() };
    return
        HdRetainedSmallVectorDataSource::New(
            TfArraySize(srcs), srcs);
}

// Specialization for texture node.
std::array<HdDataSourceBaseHandle, 6>
_ComputeConnectionsToTextureNode(const TfToken &outputName)
{
    std::array<HdDataSourceBaseHandle, 6> result;
    for (size_t i = 0; i < 6; i++) {
        result[i] = _ComputeConnection(_GetTextureNodeName(i), outputName);
    }
    return result;
}

// Helpers for the shader in UsdImagingGLPackageDrawModeShader.

// Compute color input name of the shader node.
const TfToken &
_GetColorInputName(const size_t i) {
    static const std::array<TfToken, 6> names = _AddAxesToNames("texture", "Color");
    return names[i];
}

// Compute color input name of the shader node.
const TfToken &
_GetOpacityInputName(const size_t i) {
    static const std::array<TfToken, 6> names = _AddAxesToNames("texture", "Opacity");
    return names[i];
}

// Compute shader node using UsdImagingGLPackageDrawModeShader as implementation
// source.
HdDataSourceBaseHandle
_CardsDataCache::_CardsData::_ComputeShaderNode(const _SchemaValues &values)
{
    static const std::array<HdDataSourceBaseHandle, 6> colorInputConnections =
        _ComputeConnectionsToTextureNode(_UsdUVTextureTokens->rgb);

    static const std::array<HdDataSourceBaseHandle, 6> opacityInputConnections =
        _ComputeConnectionsToTextureNode(_UsdUVTextureTokens->a);

    static const HdDataSourceBaseHandle one =
        HdRetainedTypedSampledDataSource<float>::New(1.0f);

    static const HdDataSourceBaseHandle activeTexCardConnection =
        _ComputeConnection(
            _materialNodeNameTokens->cardsTexAssign,
            _UsdPrimvarReaderTokens->result);

    // The Sdr node using the given shader file as implementation source.
    static const SdrShaderNodeConstPtr sdrNode =
        SdrRegistry::GetInstance().GetShaderNodeFromAsset(
            SdfAssetPath(UsdImagingGLPackageDrawModeShader()),
            NdrTokenMap(),
            TfToken(),
            HioGlslfxTokens->glslfx);

    static const HdTokenDataSourceHandle nodeIdentifier =
        HdRetainedTypedSampledDataSource<TfToken>::New(
            sdrNode ? sdrNode->GetIdentifier() : TfToken());

    std::vector<TfToken> parameterNames;
    std::vector<HdDataSourceBaseHandle> parameters;

    // Connect primvar reader reading cardTexAssign.
    std::vector<TfToken> inputConnectionNames = {
        _inputConnectionNameTokens->activeTexCard
    };
    std::vector<HdDataSourceBaseHandle> inputConnections = {
        activeTexCardConnection
    };

    for (size_t i = 0; i < 6; i++) {
        if (values.hasTexture[i]) {
            // If we have a texture for a face of the box, connect the
            // inputs to the texture node.
            inputConnectionNames.push_back(_GetColorInputName(i));
            inputConnections.push_back(colorInputConnections[i]);
            inputConnectionNames.push_back(_GetOpacityInputName(i));
            inputConnections.push_back(opacityInputConnections[i]);
        } else {
            // Otherwise, set input to model:drawModeColor.
            parameterNames.push_back(_GetColorInputName(i));
            parameters.push_back(values.drawModeColor);
            parameterNames.push_back(_GetOpacityInputName(i));
            parameters.push_back(one);
        }
    }

    return
        HdMaterialNodeSchema::Builder()
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

HdContainerDataSourceHandle
_CardsDataCache::_CardsData::_ComputeMaterial(const _SchemaValues &values)
{
    static const HdDataSourceBaseHandle cardsTexAssignNode =
        _CardsTexAssignNode();

    // Create material network of the shader node, the primvar reader for
    // cardsTexAssign, ...
    std::vector<TfToken> nodeNames = {
        _materialNodeNameTokens->shader,
        _materialNodeNameTokens->cardsTexAssign
    };
    std::vector<HdDataSourceBaseHandle> nodes = {
        _ComputeShaderNode(values),
        cardsTexAssignNode
    };

    // ... and the texture nodes if a texture path was specified.
    for (size_t i = 0; i < 6; i++) {
        if (values.hasTexture[i]) {
            nodeNames.push_back(
                _GetTextureNodeName(i));
            nodes.push_back(
                _CardsTextureNode(
                    values.texturePaths[i],
                    values.drawModeColor));
        }
    }

    // Connect surface terminal to the shader node.
    static const HdContainerDataSourceHandle terminals =
        HdRetainedContainerDataSource::New(
            HdMaterialTerminalTokens->surface,
            HdMaterialConnectionSchema::Builder()
                .SetUpstreamNodePath(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        _materialNodeNameTokens->shader))
                .SetUpstreamNodeOutputName(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        HdMaterialTerminalTokens->surface))
                .Build());

    static TfToken names[] = {
        HdMaterialSchemaTokens->universalRenderContext
    };
    HdDataSourceBaseHandle networks[] = {
        HdMaterialNetworkSchema::Builder()
            .SetNodes(
                HdRetainedContainerDataSource::New(
                    nodeNames.size(), nodeNames.data(), nodes.data()))
            .SetTerminals(terminals)
            .Build()
    };
    return 
        HdRetainedContainerDataSource::New(
            HdMaterialSchemaTokens->material,
            HdMaterialSchema::BuildRetained(
                TfArraySize(names), names, networks));
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
/// - cardsTexAssign (from _CardsDataCache)
/// - displayRoughness (constant)

class _CardsPrimvarsDataSource : public _PrimvarsDataSource
{
public:
    HD_DECLARE_DATASOURCE(_CardsPrimvarsDataSource);

    bool Has(const TfToken &name) override {
        if (name == HdPrimvarsSchemaTokens->points) {
            return true;
        }
        if (name == _primvarNameTokens->cardsUv) {
            return true;
        }
        if (name == _primvarNameTokens->cardsTexAssign) {
            return true;
        }
        if (name == _primvarNameTokens->displayRoughness) {
            return true;
        }
        return _PrimvarsDataSource::Has(name);
    }

    TfTokenVector GetNames() override {
        static const TfTokenVector result = _Concat(
            _PrimvarsDataSource::GetNames(),
            { HdPrimvarsSchemaTokens->points,
              _primvarNameTokens->cardsUv,
              _primvarNameTokens->cardsTexAssign,
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
        if (name == _primvarNameTokens->cardsTexAssign) {
            return 
                _PrimvarDataSource::New(
                    _dataCache->GetTexAssign(),
                    HdPrimvarSchemaTokens->uniform,
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

    bool Has(const TfToken &name) override {
        if (name == HdMeshSchemaTokens->mesh) {
            return true;
        }
        if (name == HdPrimvarsSchemaTokens->primvars) {
            return true;
        }
        if (name == HdExtentSchemaTokens->extent) {
            return true;
        }
        if (name == HdMaterialBindingSchemaTokens->materialBinding) {
            return true;
        }
        return _PrimDataSource::Has(name);
    }

    TfTokenVector GetNames() override {
        static const TfTokenVector result = _Concat(
            _PrimDataSource::GetNames(),
            { HdMeshSchemaTokens->mesh,
              HdPrimvarsSchemaTokens->primvars,
              HdExtentSchemaTokens->extent,
              HdMaterialBindingSchemaTokens->materialBinding});
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdMeshSchemaTokens->mesh) {
            return HdMeshSchema::Builder()
                .SetTopology(
                    _dataCache->GetMeshTopology())
                .SetDoubleSided(
                    HdRetainedTypedSampledDataSource<bool>::New(false))
                .Build();
        }
        if (name == HdPrimvarsSchemaTokens->primvars) {
            return _CardsPrimvarsDataSource::New(_primSource, _dataCache);
        }
        if (name == HdMaterialBindingSchemaTokens->materialBinding) {
            return HdRetainedContainerDataSource::New(
                HdMaterialBindingSchemaTokens->allPurpose,
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    _path.AppendChild(_primNameTokens->material)));
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
    HdDataSourceLocatorSet result;

    const HdDataSourceLocator nodes =
        HdMaterialSchema::GetDefaultLocator()
            .Append(HdMaterialSchemaTokens->universalRenderContext)
            .Append(HdMaterialNetworkSchemaTokens->nodes);
    const HdDataSourceLocator shaderParams =
        nodes
            .Append(_materialNodeNameTokens->shader)
            .Append(HdMaterialNodeSchemaTokens->parameters);

    for (size_t i = 0; i < 6; i++) {
        result.insert(
            nodes
                .Append(_GetTextureNodeName(i))
                .Append(HdMaterialNodeSchemaTokens->parameters)
                .Append(_UsdPrimvarReaderTokens->fallback));
        result.insert(
            shaderParams.Append(_GetColorInputName(i)));
        result.insert(
            shaderParams.Append(_GetOpacityInputName(i)));
    }

    return result;
};


class _CardsStandin : public UsdImagingGL_DrawModeStandin
{
public:
    _CardsStandin(const SdfPath &path,
             const HdContainerDataSourceHandle &primSource)
      : UsdImagingGL_DrawModeStandin(path, primSource)
      , _dataCache(std::make_shared<_CardsDataCache>(primSource))
    {
    }

    const TfTokenVector
    &_GetChildNames() const override {
        static const TfTokenVector childNames{
            _primNameTokens->cardsMesh,
            _primNameTokens->material};
        return childNames;
    }

    TfToken
    _GetChildPrimType(const TfToken &name) const override {
        if (name == _primNameTokens->material) {
            return HdPrimTypeTokens->material;
        }
        return HdPrimTypeTokens->mesh;
    }

    HdContainerDataSourceHandle
    _GetChildPrimSource(const TfToken &name) const override {
        if (name == _primNameTokens->material) {
            // We rely on the consumer calling HdScenIndex::GetPrim()
            // again when we send a prim dirtied for the material prim
            // with an empty data source locators.
            return _dataCache->GetMaterial();
        }
        return _CardsPrimDataSource::New(_path, _primSource, _dataCache);
    }

    void ProcessDirtyLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdSceneIndexObserver::DirtiedPrimEntries * entries) override
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
            _dataCache->Reset();
            for (const SdfPath &path : GetChildPrimPaths()) {
                static const HdDataSourceLocator empty;
                entries->push_back({path, empty});
            }
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
                {_path.AppendChild(_primNameTokens->cardsMesh), primDirtyLocators});
            static const HdDataSourceLocatorSet materialColorInputs =
                _ComputeMaterialColorInputLocators();
            entries->push_back(
                {_path.AppendChild(_primNameTokens->material), materialColorInputs});
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

UsdImagingGL_DrawModeStandinSharedPtr
UsdImagingGL_GetDrawModeStandin(const TfToken &drawMode,
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
