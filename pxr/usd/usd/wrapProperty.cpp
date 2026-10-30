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
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

using std::string;

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdProperty(nb::module_ &m)
{
    nb::class_<UsdProperty, UsdObject>(m, "Property")
        .def(nb::init<>())
        .def(Usd_ObjectSubclass())
        .def("GetBaseName", &UsdProperty::GetBaseName)
        .def("GetNamespace", &UsdProperty::GetNamespace)
        .def("SplitName", &UsdProperty::SplitName)

        .def("GetDisplayGroup", &UsdProperty::GetDisplayGroup)
        .def("SetDisplayGroup", &UsdProperty::SetDisplayGroup,
             nb::arg("displayGroup"))
        .def("ClearDisplayGroup", &UsdProperty::ClearDisplayGroup)
        .def("HasAuthoredDisplayGroup", &UsdProperty::HasAuthoredDisplayGroup)

        .def("GetNestedDisplayGroups", &UsdProperty::GetNestedDisplayGroups)

        .def("SetNestedDisplayGroups", &UsdProperty::SetNestedDisplayGroups,
             nb::arg("nestedGroups"))

        .def("GetPropertyStack", &UsdProperty::GetPropertyStack,
             nb::arg("time")=UsdTimeCode::Default())
        .def("GetPropertyStackWithLayerOffsets", 
             &UsdProperty::GetPropertyStackWithLayerOffsets,
             nb::arg("time")=UsdTimeCode::Default())

        .def("IsCustom", &UsdProperty::IsCustom)
        .def("SetCustom", &UsdProperty::SetCustom, nb::arg("isCustom"))

        .def("IsDefined", &UsdProperty::IsDefined)
        .def("IsAuthored", &UsdProperty::IsAuthored)
        .def("IsAuthoredAt", &UsdProperty::IsAuthoredAt, nb::arg("editTarget"))

        .def("FlattenTo", 
             (UsdProperty (UsdProperty::*)(const UsdPrim&) const)
                 &UsdProperty::FlattenTo,
             nb::arg("parent"))
        .def("FlattenTo", 
             (UsdProperty (UsdProperty::*)(const UsdPrim&, const TfToken&) const)
                 &UsdProperty::FlattenTo,
             nb::arg("parent"), nb::arg("propName"))
        .def("FlattenTo", 
             (UsdProperty (UsdProperty::*)(const UsdProperty&) const)
                 &UsdProperty::FlattenTo,
             nb::arg("property"))
        ;

    TfPyRegisterStlSequencesFromPython<UsdProperty>();
    Tf_PyRegisterCustomToPythonConverter(
        typeid(UsdProperty),
        +[](void const *p) -> PyObject * {
            return Usd_ObjectSubclassConvertToPython(
                static_cast<UsdProperty const *>(p));
        });
    TfPyContainerConversions::tuple_mapping_pair<
        std::pair<SdfPropertySpecHandle, SdfLayerOffset>>();
}
