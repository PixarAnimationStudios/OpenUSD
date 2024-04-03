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
