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
#ifndef HDXPRMAN_RENDERBUFFER_H
#define HDXPRMAN_RENDERBUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdxPrmanRenderBuffer : public HdRenderBuffer
{
public:
    HdxPrmanRenderBuffer(SdfPath const& id);
    ~HdxPrmanRenderBuffer();

    virtual bool Allocate(GfVec3i const& dimensions,
                          HdFormat format,
                          bool multiSampled) override;

    virtual unsigned int GetWidth() const override { return _width; }
    virtual unsigned int GetHeight() const override { return _height; }
    virtual unsigned int GetDepth() const override { return 1; }
    virtual HdFormat GetFormat() const override { return _format; }

    // HdPrman doesn't handle sampling decisions at the hydra level.
    virtual bool IsMultiSampled() const override { return false; }

    virtual uint8_t* Map() override {
        _mappers++;
        return _buffer.data();
    }
    virtual void Unmap() override {
        _mappers--;
    }
    virtual bool IsMapped() const override {
        return _mappers.load() != 0;
    }

    virtual bool IsConverged() const override {
        return _converged.load();
    }
    void SetConverged(bool cv) {
        _converged.store(cv);
    }

    virtual void Resolve() override {}

    // ---------------------------------------------------------------------- //
    /// \name I/O helpers
    // ---------------------------------------------------------------------- //

    // format is the input format.
    void Blit(HdFormat format, int width, int height, int offset, int stride,
              uint8_t const* data);

private:
    static size_t _GetBufferSize(GfVec2i const& dims, HdFormat format);

    virtual void _Deallocate() override;

    unsigned int _width;
    unsigned int _height;
    HdFormat _format;

    std::vector<uint8_t> _buffer;
    std::atomic<int> _mappers;
    std::atomic<bool> _converged;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDXPRMAN_RENDERBUFFER_H
