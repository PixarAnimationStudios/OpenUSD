//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    std::vector<UsdImagingDataSourceMapped::PropertyMapping>
    _GetPropertyMappings() {
        std::vector<UsdImagingDataSourceMapped::PropertyMapping> result;

        for (const TfToken &usdName :
                 UsdSchemaType::GetSchemaAttributeNames(
                     /* includeInherited = */ false)) {
            if (usdName == UsdGeomTokens->extent) {
                // Skip extent since this is already dealt with
                // in UsdImagingDataSourcePrim::Get. 
                continue;
            }
            result.push_back(
                UsdImagingDataSourceMapped::AttributeMapping{
                    usdName, HdDataSourceLocator(usdName)});
        }
        return result;
    }
    
    static
    const UsdImagingDataSourceMapped::PropertyMappings &
    _GetMappings() {
        static const UsdImagingDataSourceMapped::PropertyMappings result(
            _GetPropertyMappings(), HdSchemaType::GetDefaultLocator());
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
