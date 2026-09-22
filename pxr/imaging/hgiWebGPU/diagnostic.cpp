//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/envSetting.h"
#include "pxr/imaging/hgiWebGPU/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIWEBGPU_DEBUG, 0, "Enable debugging for HgiWebGPU");

bool
HgiWebGPUIsDebugEnabled()
{
    static bool _v = TfGetEnvSetting(HGIWEBGPU_DEBUG) == 1;
    return _v;
}

void HgiWebGPUBeginLabel(wgpu::CommandEncoder const &encoder, const char* label)
{
    if (!HgiWebGPUIsDebugEnabled() || !label) {
        return;
    }
    encoder.PushDebugGroup(label);
}

void HgiWebGPUBeginLabel(wgpu::RenderPassEncoder const &encoder, const char* label)
{
    if (!HgiWebGPUIsDebugEnabled() || !label) {
        return;
    }
    encoder.PushDebugGroup(label);
}

void HgiWebGPUBeginLabel(wgpu::ComputePassEncoder const &encoder, const char* label)
{
    if (!HgiWebGPUIsDebugEnabled() || !label) {
        return;
    }
    encoder.PushDebugGroup(label);
}

void HgiWebGPUBeginLabel(wgpu::RenderBundleEncoder const &encoder, const char* label)
{
    if (!HgiWebGPUIsDebugEnabled() || !label) {
        return;
    }
    encoder.PushDebugGroup(label);
}

void HgiWebGPUEndLabel(wgpu::CommandEncoder const &encoder)
{
    if (!HgiWebGPUIsDebugEnabled()) {
        return;
    }
    encoder.PopDebugGroup();
}

void HgiWebGPUEndLabel(wgpu::RenderPassEncoder const &encoder)
{
    if (!HgiWebGPUIsDebugEnabled()) {
        return;
    }
    encoder.PopDebugGroup();
}

void HgiWebGPUEndLabel(wgpu::ComputePassEncoder const &encoder)
{
    if (!HgiWebGPUIsDebugEnabled()) {
        return;
    }
    encoder.PopDebugGroup();
}

void HgiWebGPUEndLabel(wgpu::RenderBundleEncoder const &encoder)
{
    if (!HgiWebGPUIsDebugEnabled()) {
        return;
    }
    encoder.PopDebugGroup();
}

PXR_NAMESPACE_CLOSE_SCOPE
