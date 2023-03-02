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
#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"

#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

static
const UsdImagingDataSourceCustomPrimvars::Mappings &
_GetCustomPrimvarMappings(const UsdPrim &usdPrim)
{
    if (usdPrim.IsA<UsdGeomPointBased>()) {
        static const UsdImagingDataSourceCustomPrimvars::Mappings
            forPointBased = {
                {HdTokens->points,
                    UsdGeomTokens->points},
                {HdTokens->velocities,
                    UsdGeomTokens->velocities},
                {HdTokens->accelerations,
                    UsdGeomTokens->accelerations},
                {HdTokens->nonlinearSampleCount,
                    UsdGeomTokens->motionNonlinearSampleCount},
                {HdTokens->blurScale,
                    UsdGeomTokens->motionBlurScale},
                {HdTokens->normals,
                    UsdGeomTokens->normals},
            };

        return forPointBased;
    }

    static const UsdImagingDataSourceCustomPrimvars::Mappings empty;
    return empty;
}

UsdImagingDataSourceGprim::UsdImagingDataSourceGprim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

HdDataSourceBaseHandle
UsdImagingDataSourceGprim::Get(const TfToken &name)
{
    HdDataSourceBaseHandle result = UsdImagingDataSourcePrim::Get(name);
    if (name == HdPrimvarsSchema::GetDefaultLocator().GetFirstElement()) {
        const UsdImagingDataSourceCustomPrimvars::Mappings &mappings = 
            _GetCustomPrimvarMappings(_GetUsdPrim());
        if (!mappings.empty()) {

            HdContainerDataSourceHandle customPvs =
                UsdImagingDataSourceCustomPrimvars::New(
                    _GetSceneIndexPath(),
                    _GetUsdPrim(),
                    mappings,
                    _GetStageGlobals());

            if (HdContainerDataSourceHandle basePvs =
                    HdContainerDataSource::Cast(result)) {
                result = HdOverlayContainerDataSource::New(basePvs, customPvs);
            } else {
                result = customPvs;
            }
        }
    }

    return result;
}

/*static*/
HdDataSourceLocatorSet
UsdImagingDataSourceGprim::Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim,
        const TfTokenVector &properties)
{
    HdDataSourceLocatorSet result =
        UsdImagingDataSourcePrim::Invalidate(prim, subprim, properties);

    if (subprim.IsEmpty()) {
        const UsdImagingDataSourceCustomPrimvars::Mappings &mappings = 
            _GetCustomPrimvarMappings(prim);

        if (!mappings.empty()) {
            result.insert(UsdImagingDataSourceCustomPrimvars::Invalidate(
                properties, mappings));
        }
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
