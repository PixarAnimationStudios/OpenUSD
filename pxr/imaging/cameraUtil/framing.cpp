//
// Copyright 2020 Pixar
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

PXR_NAMESPACE_CLOSE_SCOPE


