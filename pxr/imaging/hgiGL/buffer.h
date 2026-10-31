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
/// Usually HgiGL creates and owns the underlying GL buffer name. It can also
/// wrap a name somebody else created -- an application sharing one of its own
/// buffers with Hydra -- in which case this object binds and reads it but
/// leaves deletion to its owner. See the adopting constructor and
/// HgiGLExternalBuffer. Either way the object is a full HgiGLBuffer, so every
/// path that expects one (the blit ops, the resource bindings) works unchanged.
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
    friend class HgiGLExternalBuffer;

    HGIGL_API
    HgiGLBuffer(HgiBufferDesc const & desc);

    /// Wrap the existing GL buffer name \p bufferId WITHOUT taking ownership:
    /// the destructor does not delete it, and CPU staging is refused, since
    /// there is no telling what its owner is doing with the contents. Used for
    /// buffers an application allocated and shares with Hgi; \p desc supplies
    /// the byte size and the usage the consumer may bind it for.
    HGIGL_API
    HgiGLBuffer(HgiBufferDesc const & desc, uint32_t bufferId);

private:
    HgiGLBuffer() = delete;
    HgiGLBuffer & operator=(const HgiGLBuffer&) = delete;
    HgiGLBuffer(const HgiGLBuffer&) = delete;

    uint32_t _bufferId;
    void* _cpuStaging;
    uint64_t _bindlessGPUAddress;
    // False when _bufferId belongs to somebody else; see the adopting
    // constructor. Destroying such a buffer through Hgi::DestroyBuffer is
    // safe and expected -- it tears down this wrapper only.
    bool _ownsBufferId;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
