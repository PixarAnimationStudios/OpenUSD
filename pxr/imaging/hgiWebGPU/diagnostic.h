//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_DIAGNOSTIC_H
#define PXR_IMAGING_HGI_WEBGPU_DIAGNOSTIC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiWebGPU/api.h"

PXR_NAMESPACE_OPEN_SCOPE

HGIWEBGPU_API
bool
HgiWebGPUIsDebugEnabled();

// Begin a label in a webgpu command encoder
HGIWEBGPU_API void HgiWebGPUBeginLabel(wgpu::CommandEncoder const &encoder, const char* label);
HGIWEBGPU_API void HgiWebGPUBeginLabel(wgpu::RenderPassEncoder const &encoder, const char* label);
HGIWEBGPU_API void HgiWebGPUBeginLabel(wgpu::ComputePassEncoder const &encoder, const char* label);
HGIWEBGPU_API void HgiWebGPUBeginLabel(wgpu::RenderBundleEncoder const &encoder, const char* label);

// End the last pushed label in a webgpu command encoder
HGIWEBGPU_API void HgiWebGPUEndLabel(wgpu::CommandEncoder const &encoder);
HGIWEBGPU_API void HgiWebGPUEndLabel(wgpu::RenderPassEncoder const &encoder);
HGIWEBGPU_API void HgiWebGPUEndLabel(wgpu::ComputePassEncoder const &encoder);
HGIWEBGPU_API void HgiWebGPUEndLabel(wgpu::RenderBundleEncoder const &encoder);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HGI_WEBGPU_DIAGNOSTIC_H
