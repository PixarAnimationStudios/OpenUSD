//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/editTarget.h"
#include "pxr/usd/usd/wrapUtils.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdProperty()
{
    class_<UsdProperty, bases<UsdObject> >("Property")
        .def(Usd_ObjectSubclass())
        .def("GetBaseName", &UsdProperty::GetBaseName)
        .def("GetNamespace", &UsdProperty::GetNamespace)
        .def("SplitName", &UsdProperty::SplitName,
             return_value_policy<TfPySequenceToList>())

        .def("GetDisplayGroup", &UsdProperty::GetDisplayGroup)
        .def("SetDisplayGroup", &UsdProperty::SetDisplayGroup,
             arg("displayGroup"))
        .def("ClearDisplayGroup", &UsdProperty::ClearDisplayGroup)
        .def("HasAuthoredDisplayGroup", &UsdProperty::HasAuthoredDisplayGroup)

        .def("GetNestedDisplayGroups", &UsdProperty::GetNestedDisplayGroups,
             return_value_policy<TfPySequenceToList>())

        .def("SetNestedDisplayGroups", &UsdProperty::SetNestedDisplayGroups,
             arg("nestedGroups"))

        .def("GetPropertyStack", &UsdProperty::GetPropertyStack,
             arg("time")=UsdTimeCode::Default())
        .def("GetPropertyStackWithLayerOffsets", 
             &UsdProperty::GetPropertyStackWithLayerOffsets,
             arg("time")=UsdTimeCode::Default(),
             return_value_policy<TfPySequenceToList>())

        .def("IsCustom", &UsdProperty::IsCustom)
        .def("SetCustom", &UsdProperty::SetCustom, arg("isCustom"))

        .def("IsDefined", &UsdProperty::IsDefined)
        .def("IsAuthored", &UsdProperty::IsAuthored)
        .def("IsAuthoredAt", &UsdProperty::IsAuthoredAt, arg("editTarget"))

        .def("FlattenTo", 
             (UsdProperty (UsdProperty::*)(const UsdPrim&) const)
                 &UsdProperty::FlattenTo,
             (arg("parent")))
        .def("FlattenTo", 
             (UsdProperty (UsdProperty::*)(const UsdPrim&,const TfToken&) const)
                 &UsdProperty::FlattenTo,
             (arg("parent"), arg("propName")))
        .def("FlattenTo", 
             (UsdProperty (UsdProperty::*)(const UsdProperty&) const)
                 &UsdProperty::FlattenTo,
             (arg("property")))
        ;

    TfPyRegisterStlSequencesFromPython<UsdProperty>();
    TfPyContainerConversions::tuple_mapping_pair<
        std::pair<SdfPropertySpecHandle, SdfLayerOffset>>();
}
