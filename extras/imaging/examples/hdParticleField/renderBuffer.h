//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef HDPARTICLEFIELD_HDPARTICLEFIELDRENDERBUFFER_H
#define HDPARTICLEFIELD_HDPARTICLEFIELDRENDERBUFFER_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/imaging/hd/renderBuffer.h"

#include "debugCodes.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdParticleFieldRenderBuffer
///
/// A block of memory which we are rendering into.
class HdParticleFieldRenderBuffer : public HdRenderBuffer {
  public:
    HdParticleFieldRenderBuffer(const SdfPath& bprimId);
    ~HdParticleFieldRenderBuffer() override;

    /// Get allocation information from the scene delegate.
    /// Note: overridden only to stop the render thread before
    /// potential re-allocation.
    ///   \param sceneDelegate The scene delegate backing this render buffer.
    ///   \param renderParam   The renderer-global render param.
    ///   \param dirtyBits     The invalidation state for this render buffer.
    void Sync(
        HdSceneDelegate* sceneDelegate,
        HdRenderParam* renderParam,
        HdDirtyBits* dirtyBits) override;

    /// Deallocate before deletion.
    ///   \param renderParam   The renderer-global render param.
    /// Note:overridden only to stop the render thread before
    /// potential deallocation.
    void Finalize(HdRenderParam* renderParam) override;

    /// Allocate a new buffer with the given dimensions and format.
    bool Allocate(
        const GfVec3i& dimensions, HdFormat format, bool multiSampled) override;

    /// Accessor for buffer width.
    ///   \return The width of the currently allocated buffer.
    unsigned int GetWidth() const override { return _width; }

    /// Accessor for buffer height.
    ///   \return The height of the currently allocated buffer.
    unsigned int GetHeight() const override { return _height; }

    /// Accessor for buffer depth.
    ///   \return The depth of the currently allocated buffer.
    unsigned int GetDepth() const override { return 1; }

    /// Accessor for buffer format.
    ///   \return The format of the currently allocated buffer.
    HdFormat GetFormat() const override { return _format; }

    /// Accessor for the buffer multisample state.
    ///   \return Whether the buffer is multisampled or not.
    bool IsMultiSampled() const override { return false; }

    /// Map the buffer for reading/writing. The control flow should be Map(),
    /// before any I/O, followed by memory access, followed by Unmap() when
    /// done.
    ///   \return The address of the buffer.
    void* Map() override {
        _mappers++;
        return _buffer.data();
    }

    /// Unmap the buffer.
    void Unmap() override { _mappers--; }

    /// Return whether any clients have this buffer mapped currently.
    ///   \return True if the buffer is currently mapped by someone.
    bool IsMapped() const override { return _mappers.load() != 0; }

    /// Is the buffer converged?
    ///   \return True if the buffer is converged (not currently being
    ///           rendered to).
    bool IsConverged() const override {
        bool c = _converged.load();
        TF_DEBUG(HDPARTICLEFIELD_GENERAL).Msg(
            "[%s] _converged = %d\n", TF_FUNC_NAME().c_str(), c);
        return c;
    }

    /// Set the convergence.
    ///   \param cv Whether the buffer should be marked converged or not.
    void SetConverged(bool cv) {
        TF_DEBUG(HDPARTICLEFIELD_GENERAL).Msg(
            "[%s] _converged \n", TF_FUNC_NAME().c_str());
        _converged.store(cv);
    }

    /// Resolve the sample buffer into final values.
    void Resolve() override;

    /// Write a float, vec2f, vec3f, or vec4f to the renderbuffer.
    /// This should only be called on a mapped buffer. Extra components will
    /// be silently discarded; if not enough are provided for the buffer, the
    /// remainder will be taken as 0.
    ///   \param pixel         What index to write
    ///   \param numComponents The arity of the value to write.
    ///   \param value         A float-valued vector to write.
    void Write(GfVec2i const& pixel, size_t numComponents, float const* value);

    /// Write an int, vec2i, vec3i, or vec4i to the renderbuffer.
    /// This should only be called on a mapped buffer. Extra components will
    /// be silently discarded; if not enough are provided for the buffer, the
    /// remainder will be taken as 0.
    ///   \param pixel         What index to write
    ///   \param numComponents The arity of the value to write.
    ///   \param value         An int-valued vector to write.
    void Write(GfVec2i const& pixel, size_t numComponents, int const* value);

    /// Write a vec3f and alpha, overing on top the prior color.
    void OverColor(const GfVec2i& pixel, GfVec3f color, float alpha);

    /// Clear the renderbuffer with a float, vec2f, vec3f, or vec4f.
    /// This should only be called on a mapped buffer. Extra components will
    /// be silently discarded; if not enough are provided for the buffer, the
    /// remainder will be taken as 0.
    ///   \param numComponents The arity of the value to write.
    ///   \param value         A float-valued vector to write.
    void Clear(size_t numComponents, float const* value);

    /// Clear the renderbuffer with an int, vec2i, vec3i, or vec4i.
    /// This should only be called on a mapped buffer. Extra components will
    /// be silently discarded; if not enough are provided for the buffer, the
    /// remainder will be taken as 0.
    ///   \param numComponents The arity of the value to write.
    ///   \param value         An int-valued vector to write.
    void Clear(size_t numComponents, int const* value);

  private:
    // Calculate the needed buffer size, given the allocation parameters.
    static size_t _GetBufferSize(GfVec2i const& dims, HdFormat format);

    // Release any allocated resources.
    void _Deallocate() override;

    // Buffer width.
    unsigned int _width;
    // Buffer height.
    unsigned int _height;
    // Buffer format.
    HdFormat _format;

    // The actual buffer of bytes.
    std::vector<uint8_t> _buffer;

    // The number of callers mapping this buffer.
    std::atomic<int> _mappers{0};

    // Whether the buffer has been marked as converged.
    std::atomic<bool> _converged{false};
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPARTICLEFIELD_HDPARTICLEFIELDRENDERBUFFER_H
