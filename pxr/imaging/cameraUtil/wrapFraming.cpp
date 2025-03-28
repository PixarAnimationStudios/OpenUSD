//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/cameraUtil/framing.h"

#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

std::string _Repr(const CameraUtilFraming &self)
{
    static const std::string prefix =
        TF_PY_REPR_PREFIX + "Framing(";
    static const std::string separator =
        ",\n" + std::string(prefix.size(), ' ');

    std::vector<std::string> kwargs;
    kwargs.push_back("displayWindow = " + TfPyRepr(self.displayWindow));
    kwargs.push_back("dataWindow = " + TfPyRepr(self.dataWindow));
    if (self.pixelAspectRatio != 1.0f) {
        kwargs.push_back(
            "pixelAspectRatio = " + TfPyRepr(self.pixelAspectRatio));
    }

    return prefix + TfStringJoin(kwargs, separator.c_str()) + ")";
}

}

void
wrapFraming()
{
    using This = CameraUtilFraming;

    class_<This>("Framing")
        .def(init<>())
        .def(init<const This &>())
        .def(init<const GfRange2f&,
                  const GfRect2i&,
                  float>(
                      (args("displayWindow"),
                       args("dataWindow"),
                       args("pixelAspectRatio") = 1.0)))
        .def(init<const GfRect2i>(
                      ((args("dataWindow")))))
        .def("ApplyToProjectionMatrix",
             &This::ApplyToProjectionMatrix,
             ((args("projectionMatrix"), args("windowPolicy"))))
        .def("ComputeFilmbackWindow",
             &This::ComputeFilmbackWindow,
             ((args("cameraAspectRatio"), args("windowPolicy"))))
        .def("IsValid", &This::IsValid)
        .def_readwrite("displayWindow", &This::displayWindow)
        .def_readwrite("dataWindow", &This::dataWindow)
        .def_readwrite("pixelAspectRatio", &This::pixelAspectRatio)

        .def(self == self)
        .def(self != self)

        .def("__repr__", _Repr)
    ;
}
