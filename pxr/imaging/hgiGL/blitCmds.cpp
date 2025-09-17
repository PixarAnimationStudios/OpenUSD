//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiGL/blitCmds.h"
#include "pxr/imaging/hgiGL/buffer.h"
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/device.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/hgi.h"
#include "pxr/imaging/hgiGL/ops.h"
#include "pxr/imaging/hgiGL/scopedStateHolder.h"
#include "pxr/imaging/hgiGL/texture.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLBlitCmds::HgiGLBlitCmds()
    : HgiBlitCmds()
    , _pushStack(0)
{
}

HgiGLBlitCmds::~HgiGLBlitCmds() = default;

void
HgiGLBlitCmds::PushDebugGroup(const char* label)
{
    if (HgiGLDebugEnabled()) {
        _pushStack++;
        _ops.push_back( HgiGLOps::PushDebugGroup(label) );
    }
}

void
HgiGLBlitCmds::PopDebugGroup()
{
    if (HgiGLDebugEnabled()) {
        _pushStack--;
        _ops.push_back( HgiGLOps::PopDebugGroup() );
    }
}

void
HgiGLBlitCmds::CopyTextureGpuToCpu(
    HgiTextureGpuToCpuOp const& copyOp)
{
    _ops.push_back( HgiGLOps::CopyTextureGpuToCpu(copyOp) );
}

void
HgiGLBlitCmds::CopyTextureCpuToGpu(HgiTextureCpuToGpuOp const& copyOp)
{
    _ops.push_back( HgiGLOps::CopyTextureCpuToGpu(copyOp) );
}

void
HgiGLBlitCmds::CopyBufferGpuToGpu(
    HgiBufferGpuToGpuOp const& copyOp)
{
    _ops.push_back( HgiGLOps::CopyBufferGpuToGpu(copyOp) );
}

void 
HgiGLBlitCmds::CopyBufferCpuToGpu(HgiBufferCpuToGpuOp const& copyOp)
{
    _ops.push_back( HgiGLOps::CopyBufferCpuToGpu(copyOp) );
}

void
HgiGLBlitCmds::CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp)
{
    _ops.push_back( HgiGLOps::CopyBufferGpuToCpu(copyOp) );
}

void
HgiGLBlitCmds::CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp)
{
    _ops.push_back( HgiGLOps::CopyTextureToBuffer(copyOp) );
}

void
HgiGLBlitCmds::CopyBufferToTexture(HgiBufferToTextureOp const& copyOp)
{
    _ops.push_back( HgiGLOps::CopyBufferToTexture(copyOp) );
}

void
HgiGLBlitCmds::FillBuffer(HgiBufferHandle const& buffer, uint8_t value)
{
    _ops.push_back( HgiGLOps::FillBuffer(buffer, value) );
}

void
HgiGLBlitCmds::GenerateMipMaps(HgiTextureHandle const& texture)
{
    _ops.push_back( HgiGLOps::GenerateMipMaps(texture) );
}

void
HgiGLBlitCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
    _ops.push_back( HgiGLOps::InsertMemoryBarrier(barrier) );
}

bool
HgiGLBlitCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    if (_ops.empty()) {
        return false;
    }

    TF_VERIFY(_pushStack==0, "Push and PopDebugGroup do not even out");

    // Capture OpenGL state before executing the 'ops' and restore it when this
    // function ends. We do this defensively because parts of our pipeline may
    // not set and restore all relevant gl state.
    HgiGL_ScopedStateHolder openglStateGuard;

    HgiGL* hgiGL = static_cast<HgiGL*>(hgi);
    HgiGLDevice* device = hgiGL->GetPrimaryDevice();
    device->SubmitOps(_ops);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
