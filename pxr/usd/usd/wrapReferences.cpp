//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/references.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

using std::string;

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdReferences(nb::module_ &m)
{
    nb::class_<UsdReferences>(m, "References")
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def("AddReference",
             (bool (UsdReferences::*)(const SdfReference &, UsdListPosition))
             &UsdReferences::AddReference,
             nb::arg("ref"),
             nb::arg("position")=UsdListPositionBackOfPrependList)
        .def("AddReference",
             (bool (UsdReferences::*)(const string &, const SdfPath &,
                                      const SdfLayerOffset &, UsdListPosition))
             &UsdReferences::AddReference,
             nb::arg("assetPath"), nb::arg("primPath"),
             nb::arg("layerOffset")=SdfLayerOffset(),
             nb::arg("position")=UsdListPositionBackOfPrependList)
        .def("AddReference", (bool (UsdReferences::*)(const string &,
                                                      const SdfLayerOffset &,
                                                      UsdListPosition))
             &UsdReferences::AddReference,
             nb::arg("assetPath"),
             nb::arg("layerOffset")=SdfLayerOffset(),
             nb::arg("position")=UsdListPositionBackOfPrependList)
        .def("AddInternalReference",
             &UsdReferences::AddInternalReference, 
             nb::arg("primPath"),
             nb::arg("layerOffset")=SdfLayerOffset(),
             nb::arg("position")=UsdListPositionBackOfPrependList)

        .def("RemoveReference", &UsdReferences::RemoveReference, nb::arg("ref"))
        .def("ClearReferences", &UsdReferences::ClearReferences)
        .def("SetReferences", &UsdReferences::SetReferences)
        .def("GetPrim", (UsdPrim (UsdReferences::*)()) &UsdReferences::GetPrim)
        .def("__bool__", [](UsdReferences &self) { return bool(self); })
        ;
}
