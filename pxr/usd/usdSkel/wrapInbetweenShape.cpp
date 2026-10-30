//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/inbetweenShape.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

VtVec3fArray
_GetOffsets(const UsdSkelInbetweenShape& self)
{
    VtVec3fArray points;
    self.GetOffsets(&points);
    return points;
}

VtVec3fArray
_GetNormalOffsets(const UsdSkelInbetweenShape& self)
{
    VtVec3fArray points;
    self.GetNormalOffsets(&points);
    return points;
}

nb::object
_GetWeight(const UsdSkelInbetweenShape& self)
{
    float w = 0;
    return self.GetWeight(&w) ? nb::cast(w) : nb::none();
}

bool
_SetOffsets(const UsdSkelInbetweenShape& self, nb::handle val)
{
    const VtValue vtVal =
        UsdPythonToSdfType(val, SdfValueTypeNames->Vector3fArray);
    return vtVal.IsHolding<VtVec3fArray>() ?
        self.SetOffsets(vtVal.UncheckedGet<VtVec3fArray>()) : false;
}

bool
_SetNormalOffsets(const UsdSkelInbetweenShape& self, nb::handle val)
{
    const VtValue vtVal =
        UsdPythonToSdfType(val, SdfValueTypeNames->Vector3fArray);
    return vtVal.IsHolding<VtVec3fArray>() ?
        self.SetNormalOffsets(vtVal.UncheckedGet<VtVec3fArray>()) : false;
}

UsdAttribute
_CreateNormalOffsetsAttr(const UsdSkelInbetweenShape& self,
                         nb::handle defaultValue)
{
    return self.CreateNormalOffsetsAttr(
        UsdPythonToSdfType(defaultValue, SdfValueTypeNames->Vector3fArray));
}            

} // namespace

void wrapUsdSkelInbetweenShape(nb::module_ &m)
{
    using This = UsdSkelInbetweenShape;

    nb::class_<This>(m, "InbetweenShape")

        .def(nb::init<>())
        .def(nb::init<UsdAttribute>(), nb::arg("attr"))
        .def("__bool__", [](const This& self) { return bool(self); })
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def("GetWeight", &_GetWeight)
        .def("SetWeight", &This::SetWeight, nb::arg("weight"))
        .def("HasAuthoredWeight", &This::HasAuthoredWeight)

        .def("GetOffsets", &_GetOffsets)
        .def("SetOffsets", &_SetOffsets, nb::arg("offsets"))

        .def("GetNormalOffsetsAttr", &This::GetNormalOffsetsAttr)
        .def("CreateNormalOffsetsAttr", &_CreateNormalOffsetsAttr,
             nb::arg("defaultValue").none()=nb::none())

        .def("GetNormalOffsets", &_GetNormalOffsets)
        .def("SetNormalOffsets", &_SetNormalOffsets, nb::arg("offsets"))

        .def_static("IsInbetween", &This::IsInbetween, nb::arg("attr"))

        .def("GetAttr", &This::GetAttr, nb::rv_policy::copy)
        .def("IsDefined", &This::IsDefined)
        ;

    nb::implicitly_convertible<This, UsdAttribute>();
    nb::implicitly_convertible<This, UsdProperty>();
    nb::implicitly_convertible<This, UsdObject>();
}
