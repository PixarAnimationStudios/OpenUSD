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

#ifndef PXR_USD_IMAGING_USD_VOL_IMAGING_DATA_SOURCE_FIELD_ASSET_H
#define PXR_USD_IMAGING_USD_VOL_IMAGING_DATA_SOURCE_FIELD_ASSET_H

/// \file usdImaging/dataSourceFieldAsset.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdVolImaging/api.h"

#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingDataSourceFieldAsset
///
/// A container data source representing volumeField info
///
class UsdImagingDataSourceFieldAsset : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceFieldAsset);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDVOLIMAGING_API
    ~UsdImagingDataSourceFieldAsset() override;

private:

    // Private constructor, use static New() instead.
    UsdImagingDataSourceFieldAsset(
            const SdfPath &sceneIndexPath,
            UsdPrim usdPrim,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    const SdfPath _sceneIndexPath;
    UsdPrim _usdPrim;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceFieldAsset);

/// \class UsdImagingDataSourceFieldAssetPrim
///
/// A prim data source representing UsdVolOpenVDBAsset or UsdVolField3DAsset.
///
class UsdImagingDataSourceFieldAssetPrim : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceFieldAssetPrim);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:

    // Private constructor, use static New() instead.
    UsdImagingDataSourceFieldAssetPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceFieldAssetPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_VOL_IMAGING_DATA_SOURCE_FIELD_ASSET_H
