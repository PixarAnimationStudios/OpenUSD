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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/implicit.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

void wrapUsdGeomConstraintTarget()
{
    typedef UsdGeomConstraintTarget ConstraintTarget;

    class_<ConstraintTarget>("ConstraintTarget")
        .def(init<UsdAttribute>(arg("attr")))
        .def(!self)

        .def("GetAttr", &ConstraintTarget::GetAttr,
             return_value_policy<return_by_value>())
        .def("IsDefined", &ConstraintTarget::IsDefined)

        .def("IsValid", &ConstraintTarget::IsValid)

        .def("SetIdentifier", &ConstraintTarget::SetIdentifier, arg("identifier"))
        .def("GetIdentifier", &ConstraintTarget::GetIdentifier,
             return_value_policy<return_by_value>())

        .def("Get", &ConstraintTarget::Get, (arg("time")=UsdTimeCode::Default()))
        .def("Set", &ConstraintTarget::Set, (arg("value"), 
            arg("time")=UsdTimeCode::Default()))

        .def("GetConstraintAttrName", &ConstraintTarget::GetConstraintAttrName)
            .staticmethod("GetConstraintAttrName")

        .def("ComputeInWorldSpace", _ComputeInWorldSpace,
             (arg("time")=UsdTimeCode::Default()))
        ;

    implicitly_convertible<ConstraintTarget, UsdAttribute>();
}
