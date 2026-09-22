//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HGI_WEBGPU_SPIRVTRANSFORMS_H
#define PXR_IMAGING_HGI_WEBGPU_SPIRVTRANSFORMS_H

#include "pxr/pxr.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/imaging/hgiWebGPU/api.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(
    HGIWEBGPU_DEBUG_SPIRV_TRANSFORM
);

HGIWEBGPU_API
bool ApplySpirvViewportFlip(
    std::vector<uint32_t>& spirv);

HGIWEBGPU_API
bool ApplySpirvAlphaToOneEmulation(
    std::vector<uint32_t>& spirv, uint32_t sampleCount);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
