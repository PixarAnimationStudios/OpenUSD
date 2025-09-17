//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_CAMERA_UTIL_CONFORM_WINDOW_H
#define PXR_IMAGING_CAMERA_UTIL_CONFORM_WINDOW_H

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
    CameraUtilCrop,
    /// Leave unchanged (This can result in stretching/shrinking if not pre-fit)
    CameraUtilDontConform
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
/// by applying \p policy.
///
/// Note that this function also supports mirroring about the x- or y-axis of
/// the image corresponding to flipping all signs in the second, respectively,
/// third column of the projection matrix. In other words, we get the same
/// result whether we flip the signs in the matrix and then give it to this
/// function or call this function first and flip the signs of the resulting
/// matrix.
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
