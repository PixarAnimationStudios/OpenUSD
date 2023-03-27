//
// Copyright 2020 Pixar
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
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_PRIM_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_PRIM_H

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/modelAPI.h"

#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrimvars.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceVisibility
///
/// Data source representing prim visibility for a USD imageable.
class UsdImagingDataSourceVisibility : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceVisibility);

    /// Returns the names contained in this data source.
    ///
    /// This class only returns 'visibility'.
    TfTokenVector GetNames() override;

    /// Returns the data source for the given \p 'name'.
    ///
    /// Only 'visibility' returns anything for this class.
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    /// Use to construct a new UsdImagingDataSourceVisibility.
    ///
    /// \p visibilityQuery is the USD attribute query holding visibility data.
    /// \p sceneIndexPath is the path of this object in the scene index.
    /// \p stageGlobals is the context object for the USD stage.
    ///
    /// Note: client code calls this via static New().
    UsdImagingDataSourceVisibility(
            const UsdAttributeQuery &visibilityQuery,
            const SdfPath &sceneIndexPath,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    UsdAttributeQuery _visibilityQuery;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceVisibility);


// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourcePurpose
///
/// Data source representing prim purpose for a USD imageable.
class UsdImagingDataSourcePurpose : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourcePurpose);

    /// Returns the names contained in this data source.
    ///
    /// This class only returns 'purpose'.
    TfTokenVector GetNames() override;

    /// Returns the data source for the given \p 'name'.
    ///
    /// Only 'purpose' returns anything for this class.
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    /// Use to construct a new UsdImagingDataSourcePurpose.
    ///
    /// \p purposeQuery is the USD attribute query holding purpose data.
    /// \p stageGlobals is the context object for the USD stage.
    ///
    /// Note: client code calls this via static New().
    UsdImagingDataSourcePurpose(
            const UsdAttributeQuery &purposeQuery,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    UsdAttributeQuery _purposeQuery;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourcePurpose);


// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceExtentCoordinate
///
/// Data source representing either the minimum or maximum of the local prim
/// extent.
class UsdImagingDataSourceExtentCoordinate : public HdVec3dDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceExtentCoordinate);

    /// Returns VtValue at a given \p shutterOffset for the value of this flag.
    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override;

    /// Returns bool at a given \p shutterOffset for the value of this flag.
    GfVec3d GetTypedValue(HdSampledDataSource::Time shutterOffset) override;

    /// Fills the \p outSampleTimes with the times between \p startTime and
    /// \p endTime that have valid sample data and returns \c true.
    bool GetContributingSampleTimesForInterval(
            HdSampledDataSource::Time startTime,
            HdSampledDataSource::Time endTime,
            std::vector<HdSampledDataSource::Time> *outSampleTimes) override;

private:
    /// Use to construct a new UsdImagingDataSourceExtentCoordinate.
    ///
    /// \p extentAttr is the float3 array holding all extent coordinates.
    /// \p attrPath is the USD path of the underlying extents attribute.
    /// \p index is the index of the value we want out of extentAttr.
    ///
    /// Note: client code calls this via static New().
    UsdImagingDataSourceExtentCoordinate(
            const HdVec3fArrayDataSourceHandle &extentAttr,
            const SdfPath &attrPath,
            unsigned int index);

private:
    HdVec3fArrayDataSourceHandle _extentAttr;
    SdfPath _attrPath;
    unsigned int _index;
};

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceExtent
///
/// Data source representing local prim extent.
class UsdImagingDataSourceExtent : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceExtent);

    /// Returns the names contained in this datasource.
    ///
    /// This class only returns 'min' and 'max'.
    TfTokenVector GetNames() override;

    /// Returns the data source for the given \p 'name'.
    ///
    /// Only 'min' and 'max' return anything for this class.
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    /// Use to construct a new UsdImagingDataSourceExtent.
    ///
    /// \p extentQuery is the USD attribute query holding extent data.
    /// \p sceneIndexPath is the path of this object in the scene index.
    /// \p stageGlobals is the context object for the USD stage.
    ///
    /// Note: client code calls this via static New().
    UsdImagingDataSourceExtent(
            const UsdAttributeQuery &extentQuery,
            const SdfPath &sceneIndexPath,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    // Note: the constructor takes sceneIndexPath for change-tracking,
    // but here we're storing the USD attribute path for error reporting!
    SdfPath _attrPath;
    HdVec3fArrayDataSourceHandle _extentAttr;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceExtent);


// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceXformResetXformStack
///
/// Data source representing the "reset xform stack" flag for a USD
/// xformable.
class UsdImagingDataSourceXformResetXformStack : public HdBoolDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceXformResetXformStack);

    /// Returns VtValue at a given \p shutterOffset for the value of this flag.
    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override;

    /// Returns bool at a given \p shutterOffset for the value of this flag.
    bool GetTypedValue(HdSampledDataSource::Time shutterOffset) override;

    /// Fills the \p outSampleTimes with the times between \p startTime and
    /// \p endTime that have valid sample data and returns \c true.
    bool GetContributingSampleTimesForInterval(
            HdSampledDataSource::Time startTime,
            HdSampledDataSource::Time endTime,
            std::vector<HdSampledDataSource::Time> *outSampleTimes) override {
        return false;
    }

private:
    /// Use to construct a new UsdImagingDataSourceXformResetXformStack.
    ///
    /// \p xformQuery is the USD XformQuery object that this class
    /// can extract a matrix from.
    ///
    /// \p stageGlobals represents the context object for the UsdStage with
    /// which to evaluate this attribute data source.
    ///
    /// Note: client code calls this via static New().
    UsdImagingDataSourceXformResetXformStack(
            const UsdGeomXformable::XformQuery &xformQuery,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    UsdGeomXformable::XformQuery _xformQuery;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceXformResetXformStack);

// ----------------------------------------------------------------------------

///
/// \class UsdImagingDataSourceXformMatrix
///
/// Data source representing a generic transform value accessor for a
/// USD Xformable.
///
class UsdImagingDataSourceXformMatrix : public HdMatrixDataSource
{
public:

    HD_DECLARE_DATASOURCE(UsdImagingDataSourceXformMatrix);

    /// Returns VtValue at a given \p shutterOffset for the value of this
    /// xform.
    ///
    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override;

    /// Returns GfMatrix4d at a given \p shutterOffset for the value of this
    /// xform.
    ///
    GfMatrix4d GetTypedValue(HdSampledDataSource::Time shutterOffset) override;

    /// Fills the \p outSampleTimes with the times between \p startTime and 
    /// \p endTime that have valid sample data and returns \c true.
    ///
    bool GetContributingSampleTimesForInterval(
            HdSampledDataSource::Time startTime,
            HdSampledDataSource::Time endTime,
            std::vector<HdSampledDataSource::Time> *outSampleTimes) override;

private:
    /// Use to construct a new UsdImagingDataSourceXformMatrix.
    ///
    /// \p xformQuery is the USD XformQuery object that this class
    /// can extract a matrix from.
    ///
    /// \p stageGlobals represents the context object for the UsdStage with
    /// which to evaluate this attribute data source.
    ///
    /// Note: client code calls this via static New().
    UsdImagingDataSourceXformMatrix(
            const UsdGeomXformable::XformQuery &xformQuery,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    UsdGeomXformable::XformQuery _xformQuery;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceXformMatrix);

// ----------------------------------------------------------------------------

///
/// \class UsdImagingDataSourceXform
///
/// Data source representing a container that stores a USD Xformable.
///
/// Note that this container only represents a flattened matrix right now,
/// but could be expanded in the future to include more granular XformOps,
/// which is why it exists as a separate entity.
///
class UsdImagingDataSourceXform : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceXform);

    /// Returns the names contained in this data source.
    ///
    /// This class only returns 'matrix' and 'resetXformStack'.
    ///
    TfTokenVector GetNames() override;

    /// Returns the data source for the given \p 'name'.
    ///
    /// Only 'matrix' and 'resetXformStack' return anything in this class.
    ///
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    /// Use to construct a new UsdImagingDataSourceXform.
    ///
    /// \p xformQuery is the USD XformQuery object that this class
    /// can extract a matrix from.
    /// \p sceneIndexPath is the path of this object in the scene index.
    /// \p stageGlobals represents the context object for the UsdStage with
    /// which to evaluate this attribute data source.
    ///
    /// Note: client code calls this via static New().
    UsdImagingDataSourceXform(
            const UsdGeomXformable::XformQuery &xformQuery,
            const SdfPath &sceneIndexPath,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    UsdGeomXformable::XformQuery _xformQuery;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceXform);

