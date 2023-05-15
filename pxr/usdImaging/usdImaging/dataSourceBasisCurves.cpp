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
#include "pxr/usdImaging/usdImaging/dataSourceBasisCurves.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrimvars.h"

#include "pxr/usd/usdGeom/basisCurves.h"

#include "pxr/imaging/hd/basisCurvesSchema.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceBasisCurvesTopology
::UsdImagingDataSourceBasisCurvesTopology(
        const SdfPath &sceneIndexPath,
        UsdGeomBasisCurves usdBasisCurves,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _sceneIndexPath(sceneIndexPath)
    , _usdBasisCurves(usdBasisCurves)
    , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdImagingDataSourceBasisCurvesTopology::GetNames()
{
    return {
        HdBasisCurvesTopologySchemaTokens->curveVertexCounts,
        HdBasisCurvesTopologySchemaTokens->basis,
        HdBasisCurvesTopologySchemaTokens->type,
        HdBasisCurvesTopologySchemaTokens->wrap,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceBasisCurvesTopology::Get(const TfToken &name)
{
    if (name == HdBasisCurvesTopologySchemaTokens->curveVertexCounts) {
        static const HdDataSourceLocator locator =
            HdBasisCurvesTopologySchema::GetDefaultLocator().Append(name);
        return UsdImagingDataSourceAttribute<VtIntArray>::New(
                _usdBasisCurves.GetCurveVertexCountsAttr(), _stageGlobals,
                _sceneIndexPath, locator);
    } else if (name == HdBasisCurvesTopologySchemaTokens->basis) {
        return UsdImagingDataSourceAttribute<TfToken>::New(
                _usdBasisCurves.GetBasisAttr(), _stageGlobals);
    } else if (name == HdBasisCurvesTopologySchemaTokens->type) {
        return UsdImagingDataSourceAttribute<TfToken>::New(
                _usdBasisCurves.GetTypeAttr(), _stageGlobals);
    } else if (name == HdBasisCurvesTopologySchemaTokens->wrap) {
        return UsdImagingDataSourceAttribute<TfToken>::New(
                _usdBasisCurves.GetWrapAttr(), _stageGlobals);
    }
    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceBasisCurves::UsdImagingDataSourceBasisCurves(
        const SdfPath &sceneIndexPath,
        UsdGeomBasisCurves usdBasisCurves,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _sceneIndexPath(sceneIndexPath)
    , _usdBasisCurves(usdBasisCurves)
    , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdImagingDataSourceBasisCurves::GetNames()
{
    return {
        HdBasisCurvesSchemaTokens->topology,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceBasisCurves::Get(const TfToken &name)
{
    if (name == HdBasisCurvesSchemaTokens->topology) {
        return UsdImagingDataSourceBasisCurvesTopology::New(
            _sceneIndexPath, _usdBasisCurves, _stageGlobals);
    }

    return nullptr;
}
// ----------------------------------------------------------------------------

UsdImagingDataSourceBasisCurvesPrim::UsdImagingDataSourceBasisCurvesPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceBasisCurvesPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
    result.push_back(HdBasisCurvesSchema::GetSchemaToken());

    return result;
}


HdDataSourceBaseHandle 
UsdImagingDataSourceBasisCurvesPrim::Get(const TfToken &name)
{
    if (name == HdBasisCurvesSchema::GetSchemaToken()) {
        return UsdImagingDataSourceBasisCurves::New(
                _GetSceneIndexPath(),
                UsdGeomBasisCurves(_GetUsdPrim()),
                _GetStageGlobals());
    }

    return UsdImagingDataSourceGprim::Get(name);
}

/*static*/
HdDataSourceLocatorSet
UsdImagingDataSourceBasisCurvesPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet result;

    if (subprim.IsEmpty()) {
        result = UsdImagingDataSourceGprim::Invalidate(
            prim, subprim, properties, invalidationType);

        for (const TfToken &propertyName : properties) {
            if (propertyName == UsdGeomTokens->curveVertexCounts) {
                result.insert(HdBasisCurvesTopologySchema::GetDefaultLocator()
                    .Append(
                        HdBasisCurvesTopologySchemaTokens->curveVertexCounts));
            } else if (propertyName == UsdGeomTokens->type) {
                result.insert(HdBasisCurvesTopologySchema::GetDefaultLocator()
                    .Append(HdBasisCurvesTopologySchemaTokens->type));
            } else if (propertyName == UsdGeomTokens->basis) {
                result.insert(HdBasisCurvesTopologySchema::GetDefaultLocator()
                    .Append(HdBasisCurvesTopologySchemaTokens->basis));
            } else if (propertyName == UsdGeomTokens->wrap) {
                result.insert(HdBasisCurvesTopologySchema::GetDefaultLocator()
                    .Append(HdBasisCurvesTopologySchemaTokens->wrap));
            }
        }
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
