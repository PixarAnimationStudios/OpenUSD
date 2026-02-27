//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef HDPARTICLEFIELD_HDPARTICLEFIELDRENDERPARAM_H
#define HDPARTICLEFIELD_HDPARTICLEFIELDRENDERPARAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"

#include "renderer.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdGaussianSplatsRenderParam
///
/// Object created by the render delegate to transport top-level scene state
/// to each prim during Sync().
class HdParticleFieldRenderParam final : public HdRenderParam {
  public:
    HdParticleFieldRenderParam(
        HdParticleFieldRenderer* renderer,
        HdRenderThread* renderThread,
        std::atomic<int>* sceneVersion)
        : HdRenderParam(), _renderer(renderer)
        , _renderThread(renderThread), _sceneVersion(sceneVersion) {}

    /// Accessor for the renderer
    HdParticleFieldRenderer* AcquireRendererForEdit() {
        _renderThread->StopRender();
        (*_sceneVersion)++;
        return _renderer;
    }

    virtual ~HdParticleFieldRenderParam() = default;

  private:
    HdParticleFieldRenderer* _renderer{nullptr};

    /// A handle to the global render thread.
    HdRenderThread* _renderThread{nullptr};

    /// A version counter for edits to _scene.
    std::atomic<int>* _sceneVersion;

    /// Cannot copy.
    HdParticleFieldRenderParam(
        const HdParticleFieldRenderParam&) = delete;
    HdParticleFieldRenderParam& operator=(
        const HdParticleFieldRenderParam&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPARTICLEFIELD_HDPARTICLEFIELDRENDERPARAM_H
