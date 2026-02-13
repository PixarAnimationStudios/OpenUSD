//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef HDPARTICLEFIELD_HDPARTICLEFIELDRENDERPASS_H
#define HDPARTICLEFIELD_HDPARTICLEFIELDRENDERPASS_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/renderPass.h"

#include "renderBuffer.h"
#include "renderer.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderThread;

/// \class HdGaussianSplatsRenderPass
///
/// Represents a single iteration of a render.
class HdParticleFieldRenderPass final : public HdRenderPass {
  public:
    HdParticleFieldRenderPass(
        HdRenderIndex* index, const HdRprimCollection& collection,
        HdParticleFieldRenderer* renderer, HdRenderThread* renderThread,
        std::atomic<int>* sceneVersion);
    virtual ~HdParticleFieldRenderPass();

  protected:
    /// Draw the scene with the bound renderpass state.
    ///
    /// \param renderPassState Input parameters (including viewer parameters)
    /// for this renderpass. \param renderTags Which rendertags should be drawn
    /// this pass.
    void _Execute(
        const HdRenderPassStateSharedPtr& renderPassState,
        const TfTokenVector& renderTags) override;

    /// Determine whether the sample buffer has enough samples, to be considered
    /// final.
    bool IsConverged() const override;

  private:

    // A handle to the global renderer.
    HdParticleFieldRenderer* _renderer;

    // A handle to the render thread.
    HdRenderThread* _renderThread;

    // The last settings version we rendered with.
    int _lastSettingsVersion;

    // A reference to the global scene version.
    std::atomic<int>* _sceneVersion;

    // The last scene version we rendered with.
    int _lastSceneVersion;

    // The pixels written to. Like viewport in OpenGL,
    // but coordinates are y-Down.
    GfRect2i _dataWindow;

    // The view matrix: world space to camera space
    GfMatrix4d _viewMatrix;
    // The projection matrix: camera space to NDC space (with
    // respect to the data window).
    GfMatrix4d _projMatrix;

    // The list of aov buffers this renderpass should write to.
    HdRenderPassAovBindingVector _aovBindings;

    // If no attachments are provided, provide an anonymous renderbuffer for
    // color and depth output.
    HdParticleFieldRenderBuffer _colorBuffer;
    HdParticleFieldRenderBuffer _depthBuffer;

    // Is converged?
    bool _converged = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPARTICLEFIELD_HDPARTICLEFIELDRENDERPASS_H
