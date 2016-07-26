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
#include "pxr/usd/sdf/payload.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/vt/valueFromPython.h"

#include <boost/python.hpp>
#include <boost/function.hpp>

#include <string>

using namespace boost::python;
using std::string;


static string
_Repr(const SdfPayload &self)
{
    string args;
    bool useKeywordArgs = false;

    if (not self.GetAssetPath().empty()) {
        args += TfPyRepr(self.GetAssetPath());
    } else {
        useKeywordArgs = true;
    }
    if (not self.GetPrimPath().IsEmpty()) {
        args += (args.empty() ? "": ", ");
        args += (useKeywordArgs ? "primPath=" : "") +
            TfPyRepr(self.GetPrimPath());
    } else {
        useKeywordArgs = true;
    }

    return TF_PY_REPR_PREFIX + "Payload(" + args + ")";
}

void wrapPayload()
{    
    typedef SdfPayload This;

    class_<This>( "Payload" )
        .def(init<const string &,
                  const SdfPath &>(
            ( arg("assetPath") = string(),
              arg("primPath") = SdfPath() ) ) )
        .def(init<const This &>())

        .add_property("assetPath",
            make_function(
                &This::GetAssetPath, return_value_policy<return_by_value>()),
            &This::SetAssetPath)

        .add_property("primPath",
            make_function(
                &This::GetPrimPath, return_value_policy<return_by_value>()),
            &This::SetPrimPath)

        .def(self == self)
        .def(self != self)
        .def(self < self)
        .def(self > self)
        .def(self <= self)
        .def(self >= self)

        .def("__repr__", ::_Repr)

        ;

    VtValueFromPython<SdfPayload>();

    TfPyContainerConversions::from_python_sequence<
        SdfPayloadVector,
        TfPyContainerConversions::variable_capacity_policy >();
}
