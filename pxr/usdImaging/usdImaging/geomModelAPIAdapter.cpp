//
// Copyright 2023 Pixar
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
#include "pxr/usdImaging/usdImaging/geomModelAPIAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"
#include "pxr/usdImaging/usdImaging/geomModelSchema.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usd/modelAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

std::vector<UsdImagingDataSourceMapped::AttributeMapping>
_GetAttributeMappings()
{
    std::vector<UsdImagingDataSourceMapped::AttributeMapping> result;

    for (const TfToken &usdName :
             UsdGeomModelAPI::GetSchemaAttributeNames(
                 /* includeInherited = */ false)) {

        const std::pair<std::string, bool> nameAndMatch =
            SdfPath::StripPrefixNamespace(
                usdName.GetString(), "model");
        if (nameAndMatch.second) {
            result.push_back(
                { usdName,
                  HdDataSourceLocator(TfToken(nameAndMatch.first)) });
        }
    }

    return result;
}

const UsdImagingDataSourceMapped::AttributeMappings &
_GetMappings() {
    static const UsdImagingDataSourceMapped::AttributeMappings result(
        _GetAttributeMappings(), UsdImagingGeomModelSchema::GetDefaultLocator());
    return result;
}

}

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingGeomModelAPIAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

HdContainerDataSourceHandle
UsdImagingGeomModelAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    // Reflect UsdGeomModelAPI as UsdImagingGeomModelSchema.
    HdContainerDataSourceHandle geomModelDs =
        UsdImagingDataSourceMapped::New(
            prim, prim.GetPath(), _GetMappings(), stageGlobals);

    // For model components, overlay applyDrawMode=true.
    if (UsdModelAPI(prim).IsKind(KindTokens->component)) {
        static HdContainerDataSourceHandle const applyDrawModeDs =
            UsdImagingGeomModelSchema::Builder()
                .SetApplyDrawMode(
                    HdRetainedTypedSampledDataSource<bool>::New(true))
                .Build();
        geomModelDs = HdOverlayContainerDataSource::
            OverlayedContainerDataSources(applyDrawModeDs, geomModelDs);
    }
    
    return HdRetainedContainerDataSource::New(
        UsdImagingGeomModelSchema::GetSchemaToken(), geomModelDs);
}


HdDataSourceLocatorSet
UsdImagingGeomModelAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    return UsdImagingDataSourceMapped::Invalidate(
        properties, _GetMappings());
}

PXR_NAMESPACE_CLOSE_SCOPE
