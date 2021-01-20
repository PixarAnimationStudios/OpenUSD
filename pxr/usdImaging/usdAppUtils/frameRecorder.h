//
// Copyright 2019 Pixar
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
/// Note that it is assumed that an OpenGL context has already been setup.
class UsdAppUtilsFrameRecorder
{
public:
    USDAPPUTILS_API
    UsdAppUtilsFrameRecorder();

    /// Gets the ID of the Hydra renderer plugin that will be used for
    /// recording.
    TfToken GetCurrentRendererId() const {
        return _imagingEngine.GetCurrentRendererId();
    }

    /// Sets the Hydra renderer plugin to be used for recording.
    bool SetRendererPlugin(const TfToken& id) {
        return _imagingEngine.SetRendererPlugin(id);
    }

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
    void SetColorCorrectionMode(const TfToken& colorCorrectionMode) {
        _colorCorrectionMode = colorCorrectionMode;
    }

    /// Sets the UsdGeomImageable purposes to be used for rendering
    ///
    /// We will __always__ include "default" purpose, and by default,
    /// we will also include UsdGeomTokens->proxy.  Use this method
    /// to explicitly enumerate an alternate set of purposes to be
    /// included along with "default".
    USDAPPUTILS_API
    void SetIncludedPurposes(const TfTokenVector& purposes);

    /// Records an image and writes the result to \p outputImagePath.
    ///
    /// The recorded image will represent the view from \p usdCamera looking at
    /// the imageable prims on USD stage \p stage at time \p timeCode.
    ///
    /// If \p usdCamera is not a valid camera, a camera will be computed
    /// to automatically frame the stage geometry.
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
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
