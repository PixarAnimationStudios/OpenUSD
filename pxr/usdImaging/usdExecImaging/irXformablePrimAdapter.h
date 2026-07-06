//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_IR_XFORMABLE_PRIM_ADAPTER_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_IR_XFORMABLE_PRIM_ADAPTER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdExecImaging/primAdapterInterface.h"

PXR_NAMESPACE_OPEN_SCOPE

/// The UsdExecImaging prim adapter for IrXformable prims provides data sources
/// for computed values.
///
/// Namely, it provides data sources that extract the computed value of
/// posed:space for the adapted prims. These data sources overlay on top of the
/// data sources provided by the UsdImaging prim adapter, which inserts subprims
/// for guide geometry.
///
class UsdExecImaging_IrXformablePrimAdapter final
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
