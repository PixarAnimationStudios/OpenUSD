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
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/editTarget.h"
#include "pxr/usd/usd/wrapUtils.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>



PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdProperty()
{
    boost::python::class_<UsdProperty, boost::python::bases<UsdObject> >("Property")
        .def(Usd_ObjectSubclass())
        .def("GetBaseName", &UsdProperty::GetBaseName)
        .def("GetNamespace", &UsdProperty::GetNamespace)
        .def("SplitName", &UsdProperty::SplitName,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetDisplayGroup", &UsdProperty::GetDisplayGroup)
        .def("SetDisplayGroup", &UsdProperty::SetDisplayGroup,
             boost::python::arg("displayGroup"))
        .def("ClearDisplayGroup", &UsdProperty::ClearDisplayGroup)
        .def("HasAuthoredDisplayGroup", &UsdProperty::HasAuthoredDisplayGroup)

        .def("GetNestedDisplayGroups", &UsdProperty::GetNestedDisplayGroups,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("SetNestedDisplayGroups", &UsdProperty::SetNestedDisplayGroups,
             boost::python::arg("nestedGroups"))

        .def("GetDisplayName", &UsdProperty::GetDisplayName)
        .def("SetDisplayName", &UsdProperty::SetDisplayName, boost::python::arg("name"))
        .def("ClearDisplayName", &UsdProperty::ClearDisplayName)
        .def("HasAuthoredDisplayName", &UsdProperty::HasAuthoredDisplayName)

        .def("GetPropertyStack", &UsdProperty::GetPropertyStack,
             boost::python::arg("time"))

        .def("IsCustom", &UsdProperty::IsCustom)
        .def("SetCustom", &UsdProperty::SetCustom, boost::python::arg("isCustom"))

        .def("IsDefined", &UsdProperty::IsDefined)
        .def("IsAuthored", &UsdProperty::IsAuthored)
        .def("IsAuthoredAt", &UsdProperty::IsAuthoredAt, boost::python::arg("editTarget"))

        .def("FlattenTo", 
             (UsdProperty (UsdProperty::*)(const UsdPrim&) const)
                 &UsdProperty::FlattenTo,
             (boost::python::arg("parent")))
        .def("FlattenTo", 
             (UsdProperty (UsdProperty::*)(const UsdPrim&,const TfToken&) const)
                 &UsdProperty::FlattenTo,
             (boost::python::arg("parent"), boost::python::arg("propName")))
        .def("FlattenTo", 
             (UsdProperty (UsdProperty::*)(const UsdProperty&) const)
                 &UsdProperty::FlattenTo,
             (boost::python::arg("property")))
        ;

    TfPyRegisterStlSequencesFromPython<UsdProperty>();
}
