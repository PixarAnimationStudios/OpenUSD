//
// Copyright 2024 Pixar
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
