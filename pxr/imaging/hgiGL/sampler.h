//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIGL_SAMPLER_H
#define PXR_IMAGING_HGIGL_SAMPLER_H

#include "pxr/imaging/hgi/sampler.h"

#include "pxr/imaging/hgiGL/api.h"


PXR_NAMESPACE_OPEN_SCOPE


using HgiTextureHandle = HgiHandle<class HgiTexture>;

///
/// \class HgiGLSampler
///
/// OpenGL implementation of HgiSampler
///
class HgiGLSampler final : public HgiSampler
{
public:
    HGIGL_API
    ~HgiGLSampler() override;

    HGIGL_API
    uint64_t GetRawResource() const override;

    /// Returns the gl resource id of the sampler.
    HGIGL_API
    uint32_t GetSamplerId() const;

    /// Returns the bindless gpu handle (caller must verify extension support)
    HGIGL_API
    uint64_t GetBindlessHandle(HgiTextureHandle const &textureHandle);

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLSampler(HgiSamplerDesc const& desc);

private:
    HgiGLSampler() = delete;
    HgiGLSampler & operator=(const HgiGLSampler&) = delete;
    HgiGLSampler(const HgiGLSampler&) = delete;

private:
    uint32_t _samplerId;
    uint32_t _bindlessTextureId;
    uint64_t _bindlessHandle;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
