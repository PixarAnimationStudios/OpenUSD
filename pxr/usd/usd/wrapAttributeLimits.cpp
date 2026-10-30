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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <vector>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

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
    nb::handle val)
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
    nb::handle val)
{
    return self.SetMinimum(
        UsdPythonToSdfType(val, self.GetAttribute().GetTypeName()));
}

bool
_SetMaximum(
    UsdAttributeLimits& self,
    nb::handle val)
{
    return self.SetMaximum(
        UsdPythonToSdfType(val, self.GetAttribute().GetTypeName()));
}

} // anon

void wrapUsdAttributeLimits(nb::module_ &m)
{
    TF_NB_WRAP_PUBLIC_TOKENS(
        m, "LimitsKeys", UsdLimitsKeys, USD_LIMITS_KEYS);

    using This = UsdAttributeLimits;

    nb::class_<This> clsObj(m, "AttributeLimits");

    nb::class_<This::ValidationResult>(clsObj, "ValidationResult")
        .def(nb::init<>())

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__bool__", [](const This::ValidationResult &self) { return bool(self); })

        .def_prop_ro(
            "success", &This::ValidationResult::Success)
        .def_prop_ro(
            "invalidValuesDict",
            &This::ValidationResult::GetInvalidValuesDict,
            nb::rv_policy::copy)
        .def_prop_ro(
            "conformedSubDict",
            &This::ValidationResult::GetConformedSubDict,
            nb::rv_policy::copy)

        .def("GetErrorString",
             &This::ValidationResult::GetErrorString)
        ;

    clsObj
        .def(nb::init<>())
        .def(nb::init<const UsdAttribute&, const TfToken&>(),
             nb::arg("attr"),
             nb::arg("key"))

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__bool__", [](const This &self) { return bool(self); })

        .def("IsValid", &This::IsValid)
        .def("GetAttribute", &This::GetAttribute,
             nb::rv_policy::copy)
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
             nb::arg("subDict"))
        .def("Set",
             (bool (UsdAttributeLimits::*)(const VtDictionary&))
               &This::Set,
             nb::arg("subDict"))

        .def("Get",
             (VtValue (UsdAttributeLimits::*)(const TfToken&) const)
               &This::Get,
             nb::arg("key"))
        .def("Set", _SetValue,
             nb::arg("key"),
             nb::arg("value").none())

        .def("GetMinimum",
             (VtValue (UsdAttributeLimits::*)() const)
               &This::GetMinimum)
        .def("SetMinimum", _SetMinimum,
             nb::arg("value").none())

        .def("GetMaximum",
             (VtValue (UsdAttributeLimits::*)() const)
               &This::GetMaximum)
        .def("SetMaximum", _SetMaximum,
             nb::arg("value").none())
        ;

    TfPyRegisterStlSequencesFromPython<This>();
}
