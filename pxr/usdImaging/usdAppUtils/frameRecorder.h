//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_APP_UTILS_FRAME_RECORDER_H
#define PXR_USD_IMAGING_USD_APP_UTILS_FRAME_RECORDER_H

/// \file usdAppUtils/frameRecorder.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdAppUtils/api.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usdImaging/usdImagingGL/engine.h"

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdAppUtilsFrameRecorder
///
/// A utility class for recording images of USD stages.
///
/// UsdAppUtilsFrameRecorder uses Hydra to produce recorded images of a USD
/// stage looking through a particular UsdGeomCamera on that stage at a
/// particular UsdTimeCode. The images generated will be effectively the same
/// as what you would see in the viewer in usdview.
///
/// Note that it is assumed that an OpenGL context has already been setup for
/// the UsdAppUtilsFrameRecorder if OpenGL is being used as the underlying HGI
/// device. This is not required for Metal or Vulkan.
class UsdAppUtilsFrameRecorder
{
public:
    /// The \p rendererPluginId argument indicates the renderer plugin that
    /// Hyrda should use. If the empty token is passed in, a default renderer
    /// plugin will be chosen depending on the value of \p gpuEnabled.
    /// The \p gpuEnabled argument determines if the UsdAppUtilsFrameRecorder
    /// instance will allow Hydra to use the GPU to produce images.
    /// The \p enableUsdDrawModes argument determines whether the frame
    /// recorder instance will respect USD draw modes as authored. Setting
    /// this to false causes the frame recorder to ignore draw modes.
    USDAPPUTILS_API
    UsdAppUtilsFrameRecorder(
        const TfToken& rendererPluginId = TfToken(),
        bool gpuEnabled = true,
        bool enableUsdDrawModes = true);

    /// Gets the ID of the Hydra renderer plugin that will be used for
    /// recording.
    TfToken GetCurrentRendererId() const {
        return _imagingEngine.GetCurrentRendererId();
    }

    /// Sets the Hydra renderer plugin to be used for recording.
    /// This also resets the presentation flag on the HdxPresentTask to false,
    /// to avoid the need for an OpenGL context.
    ///
    /// Note that the renderer plugins that may be set will be restricted if
    /// this UsdAppUtilsFrameRecorder instance has disabled the GPU.
    bool SetRendererPlugin(const TfToken& id) {
        const bool succeeded = _imagingEngine.SetRendererPlugin(id);
        _imagingEngine.SetEnablePresentation(false);

        return succeeded;
    }

    /// Sets the path to the render pass prim to use.
    ///
    /// \note If there is a render settings prim designated by the
    /// render pass prim via renderSource, it must also be set
    /// with SetActiveRenderSettingsPrimPath().
    USDAPPUTILS_API
    void SetActiveRenderPassPrimPath(SdfPath const& path);

    /// Sets the path to the render settings prim to use.
    ///
    /// \see SetActiveRenderPassPrimPath()
    USDAPPUTILS_API
    void SetActiveRenderSettingsPrimPath(SdfPath const& path);

    /// Sets the width of the recorded image.
    ///
    /// The height of the recorded image will be computed using this value and
    /// the aspect ratio of the camera used for recording.
    ///
    /// The default image width is 960 pixels.
    void SetImageWidth(const size_t imageWidth) {
        if (imageWidth == 0u) {
            TF_CODING_ERROR("Image width cannot be zero");
            return;
        }
        _imageWidth = imageWidth;
    }

    /// Sets the level of refinement complexity.
    ///
    /// The default complexity is "low" (1.0).
    void SetComplexity(const float complexity) {
        _complexity = complexity;
    }

    /// Sets the color correction mode to be used for recording.
    ///
    /// By default, color correction is disabled.
    USDAPPUTILS_API
    void SetColorCorrectionMode(const TfToken& colorCorrectionMode);

    /// Turns the built-in camera light on or off.
    ///
    /// When on, this will add a light at the camera's origin.
    /// This is sometimes called a "headlight".
    USDAPPUTILS_API
    void SetCameraLightEnabled(bool cameraLightEnabled);

    /// Sets the camera visibility of dome lights.
    ///
    /// When on, dome light textures will be drawn to the background as if
    /// mapped onto a sphere infinitely far away.
    USDAPPUTILS_API
    void SetDomeLightVisibility(bool domeLightsVisible);

    /// Sets the UsdGeomImageable purposes to be used for rendering
    ///
    /// We will __always__ include "default" purpose, and by default,
    /// we will also include UsdGeomTokens->proxy.  Use this method
    /// to explicitly enumerate an alternate set of purposes to be
    /// included along with "default".
    USDAPPUTILS_API
    void SetIncludedPurposes(const TfTokenVector& purposes);

    /// Sets the primary camera prim path.
    USDAPPUTILS_API
    void SetPrimaryCameraPrimPath(const SdfPath& cameraPath);

    /// Records an image and writes the result to \p outputImagePath.
    ///
    /// The recorded image will represent the view from \p usdCamera looking at
    /// the imageable prims on USD stage \p stage at time \p timeCode.
    ///
    /// If \p usdCamera is not a valid camera, a camera will be computed
    /// to automatically frame the stage geometry.
    ///
    /// When we are using a RenderSettings prim, the generated image will be
    /// written to the file indicated on the connected RenderProducts,
    /// instead of the given \p outputImagePath. Note that in this case the
    /// given \p usdCamera will later be overridden by the one authored on the
    /// RenderSettings Prim.
    ///
    /// Returns true if the image was generated and written successfully, or
    /// false otherwise.
    USDAPPUTILS_API
    bool Record(
            const UsdStagePtr& stage,
            const UsdGeomCamera& usdCamera,
            const UsdTimeCode timeCode,
            const std::string& outputImagePath);

private:
    UsdImagingGLEngine _imagingEngine;
    size_t _imageWidth;
    float _complexity;
    TfToken _colorCorrectionMode;
    TfTokenVector _purposes;
    SdfPath _renderPassPrimPath;
    SdfPath _renderSettingsPrimPath;
    bool _cameraLightEnabled;
    bool _domeLightsVisible;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
