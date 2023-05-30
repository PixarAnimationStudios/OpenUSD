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
//
#include "pxr/usdImaging/usdImaging/dataSourceMesh.h"

#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceSubdivisionTags::UsdImagingDataSourceSubdivisionTags(
        UsdGeomMesh usdMesh,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _usdMesh(usdMesh)
    , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdImagingDataSourceSubdivisionTags::GetNames()
{
    return {
        HdSubdivisionTagsSchemaTokens->faceVaryingLinearInterpolation,
        HdSubdivisionTagsSchemaTokens->interpolateBoundary,
        HdSubdivisionTagsSchemaTokens->triangleSubdivisionRule,
        HdSubdivisionTagsSchemaTokens->cornerIndices,
        HdSubdivisionTagsSchemaTokens->cornerSharpnesses,
        HdSubdivisionTagsSchemaTokens->creaseIndices,
        HdSubdivisionTagsSchemaTokens->creaseLengths,
        HdSubdivisionTagsSchemaTokens->creaseSharpnesses,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceSubdivisionTags::Get(const TfToken &name)
{
    if (name == HdSubdivisionTagsSchemaTokens->faceVaryingLinearInterpolation) {
        return UsdImagingDataSourceAttribute<TfToken>::New(
            _usdMesh.GetFaceVaryingLinearInterpolationAttr(), _stageGlobals);
    } else if (name == HdSubdivisionTagsSchemaTokens->interpolateBoundary) {
        return UsdImagingDataSourceAttribute<TfToken>::New(
            _usdMesh.GetInterpolateBoundaryAttr(), _stageGlobals);
    } else if (name == HdSubdivisionTagsSchemaTokens->triangleSubdivisionRule) {
        return UsdImagingDataSourceAttribute<TfToken>::New(
            _usdMesh.GetTriangleSubdivisionRuleAttr(), _stageGlobals);
    } else if (name == HdSubdivisionTagsSchemaTokens->cornerIndices) {
        return UsdImagingDataSourceAttribute<VtIntArray>::New(
            _usdMesh.GetCornerIndicesAttr(), _stageGlobals);
    } else if (name == HdSubdivisionTagsSchemaTokens->cornerSharpnesses) {
        return UsdImagingDataSourceAttribute<VtFloatArray>::New(
            _usdMesh.GetCornerSharpnessesAttr(), _stageGlobals);
    } else if (name == HdSubdivisionTagsSchemaTokens->creaseIndices) {
        return UsdImagingDataSourceAttribute<VtIntArray>::New(
            _usdMesh.GetCreaseIndicesAttr(), _stageGlobals);
    } else if (name == HdSubdivisionTagsSchemaTokens->creaseLengths) {
        return UsdImagingDataSourceAttribute<VtIntArray>::New(
            _usdMesh.GetCreaseLengthsAttr(), _stageGlobals);
    } else if (name == HdSubdivisionTagsSchemaTokens->creaseSharpnesses) {
        return UsdImagingDataSourceAttribute<VtFloatArray>::New(
            _usdMesh.GetCreaseSharpnessesAttr(), _stageGlobals);
    }

    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceMeshTopology::UsdImagingDataSourceMeshTopology(
        const SdfPath &sceneIndexPath,
        UsdGeomMesh usdMesh,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _sceneIndexPath(sceneIndexPath)
    , _usdMesh(usdMesh)
    , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdImagingDataSourceMeshTopology::GetNames()
{
    return {
        HdMeshTopologySchemaTokens->faceVertexCounts,
        HdMeshTopologySchemaTokens->faceVertexIndices,
        HdMeshTopologySchemaTokens->holeIndices,
        HdMeshTopologySchemaTokens->orientation,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceMeshTopology::Get(const TfToken &name)
{
    if (name == HdMeshTopologySchemaTokens->faceVertexCounts) {
        static const HdDataSourceLocator locator =
            HdMeshTopologySchema::GetDefaultLocator().Append(name);
        return UsdImagingDataSourceAttribute<VtIntArray>::New(
            _usdMesh.GetFaceVertexCountsAttr(), _stageGlobals,
            _sceneIndexPath, locator);
    } else if (name == HdMeshTopologySchemaTokens->faceVertexIndices) {
        static const HdDataSourceLocator locator =
            HdMeshTopologySchema::GetDefaultLocator().Append(name);
        return UsdImagingDataSourceAttribute<VtIntArray>::New(
            _usdMesh.GetFaceVertexIndicesAttr(), _stageGlobals,
            _sceneIndexPath, locator);
    } else if (name == HdMeshTopologySchemaTokens->holeIndices) {
        static const HdDataSourceLocator locator =
            HdMeshTopologySchema::GetDefaultLocator().Append(name);
        return UsdImagingDataSourceAttribute<VtIntArray>::New(
            _usdMesh.GetHoleIndicesAttr(), _stageGlobals,
            _sceneIndexPath, locator);
    } else if (name == HdMeshTopologySchemaTokens->orientation) {
        return UsdImagingDataSourceAttribute<TfToken>::New(
            _usdMesh.GetOrientationAttr(), _stageGlobals);
    }
    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceMesh::UsdImagingDataSourceMesh(
        const SdfPath &sceneIndexPath,
        UsdGeomMesh usdMesh,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _sceneIndexPath(sceneIndexPath)
    , _usdMesh(usdMesh)
    , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdImagingDataSourceMesh::GetNames()
{
    return {
        HdMeshSchemaTokens->topology,
        HdMeshSchemaTokens->subdivisionScheme,
        HdMeshSchemaTokens->doubleSided,
        HdMeshSchemaTokens->subdivisionTags,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceMesh::Get(const TfToken &name)
{
    if (name == HdMeshSchemaTokens->topology) {
        return UsdImagingDataSourceMeshTopology::New(
            _sceneIndexPath, _usdMesh, _stageGlobals);
    }

    if (name == HdMeshSchemaTokens->subdivisionScheme) {
        return UsdImagingDataSourceAttribute<TfToken>::New(
            _usdMesh.GetSubdivisionSchemeAttr(), _stageGlobals);
    }

    if (name == HdMeshSchemaTokens->doubleSided) {
        return UsdImagingDataSourceAttribute<bool>::New(
            _usdMesh.GetDoubleSidedAttr(), _stageGlobals);
    }

    if (name == HdMeshSchemaTokens->subdivisionTags) {
        return UsdImagingDataSourceSubdivisionTags::New(
            _usdMesh, _stageGlobals);
    }

    return nullptr;
}

// ----------------------------------------------------------------------------


UsdImagingDataSourceMeshPrim::UsdImagingDataSourceMeshPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
{
    // Note: DataSourceGprim handles the special PointBased primvars for us.
}

TfTokenVector 
UsdImagingDataSourceMeshPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
    result.push_back(HdMeshSchemaTokens->mesh);

    return result;
}

HdDataSourceBaseHandle 
UsdImagingDataSourceMeshPrim::Get(const TfToken & name)
{
    if (name == HdMeshSchemaTokens->mesh) {
        return UsdImagingDataSourceMesh::New(
            _GetSceneIndexPath(),
            UsdGeomMesh(_GetUsdPrim()),
            _GetStageGlobals());
    } else {
        return UsdImagingDataSourceGprim::Get(name);
    }
}

/*static*/
HdDataSourceLocatorSet
UsdImagingDataSourceMeshPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet locators;

    for (const TfToken &propertyName : properties) {
        if (propertyName == UsdGeomTokens->subdivisionScheme) {
            locators.insert(HdMeshSchema::GetSubdivisionSchemeLocator());
        }

        if (propertyName == UsdGeomTokens->faceVertexCounts ||
                propertyName == UsdGeomTokens->faceVertexIndices ||
                propertyName == UsdGeomTokens->holeIndices ||
                propertyName == UsdGeomTokens->orientation) {
            locators.insert(HdMeshSchema::GetTopologyLocator());
        }

        if (propertyName == UsdGeomTokens->interpolateBoundary ||
                propertyName == UsdGeomTokens->faceVaryingLinearInterpolation ||
                propertyName == UsdGeomTokens->triangleSubdivisionRule ||
                propertyName == UsdGeomTokens->creaseIndices ||
                propertyName == UsdGeomTokens->creaseLengths ||
                propertyName == UsdGeomTokens->creaseSharpnesses ||
                propertyName == UsdGeomTokens->cornerIndices ||
                propertyName == UsdGeomTokens->cornerSharpnesses) {
            // XXX UsdGeomTokens->creaseMethod, when we add support for that.
            locators.insert(HdMeshSchema::GetSubdivisionTagsLocator());
        }

        if (propertyName == UsdGeomTokens->doubleSided)  {
            locators.insert(HdMeshSchema::GetDoubleSidedLocator());
        }
    }

    // Give base classes a chance to invalidate.
    locators.insert(
        UsdImagingDataSourceGprim::Invalidate(
            prim, subprim, properties, invalidationType));
    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
