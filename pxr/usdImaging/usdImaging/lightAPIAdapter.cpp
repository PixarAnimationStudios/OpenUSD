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
#include "pxr/usdImaging/usdImaging/lightAPIAdapter.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"


#include "pxr/usdImaging/usdImaging/dataSourceMaterial.h"

#include "pxr/usd/usdLux/lightAPI.h"

#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingLightAPIAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

namespace
{

class _LightDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_LightDataSource);

    TfTokenVector
    GetNames() override
    {
        return _GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (name == HdTokens->filters) {
            SdfPathVector filterPaths;
            _lightApi.GetFiltersRel().GetForwardedTargets(&filterPaths);
            return HdCreateTypedRetainedDataSource(VtValue(filterPaths));

        }
        if (name == HdTokens->isLight) {
            return HdRetainedTypedSampledDataSource<bool>::New(true);
        }
        if (name == HdTokens->materialSyncMode) {
            if (UsdAttribute attr = _lightApi.GetMaterialSyncModeAttr()) {
                TfToken v;
                if (attr.Get(&v)) {
                    return HdRetainedTypedSampledDataSource<TfToken>::New(v);
                }
            }
        } else {

            // fallback to UsdAttribute lookup so that we still support
            // render delegates which query via GetLightParamValue rather
            // than GetMaterialResource.
            if (UsdAttribute attr =
                    UsdImagingPrimAdapter::LookupLightParamAttribute(
                        _lightApi.GetPrim(), name)) {
                return  UsdImagingDataSourceAttributeNew(
                    attr,
                    _stageGlobals,
                    _lightApi.GetPrim().GetPath(),
                    HdLightSchema::GetDefaultLocator().Append(name));
            }
        }

        return nullptr;
    }

private:

    static const TfTokenVector & _GetNames()
    {
        // Light linking fields 'lightLink' and 'shadowLink' that provide the
        // category ID for the corresponding collection  are computed by a
        // scene index downstream.
        // The collections are transported by collectionAPIAdapter.
        //
        static const TfTokenVector names = {
            HdTokens->filters,
            HdTokens->isLight,
            HdTokens->materialSyncMode,
        };

        return names;
    }

    _LightDataSource(
        const UsdLuxLightAPI &lightApi,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _lightApi(lightApi)
    , _stageGlobals(stageGlobals)
    {}

    UsdLuxLightAPI _lightApi;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

} // namespace anonymous

HdContainerDataSourceHandle
UsdImagingLightAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    if (subprim.IsEmpty()) {
        return HdRetainedContainerDataSource::New(
            HdPrimTypeTokens->material,
            UsdImagingDataSourceMaterial::New(
                prim,
                stageGlobals,
                HdMaterialTerminalTokens->light),
            HdLightSchemaTokens->light,
            _LightDataSource::New(UsdLuxLightAPI(prim), stageGlobals));
    }

    return nullptr;
}


HdDataSourceLocatorSet
UsdImagingLightAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    bool dirtiedMaterial = false;
    bool dirtiedLight = false;

    HdDataSourceLocatorSet result;
    for (const TfToken &propertyName : properties) {
        if (!dirtiedMaterial
                && TfStringStartsWith(propertyName.GetString(), "inputs:")) {
            dirtiedMaterial = true;
            // NOTE: since we don't have access to the prim itself and our
            //       light terminal is currently named for the USD path,
            //       we cannot be specific to the individual parameter.
            //       TODO: Consider whether we want to make the terminal node
            //             in the material network have a fixed name for the
            //             light case so that we could.
            result.insert(HdMaterialSchema::GetDefaultLocator());

            // since we report parameter values in the "light" data source
            // also, we need to invalidate it also
            result.insert(HdLightSchema::GetDefaultLocator());
        }

        if (!dirtiedLight && !dirtiedMaterial &&
            // This will capture other contents of light data source
            TfStringStartsWith(propertyName.GetString(), "light:")) {
            dirtiedLight = true;
            result.insert(HdLightSchema::GetDefaultLocator());
        }
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
