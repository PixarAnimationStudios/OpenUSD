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

#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_IMPLICITS_IMPL_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_IMPLICITS_IMPL_H

#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/usdImaging/usdImaging/dataSourceSchemaBased.h"

PXR_NAMESPACE_OPEN_SCOPE

template<typename HdSchemaType>
struct UsdImagingImplicitsSchemaTranslator
{
    static
    TfToken
    UsdAttributeNameToHdName(const TfToken &name)
    {
        // Skip extent since this is already dealt with
        // in UsdImagingDataSourcePrim::Get. 
        if (name == UsdGeomTokens->extent) {
            return TfToken();
        }
        return name;
    }

    static
    HdDataSourceLocator
    GetContainerLocator()
    {
        return HdSchemaType::GetDefaultLocator();
    }
};

/// \class UsdImagingDataSourceImplicitsPrim
///
/// A prim data source for a cube, ...
///
template<typename UsdSchemaType, typename HdSchemaType>
class UsdImagingDataSourceImplicitsPrim : public UsdImagingDataSourceGprim
{
public:
    using This =
        UsdImagingDataSourceImplicitsPrim<UsdSchemaType, HdSchemaType>;

    HD_DECLARE_DATASOURCE(This);

    TfTokenVector GetNames() override {
        TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
        result.push_back(_GetLocatorToken());
        return result;

    }
    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == _GetLocatorToken()) {
            return
                _DataSource::New(
                    _GetSceneIndexPath(),
                    UsdSchemaType(_GetUsdPrim()),
                    _GetStageGlobals());
        }

        return UsdImagingDataSourceGprim::Get(name);
    }

    static
    HdDataSourceLocatorSet
    Invalidate(UsdPrim const& prim,
               const TfToken &subprim,
               const TfTokenVector &properties,
               const UsdImagingPropertyInvalidationType invalidationType) {
        HdDataSourceLocatorSet locators =
            _DataSource::Invalidate(
                subprim, properties);

        locators.insert(
            UsdImagingDataSourceGprim::Invalidate(
                prim, subprim, properties, invalidationType));

        return locators;
    }

private:
    using _DataSource =
        UsdImagingDataSourceSchemaBased<
            UsdSchemaType,
            /* UsdSchemaBaseTypes = */ std::tuple<>,
            UsdImagingImplicitsSchemaTranslator<HdSchemaType>>;

    // Private constructor, use static New() instead.
    UsdImagingDataSourceImplicitsPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
      : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
    {
    }

    static 
    TfToken
    _GetLocatorTokenUncached() {
        const HdDataSourceLocator locator =
            HdSchemaType::GetDefaultLocator();
        if (locator.GetElementCount() != 1) {
            TF_CODING_ERROR("Expected data source locator with one element.");
            return TfToken();
        }
        return locator.GetFirstElement();
    }

    static
    const TfToken &
    _GetLocatorToken() {
        static const TfToken result = _GetLocatorTokenUncached();
        return result;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_IMPLICITS_IMPL_H
