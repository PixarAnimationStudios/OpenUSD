//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/dataSourceBindingAPI.h"

#include "pxr/usdImaging/usdSkelImaging/bindingSchema.h"

#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"

#include "pxr/usd/usdSkel/bindingAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

HdSampledDataSourceHandle
_DataSourceAuthoredAttributeFactory(
    const UsdAttribute &usdAttr,
    const UsdImagingDataSourceStageGlobals &stageGlobals,
    const SdfPath &sceneIndexPath,
    const HdDataSourceLocator &timeVaryingFlagLocator)
{
    UsdAttributeQuery query(usdAttr);
    if (!query.HasAuthoredValue()) {
        return nullptr;
    }

    return UsdImagingDataSourceAttributeNew(
        std::move(query), stageGlobals, sceneIndexPath, timeVaryingFlagLocator);
}

std::vector<UsdImagingDataSourceMapped::PropertyMapping>
_GetPropertyMappings()
{
    std::vector<UsdImagingDataSourceMapped::PropertyMapping> result;

    for (const TfToken &usdName :
             UsdSkelBindingAPI::GetSchemaAttributeNames(
                 /* includeInherited = */ false)) {

        const std::pair<std::string, bool> nameAndMatch =
            SdfPath::StripPrefixNamespace(
                usdName.GetString(), "skel");

        if (nameAndMatch.second) {
            result.push_back(
                UsdImagingDataSourceMapped::AttributeMapping{
                    usdName,
                    HdDataSourceLocator(TfToken(nameAndMatch.first)),
                    // The flattening scene index needs to know whether there
                    // is an authored opinion: if there is none, it needs to
                    // inherit the value from an ancestor.
                    // Thus, return nullptr if there is no authored opinion.
                    _DataSourceAuthoredAttributeFactory});
        }
    }

    result.push_back(
        UsdImagingDataSourceMapped::RelationshipMapping{
            UsdSkelTokens->skelAnimationSource,
            HdDataSourceLocator(
                UsdSkelImagingBindingSchemaTokens->animationSource),
            UsdImagingDataSourceMapped::
                GetPathFromRelationshipDataSourceFactory()});

    result.push_back(
        UsdImagingDataSourceMapped::RelationshipMapping{
            UsdSkelTokens->skelSkeleton,
            HdDataSourceLocator(
                UsdSkelImagingBindingSchemaTokens->skeleton),
            UsdImagingDataSourceMapped::
                GetPathFromRelationshipDataSourceFactory()});

    result.push_back(
        UsdImagingDataSourceMapped::RelationshipMapping{
            UsdSkelTokens->skelBlendShapeTargets,
            HdDataSourceLocator(
                UsdSkelImagingBindingSchemaTokens->blendShapeTargets),
            UsdImagingDataSourceMapped::
                GetPathArrayFromRelationshipDataSourceFactory()});

    return result;
}

const UsdImagingDataSourceMapped::PropertyMappings &
_GetMappings() {
    static const UsdImagingDataSourceMapped::PropertyMappings result(
        _GetPropertyMappings(),
        UsdSkelImagingBindingSchema::GetDefaultLocator());
    return result;
}

}

// ----------------------------------------------------------------------------

UsdSkelImagingDataSourceBindingAPI::
UsdSkelImagingDataSourceBindingAPI(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
 : _sceneIndexPath(sceneIndexPath)
 , _usdPrim(usdPrim)
 , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdSkelImagingDataSourceBindingAPI::GetNames()
{
    static const TfTokenVector result{
        UsdSkelImagingBindingSchema::GetSchemaToken() };
    return result;
}

HdDataSourceBaseHandle
UsdSkelImagingDataSourceBindingAPI::Get(const TfToken & name)
{
    if (name == UsdSkelImagingBindingSchema::GetSchemaToken()) {
        return
            UsdImagingDataSourceMapped::New(
                _usdPrim,
                _sceneIndexPath,
                _GetMappings(),
                _stageGlobals);
    }

    return nullptr;
}

HdDataSourceLocatorSet
UsdSkelImagingDataSourceBindingAPI::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    TRACE_FUNCTION();

    HdDataSourceLocatorSet locators =
        UsdImagingDataSourceMapped::Invalidate(
            properties, _GetMappings());

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
