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
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/hdEmbree/renderer.h"

#include "pxr/base/gf/matrix4d.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;

/// \class HdEmbreeRenderPass
///
/// HdRenderPass represents a single render iteration, rendering a view of the
/// scene (the HdRprimCollection) for a specific viewer (the camera/viewport
/// parameters in HdRenderPassState) to the current draw target.
///
/// This class does so by raycasting into the embree scene via HdEmbreeRenderer.
///
class HdEmbreeRenderPass final : public HdRenderPass {
public:
    /// Renderpass constructor.
    ///   \param index The render index containing scene data to render.
    ///   \param collection The initial rprim collection for this renderpass.
    ///   \param renderThread A handle to the global render thread.
    ///   \param renderer A handle to the global renderer.
    HdEmbreeRenderPass(HdRenderIndex *index,
                       HdRprimCollection const &collection,
                       HdRenderThread *renderThread,
                       HdEmbreeRenderer *renderer,
                       std::atomic<int> *sceneVersion);

    /// Renderpass destructor.
    virtual ~HdEmbreeRenderPass();

    // -----------------------------------------------------------------------
    // HdRenderPass API

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
    virtual void _MarkCollectionDirty() override {}

private:
    // Create GL resources needed for blitting.
    void _CreateBlitResources();

    // Destroy GL resources needed for blitting.
    void _DestroyBlitResources();

    // Blit to the current OpenGL framebuffer.
    void _Blit(unsigned int width, unsigned int height, const uint8_t *data);

    // A handle to the render thread.
    HdRenderThread *_renderThread;

    // A handle to the global renderer.
    HdEmbreeRenderer *_renderer;

    // A reference to the global scene version.
    std::atomic<int> *_sceneVersion;

    // The last scene version we rendered with.
    int _lastRenderedVersion;

    // The width of the viewport we're rendering into.
    unsigned int _width;
    // The height of the viewport we're rendering into.
    unsigned int _height;

    // The view matrix: world space to camera space
    GfMatrix4d _viewMatrix;
    // The projection matrix: camera space to NDC space
    GfMatrix4d _projMatrix;

    // Whether the current render has converged.
    bool _converged;

    // For rendering the final result, a GL texture handle...
    GLuint _texture;
    // ... and a GL framebuffer handle.
    GLuint _framebuffer;
    // For correctness, keep track of the context the FBO was allocated on.
    GlfGLContextSharedPtr _owningContext;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDEMBREE_RENDER_PASS_H
