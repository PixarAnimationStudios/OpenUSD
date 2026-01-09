//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef HDPARTICLEFIELD_DATASOURCEPARTICLEFIELD_H
#define HDPARTICLEFIELD_DATASOURCEPARTICLEFIELD_H

#include <pxr/usdImaging/usdImaging/dataSourceGprim.h>
#include <pxr/usdImaging/usdImaging/dataSourceStageGlobals.h>

#include <pxr/imaging/hd/dataSource.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdImagingDataSource_3DGaussianSplatPrim : public UsdImagingDataSourceGprim {
  public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSource_3DGaussianSplatPrim);

    USDIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken& name) override;

    USDIMAGING_API
    static HdDataSourceLocatorSet Invalidate(UsdPrim const& prim, const TfToken& subprim,
                                             const TfTokenVector& properties,
                                             UsdImagingPropertyInvalidationType invalidationType);

  private:
    UsdImagingDataSource_3DGaussianSplatPrim(const SdfPath& sceneIndexPath, UsdPrim usdPrim,
                                             const UsdImagingDataSourceStageGlobals& stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSource_3DGaussianSplatPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPARTICLEFIELD_DATASOURCEPARTICLEFIELD_H
