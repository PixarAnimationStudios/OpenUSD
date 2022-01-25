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

#include "pxr/usd/usdGeom/basisCurves.h"

#include "pxr/imaging/hd/basisCurvesSchema.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
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

bool
UsdImagingDataSourceBasisCurvesTopology::Has(const TfToken &name)
{
    return
        name == HdBasisCurvesTopologySchemaTokens->curveVertexCounts ||
        name == HdBasisCurvesTopologySchemaTokens->basis ||
        name == HdBasisCurvesTopologySchemaTokens->type ||
        name == HdBasisCurvesTopologySchemaTokens->wrap;
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

bool
UsdImagingDataSourceBasisCurves::Has(const TfToken &name)
{
    return
        name == HdBasisCurvesSchemaTokens->topology;
    // XXX: TODO geomsubsets
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
    // Note: DataSourceGprim handles the special PointBased primvars for us,
    // but we need to add "widths" which is defined on UsdGeomCurves.
    _AddCustomPrimvar(HdTokens->widths, UsdGeomTokens->widths);
}

bool 
UsdImagingDataSourceBasisCurvesPrim::Has(
    const TfToken &name)
{
    if (name == HdBasisCurvesSchemaTokens->basisCurves) {
        return true;
    }

    return UsdImagingDataSourceGprim::Has(name);
}

TfTokenVector 
UsdImagingDataSourceBasisCurvesPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
    result.push_back(HdBasisCurvesSchemaTokens->basisCurves);

    return result;
}

HdDataSourceBaseHandle 
UsdImagingDataSourceBasisCurvesPrim::Get(const TfToken &name)
{
    if (name == HdBasisCurvesSchemaTokens->basisCurves) {
        return UsdImagingDataSourceBasisCurves::New(
                _GetSceneIndexPath(),
                UsdGeomBasisCurves(_GetUsdPrim()),
                _GetStageGlobals());
    } else {
        return UsdImagingDataSourceGprim::Get(name);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
