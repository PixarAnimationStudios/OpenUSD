//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/cameraUtil/framing.h"

#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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
wrapFraming(nb::module_& m)
{
    using This = CameraUtilFraming;

    nb::class_<This>(m, "Framing")
        .def(nb::init<>())
        .def(nb::init<const This &>())
        .def(nb::init<const GfRange2f&,
                      const GfRect2i&,
                      float>(),
             nb::arg("displayWindow"),
             nb::arg("dataWindow"),
             nb::arg("pixelAspectRatio") = 1.0f)
        .def(nb::init<const GfRect2i&>(),
             nb::arg("dataWindow"))
        .def("ApplyToProjectionMatrix",
             &This::ApplyToProjectionMatrix,
             nb::arg("projectionMatrix"), nb::arg("windowPolicy"))
        .def("ComputeFilmbackWindow",
             &This::ComputeFilmbackWindow,
             nb::arg("cameraAspectRatio"), nb::arg("windowPolicy"))
        .def("IsValid", &This::IsValid)
        .def_rw("displayWindow", &This::displayWindow)
        .def_rw("dataWindow", &This::dataWindow)
        .def_rw("pixelAspectRatio", &This::pixelAspectRatio)

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def("__repr__", _Repr)
    ;
}
