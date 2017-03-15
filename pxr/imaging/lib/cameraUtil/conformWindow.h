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
#ifndef PXR_CAMERAUTIL_CONFORM_WINDOW_H
#define PXR_CAMERAUTIL_CONFORM_WINDOW_H

#include "pxr/pxr.h"
#include "pxr/imaging/cameraUtil/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class GfVec2d;
class GfVec4d;
class GfMatrix4d;
class GfRange2d;
class GfCamera;
class GfFrustum;

/// \enum CameraUtilConformWindowPolicy
///
/// Policy of how to conform a window to the given aspect ratio.
/// An ASCII-art explanation is given in the corresponding .cpp file.
/// 
enum CameraUtilConformWindowPolicy {
    /// Modify width
    CameraUtilMatchVertically,
    /// Modify height
    CameraUtilMatchHorizontally,
    /// Increase width or height
    CameraUtilFit,
    /// Decrease width or height
    CameraUtilCrop
};

/// Returns a window with aspect ratio \p targetAspect by applying
/// \p policy to \p window where \p window is encoded as GfRange2d.
CAMERAUTIL_API
GfRange2d
CameraUtilConformedWindow(
    const GfRange2d &window,
    CameraUtilConformWindowPolicy policy, double targetAspect);
    
/// Returns a window with aspect ratio \p targetAspect by applying
/// \p policy to \p window where \p window is encoded as vector
/// (left, right, bottom, top) similarly to RenderMan's RiScreenWindow.
CAMERAUTIL_API
GfVec4d 
CameraUtilConformedWindow(
    const GfVec4d &window,
    CameraUtilConformWindowPolicy policy, double targetAspect);

/// Returns a window with aspect ratio \p targetAspect by applying
/// \p policy to \p window where \p window is encoded as vector
/// (width, height).
CAMERAUTIL_API
GfVec2d
CameraUtilConformedWindow(
    const GfVec2d &window,
    CameraUtilConformWindowPolicy policy, double targetAspect);

/// Conforms the given \p projectionMatrix to have aspect ratio \p targetAspect
/// by applying \p policy
CAMERAUTIL_API
GfMatrix4d
CameraUtilConformedWindow(
    const GfMatrix4d &projectionMatrix,
    CameraUtilConformWindowPolicy policy, double targetAspect);

/// Conforms the given \p camera to have aspect ratio \p targetAspect
/// by applying \p policy.
CAMERAUTIL_API
void
CameraUtilConformWindow(
    GfCamera *camera,
    CameraUtilConformWindowPolicy policy, double targetAspect);

/// Conforms the given \p frustum to have aspect ratio \p targetAspect
/// by applying \p policy.
CAMERAUTIL_API
void
CameraUtilConformWindow(
    GfFrustum *frustum,
    CameraUtilConformWindowPolicy policy, double targetAspect);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
