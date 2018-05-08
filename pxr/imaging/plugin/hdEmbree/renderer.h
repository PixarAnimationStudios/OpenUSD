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
#ifndef HDEMBREE_RENDERER_H
#define HDEMBREE_RENDERER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/renderThread.h"

#include "pxr/base/gf/matrix4d.h"

#include <embree2/rtcore.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdEmbreeRenderer
///
/// HdEmbreeRenderer implements a renderer on top of Embree's raycasting
/// abilities.  This is currently a very simple renderer.  It breaks the
/// framebuffer into tiles for multithreading; sends out jittered camera
/// rays; and implements the following shading:
///  - Colors via the "color" primvar.
///  - Lighting via N dot Camera-ray, simulating a point light at the camera
///    origin.
///  - Ambient occlusion.
///
class HdEmbreeRenderer final {
public:
    /// Renderer constructor.
    HdEmbreeRenderer();

    /// Renderer destructor.
    ~HdEmbreeRenderer();

    /// Set the embree scene that this renderer should raycast into.
    ///   \param scene The embree scene to use.
    void SetScene(RTCScene scene);

    /// Specify a new viewport size for the sample/color buffer.
    ///   \param width The new viewport width.
    ///   \param height The new viewport height.
    void SetViewport(unsigned int width, unsigned int height);

    /// Set the clear color.
    ///   \param clearColor The color to use for a camera ray miss.
    void SetClearColor(const GfVec3f& clearColor);

    /// Set the camera to use for rendering.
    ///   \param viewMatrix The camera's world-to-view matrix.
    ///   \param projMatrix The camera's view-to-NDC projection matrix.
    void SetCamera(const GfMatrix4d& viewMatrix, const GfMatrix4d& projMatrix);

    /// Return the color buffer with the current partial or final frame.
    ///   \return A pointer to color data.
    const uint8_t* GetColorBuffer();

    /// Rendering entrypoint: add one sample per pixel to the whole sample
    /// buffer, and then loop until the image is converged.  After each pass,
    /// the image will be resolved into a color buffer.
    ///   \param renderThread A handle to the render thread, used for checking
    ///                       for cancellation and locking the color buffer.
    void Render(HdRenderThread *renderThread);

private:
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

    // The output buffer for the raytracing algorithm. If pixel is
    // &_sampleBuffer[y*_width+x], then pixel[0-2] represent accumulated R,G,B
    // values, over a number of render passes stored in numSamples; the average
    // color value is then pixel[0-2] / numSamples.
    std::vector<float> _sampleBuffer;
    // The number of samples (per pixel) in _sampleBuffer.
    unsigned int _numSamples;

    // The resolved output buffer, in GL_RGBA. This is an intermediate between
    // _sampleBuffer and the GL framebuffer.
    std::vector<uint8_t> _colorBuffer;

    // The width of the viewport we're rendering into.
    unsigned int _width;
    // The height of the viewport we're rendering into.
    unsigned int _height;

    // The inverse view matrix: camera space to world space.
    GfMatrix4d _inverseViewMatrix;
    // The inverse projection matrix: NDC space to camera space.
    GfMatrix4d _inverseProjMatrix;

    // The color of a ray miss.
    GfVec3f _clearColor;

    // Our handle to the embree scene.
    RTCScene _scene;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDEMBREE_RENDERER_H
