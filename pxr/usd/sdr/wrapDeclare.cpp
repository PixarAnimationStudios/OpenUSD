//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

static
std::string
_Repr(const SdrVersion& x)
{
    auto result = TF_PY_REPR_PREFIX;
    if (!x) {
        result += "Version()";
    }
    else {
        result +=
            TfStringPrintf("Version(%s, %s)",
                           TfPyRepr(x.GetMajor()).c_str(),
                           TfPyRepr(x.GetMinor()).c_str());
    }
    if (x.IsDefault()) {
        result += ".GetAsDefault()";
    }
    return result;
}

static
void
wrapVersion(nanobind::module_ &m)
{
    typedef SdrVersion This;

    nb::class_<This>(m, "Version")
        .def(nb::init<>())
        .def(nb::init<int>())
        .def(nb::init<int, int>())
        .def(nb::init<std::string>())
        .def("GetMajor", &This::GetMajor)
        .def("GetMinor", &This::GetMinor)
        .def("IsDefault", &This::IsDefault)
        .def("GetAsDefault", &This::GetAsDefault)
        .def("GetStringSuffix", &This::GetStringSuffix)
        .def("__repr__", _Repr)
        .def("__str__", &This::GetString)
        .def("__hash__", &This::GetHash)
        .def("__bool__", [](const This& self) { return !(!self); })
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(nb::self < nb::self)
        .def(nb::self <= nb::self)
        .def(nb::self > nb::self)
        .def(nb::self >= nb::self)
        ;

}

}

void wrapDeclare(nanobind::module_ &m)
{
    wrapVersion(m);

    TfNbWrapEnum<SdrVersionFilter>(m);
}
