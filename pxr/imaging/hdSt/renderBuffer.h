//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_RENDER_BUFFER_H
#define PXR_IMAGING_HD_ST_RENDER_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/textureUtils.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStResourceRegistry;
class HdStTextureIdentifier;
using HdStDynamicUvTextureObjectSharedPtr =
    std::shared_ptr<class HdStDynamicUvTextureObject>;

class HdStRenderBuffer : public HdRenderBuffer
{
public:
    HDST_API
    HdStRenderBuffer(HdStResourceRegistry *resourceRegistry, SdfPath const& id);

    HDST_API
    ~HdStRenderBuffer() override;

    HDST_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam *renderParam,
              HdDirtyBits *dirtyBits) override;

    HDST_API
    bool Allocate(GfVec3i const& dimensions,
                  HdFormat format,
                  bool multiSampled) override;

    HDST_API
    unsigned int GetWidth() const override;

    HDST_API
    unsigned int GetHeight() const override;

    HDST_API
    unsigned int GetDepth() const override;

    HDST_API
    HdFormat GetFormat() const override {return _format;}

    HDST_API
    bool IsMultiSampled() const override;

    HDST_API
    uint32_t GetMSAASampleCount() const;

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
    VtValue GetResource(bool multiSampled) const override;

    /// The identifier that can be passed to, e.g.,
    /// HdStResourceRegistry::AllocateTextureHandle so that a
    /// shader can bind this buffer as texture.
    HDST_API
    HdStTextureIdentifier GetTextureIdentifier(
        bool multiSampled);

protected:
    void _Deallocate() override;

private:
    // HdRenderBuffer::Allocate should take a scene delegate or
    // resource registry so that we do not need to save it here.
    HdStResourceRegistry * _resourceRegistry;

    // Format saved here (somewhat redundantely) since the
    // Hgi texture descriptor holds an HgiFormat instead of HdFormat.
    HdFormat _format;

    uint32_t _msaaSampleCount;

    // The GPU texture resource
    HdStDynamicUvTextureObjectSharedPtr _textureObject;

    // The GPU multi-sample texture resource (optional)
    HdStDynamicUvTextureObjectSharedPtr _textureMSAAObject;

    // The number of callers mapping this buffer.
    std::atomic<int> _mappers;
    // Texels are temp captured into this buffer between map and unmap.
    HdStTextureUtils::AlignedBuffer<uint8_t> _mappedBuffer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
