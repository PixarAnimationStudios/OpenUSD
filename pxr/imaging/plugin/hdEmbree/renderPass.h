//
// Copyright 2016 Pixar
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
#ifndef HDEMBREE_RENDER_PASS_H
#define HDEMBREE_RENDER_PASS_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/renderPass.h"

#include "pxr/base/gf/matrix4d.h"

#include <embree2/rtcore.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdEmbreeRenderPass
///
/// HdRenderPass represents a single render iteration, rendering a view of the
/// scene (the HdRprimCollection) for a specific viewer (the camera/viewport
/// parameters in HdRenderPassState) to the current draw target.
///
/// This class does so by raycasting into the embree scene.
///
class HdEmbreeRenderPass final : public HdRenderPass {
public:
    /// Renderpass constructor.
    ///   \param index The render index containing scene data to render.
    ///   \param collection The initial rprim collection for this renderpass.
    ///   \param scene The embree scene to raycast into.
    HdEmbreeRenderPass(HdRenderIndex *index,
                       HdRprimCollection const &collection,
                       RTCScene scene);

    /// Renderpass destructor.
    virtual ~HdEmbreeRenderPass();

    // -----------------------------------------------------------------------
    // HdRenderPass API

    /// Clear the sample buffer (when scene or camera changes).
    virtual void ResetImage() override;

    /// Determine whether the sample buffer has enough samples.
    ///   \return True if the image has enough samples to be considered final.
    virtual bool IsConverged() const override;

protected:

    // -----------------------------------------------------------------------
    // HdRenderPass API

    /// Draw the scene with the bound renderpass state.
    ///   \param renderPassState Input parameters (including viewer parameters)
    ///                          for this renderpass.
    ///   \param renderTags Which rendertags should be drawn this pass.
    virtual void _Execute(HdRenderPassStateSharedPtr const& renderPassState,
                          TfTokenVector const &renderTags) override;

    /// Update internal tracking to reflect a dirty collection.
    virtual void _MarkCollectionDirty() override;

private:

    // -----------------------------------------------------------------------
    // Internal API

    // Specify a new viewport size for the sample buffer. Note: the caller
    // should also call ResetImage().
    void _ResizeSampleBuffer(unsigned int width, unsigned int height);

    // Rendering entrypoint: add one sample per pixel to the sample
    // buffer. This is decomposed into parallel calls to _RenderTiles.
    void _Render();

    // Render square tiles of pixels. This function is one unit of threadpool
    // work. For each tile, iterate over pixels in the tile, generating camera
    // rays, and following them/calculating color with _TraceRay. This function
    // renders all tiles between tileStart and tileEnd.
    void _RenderTiles(size_t tileStart, size_t tileEnd);

    // Cast a ray into the scene and if it hits an object, return the
    // computed color; otherwise return _clearColor.
    GfVec3f _TraceRay(GfVec3f const& origin, GfVec3f const& dir);

    // Compute the ambient occlusion term at a given point by firing rays
    // from "position" in the hemisphere centered on "normal"; the occlusion
    // factor is the fraction of those rays that are occluded.
    //
    // Modulating surface color by (1 - occlusionFactor) is similar to taking
    // the light contribution of an infinitely far, pure white dome light.
    float _ComputeAmbientOcclusion(GfVec3f const& position,
                                   GfVec3f const& normal);

    // The sample buffer is cleared in Execute(), so this flag records whether
    // ResetImage() has been called since the last Execute().
    bool _pendingResetImage;

    // The output buffer for the raytracing algorithm. If pixel is
    // &_sampleBuffer[y*_width+x], then pixel[0-2] represent accumulated R,G,B
    // values, over a number of render passes stored in pixel[3]; the average
    // color value is then pixel[0-2] / pixel[3].
    std::vector<float> _sampleBuffer;

    // The resolved output buffer, in GL_RGBA. This is an intermediate between
    // _sampleBuffer and the GL framebuffer.
    std::vector<uint8_t> _colorBuffer;

    // The width of the viewport we're rendering into.
    unsigned int _width;
    // The height of the viewport we're rendering into.
    unsigned int _height;

    // Our handle to the embree scene.
    RTCScene _scene;

    // The inverse view matrix: camera space to world space.
    GfMatrix4d _inverseViewMatrix;
    // The inverse projection matrix: NDC space to camera space.
    GfMatrix4d _inverseProjMatrix;

    // The color of a ray miss.
    GfVec3f _clearColor;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDEMBREE_RENDER_PASS_H
