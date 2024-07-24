//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dataSourceVolume.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/volumeFieldBindingSchema.h"

#include "pxr/usd/usdVol/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceVolumeFieldBindings
::UsdImagingDataSourceVolumeFieldBindings(
        UsdVolVolume usdVolume,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _usdVolume(usdVolume)
    , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdImagingDataSourceVolumeFieldBindings::GetNames()
{
    TRACE_FUNCTION();

    // XXX: This is more expensive than necessary, because we compute
    // relationship targets in addition to enumerating relationships.
    // Maybe ask for a UsdVolVolume.GetFieldRelationships call?
    const UsdVolVolume::FieldMap fields = _usdVolume.GetFieldPaths();
    TfTokenVector names;
    for (auto const& pair : fields) {
        names.push_back(pair.first);
    }
    return names;
}

HdDataSourceBaseHandle
UsdImagingDataSourceVolumeFieldBindings::Get(const TfToken &name)
{
    TRACE_FUNCTION();

    const SdfPath path = _usdVolume.GetFieldPath(name);
    if (path.IsEmpty()) {
        return nullptr;
    }

    return HdRetainedTypedSampledDataSource<SdfPath>::New(path);
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceVolumePrim::UsdImagingDataSourceVolumePrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceVolumePrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
    result.push_back(HdVolumeFieldBindingSchema::GetSchemaToken());

    return result;
}

HdDataSourceBaseHandle
UsdImagingDataSourceVolumePrim::Get(const TfToken &name)
{
    if (name == HdVolumeFieldBindingSchema::GetSchemaToken()) {
        return UsdImagingDataSourceVolumeFieldBindings::New(
            UsdVolVolume(_GetUsdPrim()), _GetStageGlobals());
    } else {
        return UsdImagingDataSourceGprim::Get(name);
    }
}

HdDataSourceLocatorSet
UsdImagingDataSourceVolumePrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet locators =
        UsdImagingDataSourceGprim::Invalidate(
            prim, subprim, properties, invalidationType);

    static const std::string fieldPrefix =
        UsdVolTokens->field.GetString() + ":";

    for (const TfToken &propertyName : properties) {
        if (TfStringStartsWith(propertyName.GetString(), fieldPrefix)) {
            // There doesn't seem to be any client that can make use of
            // fine-grained invalidation where we sent the sub data source
            // locator of the volume field binding corresponding to this field.
            locators.insert(HdVolumeFieldBindingSchema::GetDefaultLocator());
            break;
        }
    }

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
