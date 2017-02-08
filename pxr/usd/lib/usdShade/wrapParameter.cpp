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
#include "pxr/pxr.h"
#include "pxr/usd/usdShade/parameter.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/connectableAPI.h"

#include "pxr/usd/usd/conversions.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/tuple.hpp>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


using std::vector;
using namespace boost::python;

static bool
_Set(const UsdShadeParameter &self, object val, const UsdTimeCode &time) {
    return self.Set(UsdPythonToSdfType(val, self.GetAttr().GetTypeName()), time);
}

static object
_GetConnectedSource(const UsdShadeParameter &self)
{
    UsdShadeConnectableAPI source;
    TfToken                sourceName;
    UsdShadeAttributeType  sourceType;

    if (self.GetConnectedSource(&source, &sourceName, &sourceType)){
        return make_tuple(source, sourceName, sourceType);
    }
    else {
        return object();
    }
}

void wrapUsdShadeParameter()
{
    typedef UsdShadeParameter Parameter;

    bool (Parameter::*ConnectToSource_1)(UsdShadeConnectableAPI const &,
                                   TfToken const &,
                                   UsdShadeAttributeType const) const = &Parameter::ConnectToSource;
    bool (Parameter::*ConnectToSource_2)(UsdShadeOutput const &) const =
                                    &Parameter::ConnectToSource;
    bool (Parameter::*ConnectToSource_3)(UsdShadeParameter const &) const =
                                    &Parameter::ConnectToSource;
    bool (Parameter::*ConnectToSource_4)(UsdShadeInterfaceAttribute const &) const =
                                    &Parameter::ConnectToSource;
    bool (Parameter::*ConnectToSource_5)(SdfPath const &) const =
                                    &Parameter::ConnectToSource;

    class_<Parameter>("Parameter")
        .def(init<UsdAttribute>(arg("attr")))
        .def(!self)

        .def("GetName", &Parameter::GetName)
        .def("GetTypeName", &Parameter::GetTypeName)
        .def("Set", _Set, (arg("value"), arg("time")=UsdTimeCode::Default()))
        .def("SetRenderType", &Parameter::SetRenderType,
             (arg("renderType")))
        .def("GetRenderType", &Parameter::GetRenderType)
        .def("HasRenderType", &Parameter::HasRenderType)

        .def("IsConnected", &Parameter::IsConnected)

        .def("ConnectToSource", ConnectToSource_1,
             (arg("source"), arg("sourceName"), 
              arg("sourceType")=UsdShadeAttributeType::Output))
        .def("ConnectToSource", ConnectToSource_2,
             (arg("output")))
        .def("ConnectToSource", ConnectToSource_3,
             (arg("param")))
        .def("ConnectToSource", ConnectToSource_4,
             (arg("interfaceAttribute")))
        .def("ConnectToSource", ConnectToSource_5,
             (arg("path")))

        .def("DisconnectSource", &Parameter::DisconnectSource)
        .def("ClearSource", &Parameter::ClearSource)

        .def("GetConnectedSource", _GetConnectedSource)
        .def("GetAttr", &Parameter::GetAttr,
                return_value_policy<return_by_value>())
        ;

    implicitly_convertible<Parameter, UsdAttribute>();
    to_python_converter<
        std::vector<Parameter>,
        TfPySequenceToPython<std::vector<Parameter> > >();
}


PXR_NAMESPACE_CLOSE_SCOPE

