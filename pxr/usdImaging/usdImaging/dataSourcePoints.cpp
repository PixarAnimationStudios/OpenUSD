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
#include "pxr/usdImaging/usdImaging/dataSourcePoints.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrimvars.h"

#include "pxr/usd/usdGeom/points.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourcePointsPrim::UsdImagingDataSourcePointsPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
{
}

static
const UsdImagingDataSourceCustomPrimvars::Mappings &
_GetCustomPrimvarMappings(const UsdPrim &usdPrim)
{
    static const UsdImagingDataSourceCustomPrimvars::Mappings mappings = {
        {HdPrimvarsSchemaTokens->widths, UsdGeomTokens->widths},
    };

    return mappings;
}

HdDataSourceBaseHandle
UsdImagingDataSourcePointsPrim::Get(const TfToken &name)
{
    HdDataSourceBaseHandle const result = UsdImagingDataSourceGprim::Get(name);

    if (name == HdPrimvarsSchema::GetSchemaToken()) {
        return
            HdOverlayContainerDataSource::New(
                HdContainerDataSource::Cast(result),
                UsdImagingDataSourceCustomPrimvars::New(
                    _GetSceneIndexPath(),
                    _GetUsdPrim(),
                    _GetCustomPrimvarMappings(_GetUsdPrim()),
                    _GetStageGlobals()));
    }

    return result;
}

/*static*/
HdDataSourceLocatorSet
UsdImagingDataSourcePointsPrim::Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim,
        const TfTokenVector &properties,
        const UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet result =
        UsdImagingDataSourceGprim::Invalidate(
            prim, subprim, properties, invalidationType);

    if (subprim.IsEmpty()) {
        result.insert(
            UsdImagingDataSourceCustomPrimvars::Invalidate(
                properties, _GetCustomPrimvarMappings(prim)));
    }

    return result;
}


PXR_NAMESPACE_CLOSE_SCOPE
