//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dataSourceTetMesh.h"

#include "pxr/imaging/hd/tetMeshSchema.h"
#include "pxr/imaging/hd/tetMeshTopologySchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/imaging/pxOsd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceTetMeshTopology::UsdImagingDataSourceTetMeshTopology(
        const SdfPath &sceneIndexPath,
        UsdGeomTetMesh usdTetMesh,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _sceneIndexPath(sceneIndexPath)
    , _usdTetMesh(usdTetMesh)
    , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdImagingDataSourceTetMeshTopology::GetNames()
{
    return {
        HdTetMeshTopologySchemaTokens->orientation,
        HdTetMeshTopologySchemaTokens->tetVertexIndices,
        HdTetMeshTopologySchemaTokens->surfaceFaceVertexIndices,
    };
}


HdDataSourceBaseHandle
UsdImagingDataSourceTetMeshTopology::Get(const TfToken &name)
{
    if (name == HdTetMeshTopologySchemaTokens->tetVertexIndices) {
        return UsdImagingDataSourceAttribute<VtVec4iArray>::New(
            _usdTetMesh.GetTetVertexIndicesAttr(),
            _stageGlobals, _sceneIndexPath,
            HdTetMeshTopologySchema::GetTetVertexIndicesLocator());
    } else if (name == HdTetMeshTopologySchemaTokens->surfaceFaceVertexIndices) {
        return UsdImagingDataSourceAttribute<VtVec3iArray>::New(
            _usdTetMesh.GetSurfaceFaceVertexIndicesAttr(),
            _stageGlobals, _sceneIndexPath,
            HdTetMeshTopologySchema::GetSurfaceFaceVertexIndicesLocator());
    } else if (name == HdTetMeshTopologySchemaTokens->orientation) {
        return UsdImagingDataSourceAttribute<TfToken>::New(
            _usdTetMesh.GetOrientationAttr(), _stageGlobals);
    }
    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceTetMesh::UsdImagingDataSourceTetMesh(
        const SdfPath &sceneIndexPath,
        UsdGeomTetMesh usdTetMesh,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _sceneIndexPath(sceneIndexPath)
    , _usdTetMesh(usdTetMesh)
    , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdImagingDataSourceTetMesh::GetNames()
{
    return {
        HdTetMeshSchemaTokens->topology,
        HdTetMeshSchemaTokens->doubleSided,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceTetMesh::Get(const TfToken &name)
{
    if (name == HdTetMeshSchemaTokens->topology) {
        return UsdImagingDataSourceTetMeshTopology::New(
            _sceneIndexPath, _usdTetMesh, _stageGlobals);
    }
    if (name == HdTetMeshSchemaTokens->doubleSided) {
        return UsdImagingDataSourceAttribute<bool>::New(
            _usdTetMesh.GetDoubleSidedAttr(), _stageGlobals);
    }
    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceTetMeshPrim::UsdImagingDataSourceTetMeshPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
{
    // Note: DataSourceGprim handles the special PointBased primvars for us.
}

TfTokenVector 
UsdImagingDataSourceTetMeshPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
    result.push_back(HdTetMeshSchemaTokens->tetMesh);

    return result;
}

HdDataSourceBaseHandle 
UsdImagingDataSourceTetMeshPrim::Get(const TfToken & name)
{
    if (name == HdTetMeshSchemaTokens->tetMesh) {
        return UsdImagingDataSourceTetMesh::New(
            _GetSceneIndexPath(),
            UsdGeomTetMesh(_GetUsdPrim()),
            _GetStageGlobals());
    } else {
        return UsdImagingDataSourceGprim::Get(name);
    }
}

/*static*/
HdDataSourceLocatorSet
UsdImagingDataSourceTetMeshPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet locators;

    for (const TfToken &propertyName : properties) {
        if (propertyName == UsdGeomTokens->tetVertexIndices ||
            propertyName == UsdGeomTokens->surfaceFaceVertexIndices ||
            propertyName == UsdGeomTokens->orientation) {
            locators.insert(HdTetMeshSchema::GetTopologyLocator());
        }
        if (propertyName == UsdGeomTokens->doubleSided)  {
            locators.insert(HdTetMeshSchema::GetDoubleSidedLocator());
        }
    }

    // Give base classes a chance to invalidate.
    locators.insert(
        UsdImagingDataSourceGprim::Invalidate(
            prim, subprim, properties, invalidationType));
    return locators;
}


PXR_NAMESPACE_CLOSE_SCOPE
