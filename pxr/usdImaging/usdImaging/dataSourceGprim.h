//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_GPRIM_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_GPRIM_H

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class UsdImagingDataSourceGprim
///
/// Data source representing a USD gprim. This is the common base for geometric
/// types and includes features such as materials and primvars.
///
class UsdImagingDataSourceGprim : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceGprim);

    /// Returns the data source representing \p name, if valid.
    ///
    USDIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDIMAGING_API
    static HdDataSourceLocatorSet Invalidate(
            UsdPrim const& prim,
            const TfToken &subprim,
            const TfTokenVector &properties,
            UsdImagingPropertyInvalidationType invalidationType);

protected:

    /// Use to construct a new UsdImagingDataSourceGprim.
    ///
    /// \p sceneIndexPath is the path of this object in the scene index.
    ///
    /// \p usdPrim is the USD prim object that this data source represents.
    ///
    /// \p stageGlobals represents the context object for the UsdStage with
    /// which to evaluate this attribute data source.
    ///
    /// Note: client code calls this via static New.
    USDIMAGING_API
    UsdImagingDataSourceGprim(
            const SdfPath &sceneIndexPath,
            UsdPrim usdPrim,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceGprim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_GPRIM_H
