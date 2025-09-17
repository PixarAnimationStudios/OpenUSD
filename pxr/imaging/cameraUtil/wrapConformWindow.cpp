//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file camera/wrapConformWindow.h

#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/range2d.h"
#include "pxr/base/gf/frustum.h"

#include "pxr/base/tf/pyEnum.h"

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void
wrapConformWindow()
{
    typedef GfRange2d (*Signature1)(
        const GfRange2d &, CameraUtilConformWindowPolicy, double);

    typedef GfVec2d (*Signature2)(
        const GfVec2d &, CameraUtilConformWindowPolicy, double);

    typedef GfVec4d (*Signature3)(
        const GfVec4d &, CameraUtilConformWindowPolicy, double);

    typedef GfMatrix4d (*Signature4)(
        const GfMatrix4d &, CameraUtilConformWindowPolicy, double);

    typedef void (*Signature5)(
        GfCamera *, CameraUtilConformWindowPolicy, double);

    typedef void (*Signature6)(
        GfFrustum *, CameraUtilConformWindowPolicy, double);

    def("ConformedWindow", (Signature1)&CameraUtilConformedWindow,
        (arg("window"), arg("policy"), arg("targetAspect")));
    def("ConformedWindow", (Signature2)&CameraUtilConformedWindow,
        (arg("window"), arg("policy"), arg("targetAspect")));
    def("ConformedWindow", (Signature3)&CameraUtilConformedWindow,
        (arg("window"), arg("policy"), arg("targetAspect")));
    def("ConformedWindow", (Signature4)&CameraUtilConformedWindow,
        (arg("window"), arg("policy"), arg("targetAspect")));
    
    def("ConformWindow", (Signature5)&CameraUtilConformWindow,
        (arg("camera"), arg("policy"), arg("targetAspect")));

    def("ConformWindow", (Signature6)&CameraUtilConformWindow,
        (arg("frustum"), arg("policy"), arg("targetAspect")));

    TfPyWrapEnum<CameraUtilConformWindowPolicy>();
}
