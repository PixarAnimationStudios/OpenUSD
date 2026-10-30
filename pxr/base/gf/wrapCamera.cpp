//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file gf/wrapCamera.h

#include "pxr/pxr.h"
#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/vector.h>

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

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
wrapCamera(nanobind::module_ &m)
{
    typedef GfCamera This;

    nanobind::class_<This> c(m, "Camera");

    TfNbWrapEnum<GfCamera::Projection>(c);
    TfNbWrapEnum<GfCamera::FOVDirection>(c);

    c   .def(nanobind::init<>())
        .def(nanobind::init<const This &>())
        .def(TfTypePythonClass())
        .def(nanobind::init<const GfMatrix4d &, GfCamera::Projection,
                  float, float, float, float, float,
                  const GfRange1f &, const std::vector<GfVec4f> &,
                  float, float>(),
             nanobind::arg("transform") = GfMatrix4d(1.0),
             nanobind::arg("projection") = GfCamera::Perspective,
             nanobind::arg("horizontalAperture") =
                                        GfCamera::DEFAULT_HORIZONTAL_APERTURE,
             nanobind::arg("verticalAperture") = GfCamera::DEFAULT_VERTICAL_APERTURE,
             nanobind::arg("horizontalApertureOffset") = 0.0,
             nanobind::arg("verticalApertureOffset") = 0.0,
             nanobind::arg("focalLength") = 50.0,
             nanobind::arg("clippingRange") = GfRange1f(1, 1000000),
             nanobind::arg("clippingPlanes") = std::vector<GfVec4f>(),
             nanobind::arg("fStop") = 0.0,
             nanobind::arg("focusDistance") = 0.0)
        .def_prop_rw("transform",
                      &This::GetTransform,
                      &This::SetTransform)
        .def_prop_rw("projection",
                      &This::GetProjection,
                      &This::SetProjection)
        .def_prop_rw("horizontalAperture",
                      &This::GetHorizontalAperture,
                      &This::SetHorizontalAperture)
        .def_prop_rw("verticalAperture",
                      &This::GetVerticalAperture,
                      &This::SetVerticalAperture)
        .def_prop_rw("horizontalApertureOffset",
                      &This::GetHorizontalApertureOffset,
                      &This::SetHorizontalApertureOffset)
        .def_prop_rw("verticalApertureOffset",
                      &This::GetVerticalApertureOffset,
                      &This::SetVerticalApertureOffset)
        .def_prop_ro("aspectRatio",
                      &This::GetAspectRatio)
        .def_prop_rw("focalLength",
                      &This::GetFocalLength,
                      &This::SetFocalLength)
        .def_prop_rw("clippingRange",
                      &This::GetClippingRange,
                      &This::SetClippingRange)
        .def_prop_rw("clippingPlanes",
                      &_GetClippingPlanes,
                      &This::SetClippingPlanes)
        .def_prop_ro("frustum",
                      &This::GetFrustum)
        .def_prop_rw("fStop",
                      &This::GetFStop,
                      &This::SetFStop)
        .def_prop_rw("focusDistance",
                      &This::GetFocusDistance,
                      &This::SetFocusDistance)
        .def_prop_ro("horizontalFieldOfView",
                      &_GetHorizontalFieldOfView)
        .def_prop_ro("verticalFieldOfView",
                      &_GetVerticalFieldOfView)
        .def("GetFieldOfView",
                      &This::GetFieldOfView)
        .def("SetPerspectiveFromAspectRatioAndFieldOfView",
                      &This::SetPerspectiveFromAspectRatioAndFieldOfView,
              nanobind::arg("aspectRatio"),
              nanobind::arg("fieldOfView"),
              nanobind::arg("direction"),
              nanobind::arg("horizontalAperture") = This::DEFAULT_HORIZONTAL_APERTURE)
        .def("SetOrthographicFromAspectRatioAndSize",
                      &This::SetOrthographicFromAspectRatioAndSize,
              nanobind::arg("aspectRatio"),
              nanobind::arg("orthographicSize"),
              nanobind::arg("direction"))
        .def("SetFromViewAndProjectionMatrix",
                      &This::SetFromViewAndProjectionMatrix,
              nanobind::arg("viewMatrix"),
              nanobind::arg("projMatrix"),
              nanobind::arg("focalLength") = 50)
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)

        .def("__repr__", _Repr)
            ;

    c.attr("APERTURE_UNIT") = This::APERTURE_UNIT;
    c.attr("FOCAL_LENGTH_UNIT") = This::FOCAL_LENGTH_UNIT;
    c.attr("DEFAULT_HORIZONTAL_APERTURE") = This::DEFAULT_HORIZONTAL_APERTURE;
    c.attr("DEFAULT_VERTICAL_APERTURE") = This::DEFAULT_VERTICAL_APERTURE;
}
