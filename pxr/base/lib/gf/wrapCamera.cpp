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
///
/// \file gf/wrapCamera.h
#include <boost/python/operators.hpp>

#include "pxr/base/gf/camera.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/pyEnum.h"

#include <vector>

using namespace boost::python;

static float
_GetHorizontalFieldOfView(const GfCamera &camera) {
    return camera.GetFieldOfView(GfCamera::FOVHorizontal);
}

static float
_GetVerticalFieldOfView(const GfCamera &camera) {
    return camera.GetFieldOfView(GfCamera::FOVVertical);
}

// Required because CamCamera::GetClippingPlane returns const &std::vector
// and add_property does not allow one to specify a return_value policy.
static std::vector<GfVec4f>
_GetClippingPlanes(const GfCamera &camera) {
    return camera.GetClippingPlanes();
}

static std::string _Repr(GfCamera const &self)
{
    const std::string prefix = TF_PY_REPR_PREFIX + "Camera(";
    const std::string indent(prefix.size(), ' ');
    const std::string seperator = ",\n" + indent;
    
    // Use keyword args for clarity.
    // Only supply some arguments when different from default.
    std::vector<std::string> kwargs;
    if (self.GetTransform() != GfMatrix4d(1.0))
        kwargs.push_back("transform = " + TfPyRepr(self.GetTransform()));

    kwargs.push_back("projection = " + TfPyRepr(self.GetProjection()));
    kwargs.push_back("horizontalAperture = " +
                     TfPyRepr(self.GetHorizontalAperture()));
    kwargs.push_back("verticalAperture = " +
                     TfPyRepr(self.GetVerticalAperture()));
    if (self.GetHorizontalApertureOffset() != 0.0)
        kwargs.push_back("horizontalApertureOffset = " +
                         TfPyRepr(self.GetHorizontalApertureOffset()));
    if (self.GetVerticalApertureOffset() != 0.0)
        kwargs.push_back("verticalApertureOffset = " +
                         TfPyRepr(self.GetVerticalApertureOffset()));
    kwargs.push_back("focalLength = " +
                     TfPyRepr(self.GetFocalLength()));
    if (self.GetClippingRange() != GfRange1f(1, 1000000))
        kwargs.push_back("clippingRange = " +
                         TfPyRepr(self.GetClippingRange()));
    if (not self.GetClippingPlanes().empty())
        kwargs.push_back("clippingPlanes = " +
                         TfPyRepr(self.GetClippingPlanes()));
    if (self.GetFStop() != 0.0)
        kwargs.push_back("fStop = " + TfPyRepr(self.GetFStop()));
    if (self.GetFocusDistance() != 0.0)
        kwargs.push_back("focusDistance = " +
                         TfPyRepr(self.GetFocusDistance()));

    return prefix + TfStringJoin(kwargs, seperator.c_str()) + ")";
}

void
wrapCamera()
{
    typedef GfCamera This;

    class_<This> c("Camera");

    scope s(c);

    TfPyWrapEnum<GfCamera::Projection>();
    TfPyWrapEnum<GfCamera::FOVDirection>();

    c   .def(init<const This &>())
        .def(init<const GfMatrix4d &, GfCamera::Projection,
                  float, float, float, float, float,
                  const GfRange1f &, const std::vector<GfVec4f> &,
                  float, float>
             ((args("transform") = GfMatrix4d(1.0),
               args("projection") = GfCamera::Perspective,
               args("horizontalAperture") =
                                          GfCamera::DEFAULT_HORIZONTAL_APERTURE,
               args("verticalAperture") = GfCamera::DEFAULT_VERTICAL_APERTURE,
               args("horizontalApertureOffset") = 0.0,
               args("verticalApertureOffset") = 0.0,
               args("focalLength") = 50.0,
               args("clippingRange") = GfRange1f(1, 1000000),
               args("clippingPlanes") = std::vector<GfVec4f>(),
               args("fStop") = 0.0,
               args("focusDistance") = 0.0)))
        .add_property("transform",
                      &This::GetTransform,
                      &This::SetTransform)
        .add_property("projection",
                      &This::GetProjection,
                      &This::SetProjection)
        .add_property("horizontalAperture",
                      &This::GetHorizontalAperture,
                      &This::SetHorizontalAperture)
        .add_property("verticalAperture",
                      &This::GetVerticalAperture,
                      &This::SetVerticalAperture)
        .add_property("horizontalApertureOffset",
                      &This::GetHorizontalApertureOffset,
                      &This::SetHorizontalApertureOffset)
        .add_property("verticalApertureOffset",
                      &This::GetVerticalApertureOffset,
                      &This::SetVerticalApertureOffset)
        .add_property("aspectRatio",
                      &This::GetAspectRatio)
        .add_property("focalLength",
                      &This::GetFocalLength,
                      &This::SetFocalLength)
        .add_property("clippingRange",
                      &This::GetClippingRange,
                      &This::SetClippingRange)
        .add_property("clippingPlanes",
                      &_GetClippingPlanes,
                      &This::SetClippingPlanes)
        .add_property("frustum",
                      &This::GetFrustum)
        .add_property("fStop",
                      &This::GetFStop,
                      &This::SetFStop)
        .add_property("focusDistance",
                      &This::GetFocusDistance,
                      &This::SetFocusDistance)
        .add_property("horizontalFieldOfView",
                      &_GetHorizontalFieldOfView)
        .add_property("verticalFieldOfView",
                      &_GetVerticalFieldOfView)
        .def("GetFieldOfView",
                      &This::GetFieldOfView)
        .def("SetPerspectiveFromAspectRatioAndFieldOfView",
                      &This::SetPerspectiveFromAspectRatioAndFieldOfView,
             ( arg("aspectRatio"),
               arg("fieldOfView"),
               arg("direction"),
               arg("horizontalAperture") = This::DEFAULT_HORIZONTAL_APERTURE) )
        .def("SetOrthographicFromAspectRatioAndSize",
                      &This::SetOrthographicFromAspectRatioAndSize,
             ( arg("aspectRatio"),
               arg("orthographicSize"),
               arg("direction")))

        .setattr("ZUp",
            This::ZUp)
        .setattr("YUp",
            This::YUp)

        .setattr("APERTURE_UNIT",
            This::APERTURE_UNIT)
        .setattr("FOCAL_LENGTH_UNIT",
            This::FOCAL_LENGTH_UNIT)
        .setattr("DEFAULT_HORIZONTAL_APERTURE",
            This::DEFAULT_HORIZONTAL_APERTURE)
        .setattr("DEFAULT_VERTICAL_APERTURE",
            This::DEFAULT_VERTICAL_APERTURE)

        .def(self == self)
        .def(self != self)

        .def("__repr__", _Repr)
            ;
}
