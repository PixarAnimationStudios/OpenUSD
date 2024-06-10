//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdVolImaging/dataSourceFieldAsset.h"
#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"
#include "pxr/usd/usdVol/openVDBAsset.h"
#include "pxr/usd/usdVol/field3DAsset.h"

#include "pxr/imaging/hd/volumeFieldSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceFieldAsset::UsdImagingDataSourceFieldAsset(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _sceneIndexPath(sceneIndexPath)
    , _usdPrim(usdPrim)
    , _stageGlobals(stageGlobals)
{
}

UsdImagingDataSourceFieldAsset::~UsdImagingDataSourceFieldAsset() = default;

static
TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}

template<typename FieldSubclass>
static
const TfTokenVector &
_GetStaticUsdAttributeNames()
{
    // Get names from the subclass of UsdVolFieldAsset and
    // names inherited from FieldAsset schema but stop there
    // and do not pick up, e.g., the xform attributes which are
    // handled elsewhere and under a different data source
    // locator.
    //
    static TfTokenVector result = _ConcatenateAttributeNames(
        UsdVolFieldAsset::GetSchemaAttributeNames(
            /* includeInherited = */ false),
        FieldSubclass::GetSchemaAttributeNames(
            /* includeInherited = */ false));
    return result;
}

static
const TfTokenVector &
_GetUsdAttributeNames(UsdPrim usdPrim)
{
    if (usdPrim.IsA<UsdVolOpenVDBAsset>()) {
        return _GetStaticUsdAttributeNames<UsdVolOpenVDBAsset>();
    }

    if (usdPrim.IsA<UsdVolField3DAsset>()) {
        return _GetStaticUsdAttributeNames<UsdVolField3DAsset>();
    }

    TF_CODING_ERROR("Unsupported field type.");

    static TfTokenVector empty;
    return empty;
}

TfTokenVector
UsdImagingDataSourceFieldAsset::GetNames()
{
    return _GetUsdAttributeNames(_usdPrim);
}

HdDataSourceBaseHandle
UsdImagingDataSourceFieldAsset::Get(const TfToken &name)
{
    if (UsdAttribute attr = _usdPrim.GetAttribute(name)) {
        return UsdImagingDataSourceAttributeNew(
            attr,
            _stageGlobals,
            _sceneIndexPath,
            HdVolumeFieldSchema::GetDefaultLocator().Append(name));
    } else {
        return nullptr;
    }
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceFieldAssetPrim::UsdImagingDataSourceFieldAssetPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceFieldAssetPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourcePrim::GetNames();
    result.push_back(HdVolumeFieldSchemaTokens->volumeField);
    return result;
}

HdDataSourceBaseHandle 
UsdImagingDataSourceFieldAssetPrim::Get(const TfToken & name)
{
    if (name == HdVolumeFieldSchemaTokens->volumeField) {
        return UsdImagingDataSourceFieldAsset::New(
                _GetSceneIndexPath(),
                _GetUsdPrim(),
                _GetStageGlobals());
    } 

    return UsdImagingDataSourcePrim::Get(name);
}

HdDataSourceLocatorSet
UsdImagingDataSourceFieldAssetPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet locators =
        UsdImagingDataSourcePrim::Invalidate(
            prim, subprim, properties, invalidationType);

    TfTokenVector fieldNames;
    if (prim.IsA<UsdVolOpenVDBAsset>()) {
        fieldNames =
            _GetStaticUsdAttributeNames<UsdVolOpenVDBAsset>();
    } else if (prim.IsA<UsdVolField3DAsset>()) {
        fieldNames =
            _GetStaticUsdAttributeNames<UsdVolField3DAsset>();
    } else {
        TF_CODING_ERROR("Unsupported field type.");
        return locators;
    }

    for (const TfToken &propertyName : properties) {
        if (std::find(fieldNames.begin(), fieldNames.end(), propertyName)
                != fieldNames.end()) {
            locators.insert(HdVolumeFieldSchema::GetDefaultLocator());
            break;
        }
    }

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
