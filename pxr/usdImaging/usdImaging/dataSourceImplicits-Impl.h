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

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingDataSourceImplicits
///
/// A container data source representing info for, e.g., cube data source
/// locator for a cube. Instantiate using, e.g., UsdGeomCube.
///
template<typename U, typename V>
class UsdImagingDataSourceImplicits : public HdContainerDataSource
{
public:
    using This = UsdImagingDataSourceImplicits<U, V>;

    HD_DECLARE_DATASOURCE(This);

    bool Has(const TfToken &name) override {
        for (const TfToken &usdName : _GetSchemaAttributeNames()) {
            if (name == usdName) {
                return true;
            }
        }
        return false;
    }
    TfTokenVector GetNames() override {
        return _GetSchemaAttributeNames();
    }
    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (UsdAttribute attr = _usdGeomSchema.GetPrim().GetAttribute(name)) {
            return
                UsdImagingDataSourceAttributeNew(
                    attr,
                    _stageGlobals,
                    _sceneIndexPath,
                    _GetLocatorForProperty(name));
        } else {
            return nullptr;
        }
    }

    static
    HdDataSourceLocatorSet
    Invalidate(const TfToken &subprim, const TfTokenVector &properties) {
        HdDataSourceLocatorSet locators;

        for (const TfToken &propertyName : properties) {
            for (const TfToken &usdName : _GetSchemaAttributeNames()) {
                if (propertyName == usdName) {
                    locators.insert(_GetLocatorForProperty(propertyName));
                }
            }
        }

        return locators;
    }

private:
    // Private constructor, use static New() instead.
    UsdImagingDataSourceImplicits(
            const SdfPath &sceneIndexPath,
            U usdGeomSchema,
            const UsdImagingDataSourceStageGlobals &stageGlobals)
      : _sceneIndexPath(sceneIndexPath)
      , _usdGeomSchema(usdGeomSchema)
      , _stageGlobals(stageGlobals)
    {
    }

    static
    HdDataSourceLocator
    _GetLocatorForProperty(const TfToken &name)
    {
        return V::GetDefaultLocator().Append(name);
    }

    static
    TfTokenVector
    _GetSchemaAttributeNamesUncached() {
        TfTokenVector result =
            U::GetSchemaAttributeNames(/* includeInherited = */ false);
        result.erase(
            std::remove(result.begin(), result.end(), UsdGeomTokens->extent));
        
        return result;
    }

    static
    const TfTokenVector &
    _GetSchemaAttributeNames() {
        static const TfTokenVector result = _GetSchemaAttributeNamesUncached();
        return result;
    }

private:
    const SdfPath _sceneIndexPath;
    U _usdGeomSchema;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

/// \class UsdImagingDataSourceImplicitsPrim
///
/// A prim data source for a cube, ...
///
template<typename U, typename V>
class UsdImagingDataSourceImplicitsPrim : public UsdImagingDataSourceGprim
{
public:
    using This = UsdImagingDataSourceImplicitsPrim<U, V>;

    HD_DECLARE_DATASOURCE(This);

    bool Has(const TfToken &name) override {
        if (name == _GetLocatorToken()) {
            return true;
        }
        
        return UsdImagingDataSourceGprim::Has(name);
    }
    TfTokenVector GetNames() override {
        TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
        result.push_back(_GetLocatorToken());
        return result;

    }
    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == _GetLocatorToken()) {
            return UsdImagingDataSourceImplicits<U, V>::New(
                _GetSceneIndexPath(),
                U(_GetUsdPrim()),
                _GetStageGlobals());
        }

        return UsdImagingDataSourceGprim::Get(name);
    }

    static
    HdDataSourceLocatorSet
    Invalidate(const TfToken &subprim, const TfTokenVector &properties) {
        HdDataSourceLocatorSet locators =
            UsdImagingDataSourceImplicits<U,V>::Invalidate(
                subprim, properties);

        locators.insert(
            UsdImagingDataSourceGprim::Invalidate(subprim, properties));

        return locators;
    }

private:
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
        if (V::GetDefaultLocator().GetElementCount() != 1) {
            TF_CODING_ERROR("Expected data source locator with one element.");
            return TfToken();
        }
        return V::GetDefaultLocator().GetFirstElement();
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
