//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_BLIT_CMDS_H
#define PXR_IMAGING_HGI_BLIT_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/cmds.h"
#include "pxr/imaging/hgi/texture.h"
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiTextureGpuToCpuOp;
struct HgiTextureCpuToGpuOp;
struct HgiBufferGpuToGpuOp;
struct HgiBufferCpuToGpuOp;
struct HgiBufferGpuToCpuOp;
struct HgiTextureToBufferOp;
struct HgiBufferToTextureOp;
struct HgiResolveImageOp;

using HgiBlitCmdsUniquePtr = std::unique_ptr<class HgiBlitCmds>;


/// \class HgiBlitCmds
///
/// A graphics API independent abstraction of resource copy commands.
/// HgiBlitCmds is a lightweight object that cannot be re-used after it has
/// been submitted. A new cmds object should be acquired for each frame.
///
class HgiBlitCmds : public HgiCmds
{
public:
    HGI_API
    ~HgiBlitCmds() override;

    /// Push a debug marker.
    HGI_API
    virtual void PushDebugGroup(const char* label) = 0;

    /// Pop the lastest debug.
    HGI_API
    virtual void PopDebugGroup() = 0;

    /// Copy a texture resource from GPU to CPU.
    /// Synchronization between GPU writes and CPU reads must be managed by
    /// the client by supplying the correct 'wait' flags in SubmitCmds.
    HGI_API
    virtual void CopyTextureGpuToCpu(HgiTextureGpuToCpuOp const& copyOp) = 0;

    /// Copy new data from the CPU into a GPU texture.
    HGI_API
    virtual void CopyTextureCpuToGpu(HgiTextureCpuToGpuOp const& copyOp) = 0;

    /// Copy a buffer resource from GPU to GPU.
    HGI_API
    virtual void CopyBufferGpuToGpu(HgiBufferGpuToGpuOp const& copyOp) = 0;

    /// Copy new data from CPU into GPU buffer.
    /// For example copy new data into a uniform block or storage buffer.
    HGI_API
    virtual void CopyBufferCpuToGpu(HgiBufferCpuToGpuOp const& copyOp) = 0;

    /// Copy new data from GPU into CPU buffer.
    /// Synchronization between GPU writes and CPU reads must be managed by
    /// the client by supplying the correct 'wait' flags in SubmitCmds.
    HGI_API
    virtual void CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp) = 0;

    /// Copy a texture resource into a buffer resource from GPU to GPU.
    HGI_API
    virtual void CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp) = 0;

    /// Copy a buffer resource into a texture resource from GPU to GPU.
    HGI_API
    virtual void CopyBufferToTexture(HgiBufferToTextureOp const& copyOp) = 0;

    /// Generate mip maps for a texture
    HGI_API
    virtual void GenerateMipMaps(HgiTextureHandle const& texture) = 0;
    
    /// Fill a buffer with a constant value.
    HGI_API
    virtual void FillBuffer(HgiBufferHandle const& buffer, uint8_t value) = 0;

    /// Inserts a barrier so that data written to memory by commands before
    /// the barrier is available to commands after the barrier.
    HGI_API
    virtual void InsertMemoryBarrier(HgiMemoryBarrier barrier) = 0;

protected:
    HGI_API
    HgiBlitCmds();

private:
    HgiBlitCmds & operator=(const HgiBlitCmds&) = delete;
    HgiBlitCmds(const HgiBlitCmds&) = delete;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
