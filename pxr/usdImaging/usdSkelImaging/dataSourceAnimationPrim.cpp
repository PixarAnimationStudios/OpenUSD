//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/dataSourceAnimationPrim.h"

#include "pxr/usdImaging/usdSkelImaging/animationSchema.h"

#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"

#include "pxr/usd/usdSkel/animation.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

std::vector<UsdImagingDataSourceMapped::PropertyMapping>
_GetPropertyMappings()
{
    std::vector<UsdImagingDataSourceMapped::PropertyMapping> result;

    for (const TfToken &usdName :
             UsdSkelAnimation::GetSchemaAttributeNames(
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
        UsdSkelImagingAnimationSchema::GetDefaultLocator());
    return result;
}

}

// ----------------------------------------------------------------------------

UsdSkelImagingDataSourceAnimationPrim::
UsdSkelImagingDataSourceAnimationPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
 : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector
UsdSkelImagingDataSourceAnimationPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourcePrim::GetNames();
    result.push_back(
        UsdSkelImagingAnimationSchema::GetSchemaToken());
    return result;
}

HdDataSourceBaseHandle
UsdSkelImagingDataSourceAnimationPrim::Get(const TfToken & name)
{
    if (name == UsdSkelImagingAnimationSchema::GetSchemaToken()) {
        return
            UsdImagingDataSourceMapped::New(
                _GetUsdPrim(),
                _GetSceneIndexPath(),
                _GetMappings(),
                _GetStageGlobals());
    }

    return UsdImagingDataSourcePrim::Get(name);
}

HdDataSourceLocatorSet
UsdSkelImagingDataSourceAnimationPrim::Invalidate(
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
