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

#include "pxr/pxr.h"
#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/pyEnum.h"

#include <boost/python/operators.hpp>

#include <vector>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrBaseGfWrapCamera {

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
    if (!self.GetClippingPlanes().empty())
        kwargs.push_back("clippingPlanes = " +
                         TfPyRepr(self.GetClippingPlanes()));
    if (self.GetFStop() != 0.0)
        kwargs.push_back("fStop = " + TfPyRepr(self.GetFStop()));
    if (self.GetFocusDistance() != 0.0)
        kwargs.push_back("focusDistance = " +
                         TfPyRepr(self.GetFocusDistance()));

    return prefix + TfStringJoin(kwargs, seperator.c_str()) + ")";
}

} // anonymous namespace 

void
wrapCamera()
{
    typedef GfCamera This;

    boost::python::class_<This> c("Camera");

    boost::python::scope s(c);

    TfPyWrapEnum<GfCamera::Projection>();
    TfPyWrapEnum<GfCamera::FOVDirection>();

    c   .def(boost::python::init<const This &>())
        .def(boost::python::init<const GfMatrix4d &, GfCamera::Projection,
                  float, float, float, float, float,
                  const GfRange1f &, const std::vector<GfVec4f> &,
                  float, float>
             ((boost::python::args("transform") = GfMatrix4d(1.0),
               boost::python::args("projection") = GfCamera::Perspective,
               boost::python::args("horizontalAperture") =
                                          GfCamera::DEFAULT_HORIZONTAL_APERTURE,
               boost::python::args("verticalAperture") = GfCamera::DEFAULT_VERTICAL_APERTURE,
               boost::python::args("horizontalApertureOffset") = 0.0,
               boost::python::args("verticalApertureOffset") = 0.0,
               boost::python::args("focalLength") = 50.0,
               boost::python::args("clippingRange") = GfRange1f(1, 1000000),
               boost::python::args("clippingPlanes") = std::vector<GfVec4f>(),
               boost::python::args("fStop") = 0.0,
               boost::python::args("focusDistance") = 0.0)))
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
                      &pxrBaseGfWrapCamera::_GetClippingPlanes,
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
                      &pxrBaseGfWrapCamera::_GetHorizontalFieldOfView)
        .add_property("verticalFieldOfView",
                      &pxrBaseGfWrapCamera::_GetVerticalFieldOfView)
        .def("GetFieldOfView",
                      &This::GetFieldOfView)
        .def("SetPerspectiveFromAspectRatioAndFieldOfView",
                      &This::SetPerspectiveFromAspectRatioAndFieldOfView,
             ( boost::python::arg("aspectRatio"),
               boost::python::arg("fieldOfView"),
               boost::python::arg("direction"),
               boost::python::arg("horizontalAperture") = This::DEFAULT_HORIZONTAL_APERTURE) )
        .def("SetOrthographicFromAspectRatioAndSize",
                      &This::SetOrthographicFromAspectRatioAndSize,
             ( boost::python::arg("aspectRatio"),
               boost::python::arg("orthographicSize"),
               boost::python::arg("direction")))
        .def("SetFromViewAndProjectionMatrix",
                      &This::SetFromViewAndProjectionMatrix,
             ( boost::python::arg("viewMatrix"),
               boost::python::arg("projMatrix"),
               boost::python::arg("focalLength") = 50))
        .setattr("APERTURE_UNIT",
            This::APERTURE_UNIT)
        .setattr("FOCAL_LENGTH_UNIT",
            This::FOCAL_LENGTH_UNIT)
        .setattr("DEFAULT_HORIZONTAL_APERTURE",
            This::DEFAULT_HORIZONTAL_APERTURE)
        .setattr("DEFAULT_VERTICAL_APERTURE",
            This::DEFAULT_VERTICAL_APERTURE)

        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)

        .def("__repr__", pxrBaseGfWrapCamera::_Repr)
            ;
}
