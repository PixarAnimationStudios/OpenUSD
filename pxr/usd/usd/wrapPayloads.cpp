//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/payloads.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

using std::string;

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdPayloads(nb::module_ &m)
{
    nb::class_<UsdPayloads>(m, "Payloads")
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def("AddPayload",
             (bool (UsdPayloads::*)(const SdfPayload &, UsdListPosition))
             &UsdPayloads::AddPayload,
             nb::arg("payload"),
             nb::arg("position")=UsdListPositionBackOfPrependList)
        .def("AddPayload",
             (bool (UsdPayloads::*)(const string &, const SdfPath &,
                                    const SdfLayerOffset &, UsdListPosition))
             &UsdPayloads::AddPayload,
             nb::arg("assetPath"), nb::arg("primPath"),
             nb::arg("layerOffset")=SdfLayerOffset(),
             nb::arg("position")=UsdListPositionBackOfPrependList)
        .def("AddPayload", (bool (UsdPayloads::*)(const string &,
                                                  const SdfLayerOffset &,
                                                  UsdListPosition))
             &UsdPayloads::AddPayload,
             nb::arg("assetPath"),
             nb::arg("layerOffset")=SdfLayerOffset(),
             nb::arg("position")=UsdListPositionBackOfPrependList)
        .def("AddInternalPayload",
             &UsdPayloads::AddInternalPayload, 
             nb::arg("primPath"),
             nb::arg("layerOffset")=SdfLayerOffset(),
             nb::arg("position")=UsdListPositionBackOfPrependList)

        .def("RemovePayload", &UsdPayloads::RemovePayload, nb::arg("payload"))
        .def("ClearPayloads", &UsdPayloads::ClearPayloads)
        .def("SetPayloads", &UsdPayloads::SetPayloads)
        .def("GetPrim", (UsdPrim (UsdPayloads::*)()) &UsdPayloads::GetPrim)
        .def("__bool__", [](UsdPayloads &self) { return bool(self); })
        ;
}
