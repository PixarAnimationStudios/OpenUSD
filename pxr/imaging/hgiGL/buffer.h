//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_BUFFER_H
#define PXR_IMAGING_HGI_GL_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgi/buffer.h"


PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiGLBuffer
///
/// Represents an OpenGL GPU buffer resource.
///
class HgiGLBuffer final : public HgiBuffer
{
public:
    HGIGL_API
    ~HgiGLBuffer() override;

    HGIGL_API
    size_t GetByteSizeOfResource() const override;

    HGIGL_API
    uint64_t GetRawResource() const override;

    HGIGL_API
    void* GetCPUStagingAddress() override;

    uint32_t GetBufferId() const {return _bufferId;}

    /// Returns the bindless gpu address (caller must verify extension support)
    HGIGL_API
    uint64_t GetBindlessGPUAddress();

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLBuffer(HgiBufferDesc const & desc);

private:
    HgiGLBuffer() = delete;
    HgiGLBuffer & operator=(const HgiGLBuffer&) = delete;
    HgiGLBuffer(const HgiGLBuffer&) = delete;

    uint32_t _bufferId;
    void* _cpuStaging;
    uint64_t _bindlessGPUAddress;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
