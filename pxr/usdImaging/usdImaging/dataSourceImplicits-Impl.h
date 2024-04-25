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
#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"

PXR_NAMESPACE_OPEN_SCOPE

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
        result.push_back(HdSchemaType::GetSchemaToken());
        return result;

    }
    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdSchemaType::GetSchemaToken()) {
            return
                UsdImagingDataSourceMapped::New(
                    _GetUsdPrim(),
                    _GetSceneIndexPath(),
                    _GetMappings(),
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
            UsdImagingDataSourceMapped::Invalidate(
                properties, _GetMappings());

        locators.insert(
            UsdImagingDataSourceGprim::Invalidate(
                prim, subprim, properties, invalidationType));

        return locators;
    }

private:
    static
    std::vector<UsdImagingDataSourceMapped::AttributeMapping>
    _GetAttributeMappings() {
        std::vector<UsdImagingDataSourceMapped::AttributeMapping> result;

        for (const TfToken &usdName :
                 UsdSchemaType::GetSchemaAttributeNames(
                     /* includeInherited = */ false)) {
            if (usdName == UsdGeomTokens->extent) {
                // Skip extent since this is already dealt with
                // in UsdImagingDataSourcePrim::Get. 
                continue;
            }
            result.push_back({ usdName, HdDataSourceLocator(usdName)});
        }
        return result;
    }
    
    static
    const UsdImagingDataSourceMapped::AttributeMappings &
    _GetMappings() {
        static const UsdImagingDataSourceMapped::AttributeMappings result(
            _GetAttributeMappings(), HdSchemaType::GetDefaultLocator());
        return result;
    }

    // Private constructor, use static New() instead.
    UsdImagingDataSourceImplicitsPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
      : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
    {
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_IMPLICITS_IMPL_H
