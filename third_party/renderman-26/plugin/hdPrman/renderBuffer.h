//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_BUFFER_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderBuffer.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanRenderBuffer : public HdRenderBuffer
{
public:
    HdPrmanRenderBuffer(SdfPath const& id);
    ~HdPrmanRenderBuffer() override;

    bool Allocate(GfVec3i const& dimensions,
                  HdFormat format,
                  bool multiSampled) override;

    unsigned int GetWidth() const override { return _width; }
    unsigned int GetHeight() const override { return _height; }
    unsigned int GetDepth() const override { return 1; }
    HdFormat GetFormat() const override { return _format; }

    // HdPrman doesn't handle sampling decisions at the hydra level.
    bool IsMultiSampled() const override { return false; }

    void* Map() override {
        _mappers++;
        return _buffer.data();
    }
    void Unmap() override {
        _mappers--;
    }
    bool IsMapped() const override {
        return _mappers.load() != 0;
    }

    bool IsConverged() const override {
        return _converged.load();
    }
    void SetConverged(bool cv) {
        _converged.store(cv);
    }

    void Resolve() override {}

    // ---------------------------------------------------------------------- //
    /// \name I/O helpers
    // ---------------------------------------------------------------------- //

    // format is the input format.
    void Blit(HdFormat format, int width, int height, uint8_t const* data);

private:
    static size_t _GetBufferSize(GfVec2i const& dims, HdFormat format);

    void _Deallocate() override;

    unsigned int _width;
    unsigned int _height;
    HdFormat _format;

    std::vector<uint8_t> _buffer;
    std::atomic<int> _mappers;
    std::atomic<bool> _converged;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_BUFFER_H
