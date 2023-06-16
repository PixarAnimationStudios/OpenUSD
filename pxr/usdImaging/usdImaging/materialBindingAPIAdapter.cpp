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
#include "pxr/usdImaging/usdImaging/materialBindingAPIAdapter.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingMaterialBindingAPIAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

// ----------------------------------------------------------------------------

namespace
{

class _MaterialBindingsContainerDataSource : public HdContainerDataSource
{
public:

    HD_DECLARE_DATASOURCE(_MaterialBindingsContainerDataSource);

    _MaterialBindingsContainerDataSource(const UsdPrim &prim)
    : _mbApi(prim) {
    }

    TfTokenVector GetNames() override {
        return _mbApi.GetMaterialPurposes();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        UsdRelationship bindingRel = _mbApi.GetDirectBindingRel(name);
        if (!bindingRel) {
            return nullptr;
        }

        UsdShadeMaterialBindingAPI::DirectBinding db(bindingRel);
        if (!db.IsBound()) {
            return nullptr;
        }

        return
            HdMaterialBindingSchema::Builder()
                .SetPath(
                    HdRetainedTypedSampledDataSource<SdfPath>::New(
                        db.GetMaterialPath()))
                .SetBindingStrength(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        UsdShadeMaterialBindingAPI::GetMaterialBindingStrength(
                            bindingRel)))
                .Build();
    }

private:
    UsdShadeMaterialBindingAPI _mbApi;
};

HD_DECLARE_DATASOURCE_HANDLES(_MaterialBindingsContainerDataSource);

} // anonymous namespace

// ----------------------------------------------------------------------------

HdContainerDataSourceHandle
UsdImagingMaterialBindingAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    return HdRetainedContainerDataSource::New(
        HdMaterialBindingsSchema::GetSchemaToken(),
        _MaterialBindingsContainerDataSource::New(prim)
    );
}

HdDataSourceLocatorSet
UsdImagingMaterialBindingAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{

    // QUESTION: We aren't ourselves creating any subprims but do we need to
    //           contribute to them?
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    for (const TfToken &propertyName : properties) {
        if (UsdShadeMaterialBindingAPI::CanContainPropertyName(propertyName)) {
            return HdMaterialBindingsSchema::GetDefaultLocator();
        }
    }

    return HdDataSourceLocatorSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
