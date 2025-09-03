//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_TEXTURE_H
#define PXR_IMAGING_HGI_GL_TEXTURE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgi/texture.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/weakBase.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(HgiGLTexture);

/// \class HgiGLTexture
///
/// Represents a OpenGL GPU texture resource.
///
/// Note that we inherit from TfWeakBase for deletion detection.
/// This is useful to invalidate container objects such as framebuffer objects
/// that reference a deleted texture resource as an attachment.
///
class HgiGLTexture final : public HgiTexture, public TfWeakBase
{
public:
    HGIGL_API
    ~HgiGLTexture() override;

    HGIGL_API
    size_t GetByteSizeOfResource() const override;

    HGIGL_API
    uint64_t GetRawResource() const override;

    /// Returns the OpenGL id / name of the texture.
    uint32_t GetTextureId() const {return _textureId;}

    /// Returns the bindless gpu handle (caller must verify extension support)
    HGIGL_API
    uint64_t GetBindlessHandle();

    /// This function does not do anything. There is no support for explicit 
    /// layout transition in non-explicit APIs like OpenGL. Hence this function
    /// simply returns 0 (none).
    HGIGL_API
    HgiTextureUsage SubmitLayoutChange(HgiTextureUsage newLayout) override;

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLTexture(HgiTextureDesc const & desc);

    HGIGL_API
    HgiGLTexture(HgiTextureViewDesc const & desc);

private:
    HgiGLTexture() = delete;
    HgiGLTexture & operator=(const HgiGLTexture&) = delete;
    HgiGLTexture(const HgiGLTexture&) = delete;

    uint32_t _textureId;
    uint64_t _bindlessHandle;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
