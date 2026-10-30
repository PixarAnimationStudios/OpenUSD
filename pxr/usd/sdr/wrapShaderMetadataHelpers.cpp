//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

// Python has no equivalent notion of namespaces in C++, so
// use an empty class as a namespace standin for Python.
class _MetadataHelpers{};

static nb::tuple
_ParseSdfValue(const std::string& valueStr,
               SdrShaderPropertyConstPtr property) {
    std::string err;
    VtValue value = ShaderMetadataHelpers::ParseSdfValue(valueStr, property,
                                                         &err);
    return nb::make_tuple(value, err);
}

} // anonymous namespace

void wrapShaderMetadataHelpers(nanobind::module_ &m)
{
    nb::class_<_MetadataHelpers>(m, "MetadataHelpers")
        .def(nb::init<>())
        .def_static("ParseSdfValue", _ParseSdfValue,
                    nb::arg("valueStr"),
                    nb::arg("property").none() = nullptr);
}