// ----------------------------------------------------------------------------

///
/// \class UsdImagingDataSourceModel
///
/// Data source representing model API for a USD prim.
///
class UsdImagingDataSourceModel : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceModel);

    /// Return attribute names of UsdImagingModelSchema.
    ///
    TfTokenVector GetNames() override;

    /// Returns data authored on USD prim (without resolving inherited) for
    /// attribute names of UsdImagingModelSchema.
    ///
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    /// C'tor.
    ///
    /// \p model is API schema from which this class can extract values from.
    /// \p sceneIndexPath is the path of this object in the scene index.
    /// \p stageGlobals represents the context object for the UsdStage with
    /// which to evaluate this attribute data source.
    ///
    /// Note: client code calls this via static New().
    UsdImagingDataSourceModel(
            const UsdGeomModelAPI &model,
            const SdfPath &sceneIndexPath,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    UsdGeomModelAPI _model;
    const SdfPath _sceneIndexPath;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceModel);

// ----------------------------------------------------------------------------

///
/// \class UsdImagingDataSourcePrimOrigin
///
/// Data source to access the underlying UsdPrim.
///
class UsdImagingDataSourcePrimOrigin : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourcePrimOrigin);

    TfTokenVector GetNames() override;

    /// Get(UsdImagingTokens->usdPrim) returns a data source containing
    /// the underyling UsdPrim.
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    UsdImagingDataSourcePrimOrigin(const UsdPrim &usdPrim);

private:
    UsdPrim _usdPrim;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourcePrimOrigin);

// ----------------------------------------------------------------------------

///
/// \class UsdImagingDataSourcePrim
///
/// Data source representing a basic USD prim. This class is meant to check for
/// behaviors we might expect on all nodes of the scene, including interior
/// nodes; e.g. xform, visibility. It's expected that concrete imaging types
/// would derive from this and add imaging prim-related attributes.
///
/// Note: while every prim gets at least this datasource defined on it, this
/// datasource tries to aggressively early out of attribute checks based on
/// type information (IsA/HasA) or namespace-accelerated attribute checks.
///
/// Since a Usd prim can populate multiple imaging prims (e.g. /prim, /prim.a,
/// /prim.b); and since the default prim path (/prim) is always populated; we
/// drop inherited attributes on subprims (/prim.a), letting them inherit from
/// /prim instead.  This way we don't (e.g.) double-apply transforms.
///
class UsdImagingDataSourcePrim : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourcePrim);

    /// Returns the names for which this data source can return meaningful
    /// results.
    ///
    USDIMAGING_API
    TfTokenVector GetNames() override;

    /// Returns the data source representing \p name, if valid.
    ///
    USDIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    /// Returns the hydra attribute set we should invalidate if the value of
    /// the USD properties in \p properties change.
    USDIMAGING_API
    static HdDataSourceLocatorSet Invalidate(
            UsdPrim const& prim,
            const TfToken &subprim,
            const TfTokenVector &properties);

protected:
    /// Use to construct a new UsdImagingDataSourcePrim.
    ///
    /// \p sceneIndexPath is the path of this object in the scene index.
    ///    (Note: this can be different than the usd path if one usd prim
    ///     inserts multiple imaging prims).
    ///
    /// \p usdPrim is the USD prim object that this data source represents.
    ///
    /// \p stageGlobals represents the context object for the UsdStage with
    /// which to evaluate this attribute data source.
    ///
    /// Note: client code calls this via static New().
    USDIMAGING_API
    UsdImagingDataSourcePrim(
            const SdfPath &sceneIndexPath,
            UsdPrim usdPrim,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

    // Accessors, for derived classes...
    const SdfPath &_GetSceneIndexPath() const {
        return _sceneIndexPath;
    }

    const UsdPrim &_GetUsdPrim() const {
        return _usdPrim;
    }

    const UsdImagingDataSourceStageGlobals &_GetStageGlobals() const {
        return _stageGlobals;
    }

private:
    const SdfPath _sceneIndexPath;
    UsdPrim _usdPrim;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;

    UsdImagingDataSourcePrimvarsAtomicHandle _primvars;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourcePrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_PRIM_H
