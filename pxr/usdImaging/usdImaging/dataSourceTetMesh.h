//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_TET_MESH_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_TET_MESH_H

#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/usd/usdGeom/tetMesh.h"

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceTetMeshTopology
///
/// A container data source representing tet mesh topology.
///
class UsdImagingDataSourceTetMeshTopology : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceTetMeshTopology);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    UsdImagingDataSourceTetMeshTopology(
            const SdfPath &sceneIndexPath,
            UsdGeomTetMesh usdTetMesh,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    const SdfPath _sceneIndexPath;
    UsdGeomTetMesh _usdTetMesh;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceTetMeshTopology);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceTetMesh
///
/// A container data source representing data unique to tet meshes
///
class UsdImagingDataSourceTetMesh : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceTetMesh);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    UsdImagingDataSourceTetMesh(
            const SdfPath &sceneIndexPath,
            UsdGeomTetMesh usdTetMesh,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    const SdfPath _sceneIndexPath;
    UsdGeomTetMesh _usdTetMesh;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceTetMesh);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceTetMeshPrim
///
/// A prim data source representing UsdGeomTetMesh.
///
class UsdImagingDataSourceTetMeshPrim : public UsdImagingDataSourceGprim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceTetMeshPrim);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDIMAGING_API
    static HdDataSourceLocatorSet Invalidate(
            UsdPrim const& prim,
            const TfToken &subprim,
            const TfTokenVector &properties,
            UsdImagingPropertyInvalidationType invalidationType);

private:
    UsdImagingDataSourceTetMeshPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceTetMeshPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_TET_MESH_H
