//
// Copyright 2020 Pixar
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
#include "pxr/imaging/cameraUtil/framing.h"

#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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
        .def("IsValid", &This::IsValid)
        .def_readwrite("displayWindow", &This::displayWindow)
        .def_readwrite("dataWindow", &This::dataWindow)
        .def_readwrite("pixelAspectRatio", &This::pixelAspectRatio)

        .def(self == self)
        .def(self != self)

        .def("__repr__", _Repr)
    ;
}
