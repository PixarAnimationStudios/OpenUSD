//
// Copyright 2018 Pixar
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

#include "pxr/pxr.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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
