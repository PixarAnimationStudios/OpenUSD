//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/topology.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"


PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


namespace {


tuple
_Validate(const UsdSkelTopology& self)
{
    std::string reason;
    bool success = self.Validate(&reason);
    return pxr_boost::python::make_tuple(success, reason);
}


} // namespace


void wrapUsdSkelTopology()
{
    using This = UsdSkelTopology;

    class_<This>("Topology", no_init)
        .def(init<const SdfPathVector&>())
        .def(init<const VtTokenArray&>())
        .def(init<VtIntArray>())

        .def("GetParent", &This::GetParent)

        .def("IsRoot", &This::IsRoot)
        
        .def("GetParentIndices", &This::GetParentIndices,
             return_value_policy<return_by_value>())

        .def("GetNumJoints", &This::GetNumJoints)

        .def("__len__", &This::size)

        .def("Validate", &_Validate)
        ;
}
