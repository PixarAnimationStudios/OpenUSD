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
#include "pxr/usd/usdShade/interfaceAttribute.h"
#include "pxr/usd/usdShade/shader.h"

#include "pxr/usd/usd/conversions.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/implicit.hpp>

using namespace boost::python;

static bool
_Set(const UsdShadeInterfaceAttribute &self, object val, const UsdTimeCode &time) {
    return self.Set(UsdPythonToSdfType(val, self.GetAttr().GetTypeName()), time);
}

bool _SetRecipient0(
        UsdShadeInterfaceAttribute& self,
        const TfToken& renderTarget,
        const SdfPath& recipientPath)
{
    return self.SetRecipient(renderTarget, recipientPath);
}

bool _SetRecipient1(
        UsdShadeInterfaceAttribute& self,
        const TfToken& renderTarget,
        const UsdShadeParameter& recipient)
{
    return self.SetRecipient(renderTarget, recipient);
}

void wrapUsdShadeInterfaceAttribute()
{
    typedef UsdShadeInterfaceAttribute InterfaceAttribute;

    class_<InterfaceAttribute>("InterfaceAttribute")
        .def(init<UsdAttribute>(arg("attr")))
        .def(!self)

        .def("GetName", &InterfaceAttribute::GetName)
        .def("GetRecipientParameters", &InterfaceAttribute::GetRecipientParameters)

        .def("Set", _Set, (arg("value"), arg("time")=UsdTimeCode::Default()))
        .def("SetRecipient", &_SetRecipient0)
        .def("SetRecipient", &_SetRecipient1)
        .def("SetDocumentation", &InterfaceAttribute::SetDocumentation)
        .def("GetDocumentation", &InterfaceAttribute::GetDocumentation)
        .def("SetDisplayGroup", &InterfaceAttribute::SetDisplayGroup)
        .def("GetDisplayGroup", &InterfaceAttribute::GetDisplayGroup)

        .def("GetAttr", &InterfaceAttribute::GetAttr,
                return_value_policy<return_by_value>())
        ;

    implicitly_convertible<InterfaceAttribute, UsdAttribute>();
    to_python_converter<
        std::vector<InterfaceAttribute>,
        TfPySequenceToPython<std::vector<InterfaceAttribute> > >();
}

