//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef HDPARTICLEFIELD_HDPARTICLEFIELDRENDERER_H
#define HDPARTICLEFIELD_HDPARTICLEFIELDRENDERER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/renderThread.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rect2i.h"

#include "gsRenderer.h"
#include "hd3DGaussianSplat.h"

PXR_NAMESPACE_OPEN_SCOPE

// Forward declarations.
class HdParticleFieldRenderBuffer;

/// \class HdParticleFieldRenderer
///
/// A gaussian splats renderer.
class HdParticleFieldRenderer final {
  public:
    HdParticleFieldRenderer();
    ~HdParticleFieldRenderer();

    /// Set the data window to fill (same meaning as in CameraUtilFraming
    /// with coordinate system also being y-Down).
    void SetDataWindow(const GfRect2i& dataWindow);

    /// Set the camera to use for rendering.
    ///   \param viewMatrix The camera's world-to-view matrix.
    ///   \param projMatrix The camera's view-to-NDC projection matrix.
    void SetCamera(const GfMatrix4d& viewMatrix, const GfMatrix4d& projMatrix);

    /// Set the aov bindings to use for rendering.
    ///   \param aovBindings A list of aov bindings.
    void SetAovBindings(HdRenderPassAovBindingVector const& aovBindings);

    /// Get the aov bindings being used for rendering.
    ///   \return the current aov bindings.
    HdRenderPassAovBindingVector const& GetAovBindings() const {
        return _aovBindings;
    }

    /// Set how many samples to render before considering an image converged.
    ///   \param samplesToConvergence How many samples are needed, per-pixel,
    ///                               before the image is considered finished.
    void SetSamplesToConvergence(int samplesToConvergence);

    void addGaussianSplats(const Hd3DGaussianSplat& splatPrim,
        const std::string& splatName);
    void removeGaussianSplats(const std::string& splatName);

    /// Rendering entrypoint: add one sample per pixel to the whole sample
    /// buffer, and then loop until the image is converged.  After each pass,
    /// the image will be resolved into a color buffer.
    ///   \param renderThread A handle to the render thread, used for checking
    ///                       for cancellation and locking the color buffer.
    void Render(HdRenderThread* renderThread);

    /// Clear the bound aov buffers (typically before rendering).
    void Clear();

    /// Mark the aov buffers as unconverged.
    void MarkAovBuffersUnconverged();

  private:
    // Validate the internal consistency of aov bindings provided to
    // SetAovBindings. If the aov bindings are invalid, this will issue
    // appropriate warnings. If the function returns false, Render() will fail
    // early.
    //
    // This function thunks itself using _aovBindingsNeedValidation and
    // _aovBindingsValid.
    //   \return True if the aov bindings are valid for rendering.
    bool _ValidateAovBindings();

    // Return the clear color to use for the given VtValue.
    static GfVec4f _GetClearColor(VtValue const& clearValue);

    // The bound aovs for this renderer.
    HdRenderPassAovBindingVector _aovBindings;
    // Parsed AOV name tokens.
    HdParsedAovTokenVector _aovNames;

    // Do the aov bindings need to be re-validated?
    bool _aovBindingsNeedValidation;
    // Are the aov bindings valid?
    bool _aovBindingsValid;

    // Data window - as in CameraUtilFraming.
    GfRect2i _dataWindow;

    // The width of the render buffers.
    unsigned int _width;
    // The height of the render buffers.
    unsigned int _height;

    // View matrix: world space to camera space.
    GfMatrix4d _viewMatrix;
    // Projection matrix: camera space to NDC space.
    GfMatrix4d _projMatrix;

    GaussianSplatsRenderer _gsRenderer;

    // How many samples should we render to convergence?
    int _samplesToConvergence;

    // How many samples have been completed.
    std::atomic<int> _completedSamples;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPARTICLEFIELD_HDPARTICLEFIELDRENDERER_H
