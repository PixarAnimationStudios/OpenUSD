//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_MESH_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_MESH_H

#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/usd/usdGeom/mesh.h"

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceSubdivisionTags
///
/// A container data source representing subdivision tags.
///
class UsdImagingDataSourceSubdivisionTags : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceSubdivisionTags);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    UsdImagingDataSourceSubdivisionTags(
            UsdGeomMesh usdMesh,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    UsdGeomMesh _usdMesh;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceSubdivisionTags);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceMeshTopology
///
/// A container data source representing mesh topology.
///
class UsdImagingDataSourceMeshTopology : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceMeshTopology);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    UsdImagingDataSourceMeshTopology(
            const SdfPath &sceneIndexPath,
            UsdGeomMesh usdMesh,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    const SdfPath _sceneIndexPath;
    UsdGeomMesh _usdMesh;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceMeshTopology);

// ----------------------------------------------------------------------------


/// \class UsdImagingDataSourceMesh
///
/// A container data source representing data unique to meshes
///
class UsdImagingDataSourceMesh : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceMesh);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    UsdImagingDataSourceMesh(
            const SdfPath &sceneIndexPath,
            UsdGeomMesh usdMesh,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    const SdfPath _sceneIndexPath;
    UsdGeomMesh _usdMesh;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceMesh);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceMeshPrim
///
/// A prim data source representing UsdGeomMesh.
///
class UsdImagingDataSourceMeshPrim : public UsdImagingDataSourceGprim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceMeshPrim);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDIMAGING_API
    static HdDataSourceLocatorSet Invalidate(
            UsdPrim const& prim,
            const TfToken &subprim,
            const TfTokenVector &properties,
            UsdImagingPropertyInvalidationType invalidationType);

private:
    UsdImagingDataSourceMeshPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceMeshPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_MESH_H
