//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef RMAN_USD_IMAGING_RENDER_TERMINAL_TIME_SAMPLING_ADAPTER_H
#define RMAN_USD_IMAGING_RENDER_TERMINAL_TIME_SAMPLING_ADAPTER_H

#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_RenderTerminalTimeSamplingAdapter : public UsdImagingAPISchemaAdapter
{
public:
    using BaseAdapter = UsdImagingAPISchemaAdapter;

#if PXR_VERSION >= 2505 && PXR_VERSION < 2603

    HdContainerDataSourceHandle GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfToken const& appliedInstanceName,
        const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    HdDataSourceLocatorSet InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfToken const& appliedInstanceName,
        TfTokenVector const& properties,
        UsdImagingPropertyInvalidationType invalidationType) override;

#endif // PXR_VERSION
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // RMAN_USD_IMAGING_RENDER_TERMINAL_TIME_SAMPLING_ADAPTER_H
