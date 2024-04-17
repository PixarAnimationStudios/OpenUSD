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

#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImagingDataSourceSchemaBased_Impl
{
struct _Mapping;
};

/// \class UsdImagingDataSourceSchemaBased
///
/// A container data source created from a Usd schema and optionally some
/// of its base classes which accesses the attributes on the underlying Usd
/// prim performing translation between the Usd attribute name and the key in
/// the container data source (for implementing Get) or the data source
/// locator (for implementing Invalidate).
///
/// The translation starts by taking the non-inherited attributes from the
/// the given UsdSchemaType (e.g., UsdGeomSphere) and the given UsdSchemaBaseTypes
/// and calling Translator::UsdAttributeNameToHdName(usdAttributeName) which
/// can either return the corresponding hydra token or an empty token if
/// the usd attribute should not occur in the data source.
///
/// UsdSchemaBaseTypes is a std::tuple of the base schema types that should
/// also be considered and can be std::tuple<> if there is no base schema or
/// no attribute of a base schema should be included.
///
/// The data source locator (relevant for invalidation) will be created
/// by appending the hydra token to the data source locator returned by
/// Translator::GetContainerLocator().
///
template<typename UsdSchemaType,
         typename UsdSchemaBaseTypes,
         typename Translator>
class UsdImagingDataSourceSchemaBased : public HdContainerDataSource
{
public:
    using This = UsdImagingDataSourceSchemaBased<
        UsdSchemaType, UsdSchemaBaseTypes, Translator>;

    HD_DECLARE_DATASOURCE(This);

    TfTokenVector GetNames() override;

    HdDataSourceBaseHandle Get(const TfToken &name) override;

    /// Translate usdNames to data source locators.
    static
    HdDataSourceLocatorSet
    Invalidate(const TfToken &subprim, const TfTokenVector &usdNames);

private:
    // Private constructor, use static New() instead.
    UsdImagingDataSourceSchemaBased(
            const SdfPath &sceneIndexPath,
            UsdSchemaType usdSchema,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

    using _Mapping = UsdImagingDataSourceSchemaBased_Impl::_Mapping;

    static const std::vector<_Mapping> &_GetMappings();

private:
    const SdfPath _sceneIndexPath;
    UsdSchemaType _usdSchema;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

/// Implementation

namespace UsdImagingDataSourceSchemaBased_Impl
{

struct _Mapping
{
    TfToken usdAttributeName;
    TfToken hdName;
    HdDataSourceLocator locator;
};

template<typename Translator,
         typename ThisUsdSchemaType>
void _FillMappings(std::vector<_Mapping> * const result)
{
    for (const TfToken &usdAttributeName :
             ThisUsdSchemaType::GetSchemaAttributeNames(
                 /* includeInherited = */ false))
    {
        const TfToken hdName =
            Translator::UsdAttributeNameToHdName(usdAttributeName);
        if (!hdName.IsEmpty()) {
            result->push_back(
                { usdAttributeName,
                  hdName,
                  Translator::GetContainerLocator().Append(hdName) });
        }
    }
}

template<typename Translator,
         typename UsdSchemaBaseTypes>
struct _MappingsFiller;

template<typename Translator>
struct _MappingsFiller<Translator, std::tuple<>>
{
    static void Fill(std::vector<_Mapping> * const result)
    {
    }
};

template<typename Translator,
         typename UsdSchemaType,
         typename ...UsdSchemaTypes>
struct _MappingsFiller<Translator, std::tuple<UsdSchemaType, UsdSchemaTypes...>>
{
    static void Fill(std::vector<_Mapping> * const result)
    {
        _FillMappings<Translator, UsdSchemaType>(result);
        _MappingsFiller<Translator, std::tuple<UsdSchemaTypes...>>::Fill(result);
    }
};

template<typename UsdSchemaType,
         typename UsdSchemaBasesTypes,
         typename Translator>
std::vector<_Mapping>
_GetMappings()
{
    std::vector<_Mapping> result;
    
    _FillMappings<Translator, UsdSchemaType>(&result);
    _MappingsFiller<Translator, UsdSchemaBasesTypes>::Fill(&result);

    return result;
}

TfTokenVector
inline _GetNames(const std::vector<_Mapping> &mappings)
{
    TfTokenVector result;
    for (const _Mapping &mapping : mappings) {
        result.push_back(mapping.hdName);
    }
    return result;
}

} // namespace UsdImagingDataSourceSchemaBased_Impl

template<typename UsdSchemaType,
         typename UsdSchemaBasesTypes,
         typename Translator>
TfTokenVector
UsdImagingDataSourceSchemaBased<UsdSchemaType,UsdSchemaBasesTypes,Translator>::
GetNames()
{
    static const TfTokenVector names =
        UsdImagingDataSourceSchemaBased_Impl::_GetNames(_GetMappings());
    return names;
}

template<typename UsdSchemaType,
         typename UsdSchemaBasesTypes,
         typename Translator>
HdDataSourceBaseHandle
UsdImagingDataSourceSchemaBased<UsdSchemaType,UsdSchemaBasesTypes,Translator>::
Get(const TfToken &name)
{
    for (const _Mapping &mapping : _GetMappings()) {
        if (mapping.hdName == name) {
            if (UsdAttribute attr =
                    _usdSchema.GetPrim().GetAttribute(
                        mapping.usdAttributeName)) {
                return
                    UsdImagingDataSourceAttributeNew(
                        attr,
                        _stageGlobals,
                        _sceneIndexPath,
                        mapping.locator);
            } else {
                // Has(name) has returned true, but we return
                // nullptr - an inconsistency.
                TF_CODING_ERROR(
                    "Could not get usd attribute '%s' even though "
                    "it is on the schema.",
                    mapping.usdAttributeName.GetText());
                return nullptr;
            }
        }
    }
    return nullptr;
}

template<typename UsdSchemaType,
         typename UsdSchemaBasesTypes,
         typename Translator>
HdDataSourceLocatorSet
UsdImagingDataSourceSchemaBased<UsdSchemaType,UsdSchemaBasesTypes,Translator>::
Invalidate(
    const TfToken &subprim,
    const TfTokenVector &usdNames)
{
    HdDataSourceLocatorSet locators;
    
    for (const TfToken &usdName : usdNames) {
        for (const _Mapping &mapping : _GetMappings()) {
            if (mapping.usdAttributeName == usdName) {
                locators.insert(mapping.locator);
            }
        }
    }
    
    return locators;
}

template<typename UsdSchemaType,
         typename UsdSchemaBasesTypes,
         typename Translator>
UsdImagingDataSourceSchemaBased<UsdSchemaType,UsdSchemaBasesTypes,Translator>::
UsdImagingDataSourceSchemaBased(
    const SdfPath &sceneIndexPath,
    UsdSchemaType usdSchema,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
  : _sceneIndexPath(sceneIndexPath)
  , _usdSchema(usdSchema)
  , _stageGlobals(stageGlobals)
{
}

template<typename UsdSchemaType,
         typename UsdSchemaBasesTypes,
         typename Translator>
const std::vector<UsdImagingDataSourceSchemaBased_Impl::_Mapping> &
UsdImagingDataSourceSchemaBased<UsdSchemaType,UsdSchemaBasesTypes,Translator>::
_GetMappings()
{
    static const std::vector<_Mapping> mappings =
        UsdImagingDataSourceSchemaBased_Impl::
        _GetMappings<UsdSchemaType, UsdSchemaBasesTypes, Translator>();
    return mappings;
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_SCHEMA_BASED_H
