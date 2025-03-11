//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/dataSourceSkeletonPrim.h"

#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"
#include "pxr/usdImaging/usdSkelImaging/skeletonSchema.h"

#include "pxr/usd/usdSkel/skeleton.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

std::vector<UsdImagingDataSourceMapped::PropertyMapping>
_GetPropertyMappings()
{
    std::vector<UsdImagingDataSourceMapped::PropertyMapping> result;

    for (const TfToken &usdName :
             UsdSkelSkeleton::GetSchemaAttributeNames(
                 /* includeInherited = */ false)) {
        result.push_back(
            UsdImagingDataSourceMapped::AttributeMapping{
                usdName, HdDataSourceLocator(usdName) });
    }

    return result;
}

const UsdImagingDataSourceMapped::PropertyMappings &
_GetMappings() {
    static const UsdImagingDataSourceMapped::PropertyMappings result(
        _GetPropertyMappings(),
        UsdSkelImagingSkeletonSchema::GetDefaultLocator());
    return result;
}

}

// ----------------------------------------------------------------------------

UsdSkelImagingDataSourceSkeletonPrim::UsdSkelImagingDataSourceSkeletonPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector
UsdSkelImagingDataSourceSkeletonPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
    result.push_back(UsdSkelImagingSkeletonSchema::GetSchemaToken());
    return result;
}

HdDataSourceBaseHandle
UsdSkelImagingDataSourceSkeletonPrim::Get(const TfToken & name)
{
    if (name == UsdSkelImagingSkeletonSchema::GetSchemaToken()) {
        return
            UsdImagingDataSourceMapped::New(
                _GetUsdPrim(),
                _GetSceneIndexPath(),
                _GetMappings(),
                _GetStageGlobals());
    }

    return UsdImagingDataSourceGprim::Get(name);
}

HdDataSourceLocatorSet
UsdSkelImagingDataSourceSkeletonPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    TRACE_FUNCTION();

    HdDataSourceLocatorSet locators =
        UsdImagingDataSourceMapped::Invalidate(
            properties, _GetMappings());

    locators.insert(
        UsdImagingDataSourceGprim::Invalidate(
            prim, subprim, properties, invalidationType));

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
