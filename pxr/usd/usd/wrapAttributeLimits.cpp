//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usd/attributeLimits.h"
#include "pxr/usd/usd/pyConversions.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

UsdAttributeLimits::ValidationResult
_Validate(
    UsdAttributeLimits& self,
    const VtDictionary& subDict)
{
    UsdAttributeLimits::ValidationResult result;
    self.Validate(subDict, &result);
    return result;
}

bool
_SetValue(
    UsdAttributeLimits& self,
    const TfToken& key,
    const object& val)
{
    VtValue cppVal;
    return UsdPythonToMetadataValue(
             SdfFieldKeys->Limits,
             key, val, &cppVal) &&
           self.Set(key, cppVal);
}

bool
_SetMinimum(
    UsdAttributeLimits& self,
    const object& val)
{
    return self.SetMinimum(
        UsdPythonToSdfType(val, self.GetAttribute().GetTypeName()));
}

bool
_SetMaximum(
    UsdAttributeLimits& self,
    const object& val)
{
    return self.SetMaximum(
        UsdPythonToSdfType(val, self.GetAttribute().GetTypeName()));
}

} // anon

void wrapUsdAttributeLimits()
{
    TF_PY_WRAP_PUBLIC_TOKENS(
        "LimitsKeys", UsdLimitsKeys, USD_LIMITS_KEYS);

    using This = UsdAttributeLimits;

    class_<This> clsObj("AttributeLimits");
    scope s = clsObj;

    class_<This::ValidationResult>("ValidationResult")
        .def(init<>())

        .def(self == self)
        .def(self != self)
        .def(!self)

        .add_property(
            "success", &This::ValidationResult::Success)
        .add_property(
            "invalidValuesDict",
            make_function(
                &This::ValidationResult::GetInvalidValuesDict,
                return_value_policy<return_by_value>()))
        .add_property(
            "conformedSubDict",
            make_function(
                &This::ValidationResult::GetConformedSubDict,
                return_value_policy<return_by_value>()))

        .def("GetErrorString",
             &This::ValidationResult::GetErrorString)
        ;

    clsObj
        .def(init<>())
        .def(init<const UsdAttribute&, const TfToken&>(
             (arg("attr"),
              arg("key"))))

        .def(self == self)
        .def(self != self)
        .def(!self)

        .def("IsValid", &This::IsValid)
        .def("GetAttribute", &This::GetAttribute,
             return_value_policy<return_by_value>())
        .def("GetSubDictKey", &This::GetSubDictKey)

        .def("HasAuthored",
             (bool (UsdAttributeLimits::*)() const)
               &This::HasAuthored)
        .def("Clear",
             (bool (UsdAttributeLimits::*)())
               &This::Clear)

        .def("HasAuthored",
             (bool (UsdAttributeLimits::*)(const TfToken&) const)
               &This::HasAuthored)
        .def("Clear",
             (bool (UsdAttributeLimits::*)(const TfToken&))
               &This::Clear)

        .def("HasAuthoredMinimum", &This::HasAuthoredMinimum)
        .def("ClearMinimum", &This::ClearMinimum)

        .def("HasAuthoredMaximum", &This::HasAuthoredMaximum)
        .def("ClearMaximum", &This::ClearMaximum)

        .def("Validate", _Validate,
             arg("subDict"))
        .def("Set",
             (bool (UsdAttributeLimits::*)(const VtDictionary&))
               &This::Set,
             arg("subDict"))

        .def("Get",
             (VtValue (UsdAttributeLimits::*)(const TfToken&) const)
               &This::Get,
             arg("key"))
        .def("Set", _SetValue,
             (arg("key"),
              arg("value")))

        .def("GetMinimum",
             (VtValue (UsdAttributeLimits::*)() const)
               &This::GetMinimum,
             arg("key"))
        .def("SetMinimum", _SetMinimum,
             (arg("key"),
              arg("value")))

        .def("GetMaximum",
             (VtValue (UsdAttributeLimits::*)() const)
               &This::GetMaximum,
             arg("key"))
        .def("SetMaximum", _SetMaximum,
             (arg("key"),
              arg("value")))
        ;

    TfPyRegisterStlSequencesFromPython<This>();
    to_python_converter<
        std::vector<This>,
        TfPySequenceToPython<std::vector<This>>>();
}
