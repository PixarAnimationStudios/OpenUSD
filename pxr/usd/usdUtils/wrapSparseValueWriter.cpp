//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

#include "pxr/usd/usdUtils/sparseValueWriter.h"
#include "pxr/usd/usd/pyConversions.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static void
__init__(UsdUtilsSparseAttrValueWriter *self,
         const UsdAttribute &attr, 
         const nb::object &defaultValue)
{
    new (self) UsdUtilsSparseAttrValueWriter(attr, 
            UsdPythonToSdfType(defaultValue, attr.GetTypeName()));
}

static bool
_WrapSetTimeSample(
    UsdUtilsSparseAttrValueWriter &vc, 
    const nb::object &value, 
    const UsdTimeCode time)
{
    return vc.SetTimeSample(
            UsdPythonToSdfType(value, vc.GetAttr().GetTypeName()), 
            time);
}

static bool
_WrapSetAttribute(
    UsdUtilsSparseValueWriter &vc, 
    const UsdAttribute & attr, 
    const nb::object &value,
    const UsdTimeCode time) 
{
    return vc.SetAttribute(attr,
            UsdPythonToSdfType(value, attr.GetTypeName()), 
            time);
}

static std::vector<UsdUtilsSparseAttrValueWriter>
_WrapGetSparseAttrValueWriters(
    UsdUtilsSparseValueWriter &vc) 
{
    return vc.GetSparseAttrValueWriters();
}

void wrapSparseValueWriter(nanobind::module_ &m)
{
    nb::class_<UsdUtilsSparseAttrValueWriter>(m, "SparseAttrValueWriter")
        .def("__init__", &__init__,
             nb::arg("attr"), nb::arg("defaultValue").none() = nb::none())

        .def("SetTimeSample", _WrapSetTimeSample, 
            nb::arg("value").none(), nb::arg("time"))
    ;

    nb::class_<UsdUtilsSparseValueWriter>(m, "SparseValueWriter")
        .def(nb::init<>())
        .def("SetAttribute", _WrapSetAttribute, 
            nb::arg("attr"), nb::arg("value").none(), nb::arg("time")=UsdTimeCode::Default())

        .def("GetSparseAttrValueWriters", _WrapGetSparseAttrValueWriters)
    ;
}
