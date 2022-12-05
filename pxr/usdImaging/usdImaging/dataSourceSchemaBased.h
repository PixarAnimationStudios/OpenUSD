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

#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_SCHEMA_BASED_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_SCHEMA_BASED_H

#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceSchemaBased
///
/// A container data source created from a Usd schema which accesses the
/// attributes on the underlying Usd prim performing translation between
/// the Usd attribute name, the key in the container data source and the
/// data source locator.
///
/// The translation starts by taking the non-inherited attributes from the
/// the given UsdSchemaType (e.g., UsdGeomSphere) and calling
/// Translator::UsdAttributeNameToHdName(usdAttributeName) which
/// can either return the corresponding hydra token or an empty token if
/// the usd attribute should not occur in the data source.
///
/// The data source locator (relevant for invalidation) will be created
/// by appending the hydra token to the data source locator returned by
/// Translator::GetContainerLocator().
///
template<typename UsdSchemaType,
         typename Translator>
class UsdImagingDataSourceSchemaBased : public HdContainerDataSource
{
public:
    using This = UsdImagingDataSourceSchemaBased<UsdSchemaType, Translator>;

    HD_DECLARE_DATASOURCE(This);

    TfTokenVector GetNames() override {
        static const TfTokenVector names = _GetNamesUncached();
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        for (const _NameInfo &info : _GetNameInfos()) {
            if (info.hdName == name) {
                if (UsdAttribute attr =
                        _usdSchema.GetPrim().GetAttribute(
                            info.usdAttributeName)) {
                    return
                        UsdImagingDataSourceAttributeNew(
                            attr,
                            _stageGlobals,
                            _sceneIndexPath,
                            info.locator);
                } else {
                    // Has(name) has returned true, but we return
                    // nullptr - an inconsistency.
                    TF_CODING_ERROR(
                        "Could not get usd attribute '%s' even though "
                        "it is on the schema.",
                        info.usdAttributeName.GetText());
                    return nullptr;
                }
            }
        }
        return nullptr;
    }

    /// Translate usdNames to data source locators.
    static
    HdDataSourceLocatorSet
    Invalidate(const TfToken &subprim, const TfTokenVector &usdNames) {
        HdDataSourceLocatorSet locators;

        for (const TfToken &usdName : usdNames) {
            for (const _NameInfo &info : _GetNameInfos()) {
                if (info.usdAttributeName == usdName) {
                    locators.insert(info.locator);
                }
            }
        }

        return locators;
    }

private:
    // Private constructor, use static New() instead.
    UsdImagingDataSourceSchemaBased(
            const SdfPath &sceneIndexPath,
            UsdSchemaType usdSchema,
            const UsdImagingDataSourceStageGlobals &stageGlobals)
      : _sceneIndexPath(sceneIndexPath)
      , _usdSchema(usdSchema)
      , _stageGlobals(stageGlobals)
    {
    }

    struct _NameInfo
    {
        TfToken hdName;
        TfToken usdAttributeName;
        HdDataSourceLocator locator;
    };

    static
    std::vector<_NameInfo>
    _GetNameInfosUncached()
    {
        std::vector<_NameInfo> result;
        for (const TfToken &usdAttributeName :
                 UsdSchemaType::GetSchemaAttributeNames(
                     /* includeInherited = */ false))
        {
            const TfToken hdName = Translator::UsdAttributeNameToHdName(
                usdAttributeName);
            if (!hdName.IsEmpty()) {
                result.push_back(
                    { hdName,
                      usdAttributeName,
                      Translator::GetContainerLocator().Append(hdName) });
            }
        }
        return result;
    }

    static
    const std::vector<_NameInfo> &
    _GetNameInfos()
    {
        static const std::vector<_NameInfo> result = _GetNameInfosUncached();
        return result;
    }

    static
    TfTokenVector
    _GetNamesUncached()
    {
        TfTokenVector result;
        for (const _NameInfo &info : _GetNameInfos()) {
            result.push_back(info.hdName);
        }
        return result;
    }

private:
    const SdfPath _sceneIndexPath;
    UsdSchemaType _usdSchema;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_IMPLICITS_IMPL_H
