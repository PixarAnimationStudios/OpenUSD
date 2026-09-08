//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"

#include "pxr/base/arch/defines.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIWEBGPU_ENABLE_MULTI_DRAW_INDIRECT, false,
    "Use WebGPU multi draw indirect");

HgiWebGPUCapabilities::HgiWebGPUCapabilities(wgpu::Device device)
{
    device.GetLimits(&_limits);

    _maxUniformBlockSize = 64 * 1024;
    _maxShaderStorageBlockSize = 1 * 1024 * 1024 * 1024;

    if (device.HasFeature(wgpu::FeatureName::ClipDistances)) {
        _maxClipDistances = 8;
    } else {
        _maxClipDistances = 0;
    }

#ifdef ARCH_OS_WASM_VM
    // Without this, the default value of 1 causes aligned_malloc to always
    // return 0 (null) Emscripten treats null pointers as valid, making the
    // issue VERY hard to track down
    _pageSizeAlignment = sizeof(void*);
#endif

    bool multiDrawIndirectEnabled =
        TfGetEnvSetting(HGIWEBGPU_ENABLE_MULTI_DRAW_INDIRECT);

    // https://github.com/gfx-rs/wgpu/issues/158#issuecomment-490653129
    _uniformBufferOffsetAlignment = 256;
    _SetFlag(HgiDeviceCapabilitiesBitsCppShaderPadding, false);
    _SetFlag(HgiDeviceCapabilitiesBitsBuiltinBarycentrics, false);
    _SetFlag(HgiDeviceCapabilitiesBitsDepthRangeMinusOnetoOne, false);
    _SetFlag(
        HgiDeviceCapabilitiesBitsMultiDrawIndirect, multiDrawIndirectEnabled);
    // This might be available in the future
    // https://github.com/gpuweb/gpuweb/issues/4891
    _SetFlag(HgiDeviceCapabilitiesForceEarlyFragmentTest, false);
    _SetFlag(HgiDeviceCapabilitiesBitsBindlessTextures,
        false); // WGSL not support bindless textures as of 06/2025
    _SetFlag(HgiDeviceCapabilitiesBitsBindlessBuffers,
        false); // WGSL can use similar "storage" buffers
    // Note that HgiDeviceCapabilitiesBitsPrimitiveIdEmulation requires Metal
    // tessellation, so we never support it, regardless of the state of
    // wgpu::FeatureName::PrimitiveIndex (it's not a fallback).
}

HgiWebGPUCapabilities::~HgiWebGPUCapabilities() = default;

int
HgiWebGPUCapabilities::GetAPIVersion() const
{
    return 0;
}

int
HgiWebGPUCapabilities::GetShaderVersion() const
{
    return 460;
}

PXR_NAMESPACE_CLOSE_SCOPE
