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
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/parameter.h"
#include "pxr/usd/usdShade/connectableAPI.h"

#include "pxr/usd/usd/conversions.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/tuple.hpp>

#include <vector>

using std::vector;
using namespace boost::python;

static bool
_Set(const UsdShadeOutput &self, object val, const UsdTimeCode &time) 
{
    return self.Set(UsdPythonToSdfType(val, self.GetTypeName()), time);
}

static object
_GetConnectedSource(const UsdShadeOutput &self)
{
    UsdShadeConnectableAPI source;
    TfToken outputName;
    
    if (self.GetConnectedSource(&source, &outputName)){
        return make_tuple(source, outputName);
    }
    else {
        return object();
    }
}

void wrapUsdShadeOutput()
{
    typedef UsdShadeOutput Output;

    bool (Output::*ConnectToSource_1)(UsdShadeConnectableAPI const &,
                                   TfToken const &,
                                   bool) const = &Output::ConnectToSource;
    bool (Output::*ConnectToSource_2)(UsdShadeOutput const &) const =
                                    &Output::ConnectToSource;
    bool (Output::*ConnectToSource_3)(UsdShadeParameter const &) const =
                                    &Output::ConnectToSource;

    class_<Output>("Output")
        .def(init<UsdAttribute>(arg("attr")))
        .def(!self)

        .def("GetName", &Output::GetName,
                return_value_policy<return_by_value>())
        .def("GetOutputName", &Output::GetOutputName)
        .def("GetTypeName", &Output::GetTypeName)
        .def("Set", _Set, (arg("value"), arg("time")=UsdTimeCode::Default()))
        .def("SetRenderType", &Output::SetRenderType,
             (arg("renderType")))
        .def("GetRenderType", &Output::GetRenderType)
        .def("HasRenderType", &Output::HasRenderType)

        .def("IsConnected", &Output::IsConnected)

        .def("ConnectToSource", ConnectToSource_1,
             (arg("source"), arg("outputName"), arg("outputIsParameter")=false))
        .def("ConnectToSource", ConnectToSource_2,
             (arg("output")))
        .def("ConnectToSource", ConnectToSource_3,
             (arg("param")))

        .def("DisconnectSource", &Output::DisconnectSource)
        .def("ClearSource", &Output::ClearSource)

        .def("GetConnectedSource", _GetConnectedSource)
        .def("GetAttr", &Output::GetAttr,
                return_value_policy<return_by_value>())
        ;

    implicitly_convertible<Output, UsdAttribute>();
    to_python_converter<
        std::vector<Output>,
        TfPySequenceToPython<std::vector<Output> > >();
}

