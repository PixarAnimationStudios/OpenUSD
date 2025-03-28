//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"

#include "pxr/usd/usdGeom/curves.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

static
UsdImagingDataSourceCustomPrimvars::Mappings
_Merge(const UsdImagingDataSourceCustomPrimvars::Mappings &a,
       const UsdImagingDataSourceCustomPrimvars::Mappings &b)
{
    UsdImagingDataSourceCustomPrimvars::Mappings result = a;
    for (const auto &mapping : b) {
        result.push_back(mapping);
    }
    return result;
}

static
const UsdImagingDataSourceCustomPrimvars::Mappings &
_GetCustomPrimvarMappings(const UsdPrim &usdPrim)
{
    if (usdPrim.IsA<UsdGeomPointBased>()) {
        static const UsdImagingDataSourceCustomPrimvars::Mappings
            forPointBased = {
                {HdPrimvarsSchemaTokens->points,
                    UsdGeomTokens->points},
                {HdTokens->velocities,
                    UsdGeomTokens->velocities},
                {HdTokens->accelerations,
                    UsdGeomTokens->accelerations},
                {HdTokens->nonlinearSampleCount,
                    UsdGeomTokens->motionNonlinearSampleCount},
                {HdTokens->blurScale,
                    UsdGeomTokens->motionBlurScale},
                {HdPrimvarsSchemaTokens->normals,
                    UsdGeomTokens->normals},
            };

        if (usdPrim.IsA<UsdGeomCurves>()) {
            static const UsdImagingDataSourceCustomPrimvars::Mappings
                forCurves = _Merge(
                    forPointBased,
                    {
                        {HdPrimvarsSchemaTokens->widths,
                             UsdGeomTokens->widths}});
            return forCurves;
        }

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
    HdDataSourceBaseHandle const result = UsdImagingDataSourcePrim::Get(name);
    if (name == HdPrimvarsSchema::GetSchemaToken()) {
        const UsdImagingDataSourceCustomPrimvars::Mappings &mappings = 
            _GetCustomPrimvarMappings(_GetUsdPrim());
        if (mappings.empty()) {
            return result;
        }
        return
            HdOverlayContainerDataSource::New(
                HdContainerDataSource::Cast(result),
                // Note that a attribute such as "normals" (which we
                // get through the custom primvars data source) is
                // weaker than the preferred form "primvars:normals"
                // (which we get from the base implementation
                // UsdImagingDataSourcePrim::Get).
                UsdImagingDataSourceCustomPrimvars::New(
                    _GetSceneIndexPath(),
                    _GetUsdPrim(),
                    mappings,
                    _GetStageGlobals()));
    }

    return result;
}

/*static*/
HdDataSourceLocatorSet
UsdImagingDataSourceGprim::Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim,
        const TfTokenVector &properties,
        UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet result =
        UsdImagingDataSourcePrim::Invalidate(
            prim, subprim, properties, invalidationType);

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
