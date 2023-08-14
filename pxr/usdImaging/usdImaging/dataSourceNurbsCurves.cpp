//
// Copyright 2023 Pixar
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
#include "pxr/usdImaging/usdImaging/dataSourceNurbsCurves.h"

#include "pxr/usdImaging/usdImaging/dataSourceSchemaBased.h"

#include "pxr/usd/usdGeom/nurbsCurves.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/nurbsCurvesSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

struct _NurbsCurvesTranslator
{
    static
    TfToken
    UsdAttributeNameToHdName(const TfToken &name)
    {
        // Suppress widths from UsdGeomCurves (which is a custom
        // primvar that UsdImagingDataSourceGprim gives us) and
        // pointWeights from UsdGeomNurbsCurves (which is a custom
        // primvar we process in the prim source below).
        if (name == UsdGeomTokens->widths ||
            name == UsdGeomTokens->pointWeights) {
            return TfToken();
        }

        return name;
    }

    static
    HdDataSourceLocator
    GetContainerLocator()
    {
        return HdNurbsCurvesSchema::GetDefaultLocator();
    }
};

// Data source for locator nurbsCurves
//
// Include (non-inherited) attributes of UsdGeomNurbsCurves and
// of UsdGeomCurves (to pick up curveVertexCounts).
using _NurbsCurvesDataSource = UsdImagingDataSourceSchemaBased<
    UsdGeomNurbsCurves, std::tuple<UsdGeomCurves>, _NurbsCurvesTranslator>;

const UsdImagingDataSourceCustomPrimvars::Mappings &
_GetCustomPrimvarMappings(const UsdPrim &usdPrim)
{
    static const TfToken &hdPrimvarName = UsdGeomTokens->pointWeights;

    static const UsdImagingDataSourceCustomPrimvars::Mappings mappings = {
        { hdPrimvarName, UsdGeomTokens->pointWeights },
    };

    return mappings;
}

}

// ----------------------------------------------------------------------------

UsdImagingDataSourceNurbsCurvesPrim::UsdImagingDataSourceNurbsCurvesPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceNurbsCurvesPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
    result.push_back(HdNurbsCurvesSchema::GetSchemaToken());
    return result;
}

HdDataSourceBaseHandle 
UsdImagingDataSourceNurbsCurvesPrim::Get(const TfToken & name)
{
    if (name == HdNurbsCurvesSchema::GetSchemaToken()) {
        return
            _NurbsCurvesDataSource::New(
                _GetSceneIndexPath(),
                UsdGeomNurbsCurves(_GetUsdPrim()),
                _GetStageGlobals());
    }

    if (name == HdPrimvarsSchema::GetSchemaToken()) {
        return
            HdOverlayContainerDataSource::New(
                HdContainerDataSource::Cast(
                    UsdImagingDataSourceGprim::Get(name)),
                UsdImagingDataSourceCustomPrimvars::New(
                    _GetSceneIndexPath(),
                    _GetUsdPrim(),
                    _GetCustomPrimvarMappings(_GetUsdPrim()),
                    _GetStageGlobals()));
    }

    return UsdImagingDataSourceGprim::Get(name);
}

HdDataSourceLocatorSet
UsdImagingDataSourceNurbsCurvesPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    TRACE_FUNCTION();

    HdDataSourceLocatorSet locators =
        _NurbsCurvesDataSource::Invalidate(
            subprim, properties);
    
    locators.insert(
        UsdImagingDataSourceGprim::Invalidate(
            prim, subprim, properties, invalidationType));

    locators.insert(
        UsdImagingDataSourceCustomPrimvars::Invalidate(
            properties, _GetCustomPrimvarMappings(prim)));

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
