//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/constraintTarget.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>

namespace nb = nanobind;
PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// The python wrapping of ComputeInWorldSpace does not take an xform cache.
static GfMatrix4d
_ComputeInWorldSpace(
    const UsdGeomConstraintTarget &self,
    const UsdTimeCode &time = UsdTimeCode::Default())
{
    return self.ComputeInWorldSpace(time);
}

} // anonymous namespace 

void wrapUsdGeomConstraintTarget(nanobind::module_ &m)
{
    typedef UsdGeomConstraintTarget ConstraintTarget;

    nb::class_<ConstraintTarget>(m, "ConstraintTarget")
        .def(nb::init<>())
        .def(nb::init<const UsdAttribute &>(), nb::arg("attr"))
        .def("__bool__", [](const ConstraintTarget &self) { return bool(self); })

        .def("GetAttr", &ConstraintTarget::GetAttr,
             nb::rv_policy::copy)
        .def("IsDefined", &ConstraintTarget::IsDefined)

        .def("IsValid", [](const ConstraintTarget &self) {
            return ConstraintTarget::IsValid(self);
        })
        .def("IsValid", &ConstraintTarget::IsValid)
        .def("IsValid", [](const ConstraintTarget &, const UsdAttribute &attr) {
            return ConstraintTarget::IsValid(attr);
        }, nb::arg("attr"))

        .def("SetIdentifier", &ConstraintTarget::SetIdentifier, nb::arg("identifier"))
        .def("GetIdentifier", &ConstraintTarget::GetIdentifier,
             nb::rv_policy::copy)

        .def("Get", &ConstraintTarget::Get,
             nb::arg("value"), nb::arg("time") = UsdTimeCode::Default())
        .def("Get", [](const ConstraintTarget &self, UsdTimeCode time) {
            GfMatrix4d value;
            self.Get(&value, time);
            return value;
        }, nb::arg("time") = UsdTimeCode::Default())
        .def("Set", &ConstraintTarget::Set,
             nb::arg("value"), nb::arg("time") = UsdTimeCode::Default())

        .def_static("GetConstraintAttrName", &ConstraintTarget::GetConstraintAttrName)

        .def("ComputeInWorldSpace", _ComputeInWorldSpace,
             nb::arg("time") = UsdTimeCode::Default())
        ;

    nb::implicitly_convertible<ConstraintTarget, UsdAttribute>();
    nb::implicitly_convertible<ConstraintTarget, UsdProperty>();
    nb::implicitly_convertible<ConstraintTarget, UsdObject>();
}
