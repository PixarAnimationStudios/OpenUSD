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

#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

HdSampledDataSourceHandle
_AuthoredAttributeDataSourceFactory(
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

HdDataSourceBaseHandle
_PathFromRelationshipDataSourceFactory(
    const UsdRelationship &rel,
    const UsdImagingDataSourceStageGlobals &,
    const SdfPath &,
    const HdDataSourceLocator &)
{
    if (!rel.HasAuthoredTargets()) {
        return nullptr;
    }

    SdfPathVector result;
    rel.GetForwardedTargets(&result);

    using DS = HdRetainedTypedSampledDataSource<SdfPath>;

    if (result.empty()) {
        return DS::New(SdfPath());
    }
    return DS::New(std::move(result[0]));
}

std::vector<UsdImagingDataSourceMapped::PropertyMapping>
_GetPropertyMappings()
{
    return {
        UsdImagingDataSourceMapped::RelationshipMapping{
            UsdSkelTokens->skelAnimationSource,
            HdDataSourceLocator(
                UsdSkelImagingBindingSchemaTokens->animationSource),
            // Inherited.
            //
            // If not authored, this returns a nullptr and thus
            // the flattening scene index (through
            // HdFlattenedOverlayDataSourceProvider) picks it up from an
            // ancestor.
            //
            _PathFromRelationshipDataSourceFactory},

        UsdImagingDataSourceMapped::RelationshipMapping{
            UsdSkelTokens->skelSkeleton,
            HdDataSourceLocator(
                UsdSkelImagingBindingSchemaTokens->skeleton),
            // Inherited.
            //
            // Same as for skelAnimationSource applied.
            _PathFromRelationshipDataSourceFactory},

        UsdImagingDataSourceMapped::AttributeMapping{
            UsdSkelTokens->skelJoints,
            HdDataSourceLocator(
                UsdSkelImagingBindingSchemaTokens->joints),
            // Inherited.
            //
            // If not authored, this returns a nullptr and thus the
            // flattening scene index picks it up from an ancestor.
            _AuthoredAttributeDataSourceFactory},

        UsdImagingDataSourceMapped::AttributeMapping{
            UsdSkelTokens->skelBlendShapes,
            HdDataSourceLocator(
                UsdSkelImagingBindingSchemaTokens->blendShapes)
            // Not inherited
            //
            // The default factory always produces a data source.
            },

        UsdImagingDataSourceMapped::RelationshipMapping{
            UsdSkelTokens->skelBlendShapeTargets,
            HdDataSourceLocator(
                UsdSkelImagingBindingSchemaTokens->blendShapeTargets),
            // Not inherited.
            //
            // The factory always produces a data source.
            UsdImagingDataSourceMapped::
                GetPathArrayFromRelationshipDataSourceFactory()}};
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
