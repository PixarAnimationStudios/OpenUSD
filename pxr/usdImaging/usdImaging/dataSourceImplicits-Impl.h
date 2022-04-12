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
template<typename U>
class UsdImagingDataSourceImplicits : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceImplicits<U>);

    bool Has(const TfToken &name) override {
        static const TfTokenVector usdNames = _GetSchemaAttributeNames();
        for (const TfToken &usdName : usdNames) {
            if (name == usdName) {
                return true;
            }
        }
        return false;
    }
    TfTokenVector GetNames() override {
        static const TfTokenVector usdNames = _GetSchemaAttributeNames();
        return usdNames;
    }
    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (UsdAttribute attr = _usdGeomSchema.GetPrim().GetAttribute(name)) {
            return
                UsdImagingDataSourceAttributeNew(
                    attr,
                    _stageGlobals,
                    _sceneIndexPath,
                    HdDataSourceLocator(_locatorToken, name));
        } else {
            return nullptr;
        }
    }

private:
    // Private constructor, use static New() instead.
    UsdImagingDataSourceImplicits(
            const SdfPath &sceneIndexPath,
            const TfToken &locatorToken,
            U usdGeomSchema,
            const UsdImagingDataSourceStageGlobals &stageGlobals)
      : _sceneIndexPath(sceneIndexPath)
      , _locatorToken(locatorToken)
      , _usdGeomSchema(usdGeomSchema)
      , _stageGlobals(stageGlobals)
    {
    }

    TfTokenVector
    static _GetSchemaAttributeNames()
    {
        TfTokenVector result =
            U::GetSchemaAttributeNames(/* includeInherited = */ false);
        result.erase(
            std::remove(result.begin(), result.end(), UsdGeomTokens->extent));
        
        return result;
    }

private:
    const SdfPath _sceneIndexPath;
    const TfToken _locatorToken;
    U _usdGeomSchema;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

/// \class UsdImagingDataSourceImplicitsPrim
///
/// A prim data source for a cube, ...
///
template<typename U>
class UsdImagingDataSourceImplicitsPrim : public UsdImagingDataSourceGprim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceImplicitsPrim<U>);

    bool Has(const TfToken &name) override {
        if (name == _locatorToken) {
            return true;
        }
        
        return UsdImagingDataSourceGprim::Has(name);
    }
    TfTokenVector GetNames() override {
        TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
        result.push_back(_locatorToken);
        return result;

    }
    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == _locatorToken) {
            return UsdImagingDataSourceImplicits<U>::New(
                _GetSceneIndexPath(),
                _locatorToken,
                UsdGeomCube(_GetUsdPrim()),
                _GetStageGlobals());
        }

        return UsdImagingDataSourceGprim::Get(name);
    }

private:
    // Private constructor, use static New() instead.
    UsdImagingDataSourceImplicitsPrim(
        const SdfPath &sceneIndexPath,
        const TfToken &locatorToken,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
      : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
      , _locatorToken(locatorToken)
    {
    }

private:
    const TfToken _locatorToken;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_IMPLICITS_IMPL_H
