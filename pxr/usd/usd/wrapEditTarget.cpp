//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/editTarget.h"

#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/sdf/pySpec.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdEditTarget(nb::module_ &m)
{
    nb::class_<UsdEditTarget>(m, "EditTarget")
        .def(nb::init<>())
        .def(nb::init<SdfLayerHandle>(), nb::arg("layer"))
        .def(nb::init<SdfLayerHandle, PcpNodeRef>(),
             nb::arg("layer"), nb::arg("node"))
        .def_static("ForLocalDirectVariant", &UsdEditTarget::ForLocalDirectVariant,
             nb::arg("layer"), nb::arg("varSelPath"))
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("IsNull", &UsdEditTarget::IsNull)
        .def("IsValid", &UsdEditTarget::IsValid)
        .def("GetLayer", static_cast<SdfLayerHandle (*)(UsdEditTarget const &)>(
                 [](UsdEditTarget const &et) { return et.GetLayer(); }))
        .def("GetMapFunction", &UsdEditTarget::GetMapFunction,
             nb::rv_policy::copy)
        .def("MapToSpecPath", &UsdEditTarget::MapToSpecPath, nb::arg("scenePath"))
        .def("GetPrimSpecForScenePath",
             &UsdEditTarget::GetPrimSpecForScenePath, nb::arg("scenePath"))
        .def("GetPropertySpecForScenePath",
             &UsdEditTarget::GetPropertySpecForScenePath, nb::arg("scenePath"))
        .def("GetAttributeSpecForScenePath",
             &UsdEditTarget::GetAttributeSpecForScenePath, nb::arg("scenePath"))
        .def("GetRelationshipSpecForScenePath",
             &UsdEditTarget::GetRelationshipSpecForScenePath, nb::arg("scenePath"))
        .def("GetSpecForScenePath",
             &UsdEditTarget::GetSpecForScenePath, nb::arg("scenePath"))
        .def("ComposeOver", &UsdEditTarget::ComposeOver, nb::arg("weaker"))
        ;

    // Allow passing SdLayerHandle to wrapped functions expecting UsdEditTarget.
    nb::implicitly_convertible<SdfLayer, UsdEditTarget>();
    nb::implicitly_convertible<SdfLayerHandle, UsdEditTarget>();
}
