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
#ifndef PXR_IMAGING_HD_ST_RENDER_BUFFER_H
#define PXR_IMAGING_HD_ST_RENDER_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/texture.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdStRenderBuffer : public HdRenderBuffer
{
public:
    HDST_API
    HdStRenderBuffer(Hgi* hgi, SdfPath const& id);

    HDST_API
    ~HdStRenderBuffer();

    HDST_API
    bool Allocate(GfVec3i const& dimensions,
                  HdFormat format,
                  bool multiSampled) override;

    HDST_API
    unsigned int GetWidth() const override {return _dimensions[0];}

    HDST_API
    unsigned int GetHeight() const override {return _dimensions[1];}

    HDST_API
    unsigned int GetDepth() const override {return _dimensions[2];}

    HDST_API
    HdFormat GetFormat() const override {return _format;}

    HDST_API
    bool IsMultiSampled() const override {return _multiSampled;}

    /// Map the buffer for reading. The control flow should be Map(),
    /// before any I/O, followed by memory access, followed by Unmap() when
    /// done.
    ///   \return The address of the buffer.
    HDST_API
    void* Map() override;

    /// Unmap the buffer.
    HDST_API
    void Unmap() override;

    /// Return whether any clients have this buffer mapped currently.
    ///   \return True if the buffer is currently mapped by someone.
    HDST_API
    bool IsMapped() const override {
        return _mappers.load() != 0;
    }

    /// Is the buffer converged?
    ///   \return True if the buffer is converged (not currently being
    ///           rendered to).
    HDST_API
    bool IsConverged() const override {
        return true;
    }

    /// Resolve the sample buffer into final values.
    HDST_API
    void Resolve() override;

    /// Returns the texture handle.
    HDST_API
    HgiTextureHandle GetHgiTextureHandle(bool multiSampled) const override;

private:
    // Release any allocated resources.
    void _Deallocate() override;

    Hgi* _hgi;

    GfVec3i _dimensions;
    HdFormat _format;
    HgiTextureUsageBits _usage;
    bool _multiSampled;

    // The GPU texture resource
    HgiTextureHandle _texture;

    // The GPU multi-sample texture resource (optional)
    HgiTextureHandle _textureMS;

    // The number of callers mapping this buffer.
    std::atomic<int> _mappers;
    // Texels are temp captured into this buffer between map and unmap.
    std::vector<uint8_t> _mappedBuffer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
