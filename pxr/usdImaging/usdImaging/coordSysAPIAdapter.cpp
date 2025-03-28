//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/coordSysAPIAdapter.h"

#include "pxr/usd/usdShade/coordSysAPI.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/coordSysSchema.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingCoordSysAPIAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

// ----------------------------------------------------------------------------

HdContainerDataSourceHandle
UsdImagingCoordSysAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    if (subprim.IsEmpty()) {
        UsdShadeCoordSysAPI::Binding binding =
            UsdShadeCoordSysAPI(prim, appliedInstanceName).GetLocalBinding();
        if (binding.name.IsEmpty()) {
            return nullptr;
        }

        return HdRetainedContainerDataSource::New(
            HdCoordSysBindingSchemaTokens->coordSysBinding,
            HdRetainedContainerDataSource::New(
                appliedInstanceName,
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    binding.coordSysPrimPath)));
    }

    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingCoordSysAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    if (subprim.IsEmpty()) {
        for (const TfToken &propertyName : properties) {
             // Could use coord sys name for more targeted invalidation
             // to improve performance.
            if (UsdShadeCoordSysAPI::CanContainPropertyName(propertyName)) {
                return HdCoordSysBindingSchema::GetDefaultLocator();
            }
        }
    }

    return HdDataSourceLocatorSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
