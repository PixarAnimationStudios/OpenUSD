//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_CAMERA_UTIL_FRAMING_H
#define PXR_IMAGING_CAMERA_UTIL_FRAMING_H

#include "pxr/pxr.h"
#include "pxr/imaging/cameraUtil/api.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/base/gf/rect2i.h"
#include "pxr/base/gf/range2f.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class CameraUtilFraming
///
/// Framing information. That is information determining how the filmback
/// plane of a camera maps to the pixels of the rendered image (displayWindow
/// together with pixelAspectRatio and window policy) and what pixels of the
/// image will be filled by the renderer (dataWindow).
///
/// The concepts of displayWindow and dataWindow are similar to the ones in
/// OpenEXR, including that the x- and y-axis of the coordinate system point
/// right and down, respectively.
///
/// In fact, these windows mean the same here and in OpenEXR if the
/// displayWindow has the same aspect ratio (when accounting for the
/// pixelAspectRatio) as the filmback plane of the camera (that is the
/// ratio of the horizontalAperture to verticalAperture of, e.g., Usd's Camera
/// or GfCamera).
///
/// In particular, overscan can be achieved by making the dataWindow larger
/// than the displayWindow.
///
/// If the aspect ratios differ, a window policy is applied to the
/// displayWindow to determine how the pixels correspond to the
/// filmback plane.  One such window policy is to take the largest
/// rect that fits (centered) into the displayWindow and has the
/// camera's aspect ratio. For example, if the displayWindow and dataWindow
/// are the same and both have an aspect ratio smaller than the camera, the
/// image is created by enlarging the camera frustum slightly in the bottom
/// and top direction.
///
/// When using the AOVs, the render buffer size is determined
/// independently from the framing info. However, the dataWindow is
/// supposed to be contained in the render buffer rect (in particular,
/// the dataWindow cannot contain pixels with negative coordinates -
/// this restriction does not apply if, e.g., hdPrman circumvents AOVs
/// and writes directly to EXR). In other words, unlike in OpenEXR,
/// the rect of pixels for which we allocate storage can differ from
/// the rect the renderer fills with data (dataWindow).
///
/// For example, an application can set the render buffer size to match
/// the widget size but use a dataWindow and displayWindow that only fills
/// the render buffer horizontally to have slates at the top and bottom.
///
class CameraUtilFraming final
{
public:
    /// Creates an invalid framing, i.e., with empty display and data
    /// window.
    CAMERAUTIL_API
    CameraUtilFraming();

    /// Creates a framing with given display and data window and pixel
    /// aspect ratio.
    ///
    CAMERAUTIL_API
    CameraUtilFraming(
        const GfRange2f &displayWindow,
        const GfRect2i &dataWindow,
        float pixelAspectRatio = 1.0);

    /// Creates a framing with equal display and data window (and
    /// assuming square pixels).
    ///
    CAMERAUTIL_API
    explicit CameraUtilFraming(
        const GfRect2i &dataWindow);

    /// Is display and data window non-empty.
    CAMERAUTIL_API
    bool IsValid() const;

    CAMERAUTIL_API
    bool operator==(const CameraUtilFraming& other) const;

    CAMERAUTIL_API
    bool operator!=(const CameraUtilFraming& other) const;

    /// The filmback window is the rectangle in pixel space corresponding
    /// to the filmback plane. It is obtained by conforming the display
    /// window using the camera's aspect ratio.
    ///
    /// Note that the window policy describes how the camera frustum is
    /// modified to match the display window's aspect ratio. The filmback
    /// window is transforming differently: if, e.g., the camera frustum's
    /// height had to be increased to match the displayWindow's aspect ratio
    /// (since it is less than the camera's aspect ratio and the policy is
    /// CameraUtilFit), then the filmback window height will be less than
    /// that of the displayWindow. In other words, imagine an application
    /// window too tall to display the camera. We will increase the camera
    /// frustum's height to fill the entire window. To show only what the
    /// camera would see, we need to add slates on the bottom and top.
    /// The filmback window is the rect cut out by the slates.
    ///
    CAMERAUTIL_API
    GfRange2f ComputeFilmbackWindow(
        float cameraAspectRatio,
        CameraUtilConformWindowPolicy windowPolicy) const;

    /// Given the projectionMatrix computed from a camera, applies
    /// the framing. To obtain a correct result, a rasterizer needs
    /// to use the resulting projection matrix and set the viewport
    /// to the data window.
    ///
    CAMERAUTIL_API
    GfMatrix4d ApplyToProjectionMatrix(
        const GfMatrix4d &projectionMatrix,
        CameraUtilConformWindowPolicy windowPolicy) const;

    /// The display window.
    GfRange2f displayWindow;
    
    /// The data window. That is the rect of pixels that the renderer
    /// will fill.
    GfRect2i dataWindow;

    /// The ratio of the width to the height of a pixel - same as
    /// OpenEXR.
    float pixelAspectRatio;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
