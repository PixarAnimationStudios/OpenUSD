//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static
std::string
_Repr(const NdrVersion& x)
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
wrapVersion()
{
    typedef NdrVersion This;

    class_<This>("Version", no_init)
        .def(init<>())
        .def(init<int>())
        .def(init<int, int>())
        .def(init<std::string>())
        .def("GetMajor", &This::GetMajor)
        .def("GetMinor", &This::GetMinor)
        .def("IsDefault", &This::IsDefault)
        .def("GetAsDefault", &This::GetAsDefault)
        .def("GetStringSuffix", &This::GetStringSuffix)
        .def("__repr__", _Repr)
        .def("__str__", &This::GetString)
        .def("__hash__", &This::GetHash)
        .def(!self)
        .def(self == self)
        .def(self != self)
        .def(self < self)
        .def(self <= self)
        .def(self > self)
        .def(self >= self)
        ;
}

}

void wrapDeclare()
{
    wrapVersion();

    TfPyWrapEnum<NdrVersionFilter>();
}
