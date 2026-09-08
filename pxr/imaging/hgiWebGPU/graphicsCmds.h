//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_GRAPHICS_CMDS_H
#define PXR_IMAGING_HGI_WEBGPU_GRAPHICS_CMDS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/stepFunctions.h"
#include "pxr/imaging/hgiWebGPU/baseGraphicsCmds.h"
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiGraphicsCmdsDesc;
class HgiWebGPUGraphicsPipeline;

/// \class HgiWebGPUGraphicsCmds
///
/// WebGPU implementation of HgiGraphicsEncoder.
///
class HgiWebGPUGraphicsCmds final : public HgiWebGPUBaseGraphicsCmds<HgiGraphicsCmds, wgpu::RenderPassEncoder>
{
public:
    HGIWEBGPU_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override {}

    HGIWEBGPU_API
    void SetViewport(GfVec4i const& vp) override;

    HGIWEBGPU_API
    void SetScissor(GfVec4i const& sc) override;

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPUGraphicsCmds(
        HgiWebGPU* hgi,
        HgiGraphicsCmdsDesc const& desc);

    HGIWEBGPU_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiWebGPUGraphicsCmds() = delete;
    HgiWebGPUGraphicsCmds & operator=(const HgiWebGPUGraphicsCmds&) = delete;
    HgiWebGPUGraphicsCmds(const HgiWebGPUGraphicsCmds&) = delete;

    void _EndRenderPass() override;
    bool _IsTimestampsEnabled();

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
