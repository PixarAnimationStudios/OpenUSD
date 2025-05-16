//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_RENDERER_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_RENDERER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/hd/renderPassState.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rect2i.h"

#include <embree4/rtcore.h>
#include <embree4/rtcore_ray.h>

#include <random>
#include <atomic>

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
class HdEmbreeRenderer final
{
public:
    /// Renderer constructor.
    HdEmbreeRenderer();

    /// Renderer destructor.
    ~HdEmbreeRenderer();

    /// Set the embree scene that this renderer should raycast into.
    ///   \param scene The embree scene to use.
    void SetScene(RTCScene scene);

    /// Set the data window to fill (same meaning as in CameraUtilFraming
    /// with coordinate system also being y-Down).
    void SetDataWindow(const GfRect2i &dataWindow);

    /// Set the camera to use for rendering.
    ///   \param viewMatrix The camera's world-to-view matrix.
    ///   \param projMatrix The camera's view-to-NDC projection matrix.
    void SetCamera(const GfMatrix4d& viewMatrix, const GfMatrix4d& projMatrix);

    /// Set the aov bindings to use for rendering.
    ///   \param aovBindings A list of aov bindings.
    void SetAovBindings(HdRenderPassAovBindingVector const &aovBindings);

    /// Get the aov bindings being used for rendering.
    ///   \return the current aov bindings.
    HdRenderPassAovBindingVector const& GetAovBindings() const {
        return _aovBindings;
    }

    /// Set how many samples to render before considering an image converged.
    ///   \param samplesToConvergence How many samples are needed, per-pixel,
    ///                               before the image is considered finished.
    void SetSamplesToConvergence(int samplesToConvergence);

    /// Set how many samples to use for ambient occlusion.
    ///   \param ambientOcclusionSamples How many samples are needed for
    ///                                  ambient occlusion? 0 = disable.
    void SetAmbientOcclusionSamples(int ambientOcclusionSamples);

    /// Sets whether to use scene colors while rendering.
    ///   \param enableSceneColors Whether drawing should sample color, or draw
    ///                            everything as white.
    void SetEnableSceneColors(bool enableSceneColors);

    /// Sets a number to seed the random number generator with.
    ///   \param randomNumberSeed If -1, then the random number generator
    ///                           is seeded in a non-deterministic way;
    ///                           otherwise, it is seeded with this value.
    void SetRandomNumberSeed(int randomNumberSeed);

    /// Rendering entrypoint: add one sample per pixel to the whole sample
    /// buffer, and then loop until the image is converged.  After each pass,
    /// the image will be resolved into a color buffer.
    ///   \param renderThread A handle to the render thread, used for checking
    ///                       for cancellation and locking the color buffer.
    void Render(HdRenderThread *renderThread);

    /// Clear the bound aov buffers (typically before rendering).
    void Clear();

    /// Mark the aov buffers as unconverged.
    void MarkAovBuffersUnconverged();

    /// Get the number of samples completed so far.
    int GetCompletedSamples() const;

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

    // Render square tiles of pixels. This function is one unit of threadpool
    // work. For each tile, iterate over pixels in the tile, generating camera
    // rays, and following them/calculating color with _TraceRay. This function
    // renders all tiles between tileStart and tileEnd.
    void _RenderTiles(HdRenderThread *renderThread, int sampleNum,
                      size_t tileStart, size_t tileEnd);

    // Cast a ray into the scene and if it hits an object, write to the bound
    // aov buffers.
    void _TraceRay(unsigned int x, unsigned int y,
                   GfVec3f const& origin, GfVec3f const& dir,
                   std::default_random_engine &random);

    // Compute the color at the given ray hit.
    GfVec4f _ComputeColor(RTCRayHit const& rayHit,
                          std::default_random_engine &random,
                          GfVec4f const& clearColor);
    // Compute the depth at the given ray hit.
    bool _ComputeDepth(RTCRayHit const& rayHit, float *depth, bool clip);
    // Compute the given ID at the given ray hit.
    bool _ComputeId(RTCRayHit const& rayHit, TfToken const& idType, int32_t *id);
    // Compute the normal at the given ray hit.
    bool _ComputeNormal(RTCRayHit const& rayHit, GfVec3f *normal, bool eye);
    // Compute a primvar at the given ray hit.
    bool _ComputePrimvar(RTCRayHit const& rayHit, TfToken const& primvar,
        GfVec3f *value);

    // Compute the ambient occlusion term at a given point by firing rays
    // from "position" in the hemisphere centered on "normal"; the occlusion
    // factor is the fraction of those rays that are visible.
    //
    // Modulating surface color by occlusionFactor is similar to taking
    // the light contribution of an infinitely far, pure white dome light.
    float _ComputeAmbientOcclusion(GfVec3f const& position,
                                   GfVec3f const& normal,
                                   std::default_random_engine &random);

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
    // The inverse view matrix: camera space to world space.
    GfMatrix4d _inverseViewMatrix;
    // The inverse projection matrix: NDC space to camera space.
    GfMatrix4d _inverseProjMatrix;

    // Our handle to the embree scene.
    RTCScene _scene;

    // How many samples should we render to convergence?
    int _samplesToConvergence;
    // How many samples should we use for ambient occlusion?
    int _ambientOcclusionSamples;
    // Should we enable scene colors?
    bool _enableSceneColors;
    // If other than -1, use this to seed the random number generator with.
    int _randomNumberSeed;

    // How many samples have been completed.
    std::atomic<int> _completedSamples;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_EMBREE_RENDERER_H
