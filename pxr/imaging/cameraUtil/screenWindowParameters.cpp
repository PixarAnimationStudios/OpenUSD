//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/cameraUtil/screenWindowParameters.h"

PXR_NAMESPACE_OPEN_SCOPE


CameraUtilScreenWindowParameters::CameraUtilScreenWindowParameters(
    const GfCamera &camera, GfCamera::FOVDirection fitDirection)
    : _screenWindow(
        - camera.GetHorizontalAperture()
                + 2 * camera.GetHorizontalApertureOffset(),
        + camera.GetHorizontalAperture()
                + 2 * camera.GetHorizontalApertureOffset(),
        - camera.GetVerticalAperture()
                + 2 * camera.GetVerticalApertureOffset(),
        + camera.GetVerticalAperture()
                + 2 * camera.GetVerticalApertureOffset()),
      _fieldOfView(camera.GetFieldOfView(fitDirection))
{
    if (camera.GetProjection() == GfCamera::Perspective) {
        const double denom =
            (fitDirection == GfCamera::FOVHorizontal) ?
               camera.GetHorizontalAperture() : camera.GetVerticalAperture();
        if (denom != 0.0) {
            _screenWindow /= denom;
        }
    } else {
        _screenWindow *= GfCamera::APERTURE_UNIT / 2.0;
    }

    static const GfMatrix4d zFlip(GfVec4d(1,1,-1,1));
    _zFacingViewMatrix = (zFlip * camera.GetTransform()).GetInverse();
}

PXR_NAMESPACE_CLOSE_SCOPE

