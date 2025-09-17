//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_CAMERA_UTIL_SCREEN_WINDOW_PARAMETERS_H
#define PXR_IMAGING_CAMERA_UTIL_SCREEN_WINDOW_PARAMETERS_H

#include "pxr/pxr.h"
#include "pxr/imaging/cameraUtil/api.h"
#include "pxr/base/gf/camera.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class CameraUtilScreenWindowParameters
///
/// Given a camera object, compute parameters suitable for setting up
/// RenderMan.
///
class CameraUtilScreenWindowParameters
{
public:
    /// Constructs screenwindow parameter. The optional \p fitDirection
    /// indicates in which direction the screenwindow will have length 2.
    CAMERAUTIL_API
    CameraUtilScreenWindowParameters(const GfCamera &camera,
                                     GfCamera::FOVDirection fitDirection =
                                     GfCamera::FOVHorizontal);
    
    /// The vector (left, right, bottom, top) defining the rectangle in the
    /// image plane.
    /// Give these parameters to RiScreenWindow.
    const GfVec4d & GetScreenWindow() const { return _screenWindow; }
    
    /// The field of view. More precisely, the full angle perspective field
    /// of view (in degrees) between screen space coordinates (-1,0) and
    /// (1,0).
    /// Give these parameters to RiProjection as parameter after
    /// "perspective".
    double GetFieldOfView() const { return _fieldOfView; }
    
    /// Returns the inverse of the transform for a camera that is y-Up
    /// and z-facing (vs the OpenGL camera that is (-z)-facing).
    /// Write this transform with RiConcatTransform before
    /// RiWorldBegin.
    const GfMatrix4d & GetZFacingViewMatrix() const {
        return _zFacingViewMatrix;
    }
    
private:
    GfVec4d _screenWindow;
    double _fieldOfView;
    GfMatrix4d _zFacingViewMatrix;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_CAMERA_UTIL_SCREEN_WINDOW_PARAMETERS_H
