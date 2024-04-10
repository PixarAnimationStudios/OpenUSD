//
// Copyright 2023 Pixar
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

#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_RENDER_PRIMS_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_RENDER_PRIMS_H

#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/imaging/hd/dataSource.h"


PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceRenderPassPrim
///
/// A prim data source representing UsdRenderPass.
///
class UsdImagingDataSourceRenderPassPrim : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceRenderPassPrim);

    USDIMAGING_API
    TfTokenVector GetNames() override;

    USDIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDIMAGING_API
    static
    HdDataSourceLocatorSet
    Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim,
        const TfTokenVector &properties,
        UsdImagingPropertyInvalidationType invalidationType);

private:
    // Private constructor, use static New() instead.
    UsdImagingDataSourceRenderPassPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceRenderPassPrim);


/// \class UsdImagingDataSourceRenderSettingsPrim
///
/// A prim data source representing UsdRenderSettings.
///
class UsdImagingDataSourceRenderSettingsPrim : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceRenderSettingsPrim);

    USDIMAGING_API
    TfTokenVector GetNames() override;

    USDIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDIMAGING_API
    static
    HdDataSourceLocatorSet
    Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim,
        const TfTokenVector &properties,
        UsdImagingPropertyInvalidationType invalidationType);

private:
    // Private constructor, use static New() instead.
    UsdImagingDataSourceRenderSettingsPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceRenderSettingsPrim);


/// \class UsdImagingDataSourceRenderProductPrim
///
/// A prim data source representing UsdRenderProduct.
///
class UsdImagingDataSourceRenderProductPrim : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceRenderProductPrim);

    USDIMAGING_API
    TfTokenVector GetNames() override;

    USDIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDIMAGING_API
    static
    HdDataSourceLocatorSet
    Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim,
        const TfTokenVector &properties,
        UsdImagingPropertyInvalidationType invalidationType);

private:
    // Private constructor, use static New() instead.
    UsdImagingDataSourceRenderProductPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceRenderProductPrim);


/// \class UsdImagingDataSourceRenderVarPrim
///
/// A prim data source representing UsdRenderVar.
///
class UsdImagingDataSourceRenderVarPrim : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceRenderVarPrim);

    USDIMAGING_API
    TfTokenVector GetNames() override;

    USDIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDIMAGING_API
    static
    HdDataSourceLocatorSet
    Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim,
        const TfTokenVector &properties,
        UsdImagingPropertyInvalidationType invalidationType);

private:

    // Private constructor, use static New() instead.
    UsdImagingDataSourceRenderVarPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceRenderVarPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_RENDER_PRIMS_H
