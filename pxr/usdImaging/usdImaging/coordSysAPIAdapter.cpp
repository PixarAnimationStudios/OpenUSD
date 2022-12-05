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
#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (coordSys)
    (binding)
);

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingCoordSysAPIAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

// ----------------------------------------------------------------------------

namespace
{

class _CoordsSysContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_CoordsSysContainerDataSource);

    _CoordsSysContainerDataSource(
            const UsdPrim &prim,
            const TfToken &name,
            const UsdImagingDataSourceStageGlobals &stageGlobals) :
        _coordSysAPI(prim, name),
        _stageGlobals(stageGlobals) {}

    TfTokenVector GetNames() override {
        return { _coordSysAPI.GetName() };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == _coordSysAPI.GetName()) {
            UsdShadeCoordSysAPI::Binding binding = 
                _coordSysAPI.GetLocalBinding();
            if (!binding.name.IsEmpty()) {
                return HdRetainedTypedSampledDataSource<SdfPath>::New(
                        binding.bindingRelPath);
            }
        }
        return nullptr;
    }

private:
    UsdShadeCoordSysAPI _coordSysAPI;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(_CoordsSysContainerDataSource);

} // anonymous namespace

// ----------------------------------------------------------------------------

HdContainerDataSourceHandle
UsdImagingCoordSysAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty() || appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    return HdRetainedContainerDataSource::New(
        HdCoordSysBindingSchemaTokens->coordSysBinding,
        _CoordsSysContainerDataSource::New(
            prim, appliedInstanceName, stageGlobals)
    );
}

HdDataSourceLocatorSet
UsdImagingCoordSysAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties)
{
    if (!subprim.IsEmpty() || appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    const TfTokenVector &tokens = 
        {_tokens->coordSys, appliedInstanceName, _tokens->binding};

    const std::string &bindingName = SdfPath::JoinIdentifier(tokens);

    for (const TfToken &propertyName : properties) {
        if (propertyName == bindingName) {
            return HdDataSourceLocator(
                HdCoordSysBindingSchemaTokens->coordSysBinding, 
                appliedInstanceName);
        }
    }

    return HdDataSourceLocatorSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
