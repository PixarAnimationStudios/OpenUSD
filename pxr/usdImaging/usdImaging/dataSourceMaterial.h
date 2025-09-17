//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_MATERIAL_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_MATERIAL_H

#include "pxr/imaging/hd/dataSource.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/usd/usdShade/connectableAPI.h"

#include <tbb/concurrent_unordered_map.h>

PXR_NAMESPACE_OPEN_SCOPE

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceMaterial
///
/// A data source mapping for data source material.
///
/// Creates HdMaterialSchema from a UsdShadeMaterial prim
///
class UsdImagingDataSourceMaterial : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceMaterial);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    ~UsdImagingDataSourceMaterial() override;

private:

    /// If \p fixedTerminalName is specified, the provided \p usdPrim will be
    /// treated as the terminal shader node in the graph rather than as a
    /// material (with relationships to the prims serving as terminal shader
    /// nodes). This is relevant for light and light filter cases.
    UsdImagingDataSourceMaterial(
        const UsdPrim &usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const TfToken &fixedTerminalName = TfToken()
        );


private:

    const UsdPrim _usdPrim;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;

    // Cache the networks by context name.
    using _ContextMap = tbb::concurrent_unordered_map<
        TfToken, HdDataSourceBaseHandle, TfToken::HashFunctor>;


    const TfToken _fixedTerminalName;
    _ContextMap _networks;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceMaterial);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceMaterial
///
/// A prim data source mapping for a UsdShadeMaterial prim
///
class UsdImagingDataSourceMaterialPrim : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceMaterialPrim);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    ~UsdImagingDataSourceMaterialPrim() override;

    static HdDataSourceLocatorSet Invalidate(
            const UsdPrim &prim,
            const TfToken &subprim,
            const TfTokenVector &properties,
            UsdImagingPropertyInvalidationType invalidationType);

private:
    USDIMAGING_API
    UsdImagingDataSourceMaterialPrim(
        const SdfPath &sceneIndexPath,
        const UsdPrim &usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceMaterialPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_MATERIAL_H
