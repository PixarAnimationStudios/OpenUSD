//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_VOL_IMAGING_DATA_SOURCE_PARTICLE_FIELD_H
#define PXR_USD_IMAGING_USD_VOL_IMAGING_DATA_SOURCE_PARTICLE_FIELD_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdVolImaging/api.h"
#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdImagingDataSourceParticleFieldPrim : public UsdImagingDataSourceGprim {
  public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceParticleFieldPrim);

    USDVOLIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken& name) override;

    USDVOLIMAGING_API
    static HdDataSourceLocatorSet Invalidate(
        UsdPrim const& prim, const TfToken& subprim,
        const TfTokenVector& properties,
        UsdImagingPropertyInvalidationType invalidationType);

  private:
    UsdImagingDataSourceParticleFieldPrim(
        const SdfPath& sceneIndexPath, UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals& stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceParticleFieldPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_VOL_IMAGING_DATA_SOURCE_PARTICLE_FIELD_H
