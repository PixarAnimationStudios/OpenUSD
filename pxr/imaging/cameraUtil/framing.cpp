//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/cameraUtil/framing.h"

#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

CameraUtilFraming::CameraUtilFraming()
  : pixelAspectRatio(1.0f)
{
}

CameraUtilFraming::CameraUtilFraming(
    const GfRange2f &displayWindow,
    const GfRect2i &dataWindow,
    const float pixelAspectRatio)
  : displayWindow(displayWindow)
  , dataWindow(dataWindow)
  , pixelAspectRatio(pixelAspectRatio)
{
}

CameraUtilFraming::CameraUtilFraming(
    const GfRect2i &dataWindow)
  : CameraUtilFraming(
      GfRange2f(
          GfVec2f(dataWindow.GetMinX(),     dataWindow.GetMinY()),
          GfVec2f(dataWindow.GetMaxX() + 1, dataWindow.GetMaxY() + 1)),
      dataWindow)
{
}

bool
CameraUtilFraming::IsValid() const
{
    return
        (!dataWindow.IsEmpty()) &&
        (!displayWindow.IsEmpty()) &&
        pixelAspectRatio != 0.0f;
}

bool
CameraUtilFraming::operator==(const CameraUtilFraming& other) const
{
    return
        displayWindow == other.displayWindow &&
        dataWindow == other.dataWindow &&
        pixelAspectRatio == other.pixelAspectRatio;
}

bool
CameraUtilFraming::operator!=(const CameraUtilFraming& other) const
{
    return !(*this == other);
}

template<typename T>
static GfVec2f _ComputeCenter(const T &window)
{
    return GfVec2f(window.GetMin()) + 0.5 * GfVec2f(window.GetSize());
}

static double _SafeDiv(const double a, const double b)
{
    if (b == 0.0) {
        return 1.0;
    }
    return a / b;
}

GfMatrix4d
CameraUtilFraming::ApplyToProjectionMatrix(
    const GfMatrix4d &projectionMatrix,
    const CameraUtilConformWindowPolicy windowPolicy) const
{
    const GfVec2f &dispSize = displayWindow.GetSize();
    const GfVec2f dataSize = dataWindow.GetSize();
    const double aspect =
        pixelAspectRatio * _SafeDiv(dispSize[0], dispSize[1]);

    const GfVec2f t =
        2.0f * (_ComputeCenter(displayWindow) - _ComputeCenter(dataWindow));

    return
        // Conform frustum to display window aspect ratio.
        CameraUtilConformedWindow(projectionMatrix, windowPolicy, aspect) *

        // Transform NDC with respect to conformed frustum to space
        // where unit is two pixels.
        GfMatrix4d(GfVec4d(dispSize[0], dispSize[1], 1.0, 1.0)) *

        // Apply appropriate translation.
        // Note that the coordinate system of eye space is y-Up but
        // for the data and display window is y-Down.
        GfMatrix4d().SetTranslate(GfVec3d(t[0], -t[1], 0.0)) *

        // From pixel to NDC with respect to the data window.
        GfMatrix4d(GfVec4d(1.0 / dataSize[0], 1.0 / dataSize[1], 1.0, 1.0));
}

// Switch CameraUtilFit <-> CameraUtilCrop.
//
static
CameraUtilConformWindowPolicy
_InvertPolicy(const CameraUtilConformWindowPolicy windowPolicy)
{
    switch(windowPolicy) {
    case CameraUtilFit:
        return CameraUtilCrop;
    case CameraUtilCrop:
        return CameraUtilFit;
    default:
        return windowPolicy;
    }
}

GfRange2f
CameraUtilFraming::ComputeFilmbackWindow(
     const float cameraAspectRatio,
     const CameraUtilConformWindowPolicy windowPolicy) const
{
    return
        GfRange2f(
            CameraUtilConformedWindow(
                displayWindow,
                _InvertPolicy(windowPolicy),
                _SafeDiv(cameraAspectRatio, pixelAspectRatio)));
}

PXR_NAMESPACE_CLOSE_SCOPE


