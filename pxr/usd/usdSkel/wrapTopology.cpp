//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/topology.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

nb::tuple
_Validate(const UsdSkelTopology& self)
{
    std::string reason;
    bool success = self.Validate(&reason);
    return nb::make_tuple(success, reason);
}

} // namespace

void wrapUsdSkelTopology(nb::module_ &m)
{
    using This = UsdSkelTopology;

    nb::class_<This>(m, "Topology")
        .def(nb::init<const VtTokenArray&>())
        .def(nb::init<const SdfPathVector&>())
        .def(nb::init<VtIntArray>())

        .def("GetParent", &This::GetParent)

        .def("IsRoot", &This::IsRoot)
        
        .def("GetParentIndices", &This::GetParentIndices,
             nb::rv_policy::copy)

        .def("GetNumJoints", &This::GetNumJoints)

        .def("__len__", &This::size)

        .def("Validate", &_Validate)
        ;

}
