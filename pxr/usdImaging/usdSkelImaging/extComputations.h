//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_EXT_COMPUTATIONS_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_EXT_COMPUTATIONS_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/imaging/hd/extComputationSchema.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdExtComputationContext;

extern TfEnvSetting<bool> USDSKELIMAGING_FORCE_CPU_COMPUTE;

/// Invoke the skinning ext computation.
USDSKELIMAGING_API
void
UsdSkelImagingInvokeExtComputation(
    const TfToken &skinningMethod,
    HdExtComputationContext * ctx);

/// Data source for skinning CPU computation.
USDSKELIMAGING_API
HdExtComputationCpuCallbackDataSourceHandle
UsdSkelImagingExtComputationCpuCallback(const TfToken &skinningMethod);

/// Data source for skinning GPU computation.
USDSKELIMAGING_API
HdStringDataSourceHandle
UsdSkelImagingExtComputationGlslKernel(const TfToken &skinningMethod);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
