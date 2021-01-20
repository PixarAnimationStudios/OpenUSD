//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_IMAGING_HGI_GL_TEXTURE_H
#define PXR_IMAGING_HGI_GL_TEXTURE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgi/texture.h"


PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiGLTexture
///
/// Represents a OpenGL GPU texture resource.
///
class HgiGLTexture final : public HgiTexture
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
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
