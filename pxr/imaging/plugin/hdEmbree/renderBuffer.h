//
// Copyright 2018 Pixar
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
#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_RENDER_BUFFER_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_RENDER_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderBuffer.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdEmbreeRenderBuffer : public HdRenderBuffer
{
public:
    HdEmbreeRenderBuffer(SdfPath const& id);
    ~HdEmbreeRenderBuffer() override;

    /// Get allocation information from the scene delegate.
    /// Note: Embree overrides this only to stop the render thread before
    /// potential re-allocation.
    ///   \param sceneDelegate The scene delegate backing this render buffer.
    ///   \param renderParam   The renderer-global render param.
    ///   \param dirtyBits     The invalidation state for this render buffer.
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam *renderParam,
              HdDirtyBits *dirtyBits) override;

    /// Deallocate before deletion.
    ///   \param renderParam   The renderer-global render param.
    /// Note: Embree overrides this only to stop the render thread before
    /// potential deallocation.
    void Finalize(HdRenderParam *renderParam) override;

    /// Allocate a new buffer with the given dimensions and format.
    ///   \param dimensions   Width, height, and depth of the desired buffer.
    ///                       (Only depth==1 is supported).
    ///   \param format       The format of the desired buffer, taken from the
    ///                       HdFormat enum.
    ///   \param multisampled Whether the buffer is multisampled.
    ///   \return             True if the buffer was successfully allocated,
    ///                       false with a warning otherwise.
    bool Allocate(GfVec3i const& dimensions,
                  HdFormat format,
                  bool multiSampled) override;

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
    bool IsMultiSampled() const override { return _multiSampled; }

    /// Map the buffer for reading/writing. The control flow should be Map(),
    /// before any I/O, followed by memory access, followed by Unmap() when
    /// done.
    ///   \return The address of the buffer.
    void* Map() override {
        _mappers++;
        return _buffer.data();
    }

    /// Unmap the buffer.
    void Unmap() override {
        _mappers--;
    }

    /// Return whether any clients have this buffer mapped currently.
    ///   \return True if the buffer is currently mapped by someone.
    bool IsMapped() const override {
        return _mappers.load() != 0;
    }

    /// Is the buffer converged?
    ///   \return True if the buffer is converged (not currently being
    ///           rendered to).
    bool IsConverged() const override {
        return _converged.load();
    }

    /// Set the convergence.
    ///   \param cv Whether the buffer should be marked converged or not.
    void SetConverged(bool cv) {
        _converged.store(cv);
    }

    /// Resolve the sample buffer into final values.
    void Resolve() override;

    // ---------------------------------------------------------------------- //
    /// \name I/O helpers
    // ---------------------------------------------------------------------- //

    /// Write a float, vec2f, vec3f, or vec4f to the renderbuffer.
    /// This should only be called on a mapped buffer. Extra components will
    /// be silently discarded; if not enough are provided for the buffer, the
    /// remainder will be taken as 0.
    ///   \param pixel         What index to write
    ///   \param numComponents The arity of the value to write.
    ///   \param value         A float-valued vector to write. 
    void Write(GfVec3i const& pixel, size_t numComponents, float const* value);

    /// Write an int, vec2i, vec3i, or vec4i to the renderbuffer.
    /// This should only be called on a mapped buffer. Extra components will
    /// be silently discarded; if not enough are provided for the buffer, the
    /// remainder will be taken as 0.
    ///   \param pixel         What index to write
    ///   \param numComponents The arity of the value to write.
    ///   \param value         An int-valued vector to write. 
    void Write(GfVec3i const& pixel, size_t numComponents, int const* value);

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

    // Return the sample format for the given buffer format. Sample buffers
    // are always float32 or int32, but with the same number of components
    // as the base format.
    static HdFormat _GetSampleFormat(HdFormat format);

    // Release any allocated resources.
    void _Deallocate() override;

    // Buffer width.
    unsigned int _width;
    // Buffer height.
    unsigned int _height;
    // Buffer format.
    HdFormat _format;
    // Whether the buffer is operating in multisample mode.
    bool _multiSampled;

    // The resolved output buffer.
    std::vector<uint8_t> _buffer;
    // For multisampled buffers: the input write buffer.
    std::vector<uint8_t> _sampleBuffer;
    // For multisampled buffers: the sample count buffer.
    std::vector<uint32_t> _sampleCount;

    // The number of callers mapping this buffer.
    std::atomic<int> _mappers;
    // Whether the buffer has been marked as converged.
    std::atomic<bool> _converged;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_EMBREE_RENDER_BUFFER_H
