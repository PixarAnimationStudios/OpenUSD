//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_GEOM_XFORMABLE_PRIM_ADAPTER_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_GEOM_XFORMABLE_PRIM_ADAPTER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdExecImaging/primAdapterInterface.h"

PXR_NAMESPACE_OPEN_SCOPE

/// The prim adapter for UsdGeomXformable prims overlays the corresponding hydra
/// prims with the xformable's computed local-to-world transform matrix.
///
class UsdExecImaging_GeomXformablePrimAdapter final
    : public UsdExecImagingPrimAdapterInterface
{
public:
    void BuildRequest(
        const UsdPrim &prim,
        UsdExecImagingRequestBuilderInterface &requestBuilder) const override;

    HdContainerDataSourceHandle GetPrimData(
        const SdfPath &primPath,
        const UsdExecImagingRequestAccessorInterfaceSharedPtr &requestAccessor)
            const override;

    void InvalidatePrimData(
        const SdfPath &primPath,
        const UsdExecImagingValueKey &valueKey,
        HdDataSourceLocatorSet *invalidLocators) const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif